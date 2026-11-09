#pragma once
#include "display_utf8_case.h"
namespace espcontrol::pages {
// Match sentence_cap_text's word, whitespace and UTF-8 case rules without a string allocation.
// Most consumers cross a C-string translation boundary; alarm history can retain embedded NUL bytes.
inline size_t format_status_text(char* output, size_t capacity, std::string_view input,
                                 bool stop_at_nul = true) {
  if (!output || !capacity) return 0;
  size_t used = 0;
  bool cap_next = true, last_space = false;
  output[0] = '\0';
  for (size_t offset = 0; offset < input.size();) {
    const auto ch = static_cast<unsigned char>(input[offset]);
    if (ch == '_' || ch == '-' || (ch < 0x80 && std::isspace(ch))) {
      if (used && !last_space) {
        if (used + 1 >= capacity) break;
        output[used++] = ' ';
        output[used] = '\0';
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
      if (used + 1 >= capacity) break;
      output[used++] = input[offset];
      output[used] = '\0';
      offset += length;
      last_space = false;
      continue;
    }
    if (point == 0 && stop_at_nul) break;
    if (utf8_letter_like(point)) {
      point = point < 0x80 ? static_cast<uint32_t>(cap_next ? std::toupper(static_cast<unsigned char>(point))
                                                            : std::tolower(static_cast<unsigned char>(point)))
                           : utf8_latin_case(point, cap_next);
      cap_next = false;
    }
    if (!append_utf8_codepoint(output, capacity, used, point)) break;
    offset += length;
    last_space = false;
  }
  if (used && output[used - 1] == ' ') --used;
  output[used] = '\0';
  return used;
}
}  // namespace espcontrol::pages
