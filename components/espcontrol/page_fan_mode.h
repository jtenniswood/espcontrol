#pragma once

#include <cctype>
#include <cstring>
#include <string_view>

#include "display_utf8_case.h"

namespace espcontrol::pages {
inline std::string_view fan_mode_trim(std::string_view value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
  return value;
}
inline bool fan_mode_equal(std::string_view value, std::string_view expected) {
  value = fan_mode_trim(value);
  if (value.size() != expected.size()) return false;
  for (size_t i = 0; i < value.size(); ++i)
    if (std::tolower(static_cast<unsigned char>(value[i])) != expected[i]) return false;
  return true;
}
inline bool fan_mode_preset_active(std::string_view value) {
  return !fan_mode_trim(value).empty() && !fan_mode_equal(value, "none") && !fan_mode_equal(value, "off");
}
// The existing option lexer removes quotes/brackets and excludes header tokens.
// Presence needs only its first real option, with no vector or decoded string.
inline bool fan_mode_has_options(std::string_view input) {
  char head[12]{};
  size_t used = 0, trimmed = 0;
  bool started = false;
  char quote = 0;
  const auto real_option = [&]() {
    if (!trimmed) return false;
    constexpr std::string_view headers[] = {"presetmode", "presetmodes", "direction", "oscillating"};
    for (const auto header : headers)
      if (trimmed == header.size() && std::string_view(head, trimmed) == header) return false;
    return true;
  };
  for (const char value : input) {
    if (value == '\'' || value == '"') {
      if (!quote) {
        quote = value;
        continue;
      }
      if (quote == value) {
        quote = 0;
        continue;
      }
    }
    if (!quote && (value == '[' || value == ']')) continue;
    if (!quote && value == ',') {
      if (real_option()) return true;
      used = trimmed = 0;
      started = false;
      continue;
    }
    const bool space = std::isspace(static_cast<unsigned char>(value));
    if (!started && space) continue;
    started = true;
    if (used < sizeof(head)) head[used] = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    ++used;
    if (!space) {
      trimmed = used;
      if (trimmed > sizeof(head)) return true;  // Longer than every excluded header.
    }
  }
  return real_option();
}
// Match sentence_cap_text after the fan reader's trim/lower normalization.
// Keep complete UTF-8 codepoints in caller-owned storage; the normal card
// display boundary applies the shared invalid-byte/S-with-dot-below policy.
inline size_t format_fan_mode_text(char* output, size_t capacity, std::string_view input) {
  if (!output || !capacity) return 0;
  input = fan_mode_trim(input);
  if (input.empty()) input = "None";
  size_t used = 0;
  bool cap_next = true, last_space = false;
  for (size_t offset = 0; offset < input.size();) {
    const unsigned char byte = static_cast<unsigned char>(input[offset]);
    if (byte == '_' || byte == '-' || (byte < 0x80 && std::isspace(byte))) {
      if (used && !last_space) {
        if (used >= capacity - 1) break;
        output[used++] = ' ';
        last_space = true;
      }
      cap_next = true;
      ++offset;
      continue;
    }
    uint32_t point = 0;
    size_t length = 0;
    const auto status = decode_utf8_codepoint(input, offset, point, length);
    if (status == Utf8DecodeStatus::INCOMPLETE) break;
    if (status == Utf8DecodeStatus::INVALID) {
      if (used >= capacity - 1) break;
      output[used++] = input[offset++];
      last_space = false;
      continue;
    }
    if (point == 0) break;  // The existing i18n CString boundary ends here too.
    if (utf8_letter_like(point)) {
      point = point < 0x80 ? static_cast<uint32_t>(cap_next ? std::toupper(static_cast<unsigned char>(point))
                                                            : std::tolower(static_cast<unsigned char>(point)))
                           : utf8_latin_case(point, cap_next);
      cap_next = false;
    }
    char bytes[4];
    size_t count = 0;
    if (point <= 0x7f)
      bytes[count++] = static_cast<char>(point);
    else if (point <= 0x7ff) {
      bytes[count++] = static_cast<char>(0xc0 | (point >> 6));
      bytes[count++] = static_cast<char>(0x80 | (point & 0x3f));
    } else if (point <= 0xffff) {
      bytes[count++] = static_cast<char>(0xe0 | (point >> 12));
      bytes[count++] = static_cast<char>(0x80 | ((point >> 6) & 0x3f));
      bytes[count++] = static_cast<char>(0x80 | (point & 0x3f));
    } else {
      bytes[count++] = static_cast<char>(0xf0 | (point >> 18));
      bytes[count++] = static_cast<char>(0x80 | ((point >> 12) & 0x3f));
      bytes[count++] = static_cast<char>(0x80 | ((point >> 6) & 0x3f));
      bytes[count++] = static_cast<char>(0x80 | (point & 0x3f));
    }
    if (count > capacity - used - 1) break;
    std::memcpy(output + used, bytes, count);
    used += count;
    offset += length;
    last_space = false;
  }
  if (used && output[used - 1] == ' ') --used;
  output[used] = '\0';
  return used;
}
}  // namespace espcontrol::pages
