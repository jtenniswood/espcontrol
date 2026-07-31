#ifndef ESPCONTROL_CLOCK_BAR_H
#define ESPCONTROL_CLOCK_BAR_H

#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "esphome/components/lvgl/lvgl_esphome.h"
#include "display_mode_controller.h"
#include "temperature_unit.h"

inline void format_clock_time_without_suffix(char *buf, size_t size,
                                             int hour, int minute,
                                             bool use_12h) {
  if (buf == nullptr || size == 0) return;
  if (use_12h) {
    int hour12 = hour % 12;
    if (hour12 == 0) hour12 = 12;
    snprintf(buf, size, "%d:%02d", hour12, minute);
  } else {
    snprintf(buf, size, "%02d:%02d", hour, minute);
  }
}

inline void format_fixed_decimal(char *buf, size_t size, float value, int precision) {
  if (size == 0) return;
  if (!std::isfinite(value)) {
    snprintf(buf, size, "--");
    return;
  }
  if (precision < 0) precision = 0;
  if (precision > 3) precision = 3;

  const bool negative = value < 0.0f;
  float abs_value = negative ? -value : value;
  int scale = 1;
  for (int i = 0; i < precision; i++) scale *= 10;
  int scaled = (int) (abs_value * scale + 0.5f);
  int whole = scaled / scale;
  int frac = scaled % scale;
  const char *sign = negative ? "-" : "";

  if (precision == 0) {
    snprintf(buf, size, "%s%d", sign, whole);
  } else if (precision == 1) {
    snprintf(buf, size, "%s%d.%01d", sign, whole, frac);
  } else if (precision == 2) {
    snprintf(buf, size, "%s%d.%02d", sign, whole, frac);
  } else {
    snprintf(buf, size, "%s%d.%03d", sign, whole, frac);
  }
}

inline void format_fixed_decimal_unit(char *buf, size_t size, float value,
                                      int precision, const char *unit) {
  char value_buf[24];
  format_fixed_decimal(value_buf, sizeof(value_buf), value, precision);
  snprintf(buf, size, "%s%s", value_buf, unit ? unit : "");
}

// ── Clock-bar page visibility and grid padding ─────────────────────────────

struct ClockBarVisibility {
  bool reserve_space = false;
  bool visible = false;
};

struct ClockBarResponsiveGridCard {
  lv_obj_t *page = nullptr;
  lv_obj_t *card = nullptr;
  int col = 0;
  int row = 0;
  int col_span = 1;
  int row_span = 1;
  int cols = 1;
  int rows = 1;
};

inline std::vector<ClockBarResponsiveGridCard> &clock_bar_responsive_grid_cards() {
  static std::vector<ClockBarResponsiveGridCard> cards;
  return cards;
}

inline lv_coord_t clock_bar_div_round_closest(lv_coord_t dividend, int divisor) {
  if (divisor <= 0) return 0;
  return (dividend + divisor / 2) / divisor;
}

inline lv_coord_t clock_bar_equal_fr_track_size(lv_coord_t usable,
                                                int track_count,
                                                int track_index) {
  if (track_count < 1) track_count = 1;
  if (track_index < 0) track_index = 0;
  if (track_index >= track_count) track_index = track_count - 1;
  lv_coord_t remaining_usable = usable;
  int remaining_tracks = track_count;
  for (int i = 0; i < track_count; i++) {
    lv_coord_t size = clock_bar_div_round_closest(remaining_usable, remaining_tracks);
    if (i == track_index) return size;
    remaining_usable -= size;
    remaining_tracks--;
  }
  return 0;
}

inline lv_coord_t clock_bar_grid_track_span_size(lv_coord_t total_size,
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
    size += clock_bar_equal_fr_track_size(usable, track_count, start + offset);
  }
  return size;
}

inline void clock_bar_apply_responsive_grid_card_size(
    const ClockBarResponsiveGridCard &entry) {
  if (!entry.page || !entry.card) return;
  if (entry.col_span <= 1 && entry.row_span <= 1) return;
  lv_obj_update_layout(entry.page);
  lv_coord_t width = clock_bar_grid_track_span_size(
      lv_obj_get_width(entry.page),
      lv_obj_get_style_pad_left(entry.page, LV_PART_MAIN),
      lv_obj_get_style_pad_right(entry.page, LV_PART_MAIN),
      lv_obj_get_style_pad_column(entry.page, LV_PART_MAIN),
      entry.cols,
      entry.col,
      entry.col_span);
  lv_coord_t height = clock_bar_grid_track_span_size(
      lv_obj_get_height(entry.page),
      lv_obj_get_style_pad_top(entry.page, LV_PART_MAIN),
      lv_obj_get_style_pad_bottom(entry.page, LV_PART_MAIN),
      lv_obj_get_style_pad_row(entry.page, LV_PART_MAIN),
      entry.rows,
      entry.row,
      entry.row_span);
  if (entry.col_span > 1 && width > 0) lv_obj_set_width(entry.card, width);
  if (entry.row_span > 1 && height > 0) lv_obj_set_height(entry.card, height);
}

inline void clock_bar_clear_responsive_grid_cards(lv_obj_t *page) {
  if (!page) return;
  std::vector<ClockBarResponsiveGridCard> &cards = clock_bar_responsive_grid_cards();
  cards.erase(
      std::remove_if(cards.begin(), cards.end(),
                     [page](const ClockBarResponsiveGridCard &entry) {
                       return entry.page == page;
                     }),
      cards.end());
}

inline void clock_bar_refresh_responsive_grid_cards(lv_obj_t *page = nullptr) {
  std::vector<ClockBarResponsiveGridCard> &cards = clock_bar_responsive_grid_cards();
  for (const ClockBarResponsiveGridCard &entry : cards) {
    if (page && entry.page != page) continue;
    clock_bar_apply_responsive_grid_card_size(entry);
  }
}

inline void clock_bar_register_responsive_grid_card(lv_obj_t *page,
                                                    lv_obj_t *card,
                                                    int col,
                                                    int row,
                                                    int col_span,
                                                    int row_span,
                                                    int cols,
                                                    int rows) {
  if (!page || !card) return;
  if (col_span <= 1 && row_span <= 1) return;
  ClockBarResponsiveGridCard next;
  next.page = page;
  next.card = card;
  next.col = col;
  next.row = row;
  next.col_span = col_span;
  next.row_span = row_span;
  next.cols = cols;
  next.rows = rows;

  std::vector<ClockBarResponsiveGridCard> &cards = clock_bar_responsive_grid_cards();
  for (ClockBarResponsiveGridCard &entry : cards) {
    if (entry.card == card) {
      entry = next;
      clock_bar_apply_responsive_grid_card_size(entry);
      return;
    }
  }
  cards.push_back(next);
  clock_bar_apply_responsive_grid_card_size(cards.back());
}

inline std::vector<lv_obj_t *> &clock_bar_button_grid_pages() {
  static std::vector<lv_obj_t *> pages;
  return pages;
}

inline void clock_bar_clear_button_grid_pages() {
  for (lv_obj_t *page : clock_bar_button_grid_pages()) {
    clock_bar_clear_responsive_grid_cards(page);
  }
  clock_bar_button_grid_pages().clear();
}

inline void clock_bar_register_button_grid_page(lv_obj_t *page) {
  if (!page) return;
  std::vector<lv_obj_t *> &pages = clock_bar_button_grid_pages();
  if (std::find(pages.begin(), pages.end(), page) == pages.end()) {
    pages.push_back(page);
  }
}

inline void clock_bar_set_button_grid_pages_pad_top(lv_obj_t *main_page_obj,
                                                    lv_coord_t pad_top) {
  if (main_page_obj) {
    lv_obj_set_style_pad_top(main_page_obj, pad_top, LV_PART_MAIN);
    lv_obj_update_layout(main_page_obj);
  }
  std::vector<lv_obj_t *> &pages = clock_bar_button_grid_pages();
  for (lv_obj_t *page : pages) {
    if (!page || page == main_page_obj) continue;
    lv_obj_set_style_pad_top(page, pad_top, LV_PART_MAIN);
    lv_obj_update_layout(page);
  }
  clock_bar_refresh_responsive_grid_cards();
}

inline bool clock_bar_active_on_button_grid_page(lv_obj_t *main_page_obj = nullptr) {
  lv_obj_t *active = lv_scr_act();
  if (!active) return false;
  if (main_page_obj && active == main_page_obj) return true;
  std::vector<lv_obj_t *> &pages = clock_bar_button_grid_pages();
  return std::find(pages.begin(), pages.end(), active) != pages.end();
}

inline ClockBarVisibility clock_bar_resolve_visibility(
    bool enabled,
    lv_obj_t *main_page_obj,
    espcontrol::DisplayMode display_mode,
    bool schedule_inactive) {
  ClockBarVisibility result;
  // Full-screen screensavers hide the clock bar, but the grid should keep the
  // same top padding so waking does not briefly resize the cards.
  result.reserve_space = enabled && !schedule_inactive;
  result.visible = result.reserve_space &&
      display_mode == espcontrol::DisplayMode::ACTIVE &&
      clock_bar_active_on_button_grid_page(main_page_obj);
  return result;
}

inline bool clock_bar_should_reserve_space(
    bool enabled,
    lv_obj_t *main_page_obj,
    espcontrol::DisplayMode display_mode,
    bool schedule_inactive) {
  return clock_bar_resolve_visibility(
      enabled,
      main_page_obj,
      display_mode,
      schedule_inactive).reserve_space;
}

inline bool clock_bar_should_show(
    bool enabled,
    lv_obj_t *main_page_obj,
    espcontrol::DisplayMode display_mode,
    bool schedule_inactive) {
  return clock_bar_resolve_visibility(
      enabled,
      main_page_obj,
      display_mode,
      schedule_inactive).visible;
}

// ── Temperature labels ─────────────────────────────────────────────────────

inline void format_clock_bar_temperature_single(char *buf, size_t size,
                                                const char *value_text) {
  snprintf(buf, size, "%s%s", value_text ? value_text : "-",
           display_clock_bar_temperature_suffix());
}

inline std::vector<float> &clock_bar_temperature_values() {
  static std::vector<float> values;
  return values;
}

inline std::vector<lv_obj_t *> &clock_bar_temperature_labels() {
  static std::vector<lv_obj_t *> labels;
  return labels;
}

inline void set_clock_bar_temperature_labels(lv_obj_t **labels, size_t count) {
  std::vector<lv_obj_t *> &out = clock_bar_temperature_labels();
  out.clear();
  for (size_t i = 0; labels && i < count; i++) {
    out.push_back(labels[i]);
  }
}

inline void clock_bar_set_widget_hidden(lv_obj_t *obj, bool hidden) {
  if (!obj) return;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

inline void hide_clock_bar_top_layer_widgets(lv_obj_t **temperature_labels,
                                             size_t temperature_label_count,
                                             lv_obj_t *display_time,
                                             lv_obj_t *network_status_button) {
  set_clock_bar_temperature_labels(temperature_labels, temperature_label_count);
  for (size_t i = 0; temperature_labels && i < temperature_label_count; i++) {
    clock_bar_set_widget_hidden(temperature_labels[i], true);
  }
  clock_bar_set_widget_hidden(display_time, true);
  clock_bar_set_widget_hidden(network_status_button, true);
}

inline void set_clock_bar_temperature_value_count(size_t count) {
  clock_bar_temperature_values().assign(count, NAN);
}

inline bool clock_bar_temperature_has_items() {
  return !clock_bar_temperature_values().empty();
}

inline std::string clock_bar_trim(const std::string &value) {
  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
    start++;
  }
  size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end--;
  }
  return value.substr(start, end - start);
}

inline std::vector<std::string> parse_clock_bar_temperature_entities(const std::string &value) {
  std::vector<std::string> entities;
  std::string current;
  for (char ch : value) {
    if (ch == '|' || ch == ',' || ch == '\n') {
      std::string entity = clock_bar_trim(current);
      if (!entity.empty() && std::find(entities.begin(), entities.end(), entity) == entities.end()) {
        entities.push_back(entity);
      }
      current.clear();
      if (entities.size() >= 1) return entities;
    } else {
      current.push_back(ch);
    }
  }
  std::string entity = clock_bar_trim(current);
  if (!entity.empty() && std::find(entities.begin(), entities.end(), entity) == entities.end()) {
    entities.push_back(entity);
  }
  if (entities.size() > 1) {
    entities.resize(1);
  }
  return entities;
}

inline void format_clock_bar_temperature_list(char *buf, size_t size,
                                              const std::vector<float> &values) {
  if (size == 0) return;
  buf[0] = '\0';
  const char *suffix = display_clock_bar_temperature_suffix();
  size_t used = 0;
  for (size_t i = 0; i < values.size(); i++) {
    char value_buf[16];
    if (std::isnan(values[i])) snprintf(value_buf, sizeof(value_buf), "-");
    else format_fixed_decimal(value_buf, sizeof(value_buf), values[i], 0);
    int written = snprintf(buf + used, size - used, "%s%s%s",
                           i == 0 ? "" : " / ", value_buf, suffix);
    if (written < 0) break;
    if ((size_t) written >= size - used) {
      buf[size - 1] = '\0';
      break;
    }
    used += (size_t) written;
  }
}

inline void refresh_clock_bar_temperature_label_values(
    lv_obj_t *main_page_obj, bool clock_bar_visible,
    bool indoor_enabled, bool outdoor_enabled,
    float indoor, float outdoor) {
  const bool show_on_screen =
      clock_bar_visible && clock_bar_active_on_button_grid_page(main_page_obj);
  std::vector<lv_obj_t *> &labels = clock_bar_temperature_labels();

  if (!clock_bar_temperature_has_items()) {
    if (!show_on_screen || (!indoor_enabled && !outdoor_enabled)) {
      for (lv_obj_t *label : labels) clock_bar_set_widget_hidden(label, true);
      return;
    }

    size_t label_index = 0;
    auto set_legacy_temperature = [&](float value) {
      if (label_index >= labels.size()) return;
      if (label_index >= 1) return;
      lv_obj_t *label = labels[label_index++];
      if (!label) return;
      char value_buf[16];
      if (std::isnan(value)) snprintf(value_buf, sizeof(value_buf), "-");
      else format_fixed_decimal(value_buf, sizeof(value_buf), value, 0);
      char buf[24];
      format_clock_bar_temperature_single(buf, sizeof(buf), value_buf);
      lv_label_set_text(label, buf);
      clock_bar_set_widget_hidden(label, !show_on_screen);
    };
    if (outdoor_enabled) set_legacy_temperature(outdoor);
    if (indoor_enabled) set_legacy_temperature(indoor);
    for (size_t i = label_index; i < labels.size(); i++) {
      clock_bar_set_widget_hidden(labels[i], true);
    }
    return;
  }

  if (!show_on_screen || !outdoor_enabled) {
    for (lv_obj_t *label : labels) clock_bar_set_widget_hidden(label, true);
    return;
  }

  std::vector<float> &values = clock_bar_temperature_values();
  for (size_t i = 0; i < labels.size(); i++) {
    lv_obj_t *label = labels[i];
    if (!label) continue;
    if (i >= values.size()) {
      clock_bar_set_widget_hidden(label, true);
      continue;
    }
    char value_buf[16];
    if (std::isnan(values[i])) snprintf(value_buf, sizeof(value_buf), "-");
    else format_fixed_decimal(value_buf, sizeof(value_buf), values[i], 0);
    char buf[24];
    format_clock_bar_temperature_single(buf, sizeof(buf), value_buf);
    lv_label_set_text(label, buf);
    clock_bar_set_widget_hidden(label, false);
  }
}

// ── Fixed clock-bar placement ───────────────────────────────────────────────

inline lv_coord_t clock_bar_current_screen_width(lv_coord_t fallback) {
  lv_disp_t *disp = lv_disp_get_default();
  lv_coord_t width = disp ? lv_disp_get_hor_res(disp) : 0;
  return width > 0 ? width : fallback;
}

inline lv_coord_t clock_bar_current_screen_height(lv_coord_t fallback) {
  lv_disp_t *disp = lv_disp_get_default();
  lv_coord_t height = disp ? lv_disp_get_ver_res(disp) : 0;
  return height > 0 ? height : fallback;
}

inline void clock_bar_prepare_text_label(lv_obj_t *obj, int width,
                                         lv_text_align_t align) {
  if (!obj) return;
  lv_obj_set_width(obj, width);
  lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(obj, align, LV_PART_MAIN);
}

inline void apply_clock_bar_fixed_layout(lv_obj_t *temperature_label,
                                         lv_obj_t *display_time,
                                         lv_obj_t *network_status_button,
                                         bool temperature_visible,
                                         bool time_visible,
                                         bool network_visible,
                                         int left_x, int label_y,
                                         int right_x, int network_y,
                                         int item_gap) {
  int temperature_width = item_gap - 8;
  if (temperature_width < 56) temperature_width = 56;
  if (temperature_width > 88) temperature_width = 88;

  int time_width = item_gap;
  if (time_width < 62) time_width = 62;
  if (time_width > 96) time_width = 96;

  clock_bar_prepare_text_label(
      temperature_label, temperature_width, LV_TEXT_ALIGN_LEFT);
  clock_bar_prepare_text_label(display_time, time_width, LV_TEXT_ALIGN_CENTER);

  clock_bar_set_widget_hidden(temperature_label, !temperature_visible);
  clock_bar_set_widget_hidden(display_time, !time_visible);
  clock_bar_set_widget_hidden(network_status_button, !network_visible);

  if (temperature_label) {
    lv_obj_align(temperature_label, LV_ALIGN_TOP_LEFT, left_x, label_y);
    lv_obj_move_background(temperature_label);
  }
  if (display_time) {
    lv_obj_align(display_time, LV_ALIGN_TOP_MID, 0, label_y);
    lv_obj_move_background(display_time);
  }
  if (network_status_button) {
    lv_obj_align(network_status_button, LV_ALIGN_TOP_RIGHT, -right_x, network_y);
    lv_obj_move_background(network_status_button);
  }
}

#endif
