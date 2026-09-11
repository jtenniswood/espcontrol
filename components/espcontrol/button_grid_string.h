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

// Roboto does not provide the precomposed Latin S-with-dot-below glyphs used
// by some user-supplied names and media metadata. Keep protocol/configuration
// values untouched and apply this compact ASCII fallback only at display time.
inline std::string normalize_display_text(const std::string &text) {
  static constexpr char UPPER_S_DOT_BELOW[] = "\xE1\xB9\xA2";  // U+1E62
  static constexpr char LOWER_S_DOT_BELOW[] = "\xE1\xB9\xA3";  // U+1E63

  if (text.find(UPPER_S_DOT_BELOW) == std::string::npos &&
      text.find(LOWER_S_DOT_BELOW) == std::string::npos) {
    return text;
  }

  std::string normalized;
  normalized.reserve(text.size());
  for (size_t index = 0; index < text.size();) {
    if (text.compare(index, 3, UPPER_S_DOT_BELOW) == 0) {
      normalized.push_back('S');
      index += 3;
    } else if (text.compare(index, 3, LOWER_S_DOT_BELOW) == 0) {
      normalized.push_back('s');
      index += 3;
    } else {
      normalized.push_back(text[index++]);
    }
  }
  return normalized;
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
      uint16_t code_point;
    };
    // Cover the HTML entities most likely to occur in artist names and media
    // titles without carrying the much larger browser HTML5 entity table.
    static constexpr NamedEntity named_entities[] = {
      {"&amp;", 5, '&'}, {"&quot;", 6, '"'}, {"&apos;", 6, '\''},
      {"&lt;", 4, '<'}, {"&gt;", 4, '>'},
      {"&nbsp;", 6, 0x00A0}, {"&iexcl;", 7, 0x00A1},
      {"&cent;", 6, 0x00A2}, {"&pound;", 7, 0x00A3},
      {"&yen;", 5, 0x00A5}, {"&copy;", 6, 0x00A9},
      {"&reg;", 5, 0x00AE}, {"&deg;", 5, 0x00B0},
      {"&plusmn;", 8, 0x00B1}, {"&middot;", 8, 0x00B7},
      {"&frac14;", 8, 0x00BC}, {"&frac12;", 8, 0x00BD},
      {"&frac34;", 8, 0x00BE}, {"&iquest;", 8, 0x00BF},
      {"&Agrave;", 8, 0x00C0}, {"&Aacute;", 8, 0x00C1},
      {"&Acirc;", 7, 0x00C2}, {"&Atilde;", 8, 0x00C3},
      {"&Auml;", 6, 0x00C4}, {"&Aring;", 7, 0x00C5},
      {"&AElig;", 7, 0x00C6}, {"&Ccedil;", 8, 0x00C7},
      {"&Egrave;", 8, 0x00C8}, {"&Eacute;", 8, 0x00C9},
      {"&Ecirc;", 7, 0x00CA}, {"&Euml;", 6, 0x00CB},
      {"&Igrave;", 8, 0x00CC}, {"&Iacute;", 8, 0x00CD},
      {"&Icirc;", 7, 0x00CE}, {"&Iuml;", 6, 0x00CF},
      {"&Ntilde;", 8, 0x00D1}, {"&Ograve;", 8, 0x00D2},
      {"&Oacute;", 8, 0x00D3}, {"&Ocirc;", 7, 0x00D4},
      {"&Otilde;", 8, 0x00D5}, {"&Ouml;", 6, 0x00D6},
      {"&times;", 7, 0x00D7}, {"&Oslash;", 8, 0x00D8},
      {"&Ugrave;", 8, 0x00D9}, {"&Uacute;", 8, 0x00DA},
      {"&Ucirc;", 7, 0x00DB}, {"&Uuml;", 6, 0x00DC},
      {"&Yacute;", 8, 0x00DD}, {"&szlig;", 7, 0x00DF},
      {"&agrave;", 8, 0x00E0}, {"&aacute;", 8, 0x00E1},
      {"&acirc;", 7, 0x00E2}, {"&atilde;", 8, 0x00E3},
      {"&auml;", 6, 0x00E4}, {"&aring;", 7, 0x00E5},
      {"&aelig;", 7, 0x00E6}, {"&ccedil;", 8, 0x00E7},
      {"&egrave;", 8, 0x00E8}, {"&eacute;", 8, 0x00E9},
      {"&ecirc;", 7, 0x00EA}, {"&euml;", 6, 0x00EB},
      {"&igrave;", 8, 0x00EC}, {"&iacute;", 8, 0x00ED},
      {"&icirc;", 7, 0x00EE}, {"&iuml;", 6, 0x00EF},
      {"&ntilde;", 8, 0x00F1}, {"&ograve;", 8, 0x00F2},
      {"&oacute;", 8, 0x00F3}, {"&ocirc;", 7, 0x00F4},
      {"&otilde;", 8, 0x00F5}, {"&ouml;", 6, 0x00F6},
      {"&divide;", 8, 0x00F7}, {"&oslash;", 8, 0x00F8},
      {"&ugrave;", 8, 0x00F9}, {"&uacute;", 8, 0x00FA},
      {"&ucirc;", 7, 0x00FB}, {"&uuml;", 6, 0x00FC},
      {"&yacute;", 8, 0x00FD}, {"&yuml;", 6, 0x00FF},
      {"&ndash;", 7, 0x2013}, {"&mdash;", 7, 0x2014},
      {"&lsquo;", 7, 0x2018}, {"&rsquo;", 7, 0x2019},
      {"&sbquo;", 7, 0x201A}, {"&ldquo;", 7, 0x201C},
      {"&rdquo;", 7, 0x201D}, {"&bdquo;", 7, 0x201E},
      {"&bull;", 6, 0x2022}, {"&hellip;", 8, 0x2026},
      {"&trade;", 7, 0x2122},
    };
    bool matched = false;
    for (const NamedEntity &entity : named_entities) {
      if (text.compare(index, entity.length, entity.encoded) == 0) {
        if (append_html_code_point(decoded, entity.code_point)) {
          index += entity.length;
          matched = true;
          break;
        }
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
