#pragma once

#include "pin_keypad_order.h"

// Shared alarm/screensaver PIN layout. Callers own submission and modal lifetime.
inline lv_obj_t *pin_keypad_create_button(lv_obj_t *parent, lv_coord_t width,
                                         lv_coord_t height,
                                         const char *text,
                                         const lv_font_t *font,
                                         int width_compensation_percent,
                                         uint16_t label_zoom = 256) {
  lv_coord_t radius = width < height ? width / 2 : height / 2;
  lv_obj_t *btn = control_modal_create_round_button(
    parent, width, text, font, theme_display_color(current_theme().border), theme_display_color(current_theme().surface_primary),
    width_compensation_percent);
  lv_obj_set_size(btn, width, height);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label && label_zoom != 256) {
    lv_obj_update_layout(label);
    lv_coord_t offset_x = lv_obj_get_width(label) * (256 - label_zoom) / 512;
    lv_coord_t offset_y = lv_obj_get_height(label) * (256 - label_zoom) / 512;
    lv_obj_set_style_transform_zoom(label, label_zoom, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, offset_x, offset_y);
  }
  return btn;
}

inline lv_coord_t pin_keypad_gap(lv_coord_t keypad_w, lv_coord_t keypad_h,
                                       lv_coord_t short_side) {
  lv_coord_t gap = control_modal_scaled_px(14, short_side);
  lv_coord_t max_gap_w = keypad_w > 0 ? keypad_w / 16 : 0;
  lv_coord_t max_gap_h = keypad_h > 0 ? keypad_h / 20 : 0;
  if (max_gap_w > 0 && gap > max_gap_w) gap = max_gap_w;
  if (max_gap_h > 0 && gap > max_gap_h) gap = max_gap_h;
  if (gap < 4) gap = 4;
  return gap;
}

inline lv_coord_t pin_keypad_key_size(lv_coord_t keypad_w, lv_coord_t keypad_h,
                                     lv_coord_t gap, lv_coord_t short_side) {
  lv_coord_t key_size_w = (keypad_w - gap * 2) / 3;
  lv_coord_t key_size_h = (keypad_h - gap * 3) / 4;
  lv_coord_t key_size = key_size_w < key_size_h ? key_size_w : key_size_h;
  lv_coord_t max_key_size = control_modal_scaled_px(112, short_side);
  if (max_key_size < 64) max_key_size = 64;
  if (key_size > max_key_size) key_size = max_key_size;
  if (key_size < 24) key_size = 24;
  return key_size;
}

inline lv_coord_t pin_keypad_label_y(const ControlModalLayout &layout, lv_coord_t pin_h) {
  lv_coord_t pin_y = layout.inset + (layout.back_size - pin_h) / 2;
  if (control_modal_uses_compact_portrait_tuning(layout)) {
    pin_y += control_modal_scaled_px(16, layout.short_side);
  }
  if (pin_y < layout.inset) pin_y = layout.inset;
  return pin_y;
}

inline lv_obj_t *pin_keypad_create(lv_obj_t *panel, const ControlModalLayout &layout,
                                        const lv_font_t *label_font,
                                        const lv_font_t *key_label_font,
                                        const lv_font_t *icon_font,
                                        int width_compensation_percent,
                                        lv_event_cb_t key_cb) {
  lv_obj_t *pin_lbl = nullptr;
  pin_lbl = lv_label_create(panel);
  lv_obj_set_style_text_color(pin_lbl, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
  lv_obj_set_style_text_align(pin_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(pin_lbl, label_font, LV_PART_MAIN);
  apply_text_width_compensation(pin_lbl);
  lv_coord_t pin_w = layout.panel_w - (layout.inset + layout.back_size) * 2;
  if (pin_w < 60) pin_w = layout.panel_w - layout.inset * 2;
  lv_obj_set_width(pin_lbl, pin_w);
  lv_label_set_display_text(pin_lbl, espcontrol_i18n("Enter Pin"));
  lv_obj_update_layout(pin_lbl);
  lv_coord_t pin_h = lv_obj_get_height(pin_lbl);
  lv_coord_t pin_y = pin_keypad_label_y(layout, pin_h);
  lv_obj_align(pin_lbl, LV_ALIGN_TOP_MID, 0, pin_y);

  lv_coord_t pin_button_gap = control_modal_scaled_px(24, layout.short_side);
  if (pin_button_gap < 10) pin_button_gap = 10;
  lv_coord_t keypad_top = layout.inset + layout.back_size + pin_button_gap;
  lv_coord_t keypad_bottom = layout.panel_h - layout.inset;
  lv_coord_t keypad_w = layout.panel_w - layout.inset * 2;
  lv_coord_t keypad_h = keypad_bottom - keypad_top;
  lv_coord_t gap = pin_keypad_gap(keypad_w, keypad_h, layout.short_side);
  lv_coord_t key_size = pin_keypad_key_size(keypad_w, keypad_h, gap, layout.short_side);
  lv_coord_t total_w = key_size * 3 + gap * 2;
  lv_coord_t total_h = key_size * 4 + gap * 3;
  lv_coord_t start_x = (layout.panel_w - total_w) / 2;
  lv_coord_t start_y = keypad_top + (keypad_h - total_h) / 2;
  if (start_y < keypad_top) start_y = keypad_top;

  const char *key_data[12] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9",
    "back", "0", "submit",
  };

  // Every opening gets a new Fisher-Yates permutation; clear/submit stay fixed.
  const auto digits = scrambled_pin_digits([]() { return esphome::random_uint32(); });
  static const char *digit_text[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
  for (int i = 0; i < 10; ++i) key_data[i < 9 ? i : 10] = digit_text[digits[i]];

  for (int i = 0; i < 12; i++) {
    const char *text = key_data[i];
    const lv_font_t *key_font = key_label_font;
    uint16_t key_zoom = 256;
    if (strcmp(text, "back") == 0) {
      text = "\U000F0156";
      key_font = icon_font;
      key_zoom = 170;
    } else if (strcmp(text, "submit") == 0) {
      text = find_icon("Check");
      key_font = icon_font;
      key_zoom = 170;
    }

    lv_obj_t *key_btn = pin_keypad_create_button(
      panel, key_size, key_size, text, key_font,
      width_compensation_percent, key_zoom);
    if (strcmp(key_data[i], "submit") == 0) {
      lv_obj_set_style_bg_color(key_btn, lv_color_hex(DEFAULT_ACCENT_COLOR), LV_PART_MAIN);
      lv_obj_set_style_border_color(key_btn, lv_color_hex(DEFAULT_ACCENT_COLOR), LV_PART_MAIN);
      lv_obj_t *key_lbl = lv_obj_get_child(key_btn, 0);
      if (key_lbl) lv_obj_set_style_text_color(key_lbl, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
    }
    int row = i / 3;
    int col = i % 3;
    lv_coord_t x = start_x + col * (key_size + gap);
    lv_coord_t y = start_y + row * (key_size + gap);
    lv_obj_set_pos(key_btn, x, y);
    lv_obj_add_event_cb(key_btn, key_cb, LV_EVENT_CLICKED,
      const_cast<char *>(key_data[i]));
  }

  return pin_lbl;
}
