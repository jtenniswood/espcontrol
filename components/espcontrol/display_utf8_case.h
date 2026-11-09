#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

enum class Utf8DecodeStatus : uint8_t {
  VALID,
  INVALID,
  INCOMPLETE,
};

struct Utf8CasePair {
  uint32_t upper;
  uint32_t lower;
};

inline Utf8DecodeStatus decode_utf8_codepoint(std::string_view text, size_t offset,
                                               uint32_t &codepoint, size_t &length) {
  codepoint = 0;
  length = 0;
  if (offset >= text.size()) return Utf8DecodeStatus::INCOMPLETE;

  const unsigned char lead = static_cast<unsigned char>(text[offset]);
  if (lead < 0x80) {
    codepoint = lead;
    length = 1;
    return Utf8DecodeStatus::VALID;
  }

  size_t expected = 0;
  uint32_t value = 0;
  uint32_t minimum = 0;
  if (lead >= 0xC2 && lead <= 0xDF) {
    expected = 2;
    value = lead & 0x1F;
    minimum = 0x80;
  } else if (lead >= 0xE0 && lead <= 0xEF) {
    expected = 3;
    value = lead & 0x0F;
    minimum = 0x800;
  } else if (lead >= 0xF0 && lead <= 0xF4) {
    expected = 4;
    value = lead & 0x07;
    minimum = 0x10000;
  } else {
    length = 1;
    return Utf8DecodeStatus::INVALID;
  }

  for (size_t i = 1; i < expected; i++) {
    if (offset + i >= text.size()) return Utf8DecodeStatus::INCOMPLETE;
    const unsigned char continuation = static_cast<unsigned char>(text[offset + i]);
    if ((continuation & 0xC0) != 0x80) {
      length = 1;
      return Utf8DecodeStatus::INVALID;
    }
    value = (value << 6) | (continuation & 0x3F);
  }
  if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
    length = 1;
    return Utf8DecodeStatus::INVALID;
  }

  codepoint = value;
  length = expected;
  return Utf8DecodeStatus::VALID;
}

inline void append_utf8_codepoint(std::string &out, uint32_t codepoint) {
  if (codepoint <= 0x7F) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

inline const Utf8CasePair *utf8_latin_case_pairs(size_t &count) {
  // Locale-dependent Turkish dotted/dotless I are deliberately left unchanged.
  static const Utf8CasePair pairs[] = {
    {0x00C0, 0x00E0}, {0x00C1, 0x00E1}, {0x00C2, 0x00E2}, {0x00C3, 0x00E3},
    {0x00C4, 0x00E4}, {0x00C5, 0x00E5}, {0x00C6, 0x00E6}, {0x00C7, 0x00E7},
    {0x00C8, 0x00E8}, {0x00C9, 0x00E9}, {0x00CA, 0x00EA}, {0x00CB, 0x00EB},
    {0x00CC, 0x00EC}, {0x00CD, 0x00ED}, {0x00CE, 0x00EE}, {0x00CF, 0x00EF},
    {0x00D0, 0x00F0}, {0x00D1, 0x00F1}, {0x00D2, 0x00F2}, {0x00D3, 0x00F3},
    {0x00D4, 0x00F4}, {0x00D5, 0x00F5}, {0x00D6, 0x00F6}, {0x00D8, 0x00F8},
    {0x00D9, 0x00F9}, {0x00DA, 0x00FA}, {0x00DB, 0x00FB}, {0x00DC, 0x00FC},
    {0x00DD, 0x00FD}, {0x00DE, 0x00FE}, {0x0102, 0x0103}, {0x0104, 0x0105},
    {0x0106, 0x0107}, {0x010C, 0x010D}, {0x010E, 0x010F}, {0x0110, 0x0111},
    {0x0118, 0x0119}, {0x011A, 0x011B}, {0x011E, 0x011F}, {0x0139, 0x013A}, {0x013D, 0x013E}, {0x0141, 0x0142},
    {0x0143, 0x0144}, {0x0147, 0x0148}, {0x0150, 0x0151}, {0x0154, 0x0155},
    {0x0158, 0x0159}, {0x015A, 0x015B}, {0x015E, 0x015F}, {0x0160, 0x0161},
    {0x0164, 0x0165}, {0x016E, 0x016F}, {0x0170, 0x0171}, {0x0179, 0x017A},
    {0x017B, 0x017C}, {0x017D, 0x017E}, {0x0218, 0x0219}, {0x021A, 0x021B},
    {0x1E62, 0x1E63},
  };
  count = sizeof(pairs) / sizeof(pairs[0]);
  return pairs;
}

inline uint32_t utf8_latin_case(uint32_t codepoint, bool uppercase) {
  size_t count = 0;
  const Utf8CasePair *pairs = utf8_latin_case_pairs(count);
  for (size_t i = 0; i < count; i++) {
    if (codepoint == pairs[i].upper || codepoint == pairs[i].lower) {
      return uppercase ? pairs[i].upper : pairs[i].lower;
    }
  }
  return codepoint;
}

inline bool utf8_letter_like(uint32_t codepoint) {
  if (codepoint < 0x80) {
    return std::isalpha(static_cast<unsigned char>(codepoint));
  }
  // Latin ranges exclude the multiplication and division symbols.
  if ((codepoint >= 0x00C0 && codepoint <= 0x00D6) ||
      (codepoint >= 0x00D8 && codepoint <= 0x00F6) ||
      (codepoint >= 0x00F8 && codepoint <= 0x02AF) ||
      (codepoint >= 0x1E00 && codepoint <= 0x1EFF)) {
    return true;
  }
  // Unicode 16.0 Letter-category ranges within the supported script blocks.
  // Punctuation, symbols, combining marks and unassigned positions must not
  // consume a pending capital letter (for example Greek U+037E or Hebrew U+05BE).
  return (codepoint >= 0x0370 && codepoint <= 0x0374) ||
         (codepoint >= 0x0376 && codepoint <= 0x0377) ||
         (codepoint >= 0x037A && codepoint <= 0x037D) ||
         codepoint == 0x037F || codepoint == 0x0386 ||
         (codepoint >= 0x0388 && codepoint <= 0x038A) ||
         codepoint == 0x038C ||
         (codepoint >= 0x038E && codepoint <= 0x03A1) ||
         (codepoint >= 0x03A3 && codepoint <= 0x03F5) ||
         (codepoint >= 0x03F7 && codepoint <= 0x03FF) ||
         (codepoint >= 0x0400 && codepoint <= 0x0481) ||
         (codepoint >= 0x048A && codepoint <= 0x052F) ||
         (codepoint >= 0x05D0 && codepoint <= 0x05EA) ||
         (codepoint >= 0x05EF && codepoint <= 0x05F2);
}

// Allocation-free counterpart for configuration-owned status histories.
inline bool append_utf8_codepoint(char* output, size_t capacity, size_t& used, uint32_t point) {
  char bytes[4];
  size_t count = 0;
  if (point <= 0x7F)
    bytes[count++] = static_cast<char>(point);
  else if (point <= 0x7FF) {
    bytes[count++] = static_cast<char>(0xC0 | (point >> 6));
    bytes[count++] = static_cast<char>(0x80 | (point & 0x3F));
  } else if (point <= 0xFFFF) {
    bytes[count++] = static_cast<char>(0xE0 | (point >> 12));
    bytes[count++] = static_cast<char>(0x80 | ((point >> 6) & 0x3F));
    bytes[count++] = static_cast<char>(0x80 | (point & 0x3F));
  } else {
    bytes[count++] = static_cast<char>(0xF0 | (point >> 18));
    bytes[count++] = static_cast<char>(0x80 | ((point >> 12) & 0x3F));
    bytes[count++] = static_cast<char>(0x80 | ((point >> 6) & 0x3F));
    bytes[count++] = static_cast<char>(0x80 | (point & 0x3F));
  }
  if (!output || used >= capacity || count >= capacity - used) return false;
  for (size_t i = 0; i < count; ++i) output[used++] = bytes[i];
  output[used] = '\0';
  return true;
}

inline bool append_title_case_character(const std::string& text, size_t& offset, std::string& out,
                                        bool& cap_next) {
  uint32_t codepoint = 0;
  size_t length = 0;
  const Utf8DecodeStatus status = decode_utf8_codepoint(text, offset, codepoint, length);
  if (status == Utf8DecodeStatus::INCOMPLETE) return false;
  if (status == Utf8DecodeStatus::INVALID) {
    out.push_back(text[offset]);
    offset += length;
    return true;
  }

  if (utf8_letter_like(codepoint)) {
    if (codepoint < 0x80) {
      const unsigned char ascii = static_cast<unsigned char>(codepoint);
      codepoint = static_cast<uint32_t>(cap_next ? std::toupper(ascii) : std::tolower(ascii));
    } else {
      codepoint = utf8_latin_case(codepoint, cap_next);
    }
    cap_next = false;
  }
  append_utf8_codepoint(out, codepoint);
  offset += length;
  return true;
}
