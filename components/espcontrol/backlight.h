#ifndef ESPCONTROL_BACKLIGHT_H
#define ESPCONTROL_BACKLIGHT_H

// =============================================================================
// BACKLIGHT - Brightness scheduling, sunrise/sunset, and UI helpers
// =============================================================================
// Shared C++ utilities for backlight schedule logic and screensaver layout.
// Extracted from YAML lambdas so the logic is testable and syntax-highlighted,
// while YAML retains only thin id() wiring.
// =============================================================================
#pragma once
#include <string>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "clock_bar.h"
#include "display_mode_controller.h"
#include "sun_calc.h"
#include "temperature_unit.h"

#ifdef USE_ESP32
#include <esp_sleep.h>
#include <esp_system.h>
#endif

using BacklightDisplayTakeoverCallback = void (*)();

inline BacklightDisplayTakeoverCallback &backlight_display_takeover_callback() {
  static BacklightDisplayTakeoverCallback callback = nullptr;
  return callback;
}

inline void set_backlight_display_takeover_callback(BacklightDisplayTakeoverCallback callback) {
  backlight_display_takeover_callback() = callback;
}

inline void backlight_close_modals_for_display_takeover() {
  BacklightDisplayTakeoverCallback callback = backlight_display_takeover_callback();
  if (callback) callback();
}

// ── Sunrise/sunset recalculation ─────────────────────────────────────

struct SunCalcResult {
  int rise_h, rise_m, set_h, set_m;
  bool valid;
  char sunrise_str[16];
  char sunset_str[16];
};

inline SunCalcResult recalc_sunrise_sunset(
    int year, int month, int day,
    const std::string &tz_option, bool use_12h = true) {
  SunCalcResult r = {};

  std::string effective_tz_option = effective_timezone_option(tz_option);
  std::string tz_id = timezone_id_from_option(effective_tz_option);
  float tz_offset = utc_offset_hours_for_date(year, month, day, effective_tz_option);

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

  int tz_c = (int)((tz_offset >= 0 ? tz_offset : -tz_offset) * 10.0f + 0.5f);
  ESP_LOGI("backlight",
           "Sunrise %02d:%02d, Sunset %02d:%02d "
           "(tz=%s%d.%d)",
           rh, rm, sh, sm,
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

inline bool screen_schedule_blocks_cover_art(const std::string &trigger,
                                             bool enabled,
                                             bool presence_detected,
                                             bool time_valid,
                                             int now_h,
                                             int on_hour,
                                             int off_hour) {
  return screen_schedule_waiting_for_time(trigger, enabled, time_valid) ||
         screen_schedule_night_active(trigger, enabled, presence_detected,
                                      time_valid, now_h, on_hour, off_hour);
}

// ── Screensaver action helpers ────────────────────────────────────────

inline bool screensaver_action_clock_mode(const std::string &action) {
  return action == "Clock" || action == "clock";
}

inline bool screensaver_action_dimmed_mode(const std::string &action) {
  return action == "Screen Dimmed" || action == "screen_dimmed" ||
         action == "Dimmed" || action == "dimmed" || action == "dim";
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
