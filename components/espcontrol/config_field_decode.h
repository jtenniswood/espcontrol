#pragma once

#include <cstddef>
#include <string>

// Shared by the saved-card parser and HTTP backup export.
inline int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

inline bool valid_utf8_bytes(const std::string &value) {
  size_t index = 0;
  while (index < value.size()) {
    const unsigned char first = static_cast<unsigned char>(value[index]);
    if (first <= 0x7F) { ++index; continue; }
    size_t count = 0;
    if (first >= 0xC2 && first <= 0xDF) count = 1;
    else if (first >= 0xE0 && first <= 0xEF) count = 2;
    else if (first >= 0xF0 && first <= 0xF4) count = 3;
    else return false;
    if (index + count >= value.size()) return false;
    const unsigned char second = static_cast<unsigned char>(value[index + 1]);
    if ((second & 0xC0) != 0x80) return false;
    if (first == 0xE0 && second < 0xA0) return false;
    if (first == 0xED && second > 0x9F) return false;
    if (first == 0xF0 && second < 0x90) return false;
    if (first == 0xF4 && second > 0x8F) return false;
    for (size_t offset = 2; offset <= count; ++offset) {
      if ((static_cast<unsigned char>(value[index + offset]) & 0xC0) != 0x80) return false;
    }
    index += count + 1;
  }
  return true;
}

inline std::string decode_compact_field(const std::string &value, size_t start, size_t len) {
  if (start > value.size()) return "";
  size_t end = start + len;
  if (end < start || end > value.size()) end = value.size();
  std::string out;
  out.reserve(end - start);
  for (size_t i = start; i < end;) {
    if (value[i] == '%' && i + 2 < end) {
      size_t run_end = i;
      std::string decoded;
      while (run_end + 2 < end && value[run_end] == '%') {
        int hi = hex_digit(value[run_end + 1]);
        int lo = hex_digit(value[run_end + 2]);
        if (hi < 0 || lo < 0) break;
        decoded.push_back(static_cast<char>((hi << 4) | lo));
        run_end += 3;
      }
      if (run_end > i) {
        if (valid_utf8_bytes(decoded)) out += decoded;
        else out.append(value, i, run_end - i);
        i = run_end;
        continue;
      }
    }
    out.push_back(value[i]);
    ++i;
  }
  return out;
}

inline std::string decode_compact_field(const std::string &value) {
  return decode_compact_field(value, 0, value.size());
}
