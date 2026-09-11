#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace espcontrol {
namespace home_assistant_endpoint {

enum class Mode : uint8_t { AUTOMATIC = 0, MANUAL = 1 };
enum class Source : uint8_t { DISCOVERING = 0, AUTOMATIC = 1, FALLBACK = 2, MANUAL = 3 };

struct ServiceRecord {
  std::vector<std::string> addresses;
  uint16_t port{0};
  std::string internal_url;
  bool landing_page{false};
};

inline std::string trim_copy(std::string value) {
  const char *whitespace = " \t\r\n";
  const size_t start = value.find_first_not_of(whitespace);
  if (start == std::string::npos) return {};
  const size_t end = value.find_last_not_of(whitespace);
  return value.substr(start, end - start + 1);
}

inline bool parse_ipv6_words(const std::string &value,
                             std::array<uint16_t, 8> &words) {
  const size_t compressed = value.find("::");
  if (compressed != std::string::npos &&
      value.find("::", compressed + 2) != std::string::npos)
    return false;
  auto parse_side = [&](size_t begin, size_t end,
                        std::vector<uint16_t> &output) {
    size_t cursor = begin;
    while (cursor < end) {
      const size_t colon = value.find(':', cursor);
      const size_t token_end = colon == std::string::npos || colon > end ? end : colon;
      if (token_end == cursor || token_end - cursor > 4) return false;
      uint16_t word = 0;
      for (size_t index = cursor; index < token_end; ++index) {
        const char c = value[index];
        const int digit = c >= '0' && c <= '9' ? c - '0'
                            : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                  : -1;
        if (digit < 0) return false;
        word = static_cast<uint16_t>((word << 4) | digit);
      }
      output.push_back(word);
      cursor = token_end + 1;
    }
    return true;
  };
  std::vector<uint16_t> left;
  std::vector<uint16_t> right;
  const size_t left_end = compressed == std::string::npos ? value.size() : compressed;
  const size_t right_begin = compressed == std::string::npos ? value.size() : compressed + 2;
  if (!parse_side(0, left_end, left) ||
      !parse_side(right_begin, value.size(), right))
    return false;
  if (compressed == std::string::npos) {
    if (left.size() != words.size()) return false;
  } else if (left.size() + right.size() >= words.size()) {
    return false;
  }
  words.fill(0);
  std::copy(left.begin(), left.end(), words.begin());
  std::copy(right.begin(), right.end(), words.end() - right.size());
  return true;
}

inline std::string canonical_ipv6(const std::string &value) {
  std::array<uint16_t, 8> words{};
  if (!parse_ipv6_words(value, words)) return value;
  static const char *const digits = "0123456789abcdef";
  std::string canonical;
  for (size_t index = 0; index < words.size(); ++index) {
    if (index != 0) canonical.push_back(':');
    uint16_t word = words[index];
    bool emitted = false;
    for (int shift = 12; shift >= 0; shift -= 4) {
      const uint8_t digit = static_cast<uint8_t>((word >> shift) & 0x0F);
      if (digit != 0 || emitted || shift == 0) {
        canonical.push_back(digits[digit]);
        emitted = true;
      }
    }
  }
  return canonical;
}

inline std::string normalize_address(std::string value) {
  value = trim_copy(value);
  if (value.size() >= 2 && value.front() == '[') {
    const size_t close = value.find(']');
    if (close != std::string::npos) value = value.substr(1, close - 1);
  } else {
    const size_t first_colon = value.find(':');
    const size_t last_colon = value.rfind(':');
    if (first_colon != std::string::npos && first_colon == last_colon &&
        value.find('.') != std::string::npos) {
      value.resize(first_colon);
    }
  }
  const size_t scope = value.find('%');
  if (scope != std::string::npos) value.resize(scope);
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (value.rfind("::ffff:", 0) == 0 && value.find('.') != std::string::npos)
    return value.substr(7);
  if (value.find(':') != std::string::npos) {
    std::array<uint16_t, 8> words{};
    if (parse_ipv6_words(value, words) && words[0] == 0 && words[1] == 0 &&
        words[2] == 0 && words[3] == 0 && words[4] == 0 &&
        words[5] == 0xFFFF) {
      return std::to_string(words[6] >> 8) + "." +
             std::to_string(words[6] & 0xFF) + "." +
             std::to_string(words[7] >> 8) + "." +
             std::to_string(words[7] & 0xFF);
    }
    value = canonical_ipv6(value);
  }
  return value;
}

inline std::string display_host(const std::string &address) {
  const std::string normalized = normalize_address(address);
  if (normalized.find(':') != std::string::npos) return "[" + normalized + "]";
  return normalized;
}

inline std::string normalize_protocol(std::string value) {
  value = trim_copy(value);
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value == "https" ? "https" : "http";
}

inline std::string protocol_from_internal_url(const std::string &url,
                                              const std::string &fallback) {
  const size_t scheme_end = url.find("://");
  if (scheme_end == std::string::npos) return normalize_protocol(fallback);
  std::string scheme = url.substr(0, scheme_end);
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (scheme != "http" && scheme != "https") {
    return normalize_protocol(fallback);
  }
  return scheme;
}

inline std::string build_origin(const std::string &protocol,
                                const std::string &address, uint16_t port) {
  const std::string host = display_host(address);
  if (host.empty() || port == 0) return {};
  return normalize_protocol(protocol) + "://" + host + ":" + std::to_string(port);
}

inline bool record_matches_client(const ServiceRecord &record,
                                  const std::string &client_address) {
  if (record.landing_page || record.port == 0) return false;
  const std::string client = normalize_address(client_address);
  if (client.empty()) return false;
  for (const std::string &address : record.addresses) {
    if (normalize_address(address) == client) return true;
  }
  return false;
}

inline std::string select_discovered_origin(
    const std::vector<ServiceRecord> &records, const std::string &client_address,
    const std::string &fallback_protocol) {
  std::string matched_origin;
  bool found_match = false;
  for (const ServiceRecord &record : records) {
    if (!record_matches_client(record, client_address)) continue;
    // The native API connection identifies the host but not the HTTP service
    // when multiple Home Assistant instances share that host. Do not choose a
    // token-bearing destination based on mDNS result order.
    if (found_match) return {};
    matched_origin = build_origin(
        protocol_from_internal_url(record.internal_url, fallback_protocol),
        client_address, record.port);
    found_match = true;
  }
  return matched_origin;
}

inline Mode infer_legacy_mode(const std::string &protocol, uint16_t port) {
  return normalize_protocol(protocol) == "http" && port == 8123
             ? Mode::AUTOMATIC
             : Mode::MANUAL;
}

}  // namespace home_assistant_endpoint
}  // namespace espcontrol
