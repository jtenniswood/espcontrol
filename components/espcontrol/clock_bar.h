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
#include "temperature_unit.h"

static const size_t CLOCK_BAR_TEMPERATURE_SLOT_COUNT = 6;
static const size_t CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT = 1;

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

inline bool clock_bar_blocked_by_overlay(bool display_asleep,
                                         bool screen_schedule_asleep,
                                         bool clock_screensaver_active,
                                         bool cover_art_screensaver_active,
                                         bool display_off_screensaver_active,
                                         bool dimmed_screensaver_active) {
  return display_asleep ||
         screen_schedule_asleep ||
         clock_screensaver_active ||
         cover_art_screensaver_active ||
         display_off_screensaver_active ||
         dimmed_screensaver_active;
}

inline ClockBarVisibility clock_bar_resolve_visibility(
    bool enabled,
    lv_obj_t *main_page_obj,
    bool display_asleep,
    bool screen_schedule_asleep,
    bool clock_screensaver_active,
    bool cover_art_screensaver_active,
    bool display_off_screensaver_active,
    bool dimmed_screensaver_active) {
  ClockBarVisibility result;
  // Full-screen screensavers hide the clock bar, but the grid should keep the
  // same top padding so waking does not briefly resize the cards.
  result.reserve_space = enabled && !screen_schedule_asleep;
  result.visible = result.reserve_space &&
      !clock_bar_blocked_by_overlay(
          display_asleep,
          screen_schedule_asleep,
          clock_screensaver_active,
          cover_art_screensaver_active,
          display_off_screensaver_active,
          dimmed_screensaver_active) &&
      clock_bar_active_on_button_grid_page(main_page_obj);
  return result;
}

inline bool clock_bar_should_reserve_space(
    bool enabled,
    lv_obj_t *main_page_obj,
    bool display_asleep,
    bool screen_schedule_asleep,
    bool clock_screensaver_active,
    bool cover_art_screensaver_active,
    bool display_off_screensaver_active,
    bool dimmed_screensaver_active) {
  return clock_bar_resolve_visibility(
      enabled,
      main_page_obj,
      display_asleep,
      screen_schedule_asleep,
      clock_screensaver_active,
      cover_art_screensaver_active,
      display_off_screensaver_active,
      dimmed_screensaver_active).reserve_space;
}

inline bool clock_bar_should_show(
    bool enabled,
    lv_obj_t *main_page_obj,
    bool display_asleep,
    bool screen_schedule_asleep,
    bool clock_screensaver_active,
    bool cover_art_screensaver_active,
    bool display_off_screensaver_active,
    bool dimmed_screensaver_active) {
  return clock_bar_resolve_visibility(
      enabled,
      main_page_obj,
      display_asleep,
      screen_schedule_asleep,
      clock_screensaver_active,
      cover_art_screensaver_active,
      display_off_screensaver_active,
      dimmed_screensaver_active).visible;
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
  for (size_t i = 0; labels && i < count && i < CLOCK_BAR_TEMPERATURE_SLOT_COUNT; i++) {
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
      if (entities.size() >= CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT) return entities;
    } else {
      current.push_back(ch);
    }
  }
  std::string entity = clock_bar_trim(current);
  if (!entity.empty() && std::find(entities.begin(), entities.end(), entity) == entities.end()) {
    entities.push_back(entity);
  }
  if (entities.size() > CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT) {
    entities.resize(CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT);
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
      if (label_index >= CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT) return;
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

// ── Saved layout parsing and placement ─────────────────────────────────────

enum ClockBarItemId {
  CLOCK_BAR_ITEM_TEMPERATURE = 0,
  CLOCK_BAR_ITEM_TIME = CLOCK_BAR_TEMPERATURE_SLOT_COUNT,
  CLOCK_BAR_ITEM_NETWORK = CLOCK_BAR_TEMPERATURE_SLOT_COUNT + 1,
  CLOCK_BAR_ITEM_COUNT = CLOCK_BAR_TEMPERATURE_SLOT_COUNT + 2,
};

enum ClockBarSectionId {
  CLOCK_BAR_SECTION_LEFT = 0,
  CLOCK_BAR_SECTION_MIDDLE = 1,
  CLOCK_BAR_SECTION_RIGHT = 2,
  CLOCK_BAR_SECTION_COUNT = 3,
};

inline int clock_bar_section_id(const std::string &value, size_t start, size_t end);
inline int clock_bar_item_id(const std::string &value, size_t start, size_t end);

struct ClockBarParsedLayout {
  int section[CLOCK_BAR_ITEM_COUNT];
  int order[CLOCK_BAR_ITEM_COUNT];
  int count[CLOCK_BAR_SECTION_COUNT];
};

inline void clock_bar_clear_layout(ClockBarParsedLayout &layout) {
  for (int i = 0; i < CLOCK_BAR_ITEM_COUNT; i++) {
    layout.section[i] = -1;
    layout.order[i] = 0;
  }
  for (int i = 0; i < CLOCK_BAR_SECTION_COUNT; i++) layout.count[i] = 0;
}

inline int clock_bar_section_id(const std::string &value) {
  return clock_bar_section_id(value, 0, value.size());
}

inline void clock_bar_trim_span(const std::string &value, size_t &start, size_t &end) {
  while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) start++;
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) end--;
}

inline bool clock_bar_span_equals(const std::string &value, size_t start, size_t end, const char *text) {
  clock_bar_trim_span(value, start, end);
  const size_t len = strlen(text);
  return end - start == len && value.compare(start, len, text) == 0;
}

inline int clock_bar_section_id(const std::string &value, size_t start, size_t end) {
  if (clock_bar_span_equals(value, start, end, "left")) return CLOCK_BAR_SECTION_LEFT;
  if (clock_bar_span_equals(value, start, end, "middle")) return CLOCK_BAR_SECTION_MIDDLE;
  if (clock_bar_span_equals(value, start, end, "right")) return CLOCK_BAR_SECTION_RIGHT;
  return -1;
}

inline int clock_bar_item_id(const std::string &value) {
  return clock_bar_item_id(value, 0, value.size());
}

inline int clock_bar_item_id(const std::string &value, size_t start, size_t end) {
  if (clock_bar_span_equals(value, start, end, "temperature")) return CLOCK_BAR_ITEM_TEMPERATURE;
  if (clock_bar_span_equals(value, start, end, "time")) return CLOCK_BAR_ITEM_TIME;
  if (clock_bar_span_equals(value, start, end, "network")) return CLOCK_BAR_ITEM_NETWORK;

  clock_bar_trim_span(value, start, end);
  static const char prefix[] = "temperature_";
  const size_t prefix_len = sizeof(prefix) - 1;
  if (end - start <= prefix_len || value.compare(start, prefix_len, prefix) != 0) return -1;

  int slot = 0;
  for (size_t i = start + prefix_len; i < end; i++) {
    if (value[i] < '0' || value[i] > '9') return -1;
    slot = slot * 10 + (value[i] - '0');
  }
  if (slot >= 2 && slot <= (int) CLOCK_BAR_TEMPERATURE_SLOT_COUNT) {
    return CLOCK_BAR_ITEM_TEMPERATURE + slot - 1;
  }
  return -1;
}

inline void clock_bar_add_item(ClockBarParsedLayout &layout, int section, int item) {
  if (section < 0 || section >= CLOCK_BAR_SECTION_COUNT ||
      item < 0 || item >= CLOCK_BAR_ITEM_COUNT ||
      layout.section[item] >= 0) {
    return;
  }
  layout.section[item] = section;
  layout.order[item] = layout.count[section]++;
}

inline void clock_bar_add_missing_default_items(ClockBarParsedLayout &layout) {
  clock_bar_add_item(layout, CLOCK_BAR_SECTION_LEFT, CLOCK_BAR_ITEM_TEMPERATURE);
  clock_bar_add_item(layout, CLOCK_BAR_SECTION_MIDDLE, CLOCK_BAR_ITEM_TIME);
  clock_bar_add_item(layout, CLOCK_BAR_SECTION_RIGHT, CLOCK_BAR_ITEM_NETWORK);
}

inline ClockBarParsedLayout parse_clock_bar_layout(const std::string &layout_text) {
  ClockBarParsedLayout layout;
  clock_bar_clear_layout(layout);

  size_t segment_start = 0;
  while (segment_start <= layout_text.size()) {
    size_t segment_end = layout_text.find('|', segment_start);
    if (segment_end == std::string::npos) segment_end = layout_text.size();
    size_t colon = layout_text.find(':', segment_start);
    if (colon != std::string::npos && colon < segment_end) {
      int section = clock_bar_section_id(layout_text, segment_start, colon);
      size_t item_start = colon + 1;
      while (section >= 0 && item_start <= segment_end) {
        size_t item_end = layout_text.find(',', item_start);
        if (item_end == std::string::npos || item_end > segment_end) item_end = segment_end;
        clock_bar_add_item(
            layout,
            section,
            clock_bar_item_id(layout_text, item_start, item_end));
        if (item_end == segment_end) break;
        item_start = item_end + 1;
      }
    }
    if (segment_end == layout_text.size()) break;
    segment_start = segment_end + 1;
  }

  clock_bar_add_missing_default_items(layout);
  return layout;
}

inline void align_clock_bar_widget(lv_obj_t *obj, int section, int order, int count,
                                   int left_x, int y, int right_x, int item_gap) {
  if (!obj) return;
  if (section == CLOCK_BAR_SECTION_LEFT) {
    lv_obj_align(obj, LV_ALIGN_TOP_LEFT, left_x + order * item_gap, y);
  } else if (section == CLOCK_BAR_SECTION_MIDDLE) {
    int x = ((order * 2) - (count - 1)) * item_gap / 2;
    lv_obj_align(obj, LV_ALIGN_TOP_MID, x, y);
  } else if (section == CLOCK_BAR_SECTION_RIGHT) {
    int x = -(right_x + (count - 1 - order) * item_gap);
    lv_obj_align(obj, LV_ALIGN_TOP_RIGHT, x, y);
  }
}

inline bool clock_bar_item_is_temperature(int item) {
  return item >= CLOCK_BAR_ITEM_TEMPERATURE &&
         item < CLOCK_BAR_ITEM_TEMPERATURE + (int) CLOCK_BAR_TEMPERATURE_SLOT_COUNT;
}

inline int clock_bar_item_text_box_width(int item, int item_gap) {
  if (clock_bar_item_is_temperature(item)) {
    int width = item_gap - 8;
    if (width < 56) width = 56;
    if (width > 88) width = 88;
    return width;
  }
  if (item == CLOCK_BAR_ITEM_TIME) {
    int width = item_gap;
    if (width < 62) width = 62;
    if (width > 96) width = 96;
    return width;
  }
  return 0;
}

struct ClockBarLayoutBox {
  lv_obj_t *obj = nullptr;
  int item = -1;
  int section = -1;
  int order = 0;
  int width = 0;
  int y = 0;
};

inline int clock_bar_visual_gap_px(int gap) {
  if (gap < 0) return 0;
  if (gap > 32) return 32;
  return gap;
}

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

inline int clock_bar_icon_fallback_width(int item_gap) {
  int width = item_gap / 2;
  if (width < 38) width = 38;
  if (width > 48) width = 48;
  return width;
}

inline int clock_bar_measure_item_width(lv_obj_t *obj, int item, int item_gap) {
  if (!obj) return 0;
  int text_width = clock_bar_item_text_box_width(item, item_gap);
  if (text_width > 0) {
    lv_obj_set_width(obj, text_width);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    return text_width;
  }

  lv_obj_update_layout(obj);
  int width = lv_obj_get_width(obj);
  if (width <= 0) width = clock_bar_icon_fallback_width(item_gap);
  return width;
}

inline void clock_bar_add_layout_box(ClockBarLayoutBox *boxes,
                                     int &box_count,
                                     const ClockBarParsedLayout &layout,
                                     lv_obj_t *obj,
                                     int item,
                                     int y,
                                     int item_gap) {
  if (!obj || !boxes || box_count >= CLOCK_BAR_ITEM_COUNT) return;
  if (item < 0 || item >= CLOCK_BAR_ITEM_COUNT) return;
  int section = layout.section[item];
  if (section < 0 || section >= CLOCK_BAR_SECTION_COUNT) return;

  ClockBarLayoutBox &box = boxes[box_count++];
  box.obj = obj;
  box.item = item;
  box.section = section;
  box.order = layout.order[item];
  box.width = clock_bar_measure_item_width(obj, item, item_gap);
  box.y = y;
}

inline void clock_bar_align_box_text(const ClockBarLayoutBox &box) {
  if (!box.obj || box.item == CLOCK_BAR_ITEM_NETWORK) return;
  lv_text_align_t align = LV_TEXT_ALIGN_CENTER;
  if (box.section == CLOCK_BAR_SECTION_LEFT) align = LV_TEXT_ALIGN_LEFT;
  else if (box.section == CLOCK_BAR_SECTION_RIGHT) align = LV_TEXT_ALIGN_RIGHT;
  lv_obj_set_style_text_align(box.obj, align, LV_PART_MAIN);
}

inline ClockBarLayoutBox *clock_bar_box_at_order(ClockBarLayoutBox *boxes,
                                                 int box_count,
                                                 int section,
                                                 int order) {
  for (int i = 0; i < box_count; i++) {
    if (boxes[i].section == section && boxes[i].order == order) return &boxes[i];
  }
  return nullptr;
}

inline int clock_bar_section_box_count(ClockBarLayoutBox *boxes,
                                       int box_count,
                                       int section) {
  int count = 0;
  for (int i = 0; i < box_count; i++) {
    if (boxes[i].section == section) count++;
  }
  return count;
}

inline int clock_bar_section_width(ClockBarLayoutBox *boxes,
                                   int box_count,
                                   int section,
                                   int visual_gap) {
  int width = 0;
  int count = 0;
  for (int order = 0; order < CLOCK_BAR_ITEM_COUNT; order++) {
    ClockBarLayoutBox *box = clock_bar_box_at_order(boxes, box_count, section, order);
    if (!box) continue;
    if (count > 0) width += visual_gap;
    width += box->width;
    count++;
  }
  return width;
}

inline int clock_bar_section_start_x(ClockBarLayoutBox *boxes,
                                     int box_count,
                                     int section,
                                     int screen_width,
                                     int left_x,
                                     int right_x,
                                     int visual_gap) {
  int total_width = clock_bar_section_width(boxes, box_count, section, visual_gap);
  if (section == CLOCK_BAR_SECTION_LEFT) return left_x;
  if (section == CLOCK_BAR_SECTION_RIGHT) return screen_width - right_x - total_width;
  if (section == CLOCK_BAR_SECTION_MIDDLE) return (screen_width - total_width) / 2;
  return left_x;
}

inline void align_clock_bar_layout_section(ClockBarLayoutBox *boxes,
                                           int box_count,
                                           int section,
                                           int screen_width,
                                           int left_x,
                                           int right_x,
                                           int visual_gap) {
  int x = clock_bar_section_start_x(
      boxes, box_count, section, screen_width, left_x, right_x, visual_gap);
  int placed = 0;
  int expected = clock_bar_section_box_count(boxes, box_count, section);
  for (int order = 0; placed < expected && order < CLOCK_BAR_ITEM_COUNT; order++) {
    ClockBarLayoutBox *box = clock_bar_box_at_order(boxes, box_count, section, order);
    if (!box || !box->obj) continue;
    clock_bar_align_box_text(*box);
    lv_obj_align(box->obj, LV_ALIGN_TOP_LEFT, x, box->y);
    lv_obj_move_background(box->obj);
    x += box->width + visual_gap;
    placed++;
  }
}

inline bool clock_bar_layout_item_visible(int item, size_t temperature_count,
                                          bool time_visible,
                                          bool network_visible) {
  if (clock_bar_item_is_temperature(item)) {
    return (size_t) (item - CLOCK_BAR_ITEM_TEMPERATURE) < temperature_count;
  }
  if (item == CLOCK_BAR_ITEM_TIME) return time_visible;
  if (item == CLOCK_BAR_ITEM_NETWORK) return network_visible;
  return false;
}

inline size_t clock_bar_visible_temperature_count(bool indoor_enabled,
                                                  bool outdoor_enabled) {
  if (clock_bar_temperature_has_items()) {
    size_t count = clock_bar_temperature_values().size();
    if (!outdoor_enabled) return 0;
    return count > CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT
               ? CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT
               : count;
  }

  size_t count = 0;
  if (outdoor_enabled) count++;
  if (indoor_enabled) count++;
  return count > CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT
             ? CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT
             : count;
}

inline ClockBarParsedLayout clock_bar_fixed_layout() {
  ClockBarParsedLayout layout;
  clock_bar_clear_layout(layout);
  clock_bar_add_missing_default_items(layout);
  return layout;
}

inline ClockBarParsedLayout compact_clock_bar_layout(
    const ClockBarParsedLayout &layout,
    size_t temperature_count,
    bool time_visible,
    bool network_visible) {
  ClockBarParsedLayout compact;
  clock_bar_clear_layout(compact);

  for (int section = 0; section < CLOCK_BAR_SECTION_COUNT; section++) {
    for (int order = 0; order < layout.count[section]; order++) {
      for (int item = 0; item < CLOCK_BAR_ITEM_COUNT; item++) {
        if (layout.section[item] != section || layout.order[item] != order) continue;
        if (!clock_bar_layout_item_visible(
                item, temperature_count, time_visible, network_visible)) {
          continue;
        }
        clock_bar_add_item(compact, section, item);
      }
    }
  }
  return compact;
}

inline void apply_clock_bar_layout(const std::string &layout_text,
                                   lv_obj_t **temperature_labels,
                                   size_t temperature_label_count,
                                   lv_obj_t *display_time,
                                   lv_obj_t *network_status_button,
                                   bool time_visible,
                                   bool network_visible,
                                   bool indoor_temperature_visible,
                                   bool outdoor_temperature_visible,
                                   int screen_width,
                                   int left_x, int label_y,
                                   int right_x, int network_y,
                                   int item_gap,
                                   int visual_gap) {
  (void) layout_text;
  for (size_t i = CLOCK_BAR_VISIBLE_TEMPERATURE_SLOT_COUNT;
       temperature_labels && i < temperature_label_count; i++) {
    clock_bar_set_widget_hidden(temperature_labels[i], true);
  }
  ClockBarParsedLayout parsed_layout = clock_bar_fixed_layout();
  ClockBarParsedLayout layout = compact_clock_bar_layout(
      parsed_layout,
      clock_bar_visible_temperature_count(indoor_temperature_visible,
                                          outdoor_temperature_visible),
      time_visible,
      network_visible);
  ClockBarLayoutBox boxes[CLOCK_BAR_ITEM_COUNT];
  int box_count = 0;
  for (size_t i = 0; i < temperature_label_count && i < CLOCK_BAR_TEMPERATURE_SLOT_COUNT; i++) {
    int item = CLOCK_BAR_ITEM_TEMPERATURE + (int) i;
    clock_bar_add_layout_box(boxes, box_count, layout,
                             temperature_labels[i], item, label_y, item_gap);
  }
  clock_bar_add_layout_box(boxes, box_count, layout,
                           display_time, CLOCK_BAR_ITEM_TIME, label_y, item_gap);
  clock_bar_add_layout_box(boxes, box_count, layout,
                           network_status_button, CLOCK_BAR_ITEM_NETWORK, network_y, item_gap);

  int gap = clock_bar_visual_gap_px(visual_gap);
  align_clock_bar_layout_section(boxes, box_count, CLOCK_BAR_SECTION_LEFT,
                                 screen_width, left_x, right_x, gap);
  align_clock_bar_layout_section(boxes, box_count, CLOCK_BAR_SECTION_MIDDLE,
                                 screen_width, left_x, right_x, gap);
  align_clock_bar_layout_section(boxes, box_count, CLOCK_BAR_SECTION_RIGHT,
                                 screen_width, left_x, right_x, gap);
}

#endif
