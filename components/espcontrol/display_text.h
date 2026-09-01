#pragma once

#include <cstring>
#include <string>

#include "esphome/components/lvgl/lvgl_esphome.h"
#include "button_grid_string.h"

// LVGL copies label text, so a normalized temporary remains valid for the
// duration of this call. Avoid allocating for the overwhelmingly common case.
inline void lv_label_set_display_text(lv_obj_t *label, const char *text) {
  if (text == nullptr ||
      (std::strstr(text, "\xE1\xB9\xA2") == nullptr &&
       std::strstr(text, "\xE1\xB9\xA3") == nullptr)) {
    lv_label_set_text(label, text);
    return;
  }
  const std::string normalized = normalize_display_text(text);
  lv_label_set_text(label, normalized.c_str());
}
