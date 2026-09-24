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
  std::string identity;
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
  if (value.empty() || (value.front() == ':' && value.rfind("::", 0) != 0) ||
      (value.back() == ':' && (value.size() < 2 || value[value.size() - 2] != ':')))
    return false;
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

// Parse origins only. Never log rejected input: it may contain credentials.
inline std::string parse_origin(std::string value) {
  value = trim_copy(value);
  if (value.empty() || value.size() > 512 ||
      value.find_first_of("@?#\\ \t\r\n") != std::string::npos) return {};
  const size_t scheme_end = value.find("://");
  if (scheme_end == std::string::npos) return {};
  std::string scheme = value.substr(0, scheme_end);
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (scheme != "http" && scheme != "https") return {};
  std::string authority = value.substr(scheme_end + 3);
  const size_t slash = authority.find('/');
  if (slash != std::string::npos) {
    if (slash != authority.size() - 1) return {};
    authority.pop_back();
  }
  std::string host, port_text;
  bool explicit_port = false;
  if (!authority.empty() && authority.front() == '[') {
    const size_t close = authority.find(']');
    if (close == std::string::npos) return {};
    host = authority.substr(1, close - 1);
    std::transform(host.begin(), host.end(), host.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::array<uint16_t, 8> words{};
    if (!parse_ipv6_words(host, words)) return {};
    host = "[" + canonical_ipv6(host) + "]";
    if (close + 1 != authority.size()) {
      if (authority[close + 1] != ':') return {};
      explicit_port = true;
      port_text = authority.substr(close + 2);
    }
  } else {
    const size_t colon = authority.find(':');
    host = authority.substr(0, colon);
    if (colon != std::string::npos) {
      explicit_port = true;
      port_text = authority.substr(colon + 1);
    }
    if (host.empty() || host.front() == '.' || host.front() == '-' ||
        host.back() == '-') return {};
    for (unsigned char c : host)
      if (!std::isalnum(c) && c != '.' && c != '-') return {};
    std::transform(host.begin(), host.end(), host.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  }
  unsigned port = scheme == "https" ? 443 : 80;
  if (explicit_port) {
    if (port_text.empty() || port_text.size() > 5) return {};
    port = 0;
    for (unsigned char c : port_text) {
      if (!std::isdigit(c)) return {};
      port = port * 10 + (c - '0');
    }
    if (port == 0 || port > 65535) return {};
  }
  // Canonical effective ports make origin comparisons independent of spelling.
  return scheme + "://" + host + ":" + std::to_string(port);
}

inline bool local_address(const std::string &value) {
  const std::string address = normalize_address(value);
  std::array<uint16_t, 8> words{};
  if (parse_ipv6_words(address, words)) {
    return (words[0] & 0xfe00) == 0xfc00 ||
           (words[0] & 0xffc0) == 0xfe80 || address == "0:0:0:0:0:0:0:1";
  }
  unsigned parts[4]{};
  size_t cursor = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const size_t start = cursor;
    while (cursor < address.size() && std::isdigit(static_cast<unsigned char>(address[cursor]))) {
      parts[i] = parts[i] * 10 + address[cursor++] - '0';
      if (parts[i] > 255) return false;
    }
    if (cursor == start) return false;
    if (i != 3 && (cursor == address.size() || address[cursor++] != '.')) return false;
  }
  if (cursor != address.size()) return false;
  return parts[0] == 10 || parts[0] == 127 ||
      (parts[0] == 172 && parts[1] >= 16 && parts[1] <= 31) ||
      (parts[0] == 192 && parts[1] == 168) || (parts[0] == 169 && parts[1] == 254);
}

inline bool origin_host_matches_client(const std::string &origin,
                                       const std::string &client_address) {
  const size_t scheme = origin.find("://");
  if (scheme == std::string::npos) return false;
  const size_t start = scheme + 3;
  const size_t end = origin.find(':', origin[start] == '[' ? origin.find(']', start) + 1 : start);
  if (end == std::string::npos || end <= start) return false;
  std::string host = origin.substr(start, end - start);
  if (host.size() >= 2 && host.front() == '[') host = host.substr(1, host.size() - 2);
  return normalize_address(host) == normalize_address(client_address);
}

struct Candidate {
  std::string origin;
  Source source{Source::AUTOMATIC};
  const char *reason{"advertised local URL"};
  bool require_local_destination{false};
};
struct Discovery {
  std::vector<Candidate> candidates;
  bool ambiguous{false};
  bool invalid_internal_url{false};
};

inline Discovery discover(const std::vector<ServiceRecord> &records,
                          const std::string &client, const std::string &protocol,
                          uint16_t fallback_port) {
  Discovery result;
  const ServiceRecord *matched = nullptr;
  for (const auto &record : records) {
    if (!record_matches_client(record, client)) continue;
    if (matched && (matched->identity != record.identity || matched->port != record.port ||
                    (!matched->internal_url.empty() && !record.internal_url.empty() &&
                     parse_origin(matched->internal_url) != parse_origin(record.internal_url)))) {
      result.ambiguous = true;
      break;
    }
    if (!matched || !record.internal_url.empty()) matched = &record;
  }
  auto add = [&](std::string origin, Source source, const char *reason,
                 bool require_local_destination = false) {
    origin = parse_origin(origin);
    if (origin.empty()) return;
    for (const auto &candidate : result.candidates)
      if (candidate.origin == origin) return;
    result.candidates.push_back({std::move(origin), source, reason, require_local_destination});
  };
  if (matched && !result.ambiguous) {
    const std::string internal = parse_origin(matched->internal_url);
    result.invalid_internal_url = !matched->internal_url.empty() && internal.empty();
    // mDNS is unauthenticated. Hostnames from its TXT record may be used only
    // when the probe confirms they resolve to a private/local network target.
    // The TXT hostname is unauthenticated and later image URLs carry HA
    // tokens. Automatic mode only accepts an origin whose host is the
    // connected API peer IP. A user-entered Manual host is an
    // explicit trust decision and remains available for TLS/FQDN deployments.
    if (!internal.empty() && origin_host_matches_client(internal, client))
      add(internal, Source::AUTOMATIC, "advertised local URL");
    else if (!matched->internal_url.empty())
      result.invalid_internal_url = true;
    add(build_origin(protocol, client, matched->port), Source::AUTOMATIC, "advertised local service");
    if (local_address(client))
      add(build_origin(normalize_protocol(protocol) == "http" ? "https" : "http", client, matched->port),
          Source::AUTOMATIC, "alternate local protocol");
  }
  add(build_origin(protocol, client, fallback_port), Source::FALLBACK, "configured fallback");
  return result;
}

inline Mode infer_legacy_mode(const std::string &protocol, uint16_t port) {
  return normalize_protocol(protocol) == "http" && port == 8123
             ? Mode::AUTOMATIC
             : Mode::MANUAL;
}

}  // namespace home_assistant_endpoint
}  // namespace espcontrol
