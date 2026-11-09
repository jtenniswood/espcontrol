#pragma once

#include "button_grid_order_parser.h"

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Parse a 6-char hex color string (no # prefix) into a uint32_t RGB value
inline uint32_t parse_hex_color(const std::string &hex, bool &valid) {
  valid = hex.length() == 6;
  if (!valid) return 0;
  return strtoul(hex.c_str(), nullptr, 16);
}

inline int normalize_width_compensation_percent(int percent) {
  if (percent <= 0) return 100;
  if (percent < 50) return 50;
  if (percent > 150) return 150;
  return percent;
}

inline int width_compensation_scale(int percent) {
  percent = normalize_width_compensation_percent(percent);
  return 256 * percent / 100;
}

inline bool &width_compensation_vertical_axis() {
  static bool vertical = false;
  return vertical;
}

inline void set_width_compensation_vertical_axis(bool vertical) {
  width_compensation_vertical_axis() = vertical;
}

inline int &icon_width_compensation_percent() {
  static int percent = 100;
  return percent;
}

inline void set_icon_width_compensation_percent(int percent) {
  icon_width_compensation_percent() = normalize_width_compensation_percent(percent);
}

inline int &text_width_compensation_percent() {
  static int percent = 100;
  return percent;
}

inline void set_text_width_compensation_percent(int percent) {
  text_width_compensation_percent() = normalize_width_compensation_percent(percent);
}

inline lv_coord_t compensated_width(lv_coord_t width, int percent) {
  if (width_compensation_vertical_axis()) return width;
  percent = normalize_width_compensation_percent(percent);
  return width * percent / 100;
}

inline void apply_width_compensation(lv_obj_t *obj, int percent) {
  if (!obj) return;
  int scale = width_compensation_scale(percent);
  lv_obj_set_style_transform_scale_x(obj, width_compensation_vertical_axis() ? 256 : scale, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(obj, width_compensation_vertical_axis() ? scale : 256, LV_PART_MAIN);
}

inline void apply_text_width_compensation(lv_obj_t *obj) {
  apply_width_compensation(obj, text_width_compensation_percent());
}

inline void apply_icon_width_compensation(lv_obj_t *obj, int zoom = 256) {
  if (!obj) return;
  int compensated_zoom = zoom * icon_width_compensation_percent() / 100;
  lv_obj_set_style_transform_scale_x(
    obj, width_compensation_vertical_axis() ? zoom : compensated_zoom, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(
    obj, width_compensation_vertical_axis() ? compensated_zoom : zoom, LV_PART_MAIN);
}

inline void apply_slot_text_width_compensation(const BtnSlot &s, int percent) {
  (void) percent;
  apply_text_width_compensation(s.text_lbl);
  apply_text_width_compensation(s.sensor_container);
  apply_text_width_compensation(s.subpage_lbl);
}

inline lv_coord_t grid_div_round_closest(lv_coord_t dividend, int divisor) {
  if (divisor <= 0) return 0;
  return (dividend + divisor / 2) / divisor;
}

inline lv_coord_t grid_equal_fr_track_size(lv_coord_t usable, int track_count, int track_index) {
  if (track_count < 1) track_count = 1;
  if (track_index < 0) track_index = 0;
  if (track_index >= track_count) track_index = track_count - 1;
  lv_coord_t remaining_usable = usable;
  int remaining_tracks = track_count;
  for (int i = 0; i < track_count; i++) {
    lv_coord_t size = grid_div_round_closest(remaining_usable, remaining_tracks);
    if (i == track_index) return size;
    remaining_usable -= size;
    remaining_tracks--;
  }
  return 0;
}

inline lv_coord_t grid_track_span_size(lv_coord_t total_size,
                                       lv_coord_t pad_start,
                                       lv_coord_t pad_end,
                                       lv_coord_t gap,
                                       int track_count,
                                       int start,
                                       int span) {
  if (track_count < 1) track_count = 1;
  if (start < 0) start = 0;
  if (start >= track_count) start = track_count - 1;
  if (span < 1) span = 1;
  if (span > track_count - start) span = track_count - start;
  lv_coord_t usable = total_size - pad_start - pad_end - gap * (track_count - 1);
  if (usable <= 0) return 0;
  lv_coord_t size = gap * (span - 1);
  for (int offset = 0; offset < span; offset++) {
    size += grid_equal_fr_track_size(usable, track_count, start + offset);
  }
  return size;
}

inline void set_grid_card_cell(lv_obj_t *btn,
                               lv_obj_t *grid,
                               int col,
                               int row,
                               int col_span,
                               int row_span,
                               int cols,
                               int rows) {
  if (!btn) return;
  if (col_span < 1) col_span = 1;
  if (row_span < 1) row_span = 1;
  lv_grid_align_t col_align = col_span > 1 ? LV_GRID_ALIGN_START : LV_GRID_ALIGN_STRETCH;
  lv_grid_align_t row_align = row_span > 1 ? LV_GRID_ALIGN_START : LV_GRID_ALIGN_STRETCH;
  lv_obj_set_grid_cell(btn, col_align, col, col_span, row_align, row, row_span);

  if (!grid) return;
  if (card_span_is_single(row_span, col_span)) {
    clock_bar_unregister_responsive_grid_card(btn);
    return;
  }
  clock_bar_register_responsive_grid_card(
    grid, btn, col, row, col_span, row_span, cols, rows);
}

// ── Button visuals ────────────────────────────────────────────────────

// Apply on/off background colors to a button's checked/pressed/default states
inline void bind_card_text_contrast_events(lv_obj_t *btn);

inline void apply_button_colors(lv_obj_t *btn, bool has_on, uint32_t on_val,
                                bool has_off, uint32_t off_val) {
  if (has_on) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(on_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
    lv_obj_set_style_text_color(btn, lv_color_hex(display_text_color_for_bg(on_val)),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
    lv_obj_set_style_bg_color(btn, lv_color_hex(on_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
    lv_obj_set_style_text_color(btn, lv_color_hex(display_text_color_for_bg(on_val)),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  }
  if (has_off) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(off_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(btn, lv_color_hex(display_text_color_for_bg(off_val)),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  if (has_on || has_off) bind_card_text_contrast_events(btn);
}

inline void apply_button_on_pattern(lv_obj_t *btn, const std::string &options,
                                    bool has_on, uint32_t on_val) {
  if (!btn || !has_on) return;
  if (normalize_card_on_pattern(cfg_option_value(options, "on_pattern")) != "stripes") return;
  lv_style_selector_t checked =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
    static_cast<lv_style_selector_t>(LV_STATE_CHECKED);
  lv_style_selector_t pressed =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
    static_cast<lv_style_selector_t>(LV_STATE_PRESSED);
  lv_color_t highlight = lv_color_hex(card_pattern_highlight_color(on_val));
  lv_obj_set_style_bg_grad_color(btn, highlight, checked);
  lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, checked);
  lv_obj_set_style_bg_grad_color(btn, highlight, pressed);
  lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, pressed);
}

inline void apply_card_descendant_text_color(lv_obj_t *obj, lv_color_t color) {
  if (!obj) return;
  int32_t count = static_cast<int32_t>(lv_obj_get_child_cnt(obj));
  for (int32_t i = 0; i < count; i++) {
    lv_obj_t *child = lv_obj_get_child(obj, i);
    if (!child) continue;
    // A component-owned foreground (such as forecast temperatures) keeps its
    // colour while ordinary labels follow the card's pressed/checked contrast.
    if (!lv_obj_has_flag(child, LV_OBJ_FLAG_USER_2))
      lv_obj_set_style_text_color(child, color, LV_PART_MAIN);
    apply_card_descendant_text_color(child, color);
  }
}

inline void sync_card_checked_text_color(lv_obj_t *btn) {
  if (!btn) return;
  const lv_color_t text_color = lv_obj_get_style_text_color(btn, LV_PART_MAIN);
  apply_card_descendant_text_color(btn, text_color);
}

inline void card_text_contrast_event_cb(lv_event_t *event) {
  const auto code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sync_card_checked_text_color(static_cast<lv_obj_t *>(lv_event_get_target(event)));
  }
}

inline void bind_card_text_contrast_events(lv_obj_t *btn) {
  lv_obj_remove_event_cb_with_user_data(btn, card_text_contrast_event_cb, nullptr);
  lv_obj_add_event_cb(btn, card_text_contrast_event_cb, LV_EVENT_ALL, nullptr);
}

inline void set_card_checked_state(lv_obj_t *btn, bool checked) {
  if (!btn) return;
  if (checked) lv_obj_add_state(btn, LV_STATE_CHECKED);
  else lv_obj_clear_state(btn, LV_STATE_CHECKED);
  sync_card_checked_text_color(btn);
}

// Match the main-page button widget label behavior so longer titles wrap
// instead of running off the edge of the tile.
inline void configure_button_label_wrap(lv_obj_t *label) {
  if (!label) return;
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, lv_pct(100));
}

inline void set_wrapped_button_label_text(lv_obj_t *label, const std::string &text) {
  if (!label) return;
  configure_button_label_wrap(label);
  lv_label_set_display_text(label, text.c_str());
  lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

inline void set_subpage_chevron_visible(BtnSlot &s, bool visible,
                                        int x_offset = 0,
                                        int y_offset = 2,
                                        int text_width_percent = 94) {
  if (!s.subpage_lbl) return;
  (void) visible;
  (void) x_offset;
  (void) y_offset;
  (void) text_width_percent;
  lv_obj_add_flag(s.subpage_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.text_lbl) configure_button_label_wrap(s.text_lbl);
}
