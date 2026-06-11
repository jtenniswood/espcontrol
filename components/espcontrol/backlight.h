#ifndef ESPCONTROL_BACKLIGHT_H
#define ESPCONTROL_BACKLIGHT_H

// =============================================================================
// BACKLIGHT - Brightness scheduling, sunrise/sunset, and UI helpers
// =============================================================================
// Shared C++ utilities for backlight schedule logic and temperature label
// management. Extracted from YAML lambdas so the logic is testable and
// syntax-highlighted, while YAML retains only thin id() wiring.
// =============================================================================
#pragma once
#include <string>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "sun_calc.h"
#include "temperature_unit.h"

#ifdef USE_ESP32
#include <esp_sleep.h>
#include <esp_system.h>
#endif

static const size_t CLOCK_BAR_TEMPERATURE_SLOT_COUNT = 6;

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

// ── Clock-bar page visibility ────────────────────────────────────────

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

// ── Sunrise/sunset recalculation ─────────────────────────────────────

struct SunCalcResult {
  int rise_h, rise_m, set_h, set_m;
  bool valid;
  char sunrise_str[16];
  char sunset_str[16];
};

inline int fixed_decimal_scale(int precision) {
  if (precision <= 0) return 1;
  if (precision == 1) return 10;
  if (precision == 2) return 100;
  return 1000;
}

inline void format_fixed_decimal(char *buf, size_t size, float value, int precision) {
  if (size == 0) return;
  if (!std::isfinite(value)) {
    snprintf(buf, size, "--");
    return;
  }

  if (precision < 0) precision = 0;
  if (precision > 3) precision = 3;

  bool negative = value < 0.0f;
  float abs_value = negative ? -value : value;
  int scale = fixed_decimal_scale(precision);
  int scaled = (int)(abs_value * scale + 0.5f);
  if (scaled == 0) negative = false;

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

inline void hide_clock_bar_top_layer_widgets(lv_obj_t **temperature_labels,
                                             size_t temperature_label_count,
                                             lv_obj_t *display_time,
                                             lv_obj_t *network_status_button,
                                             lv_obj_t *weather_icon_container) {
  set_clock_bar_temperature_labels(temperature_labels, temperature_label_count);
  for (size_t i = 0; temperature_labels && i < temperature_label_count; i++) {
    if (temperature_labels[i]) lv_obj_add_flag(temperature_labels[i], LV_OBJ_FLAG_HIDDEN);
  }
  if (display_time) lv_obj_add_flag(display_time, LV_OBJ_FLAG_HIDDEN);
  if (network_status_button) lv_obj_add_flag(network_status_button, LV_OBJ_FLAG_HIDDEN);
  if (weather_icon_container) lv_obj_add_flag(weather_icon_container, LV_OBJ_FLAG_HIDDEN);
}

inline void set_clock_bar_temperature_value_count(size_t count) {
  clock_bar_temperature_values().assign(count, NAN);
}

inline bool clock_bar_temperature_has_items() {
  return !clock_bar_temperature_values().empty();
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

inline SunCalcResult recalc_sunrise_sunset(
    int year, int month, int day,
    const std::string &tz_option, bool use_12h = true) {
  SunCalcResult r = {};

  std::string tz_id = timezone_id_from_option(tz_option);
  float tz_offset = utc_offset_hours_for_date(year, month, day, tz_option);

  float lat, lon;
  if (!lookup_tz_coords(tz_id, lat, lon)) {
    ESP_LOGW("backlight", "No coordinates for timezone %s", tz_id.c_str());
    r.valid = false;
    return r;
  }

  calc_sunrise_sunset(year, month, day, lat, lon, tz_offset,
                      r.rise_h, r.rise_m, r.set_h, r.set_m);
  r.valid = true;

  int rh = r.rise_h, rm = r.rise_m;
  if (use_12h) {
    snprintf(r.sunrise_str, sizeof(r.sunrise_str), "%d:%02d AM",
             (rh == 0) ? 12 : (rh > 12 ? rh - 12 : rh), rm);
    if (rh >= 12)
      snprintf(r.sunrise_str, sizeof(r.sunrise_str), "%d:%02d PM",
               (rh == 12) ? 12 : rh - 12, rm);
  } else {
    snprintf(r.sunrise_str, sizeof(r.sunrise_str), "%02d:%02d", rh, rm);
  }

  int sh = r.set_h, sm = r.set_m;
  if (use_12h) {
    snprintf(r.sunset_str, sizeof(r.sunset_str), "%d:%02d PM",
             (sh == 12) ? 12 : (sh > 12 ? sh - 12 : sh), sm);
    if (sh < 12)
      snprintf(r.sunset_str, sizeof(r.sunset_str), "%d:%02d AM",
               (sh == 0) ? 12 : sh, sm);
  } else {
    snprintf(r.sunset_str, sizeof(r.sunset_str), "%02d:%02d", sh, sm);
  }

  int lat_c = (int)((lat >= 0 ? lat : -lat) * 100.0f + 0.5f);
  int lon_c = (int)((lon >= 0 ? lon : -lon) * 100.0f + 0.5f);
  int tz_c = (int)((tz_offset >= 0 ? tz_offset : -tz_offset) * 10.0f + 0.5f);
  ESP_LOGI("backlight",
           "Sunrise %02d:%02d, Sunset %02d:%02d "
           "(lat=%s%d.%02d lon=%s%d.%02d tz=%s%d.%d)",
           rh, rm, sh, sm,
           lat < 0 ? "-" : "", lat_c / 100, lat_c % 100,
           lon < 0 ? "-" : "", lon_c / 100, lon_c % 100,
           tz_offset < 0 ? "-" : "", tz_c / 10, tz_c % 10);

  return r;
}

// ── Brightness calculation ───────────────────────────────────────────

inline float calc_brightness_pct(
    bool sunrise_valid, int rise_h, int rise_m, int set_h, int set_m,
    int now_h, int now_m, bool *is_daytime,
    float day_pct, float night_pct) {
  if (!sunrise_valid) return day_pct;
  int now_min = now_h * 60 + now_m;
  int rise_min = rise_h * 60 + rise_m;
  int set_min = set_h * 60 + set_m;
  *is_daytime = (now_min >= rise_min && now_min < set_min);
  return *is_daytime ? day_pct : night_pct;
}

// ── Daylight transition detection ────────────────────────────────────

inline bool check_daylight_transition(
    bool sunrise_valid, int rise_h, int rise_m, int set_h, int set_m,
    int now_h, int now_m, bool last_is_day) {
  if (!sunrise_valid) return false;
  int now_min = now_h * 60 + now_m;
  bool is_day = (now_min >= rise_h * 60 + rise_m) &&
                (now_min < set_h * 60 + set_m);
  return is_day != last_is_day;
}

inline bool parse_time_of_day(const std::string &value, int &hour, int &minute) {
  int h = -1;
  int m = -1;
  if (std::sscanf(value.c_str(), " %d:%d", &h, &m) != 2) return false;
  if (h < 0 || h > 23 || m < 0 || m > 59) return false;
  hour = h;
  minute = m;
  return true;
}

inline bool brightness_schedule_times(
    bool automatic_times_enabled,
    bool sunrise_valid, int sunrise_h, int sunrise_m, int sunset_h, int sunset_m,
    const std::string &manual_dawn, const std::string &manual_dusk,
    int &rise_h, int &rise_m, int &set_h, int &set_m) {
  if (automatic_times_enabled) {
    rise_h = sunrise_h;
    rise_m = sunrise_m;
    set_h = sunset_h;
    set_m = sunset_m;
    return sunrise_valid;
  }

  int dawn_h = 6;
  int dawn_m = 0;
  int dusk_h = 18;
  int dusk_m = 0;
  bool dawn_valid = parse_time_of_day(manual_dawn, dawn_h, dawn_m);
  bool dusk_valid = parse_time_of_day(manual_dusk, dusk_h, dusk_m);
  rise_h = dawn_h;
  rise_m = dawn_m;
  set_h = dusk_h;
  set_m = dusk_m;
  return dawn_valid && dusk_valid;
}

// ── Screen schedule helpers ───────────────────────────────────────────

inline bool screen_schedule_in_window(int now_h, int on_hour, int off_hour) {
  if (on_hour < 0) on_hour = 0;
  if (on_hour > 23) on_hour = 23;
  if (off_hour < 0) off_hour = 0;
  if (off_hour > 23) off_hour = 23;
  if (on_hour < off_hour) return now_h >= on_hour && now_h < off_hour;
  if (on_hour > off_hour) return now_h >= on_hour || now_h < off_hour;
  return true;
}

inline bool screen_schedule_always_on_mode(const std::string &mode) {
  return mode == "Screen Dimmed" || mode == "screen_dimmed" ||
         mode == "Dimmed" || mode == "dimmed" || mode == "dim" ||
         mode == "Always On" || mode == "always_on" || mode == "always";
}

inline bool screen_schedule_clock_mode(const std::string &mode) {
  return mode == "Clock" || mode == "clock";
}

inline bool screen_schedule_sensor_trigger(const std::string &trigger) {
  return trigger == "Sensor" || trigger == "sensor";
}

inline bool screen_schedule_disabled_trigger(const std::string &trigger) {
  return trigger == "Disabled" || trigger == "disabled" || trigger == "Off" ||
         trigger == "off";
}

inline bool screen_schedule_time_trigger(const std::string &trigger) {
  return !screen_schedule_disabled_trigger(trigger) &&
         !screen_schedule_sensor_trigger(trigger);
}

inline bool screen_schedule_waiting_for_time(const std::string &trigger,
                                             bool enabled,
                                             bool time_valid) {
  return enabled && screen_schedule_time_trigger(trigger) && !time_valid;
}

inline bool screen_schedule_night_active(const std::string &trigger,
                                         bool enabled,
                                         bool presence_detected,
                                         bool time_valid,
                                         int now_h,
                                         int on_hour,
                                         int off_hour) {
  if (!enabled || screen_schedule_disabled_trigger(trigger)) return false;
  if (screen_schedule_sensor_trigger(trigger)) return !presence_detected;
  if (!time_valid) return false;
  return !screen_schedule_in_window(now_h, on_hour, off_hour);
}

inline bool screen_schedule_normal_active(const std::string &trigger,
                                          bool enabled,
                                          bool presence_detected,
                                          bool time_valid,
                                          int now_h,
                                          int on_hour,
                                          int off_hour) {
  if (!enabled || screen_schedule_disabled_trigger(trigger)) return false;
  if (screen_schedule_sensor_trigger(trigger)) return presence_detected;
  if (!time_valid) return false;
  return screen_schedule_in_window(now_h, on_hour, off_hour);
}

// ── Screensaver action helpers ────────────────────────────────────────

inline bool screensaver_action_clock_mode(const std::string &action) {
  return action == "Clock" || action == "clock";
}

inline bool screensaver_action_dimmed_mode(const std::string &action) {
  return action == "Screen Dimmed" || action == "screen_dimmed" ||
         action == "Dimmed" || action == "dimmed" || action == "dim";
}

// ── Temperature label visibility ─────────────────────────────────────

inline void refresh_clock_bar_temperature_label_values(
    lv_obj_t *main_page_obj, bool clock_bar_visible,
    bool indoor_enabled, bool outdoor_enabled,
    float indoor, float outdoor) {
  const bool show_on_screen =
      clock_bar_visible && clock_bar_active_on_button_grid_page(main_page_obj);
  if (!clock_bar_temperature_has_items()) {
    std::vector<lv_obj_t *> &labels = clock_bar_temperature_labels();
    if (!show_on_screen || (!indoor_enabled && !outdoor_enabled)) {
      for (size_t i = 0; i < labels.size(); i++) {
        if (labels[i]) lv_obj_add_flag(labels[i], LV_OBJ_FLAG_HIDDEN);
      }
      return;
    }

    size_t label_index = 0;
    auto set_legacy_temperature = [&](float value) {
      if (label_index >= labels.size()) return;
      lv_obj_t *label = labels[label_index++];
      if (!label) return;
      char value_buf[16];
      if (std::isnan(value)) snprintf(value_buf, sizeof(value_buf), "-");
      else format_fixed_decimal(value_buf, sizeof(value_buf), value, 0);
      char buf[24];
      format_clock_bar_temperature_single(buf, sizeof(buf), value_buf);
      lv_label_set_text(label, buf);
      if (show_on_screen) lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    };
    if (outdoor_enabled) set_legacy_temperature(outdoor);
    if (indoor_enabled) set_legacy_temperature(indoor);
    for (size_t i = label_index; i < labels.size(); i++) {
      if (labels[i]) lv_obj_add_flag(labels[i], LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  std::vector<lv_obj_t *> &labels = clock_bar_temperature_labels();
  if (!show_on_screen) {
    for (size_t i = 0; i < labels.size(); i++) {
      if (labels[i]) lv_obj_add_flag(labels[i], LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }
  std::vector<float> &values = clock_bar_temperature_values();
  for (size_t i = 0; i < labels.size(); i++) {
    lv_obj_t *label = labels[i];
    if (!label) continue;
    if (i >= values.size()) {
      lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    char value_buf[16];
    if (std::isnan(values[i])) snprintf(value_buf, sizeof(value_buf), "-");
    else format_fixed_decimal(value_buf, sizeof(value_buf), values[i], 0);
    char buf[24];
    format_clock_bar_temperature_single(buf, sizeof(buf), value_buf);
    lv_label_set_text(label, buf);
    if (show_on_screen) lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
  }
}

// ── Clock bar layout helpers ────────────────────────────────────────

enum ClockBarItemId {
  CLOCK_BAR_ITEM_TEMPERATURE = 0,
  CLOCK_BAR_ITEM_TIME = CLOCK_BAR_TEMPERATURE_SLOT_COUNT,
  CLOCK_BAR_ITEM_NETWORK = CLOCK_BAR_TEMPERATURE_SLOT_COUNT + 1,
  CLOCK_BAR_ITEM_WEATHER = CLOCK_BAR_TEMPERATURE_SLOT_COUNT + 2,
  CLOCK_BAR_ITEM_COUNT = CLOCK_BAR_TEMPERATURE_SLOT_COUNT + 3,
};

enum ClockBarSectionId {
  CLOCK_BAR_SECTION_LEFT = 0,
  CLOCK_BAR_SECTION_MIDDLE = 1,
  CLOCK_BAR_SECTION_RIGHT = 2,
  CLOCK_BAR_SECTION_COUNT = 3,
};

struct ClockBarParsedLayout {
  int section[CLOCK_BAR_ITEM_COUNT];
  int order[CLOCK_BAR_ITEM_COUNT];
  int count[CLOCK_BAR_SECTION_COUNT];
};

inline bool clock_bar_token_matches(const char *start, size_t len, const char *value) {
  size_t value_len = strlen(value);
  return len == value_len && strncmp(start, value, len) == 0;
}

inline int clock_bar_section_id(const char *start, size_t len) {
  if (clock_bar_token_matches(start, len, "left")) return CLOCK_BAR_SECTION_LEFT;
  if (clock_bar_token_matches(start, len, "middle")) return CLOCK_BAR_SECTION_MIDDLE;
  if (clock_bar_token_matches(start, len, "right")) return CLOCK_BAR_SECTION_RIGHT;
  return -1;
}

inline int clock_bar_item_id(const char *start, size_t len) {
  if (clock_bar_token_matches(start, len, "temperature")) return CLOCK_BAR_ITEM_TEMPERATURE;
  const char prefix[] = "temperature_";
  const size_t prefix_len = sizeof(prefix) - 1;
  if (len > prefix_len && strncmp(start, prefix, prefix_len) == 0) {
    int slot = 0;
    for (size_t i = prefix_len; i < len; i++) {
      if (start[i] < '0' || start[i] > '9') return -1;
      slot = slot * 10 + (start[i] - '0');
    }
    if (slot >= 2 && slot <= CLOCK_BAR_TEMPERATURE_SLOT_COUNT) {
      return CLOCK_BAR_ITEM_TEMPERATURE + slot - 1;
    }
  }
  if (clock_bar_token_matches(start, len, "time")) return CLOCK_BAR_ITEM_TIME;
  if (clock_bar_token_matches(start, len, "network")) return CLOCK_BAR_ITEM_NETWORK;
  if (clock_bar_token_matches(start, len, "weather")) return CLOCK_BAR_ITEM_WEATHER;
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

inline ClockBarParsedLayout parse_clock_bar_layout(const std::string &layout_text) {
  ClockBarParsedLayout layout;
  for (int i = 0; i < CLOCK_BAR_ITEM_COUNT; i++) {
    layout.section[i] = -1;
    layout.order[i] = 0;
  }
  for (int i = 0; i < CLOCK_BAR_SECTION_COUNT; i++) layout.count[i] = 0;

  const char *text = layout_text.c_str();
  const size_t size = layout_text.size();
  size_t segment_start = 0;

  while (segment_start <= size) {
    size_t segment_end = segment_start;
    while (segment_end < size && text[segment_end] != '|') segment_end++;

    size_t colon = segment_start;
    while (colon < segment_end && text[colon] != ':') colon++;
    if (colon < segment_end) {
      int section = clock_bar_section_id(text + segment_start, colon - segment_start);
      size_t item_start = colon + 1;
      while (section >= 0 && item_start <= segment_end) {
        size_t item_end = item_start;
        while (item_end < segment_end && text[item_end] != ',') item_end++;
        int item = clock_bar_item_id(text + item_start, item_end - item_start);
        clock_bar_add_item(layout, section, item);
        item_start = item_end + 1;
      }
    }

    if (segment_end == size) break;
    segment_start = segment_end + 1;
  }

  clock_bar_add_item(layout, CLOCK_BAR_SECTION_LEFT, CLOCK_BAR_ITEM_TEMPERATURE);
  for (int i = 1; i < CLOCK_BAR_TEMPERATURE_SLOT_COUNT; i++) {
    clock_bar_add_item(layout, CLOCK_BAR_SECTION_LEFT, CLOCK_BAR_ITEM_TEMPERATURE + i);
  }
  clock_bar_add_item(layout, CLOCK_BAR_SECTION_MIDDLE, CLOCK_BAR_ITEM_TIME);
  clock_bar_add_item(layout, CLOCK_BAR_SECTION_RIGHT, CLOCK_BAR_ITEM_NETWORK);
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
         item < CLOCK_BAR_ITEM_TEMPERATURE + CLOCK_BAR_TEMPERATURE_SLOT_COUNT;
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

inline int clock_bar_item_at_order(const ClockBarParsedLayout &layout,
                                   int section,
                                   int order) {
  for (int item = 0; item < CLOCK_BAR_ITEM_COUNT; item++) {
    if (layout.section[item] == section && layout.order[item] == order) return item;
  }
  return -1;
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
  if (section == CLOCK_BAR_SECTION_LEFT) {
    return left_x;
  }
  if (section == CLOCK_BAR_SECTION_RIGHT) {
    return screen_width - right_x - total_width;
  }
  if (section == CLOCK_BAR_SECTION_MIDDLE) {
    return (screen_width - total_width) / 2;
  }
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
    lv_obj_align(box->obj, LV_ALIGN_TOP_LEFT, x, box->y);
    lv_obj_move_background(box->obj);
    x += box->width + visual_gap;
    placed++;
  }
}

inline bool clock_bar_layout_item_visible(int item, size_t temperature_count,
                                          bool time_visible,
                                          bool network_visible,
                                          bool weather_visible) {
  if (clock_bar_item_is_temperature(item)) {
    return (size_t) (item - CLOCK_BAR_ITEM_TEMPERATURE) < temperature_count;
  }
  if (item == CLOCK_BAR_ITEM_TIME) return time_visible;
  if (item == CLOCK_BAR_ITEM_NETWORK) return network_visible;
  if (item == CLOCK_BAR_ITEM_WEATHER) return weather_visible;
  return false;
}

inline size_t clock_bar_visible_temperature_count(bool indoor_enabled,
                                                  bool outdoor_enabled) {
  if (clock_bar_temperature_has_items()) {
    size_t count = clock_bar_temperature_values().size();
    return count > CLOCK_BAR_TEMPERATURE_SLOT_COUNT ? CLOCK_BAR_TEMPERATURE_SLOT_COUNT : count;
  }

  size_t count = 0;
  if (outdoor_enabled) count++;
  if (indoor_enabled) count++;
  return count;
}

inline ClockBarParsedLayout compact_clock_bar_layout(
    const ClockBarParsedLayout &layout,
    size_t temperature_count,
    bool time_visible,
    bool network_visible,
    bool weather_visible) {
  ClockBarParsedLayout compact;
  for (int i = 0; i < CLOCK_BAR_ITEM_COUNT; i++) {
    compact.section[i] = -1;
    compact.order[i] = 0;
  }
  for (int i = 0; i < CLOCK_BAR_SECTION_COUNT; i++) compact.count[i] = 0;

  for (int section = 0; section < CLOCK_BAR_SECTION_COUNT; section++) {
    for (int order = 0; order < layout.count[section]; order++) {
      for (int item = 0; item < CLOCK_BAR_ITEM_COUNT; item++) {
        if (layout.section[item] != section || layout.order[item] != order) continue;
        if (!clock_bar_layout_item_visible(
                item, temperature_count, time_visible, network_visible, weather_visible)) {
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
                                   lv_obj_t *weather_icon_container,
                                   bool time_visible,
                                   bool network_visible,
                                   bool weather_visible,
                                   bool indoor_temperature_visible,
                                   bool outdoor_temperature_visible,
                                   int screen_width,
                                   int left_x, int label_y,
                                   int right_x, int network_y,
                                   int item_gap,
                                   int visual_gap) {
  ClockBarParsedLayout parsed_layout = parse_clock_bar_layout(layout_text);
  ClockBarParsedLayout layout = compact_clock_bar_layout(
      parsed_layout,
      clock_bar_visible_temperature_count(indoor_temperature_visible,
                                          outdoor_temperature_visible),
      time_visible,
      network_visible,
      weather_visible);
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
  clock_bar_add_layout_box(boxes, box_count, layout,
                           weather_icon_container, CLOCK_BAR_ITEM_WEATHER, network_y, item_gap);

  int gap = clock_bar_visual_gap_px(visual_gap);
  align_clock_bar_layout_section(boxes, box_count, CLOCK_BAR_SECTION_LEFT,
                                 screen_width, left_x, right_x, gap);
  align_clock_bar_layout_section(boxes, box_count, CLOCK_BAR_SECTION_MIDDLE,
                                 screen_width, left_x, right_x, gap);
  align_clock_bar_layout_section(boxes, box_count, CLOCK_BAR_SECTION_RIGHT,
                                 screen_width, left_x, right_x, gap);
}

// ── Screensaver layout helpers ──────────────────────────────────────

inline void screensaver_fill_screen(lv_obj_t *obj) {
  if (!obj) return;
  lv_obj_set_pos(obj, 0, 0);
  lv_obj_set_size(obj, lv_pct(100), lv_pct(100));
}

inline void refresh_screensaver_fullscreen(lv_obj_t *clock_overlay,
                                           lv_obj_t *dim_guard) {
  screensaver_fill_screen(clock_overlay);
  screensaver_fill_screen(dim_guard);
}

inline uint32_t parse_clock_screensaver_text_color(const std::string &hex) {
  if (hex.size() != 6) return 0xFFFFFF;
  for (char ch : hex) {
    bool digit = ch >= '0' && ch <= '9';
    bool upper = ch >= 'A' && ch <= 'F';
    bool lower = ch >= 'a' && ch <= 'f';
    if (!digit && !upper && !lower) return 0xFFFFFF;
  }
  return strtoul(hex.c_str(), nullptr, 16);
}

inline void apply_clock_screensaver_text_color(lv_obj_t *label,
                                               const std::string &hex) {
  if (!label) return;
  lv_obj_set_style_text_color(
    label,
    lv_color_hex(parse_clock_screensaver_text_color(hex)),
    LV_PART_MAIN);
}

inline void position_clock_screensaver_label(lv_obj_t *overlay, lv_obj_t *label,
                                             int minute) {
  if (!label) return;
  if (!overlay) overlay = lv_obj_get_parent(label);
  screensaver_fill_screen(overlay);
  if (overlay) lv_obj_update_layout(overlay);

  lv_coord_t screen_w = overlay ? lv_obj_get_width(overlay) : 0;
  lv_coord_t screen_h = overlay ? lv_obj_get_height(overlay) : 0;
  lv_disp_t *disp = lv_disp_get_default();
  if (screen_w <= 0 && disp) screen_w = lv_disp_get_hor_res(disp);
  if (screen_h <= 0 && disp) screen_h = lv_disp_get_ver_res(disp);
  if (screen_w <= 0) screen_w = 480;
  if (screen_h <= 0) screen_h = 480;

  lv_obj_update_layout(label);
  lv_coord_t w = lv_obj_get_width(label);
  lv_coord_t h = lv_obj_get_height(label);
  int ox = (minute * 7) % 61 - 30;
  int oy = (minute * 13) % 41 - 20;
  lv_obj_set_pos(label, screen_w / 2 + ox - w / 2,
                 screen_h / 2 + oy - h / 2);
}

// ── Firmware update interval ─────────────────────────────────────────

inline bool should_check_update(int counter, const std::string &freq) {
  int threshold = 24;
  if (freq == "Hourly") threshold = 1;
  else if (freq == "Weekly") threshold = 168;
  else if (freq == "Monthly") threshold = 720;
  return counter % threshold == 0;
}

#endif  // ESPCONTROL_BACKLIGHT_H
