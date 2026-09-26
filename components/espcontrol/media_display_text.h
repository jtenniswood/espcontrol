#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "media_emoji_ranges.h"

namespace espcontrol::media {
namespace text_detail {

struct Character {
  uint32_t value = 0;
  size_t bytes = 0;
};

// Reject malformed/partial UTF-8 without reading beyond the bounded input.
inline Character character_at(const std::string &text, size_t offset) {
  if (offset >= text.size()) return {};
  const auto lead = static_cast<uint8_t>(text[offset]);
  if (lead < 0x80) return {lead, 1};
  size_t bytes = lead >= 0xC2 && lead <= 0xDF ? 2
    : lead >= 0xE0 && lead <= 0xEF ? 3
    : lead >= 0xF0 && lead <= 0xF4 ? 4 : 0;
  if (bytes == 0 || bytes > text.size() - offset) return {};
  uint32_t value = lead & (0x7F >> bytes);
  for (size_t i = 1; i < bytes; ++i) {
    const auto byte = static_cast<uint8_t>(text[offset + i]);
    if ((byte & 0xC0) != 0x80) return {};
    value = (value << 6) | (byte & 0x3F);
  }
  if ((bytes == 2 && value < 0x80) || (bytes == 3 && value < 0x800) ||
      (bytes == 4 && value < 0x10000) || value > 0x10FFFF ||
      (value >= 0xD800 && value <= 0xDFFF)) return {};
  return {value, bytes};
}

template<size_t N>
inline bool contains(const Range (&ranges)[N], uint32_t value) {
  size_t lo = 0, hi = N;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (value < ranges[mid].first) hi = mid;
    else if (value > ranges[mid].last) lo = mid + 1;
    else return true;
  }
  return false;
}

// Return the first byte after an emoji, including its selectors, modifiers,
// tags and joined components. Plain text symbols such as copyright and digits
// remain text unless explicitly used in an emoji/keycap sequence.
inline size_t emoji_end(const std::string &text, size_t start, Character ch) {
  size_t end = start + ch.bytes;
  auto next = character_at(text, end);
  if (ch.value == '#' || ch.value == '*' ||
      (ch.value >= '0' && ch.value <= '9')) {
    if (next.value == 0xFE0F) {
      end += next.bytes;
      next = character_at(text, end);
    }
    return next.value == 0x20E3 ? end + next.bytes : start;
  }
  // These dual-use symbols are part of the existing text font. Other emoji
  // (including a bare rain cloud/heart) are omitted even without VS16.
  const bool text_symbol = ch.value == 0xA9 || ch.value == 0xAE || ch.value == 0x2122;
  if (!contains(EMOJI, ch.value) || next.value == 0xFE0E ||
      (text_symbol && next.value != 0xFE0F)) {
    return start;
  }
  for (;;) {
    next = character_at(text, end);
    if (next.value == 0xFE0F || next.value == 0xFE0E ||
        (next.value >= 0x1F3FB && next.value <= 0x1F3FF) ||
        (next.value >= 0xE0020 && next.value <= 0xE007F)) {
      end += next.bytes;
    } else if (next.value == 0x200D) {
      const auto joined = character_at(text, end + next.bytes);
      end += next.bytes;
      if (!joined.bytes || !contains(EMOJI, joined.value)) break;
      end += joined.bytes;
    } else {
      break;
    }
  }
  return end;
}

}  // namespace text_detail

// Display-only conversion. Never use this text for track identity, artwork
// lookup, source selection or Home Assistant actions. max_bytes is an output
// limit; only whole UTF-8 characters are emitted.
inline std::string normalize_media_display_text(const std::string &text,
                                               size_t max_bytes = std::string::npos) {
  using namespace text_detail;
  std::string output;
  output.reserve(text.size() < max_bytes ? text.size() : max_bytes);
  bool pending_space = false;
  for (size_t index = 0; index < text.size();) {
    const auto ch = character_at(text, index);
    if (!ch.bytes) { ++index; continue; }
    // Emoji presentation controls have no visible glyph, even when an input
    // limit or malformed source leaves them detached from their base.
    if (ch.value == 0xFE0E || ch.value == 0xFE0F || ch.value == 0x20E3 ||
        (ch.value >= 0xE0020 && ch.value <= 0xE007F)) {
      index += ch.bytes;
      continue;
    }
    const size_t end = emoji_end(text, index, ch);
    if (end != index) { index = end; continue; }
    uint32_t value = ch.value;
    if (value >= 0xFF01 && value <= 0xFF5E) value -= 0xFEE0;
    if (value == 0x3000 || value == 0xA0) value = ' ';
    if (value == ' ' || (value >= '\t' && value <= '\r')) {
      pending_space = !output.empty();
      index += ch.bytes;
      continue;
    }
    if (value < 0x20 || value == 0x7F) { index += ch.bytes; continue; }
    const size_t bytes = value < 0x80 ? 1 : ch.bytes;
    const size_t needed = bytes + (pending_space ? 1 : 0);
    if (needed > max_bytes - output.size()) break;
    if (pending_space) output.push_back(' ');
    pending_space = false;
    if (value < 0x80) output.push_back(static_cast<char>(value));
    else output.append(text, index, ch.bytes);
    index += ch.bytes;
    // Text-presentation selectors are invisible, not font glyphs.
    if (contains(EMOJI, ch.value)) {
      const auto selector = character_at(text, index);
      if (selector.value == 0xFE0E) index += selector.bytes;
    }
  }
  return output;
}

}  // namespace espcontrol::media
