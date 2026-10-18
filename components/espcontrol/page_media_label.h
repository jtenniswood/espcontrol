#pragma once

#include <algorithm>
#include <cstring>
#include <string_view>

#include "bounded_display_text.h"
#include "lvgl.h"
#include "media_display_text.h"

namespace espcontrol::pages {
inline size_t media_label_byte(std::string_view text, uint32_t character) {
  size_t byte = 0;
  while (character-- && byte < text.size()) byte += media::text_detail::character_at(text, byte).bytes;
  return byte;
}
// The caller configures font, width, spacing and position before this call.
// LVGL does not support native DOTS with static text. Compute the same cut using
// its public measurement APIs, then keep a pre-ellipsized fixed buffer in WRAP
// mode. A temporary text pointer is used only between synchronous getters and
// is always replaced before returning; no layout/refresh runs in that interval.
inline void write_media_label(lv_obj_t* label, char* storage, size_t capacity, std::string_view formatted,
                              uint8_t line_limit, bool geometry_ready = false) {
  if (!label || !storage || !capacity) return;
  char text[260];
  const auto copied = copy_display_text(text, sizeof(text) - 1, formatted, 256);
  size_t length = copied.bytes;
  if (lv_label_get_long_mode(label) != LV_LABEL_LONG_WRAP) lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  if (!geometry_ready) lv_obj_update_layout(label);
  const auto* font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
  const auto spacing = lv_obj_get_style_text_line_space(label, LV_PART_MAIN);
  const auto letter_spacing = lv_obj_get_style_text_letter_space(label, LV_PART_MAIN);
  lv_point_t size;
  lv_text_get_size(&size, text, font, letter_spacing, spacing, lv_obj_get_content_width(label),
                   LV_TEXT_FLAG_NONE);
  if (line_limit) {
    const auto maximum = std::max<int32_t>(1, font->line_height * line_limit + spacing * (line_limit - 1));
    if (!geometry_ready) {
      lv_obj_set_height(label, line_limit == 1 ? maximum : std::min<int32_t>(maximum, size.y));
      lv_obj_update_layout(label);
    }
    const auto height = lv_obj_get_content_height(label), width = lv_obj_get_content_width(label);
    size_t characters = 0;
    for (size_t byte = 0; byte < length; ++characters)
      byte += media::text_detail::character_at({text, length}, byte).bytes;
    if (size.y > height && size.y > font->line_height && characters > 3 && font->line_height + spacing > 0) {
      lv_point_t point{width - (lv_font_get_glyph_width(font, '.', '.') + letter_spacing) * 3, height};
      const auto remainder = point.y % (font->line_height + spacing);
      point.y -= remainder;
      point.y += remainder >= font->line_height ? font->line_height : -spacing;
      lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
      lv_label_set_text_static(label, text);
      auto byte = media_label_byte({text, length}, lv_label_get_letter_on(label, &point, false));
      while (byte + 3 > length && byte) {
        --byte;
        while (byte && (static_cast<unsigned char>(text[byte]) & 0xC0) == 0x80) --byte;
      }
      // Leave DOTS before any layout can execute; the final buffer owns dots.
      lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
      std::memcpy(text + byte, "...", 3);
      length = byte + 3;
      text[length] = '\0';
    }
  } else if (!geometry_ready)
    lv_obj_set_height(label, LV_SIZE_CONTENT);
  bool changed = false;
  copy_display_text(storage, capacity, {text, length}, SIZE_MAX, &changed);
  if (changed || lv_label_get_text(label) != storage) lv_label_set_text_static(label, storage);
}
}  // namespace espcontrol::pages
