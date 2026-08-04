#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "esphome/core/string_ref.h"

// Copy an ESPHome StringRef while applying the existing fixed display limit.
// Keeping this at the module boundary avoids assuming the source is null
// terminated at the requested length.
inline std::string string_ref_limited(esphome::StringRef value, size_t max_len) {
  size_t len = value.size();
  if (len > max_len) len = max_len;
  return std::string(value.c_str(), len);
}

inline bool append_html_code_point(std::string &output, uint32_t code_point) {
  if (code_point == 0 || code_point > 0x10FFFF ||
      (code_point >= 0xD800 && code_point <= 0xDFFF)) {
    return false;
  }
  if (code_point <= 0x7F) {
    output.push_back(static_cast<char>(code_point));
  } else if (code_point <= 0x7FF) {
    output.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
  } else if (code_point <= 0xFFFF) {
    output.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
  } else {
    output.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
  }
  return true;
}

inline uint32_t remap_legacy_html_code_point(uint32_t code_point) {
  if (code_point < 0x80 || code_point > 0x9F) return code_point;
  static constexpr uint16_t windows_1252[] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
  };
  return windows_1252[code_point - 0x80];
}

// Home Assistant integrations can expose HTML-escaped media metadata. Decode
// the common named entities plus numeric character references before sending
// titles and artist names to LVGL labels.
inline std::string decode_html_entities(const std::string &text) {
  if (text.find('&') == std::string::npos) return text;

  std::string decoded;
  decoded.reserve(text.size());
  size_t index = 0;
  while (index < text.size()) {
    if (text[index] != '&') {
      decoded.push_back(text[index++]);
      continue;
    }

    struct NamedEntity {
      const char *encoded;
      size_t length;
      char value;
    };
    static constexpr NamedEntity named_entities[] = {
      {"&amp;", 5, '&'}, {"&quot;", 6, '"'}, {"&apos;", 6, '\''},
      {"&lt;", 4, '<'}, {"&gt;", 4, '>'},
    };
    bool matched = false;
    for (const NamedEntity &entity : named_entities) {
      if (text.compare(index, entity.length, entity.encoded) == 0) {
        decoded.push_back(entity.value);
        index += entity.length;
        matched = true;
        break;
      }
    }
    if (matched) continue;

    if (index + 3 < text.size() && text[index + 1] == '#') {
      size_t digit = index + 2;
      uint32_t base = 10;
      if (digit < text.size() && (text[digit] == 'x' || text[digit] == 'X')) {
        base = 16;
        ++digit;
      }
      const size_t first_digit = digit;
      uint32_t code_point = 0;
      bool valid = true;
      while (digit < text.size() && text[digit] != ';') {
        const char ch = text[digit];
        uint32_t value = 0;
        if (ch >= '0' && ch <= '9') value = static_cast<uint32_t>(ch - '0');
        else if (base == 16 && ch >= 'a' && ch <= 'f')
          value = static_cast<uint32_t>(ch - 'a' + 10);
        else if (base == 16 && ch >= 'A' && ch <= 'F')
          value = static_cast<uint32_t>(ch - 'A' + 10);
        else {
          valid = false;
          break;
        }
        if (code_point > (0x10FFFF - value) / base) {
          valid = false;
          break;
        }
        code_point = code_point * base + value;
        ++digit;
      }
      if (valid && digit > first_digit && digit < text.size() &&
          text[digit] == ';') {
        if (append_html_code_point(
              decoded, remap_legacy_html_code_point(code_point))) {
          index = digit + 1;
          continue;
        }
      }
    }

    decoded.push_back(text[index++]);
  }
  return decoded;
}
