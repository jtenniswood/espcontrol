#!/usr/bin/env python3
"""Compile and run host-side checks for pure firmware parser helpers."""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parent.parent
CONFIG_DIR = ROOT / "common" / "config"
PARSER_HEADER = ROOT / "components" / "espcontrol" / "button_grid_config_parser.h"
DISPLAY_COLOR_HEADER = ROOT / "components" / "espcontrol" / "display_color.h"
SCREEN_LOCK_STATE_HEADER = ROOT / "components" / "espcontrol" / "screen_lock_state.h"
CONTRACT_HEADER = ROOT / "components" / "espcontrol" / "button_grid_contract_generated.h"
CARD_RUNTIME_HEADER = ROOT / "components" / "espcontrol" / "button_grid_card_runtime.h"
SAVED_CONFIG_VACUUM_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_vacuum_generated.h"
SAVED_CONFIG_SENSOR_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_sensor_generated.h"
SAVED_CONFIG_ACTION_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_action_generated.h"
SAVED_CONFIG_MEDIA_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_media_generated.h"
SAVED_CONFIG_STATIC_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_static_generated.h"
SAVED_CONFIG_FAN_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_fan_generated.h"
SAVED_CONFIG_DATE_TIME_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_date_time_generated.h"
SAVED_CONFIG_MOWER_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_mower_generated.h"
SAVED_CONFIG_OCCUPANCY_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_occupancy_generated.h"
SAVED_CONFIG_ACCESS_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_access_generated.h"
SAVED_CONFIG_SECURITY_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_security_generated.h"
SAVED_CONFIG_WEATHER_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_weather_generated.h"
SAVED_CONFIG_IMAGE_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_image_generated.h"
SAVED_CONFIG_CLIMATE_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_climate_generated.h"
SAVED_CONFIG_LIGHT_CONTROL_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_light_control_generated.h"
SAVED_CONFIG_WEBHOOK_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_webhook_generated.h"
SAVED_CONFIG_SUBPAGE_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_subpage_generated.h"
SAVED_CONFIG_SWITCH_HEADER = ROOT / "components" / "espcontrol" / "button_grid_saved_config_switch_generated.h"
BACKLIGHT_HEADER = ROOT / "components" / "espcontrol" / "backlight.h"
DISPLAY_MODE_CONTROLLER_HEADER = ROOT / "components" / "espcontrol" / "display_mode_controller.h"
CLOCK_BAR_HEADER = ROOT / "components" / "espcontrol" / "clock_bar.h"
LAYOUT_HEADER = ROOT / "components" / "espcontrol" / "button_grid_layout.h"
LIMITS_HEADER = ROOT / "components" / "espcontrol" / "button_grid_limits.h"
STRING_HEADER = ROOT / "components" / "espcontrol" / "button_grid_string.h"
BUTTON_GRID_FACADE = ROOT / "components" / "espcontrol" / "button_grid.h"
CARD_NORMALIZATION_FIXTURES = ROOT / "common" / "config" / "card_normalization_fixtures.json"
DEVICES_DIR = ROOT / "devices"
IMAGE_CARD_NORMALIZATION_FIXTURES = ROOT / "common" / "config" / "image_card_normalization_fixtures.json"


CPP_SOURCE = r'''
#include <algorithm>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

template <typename... Args>
inline void esp_log_ignore(Args&&...) {}
#define ESP_LOGW(...) esp_log_ignore(__VA_ARGS__)
#define ESP_LOGI(...) esp_log_ignore(__VA_ARGS__)

namespace esphome {
namespace text { class Text {}; }
class StringRef {
 public:
  StringRef(const char *value) : value_(value ? value : "") {}
  const char *c_str() const { return value_; }
  size_t size() const { return std::strlen(value_); }
 private:
  const char *value_;
};
}

struct lv_obj_t {
  int flags = 0;
  std::string text;
};
constexpr int MAX_GRID_SLOTS = 25;
inline int bounded_grid_slots(int num_slots) {
  if (num_slots < 0) return 0;
  return num_slots > MAX_GRID_SLOTS ? MAX_GRID_SLOTS : num_slots;
}
struct BtnSlot {
  esphome::text::Text *config = nullptr;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  lv_obj_t *sensor_container = nullptr;
  lv_obj_t *sensor_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *subpage_lbl = nullptr;
};
struct lv_disp_t {};
struct lv_font_t {};
using lv_coord_t = int;
using lv_style_selector_t = int;
using lv_color_t = int;
using lv_grid_align_t = int;
using lv_text_align_t = int;
static lv_disp_t lv_test_default_disp;
static bool lv_test_disp_available = false;
static int lv_test_hor_res = 480;
static int lv_test_ver_res = 480;
static int lv_obj_move_background_calls = 0;
static lv_obj_t *lv_active_screen = nullptr;
inline const char *espcontrol_i18n(const char *text) { return text ? text : ""; }
inline std::string espcontrol_i18n(const std::string &text) { return text; }
constexpr int LV_PART_MAIN = 0;
constexpr int LV_STATE_CHECKED = 1;
constexpr int LV_STATE_PRESSED = 2;
constexpr int LV_STATE_DEFAULT = 0;
constexpr int LV_STATE_DISABLED = 4;
constexpr int LV_LABEL_LONG_WRAP = 0;
constexpr int LV_LABEL_LONG_CLIP = 1;
constexpr int LV_TEXT_ALIGN_LEFT = 0;
constexpr int LV_TEXT_ALIGN_CENTER = 1;
constexpr int LV_TEXT_ALIGN_RIGHT = 2;
constexpr int LV_ALIGN_TOP_LEFT = 0;
constexpr int LV_ALIGN_TOP_MID = 1;
constexpr int LV_ALIGN_TOP_RIGHT = 2;
constexpr int LV_ALIGN_BOTTOM_LEFT = 0;
[[maybe_unused]] constexpr int LV_ALIGN_BOTTOM_RIGHT = 1;
constexpr int LV_GRID_ALIGN_START = 0;
constexpr int LV_GRID_ALIGN_STRETCH = 1;
constexpr int LV_OPA_COVER = 255;
constexpr int LV_OPA_50 = 128;
constexpr int LV_OBJ_FLAG_CLICKABLE = 1;
constexpr int LV_OBJ_FLAG_HIDDEN = 2;
constexpr int LV_GRAD_DIR_HOR = 1;
inline int lv_color_hex(uint32_t value) { return static_cast<int>(value); }
inline int lv_pct(int value) { return value; }
inline lv_obj_t *lv_scr_act() { return lv_active_screen; }
inline void lv_obj_set_style_transform_scale_x(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_transform_scale_y(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_bg_color(lv_obj_t *, int, lv_style_selector_t) {}
inline void lv_obj_set_style_bg_grad_color(lv_obj_t *, lv_color_t, lv_style_selector_t) {}
inline void lv_obj_set_style_bg_grad_dir(lv_obj_t *, int, lv_style_selector_t) {}
inline void lv_obj_set_style_text_color(lv_obj_t *, lv_color_t, lv_style_selector_t) {}
inline void lv_obj_set_style_text_align(lv_obj_t *, int, lv_style_selector_t) {}
inline lv_color_t lv_obj_get_style_text_color(lv_obj_t *, lv_style_selector_t) { return 0; }
inline void lv_obj_set_style_opa(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_text_opa(lv_obj_t *, int, int) {}
inline void lv_obj_add_state(lv_obj_t *, int) {}
inline void lv_obj_clear_state(lv_obj_t *, int) {}
inline bool lv_obj_has_state(lv_obj_t *, int) { return false; }
inline void lv_obj_add_flag(lv_obj_t *obj, int flag) { if (obj) obj->flags |= flag; }
inline void lv_obj_clear_flag(lv_obj_t *obj, int flag) { if (obj) obj->flags &= ~flag; }
inline bool lv_obj_has_flag(lv_obj_t *obj, int flag) { return obj && (obj->flags & flag); }
inline uint32_t lv_obj_get_child_cnt(lv_obj_t *) { return 0; }
inline lv_obj_t *lv_obj_get_child(lv_obj_t *, uint32_t) { return nullptr; }
inline int lv_obj_get_width(lv_obj_t *) { return 480; }
inline int lv_obj_get_height(lv_obj_t *) { return 480; }
inline int lv_obj_get_style_pad_left(lv_obj_t *, int) { return 0; }
inline int lv_obj_get_style_pad_right(lv_obj_t *, int) { return 0; }
inline int lv_obj_get_style_pad_top(lv_obj_t *, int) { return 0; }
inline int lv_obj_get_style_pad_bottom(lv_obj_t *, int) { return 0; }
inline int lv_obj_get_style_pad_column(lv_obj_t *, int) { return 0; }
inline int lv_obj_get_style_pad_row(lv_obj_t *, int) { return 0; }
inline lv_obj_t *lv_obj_get_parent(lv_obj_t *) { return nullptr; }
inline lv_disp_t *lv_disp_get_default() { return lv_test_disp_available ? &lv_test_default_disp : nullptr; }
inline int lv_disp_get_hor_res(lv_disp_t *) { return lv_test_hor_res; }
inline int lv_disp_get_ver_res(lv_disp_t *) { return lv_test_ver_res; }
inline void lv_label_set_long_mode(lv_obj_t *, int) {}
inline void lv_obj_set_size(lv_obj_t *, int, int) {}
inline void lv_obj_set_width(lv_obj_t *, int) {}
inline void lv_obj_set_height(lv_obj_t *, int) {}
inline void lv_obj_set_pos(lv_obj_t *, int, int) {}
inline void lv_obj_set_grid_cell(lv_obj_t *, int, int, int, int, int, int) {}
inline void lv_obj_set_style_pad_top(lv_obj_t *, int, int) {}
inline void lv_obj_update_layout(lv_obj_t *) {}
inline void lv_label_set_text(lv_obj_t *obj, const char *text) { if (obj) obj->text = text ? text : ""; }
inline void lv_obj_align(lv_obj_t *, int, int, int) {}
inline void lv_obj_move_foreground(lv_obj_t *) {}
inline void lv_obj_move_background(lv_obj_t *) { lv_obj_move_background_calls++; }

#include "temperature_unit.h"
#include "button_grid_config_parser.h"
#include "backlight.h"
#include "button_grid_layout.h"

int main() {
  int row_span = 0;
  int col_span = 0;
  grid_token_spans('\0', row_span, col_span);
  assert(row_span == 1 && col_span == 1);
  grid_token_spans('d', row_span, col_span);
  assert(row_span == 2 && col_span == 1);
  grid_token_spans('w', row_span, col_span);
  assert(row_span == 1 && col_span == 2);
  grid_token_spans('b', row_span, col_span);
  assert(row_span == 2 && col_span == 2);
  grid_token_spans('t', row_span, col_span);
  assert(row_span == 3 && col_span == 1);
  grid_token_spans('x', row_span, col_span);
  assert(row_span == 1 && col_span == 3);

  assert(clock_bar_equal_fr_track_size(434, 3, 0) == 145);
  assert(clock_bar_equal_fr_track_size(434, 3, 1) == 145);
  assert(clock_bar_equal_fr_track_size(434, 3, 2) == 144);
  assert(clock_bar_grid_track_span_size(480, 8, 8, 15, 3, 0, 1) == 145);
  assert(clock_bar_grid_track_span_size(480, 8, 8, 15, 3, 0, 2) == 305);
  assert(clock_bar_grid_track_span_size(480, 8, 8, 15, 3, 1, 2) == 304);
  assert(clock_bar_grid_track_span_size(480, 8, 8, 15, 3, 0, 3) == 464);
  assert(clock_bar_grid_track_span_size(1024, 5, 5, 10, 5, 0, 2) == 400);
  assert(clock_bar_grid_track_span_size(1024, 5, 5, 10, 5, 3, 2) == 399);
  assert(clock_bar_grid_track_span_size(600, 42, 5, 10, 3, 0, 2) == 366);
  lv_test_disp_available = false;
  assert(clock_bar_current_screen_width(1024) == 1024);
  assert(clock_bar_current_screen_height(600) == 600);
  lv_test_hor_res = 800;
  lv_test_ver_res = 1280;
  lv_test_disp_available = true;
  assert(clock_bar_current_screen_width(1024) == 800);
  assert(clock_bar_current_screen_height(600) == 1280);
  lv_test_disp_available = false;

  lv_obj_t main_page;
  lv_active_screen = &main_page;
  auto awake_clock_bar = clock_bar_resolve_visibility(
    true, &main_page, false, false, false, false, false, false);
  assert(awake_clock_bar.reserve_space);
  assert(awake_clock_bar.visible);

  auto clock_screensaver_clock_bar = clock_bar_resolve_visibility(
    true, &main_page, false, false, true, false, false, false);
  assert(clock_screensaver_clock_bar.reserve_space);
  assert(!clock_screensaver_clock_bar.visible);

  auto dismissing_screensaver_clock_bar = clock_bar_resolve_visibility(
    true, &main_page, true, false, false, false, false, false);
  assert(dismissing_screensaver_clock_bar.reserve_space);
  assert(!dismissing_screensaver_clock_bar.visible);

  auto screen_schedule_clock_bar = clock_bar_resolve_visibility(
    true, &main_page, true, true, true, false, false, false);
  assert(!screen_schedule_clock_bar.reserve_space);
  assert(!screen_schedule_clock_bar.visible);

  auto clock_bar_entities = parse_clock_bar_temperature_entities(
    " sensor.outdoor | sensor.indoor, sensor.outdoor\nsensor.loft,, ");
  assert(clock_bar_entities.size() == 1);
  assert(clock_bar_entities[0] == "sensor.outdoor");

  set_clock_bar_temperature_value_count(1);
  lv_obj_t temperature_1;
  lv_obj_t display_time;
  lv_obj_t network_status_button;
  lv_obj_t *temperature_labels[] = {
    &temperature_1,
  };
  lv_obj_move_background_calls = 0;
  apply_clock_bar_fixed_layout(
    &temperature_1,
    &display_time,
    &network_status_button,
    true, true, true,
    12, 17, 20, 10, 80);
  assert(lv_obj_move_background_calls == 3);
  hide_clock_bar_top_layer_widgets(
    temperature_labels, 1, &display_time, &network_status_button);
  assert(lv_obj_has_flag(&temperature_1, LV_OBJ_FLAG_HIDDEN));
  assert(lv_obj_has_flag(&display_time, LV_OBJ_FLAG_HIDDEN));
  assert(lv_obj_has_flag(&network_status_button, LV_OBJ_FLAG_HIDDEN));
  set_clock_bar_temperature_value_count(0);

  assert(cfg_field("light.kitchen;Kitchen;Auto;Lightbulb", 0) == "light.kitchen");
  assert(cfg_field("light.kitchen;Kitchen;Auto;Lightbulb", 3) == "Lightbulb");
  assert(cfg_field("light.kitchen;Kitchen", 4) == "");

  auto compact = parse_cfg("~sensor.energy,Energy%2C%20Now,Gauge,Auto,sensor.energy,kWh,sensor,1,large_numbers");
  assert(compact.entity == "sensor.energy");
  assert(compact.label == "Energy, Now");
  assert(compact.unit == "kWh");
  assert(compact.type == "sensor");
  assert(compact.precision == "1");
  assert(card_large_numbers_enabled(compact));
  auto large_off = parse_cfg(";;;;sensor.energy;kWh;sensor;1;large_numbers=off");
  assert(large_off.options == "large_numbers=off");
  assert(!card_large_numbers_enabled(large_off));
  assert(card_large_numbers_disabled(large_off));
  auto state_labels = parse_cfg(";;;;sensor.bin_level;;sensor;text;state_labels,state_input=high,state_output=Please%20empty,state_input_2=low,state_output_2=Full");
  assert(sensor_state_labels_enabled(state_labels));
  assert(state_labels.options == "state_labels,state_input=high,state_output=Please empty,state_input_2=low,state_output_2=Full");
  assert(sensor_state_display_text(state_labels, "low") == "Full");
  assert(sensor_state_display_text(state_labels, "high") == "Please empty");
  assert(sensor_state_display_text(state_labels, "High") == "Please empty");
  assert(sensor_state_display_text(state_labels, "medium") == "Medium");
  assert(sensor_state_display_text(state_labels, "medium-high") == "Medium-High");
  assert(text_sensor_display_text("pre-wash") == "Pre-Wash");
  assert(text_sensor_display_text("pre_wash") == "Pre Wash");
  auto legacy_state_labels = parse_cfg(";;;;sensor.bin_level;;sensor;text;state_labels,state_high_label=Please%20empty");
  assert(legacy_state_labels.options == "state_labels,state_input=high,state_output=Please empty");
  auto numeric_state_labels = parse_cfg(";;;;sensor.bin_level;;sensor;0;state_labels,state_high_label=Please%20empty");
  assert(numeric_state_labels.options == "");
  auto icon_sensor = parse_cfg(";;;;binary_sensor.patio_door;;sensor;icon;large_numbers");
  assert(icon_sensor.precision == "icon");
  assert(icon_sensor.options == "");
  assert(!card_large_numbers_enabled(icon_sensor));

  auto clock = parse_cfg(";;;;;;clock;;large_numbers");
  assert(clock.type == "clock");
  assert(clock.options == "large_numbers");
  assert(card_large_numbers_enabled(clock));

  auto weather_today = parse_cfg("weather.home;;;;;;weather;today;large_numbers");
  assert(weather_today.type == "weather");
  assert(weather_today.precision == "today");
  assert(weather_today.options == "large_numbers");
  assert(card_large_numbers_enabled(weather_today));
  auto weather_invalid_mode = parse_cfg("weather.home;;;;;;weather;bad;large_numbers");
  assert(weather_invalid_mode.type == "weather");
  assert(weather_invalid_mode.precision == "");
  assert(weather_invalid_mode.options == "");
  assert(!card_large_numbers_enabled(weather_invalid_mode));
  auto legacy_weather_forecast = parse_cfg("weather.home;Weather;Auto;Auto;;;weather_forecast");
  assert(legacy_weather_forecast.type == "weather");
  assert(legacy_weather_forecast.precision == "tomorrow");
  assert(legacy_weather_forecast.label == "");

  auto image_default = parse_cfg("camera.front_door;;Auto;Auto;;;image;;");
  assert(image_default.type == "image");
  assert(image_default.options == "");
  assert(image_card_refresh_interval_ms(image_default) == 0);
  assert(!image_card_timer_only_refresh(image_default));
  assert(!image_card_modal_fit_enabled(image_default));
  auto lawn_mower = parse_cfg("lawn_mower.backyard;Backyard Mower;Auto;Bell;pause_resume;ignored;lawn_mower;2;large_numbers");
  assert(lawn_mower.type == "lawn_mower");
  assert(lawn_mower.entity == "lawn_mower.backyard");
  assert(lawn_mower.sensor == "pause_resume");
  assert(lawn_mower.unit == "");
  assert(lawn_mower.precision == "");
  assert(lawn_mower.options == "");
  assert(lawn_mower.icon == "Robot Mower");
  assert(lawn_mower.icon_on == "Auto");
  auto lawn_mower_dock = parse_cfg("lawn_mower.backyard;Dock;Auto;Auto;dock;;lawn_mower");
  assert(lawn_mower_dock.sensor == "dock");
  assert(lawn_mower_dock.icon == "Robot Mower Outline");
  auto lawn_mower_invalid = parse_cfg("lawn_mower.backyard;Start;Auto;Auto;bad_mode;;lawn_mower");
  assert(lawn_mower_invalid.sensor == "start_mowing");
  assert(lawn_mower_invalid.icon == "Robot Mower");
  assert(card_runtime_lawn_mower_state_mode("status"));
  assert(card_runtime_lawn_mower_state_mode("start_mowing"));
  assert(card_runtime_lawn_mower_state_mode("dock"));
  assert(card_runtime_lawn_mower_state_mode("pause_resume"));
  auto image_label = parse_cfg("camera.front_door;Front Door;Auto;Auto;;;image;;image_label");
  assert(image_label.type == "image");
  assert(image_label.label == "Front Door");
  assert(image_label.options == "image_label");
  assert(image_card_label_enabled(image_label));
  auto image_icon = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_icon");
  assert(image_icon.type == "image");
  assert(image_icon.options == "image_icon");
  assert(image_card_icon_enabled(image_icon));
  auto image_label_icon = parse_cfg("camera.front_door;Front Door;Auto;Auto;;;image;;image_label,image_icon");
  assert(image_label_icon.label == "Front Door");
  assert(image_label_icon.options == "image_label,image_icon");
  assert(image_card_label_enabled(image_label_icon));
  assert(image_card_icon_enabled(image_label_icon));
  auto image_label_refresh = parse_cfg("~camera.front_door,Front%20Door,Auto,Auto,,,image,,image_label%2Cimage_refresh=30%2Cimage_refresh_mode=timer");
  assert(image_label_refresh.label == "Front Door");
  assert(image_label_refresh.options == "image_label");
  assert(image_card_refresh_interval_ms(image_label_refresh) == 0);
  assert(!image_card_timer_only_refresh(image_label_refresh));
  auto image_fit_refresh = parse_cfg("~camera.front_door,,Auto,Auto,,,image,,image_modal_mode=fit%2Cimage_refresh=30%2Cimage_refresh_mode=timer");
  assert(image_fit_refresh.options == "image_modal_mode=fit");
  assert(image_card_modal_fit_enabled(image_fit_refresh));
  auto image_fill_default = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_modal_mode=fill,image_refresh=30");
  assert(image_fill_default.options == "");
  assert(!image_card_modal_fit_enabled(image_fill_default));
  auto image_bad_modal = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_modal_mode=stretch,image_refresh=30");
  assert(image_bad_modal.options == "");
  assert(!image_card_modal_fit_enabled(image_bad_modal));
  auto image_ignored_label = parse_cfg("camera.front_door;Front Door;Auto;Auto;;;image;;");
  assert(image_ignored_label.label == "");
  assert(!image_card_label_enabled(image_ignored_label));
  auto image_refresh = parse_cfg("~camera.front_door,,Auto,Auto,,,image,,image_refresh=30%2Cimage_refresh_mode=timer");
  assert(image_refresh.type == "image");
  assert(image_refresh.options == "");
  assert(image_card_refresh_interval_ms(image_refresh) == 0);
  assert(!image_card_timer_only_refresh(image_refresh));
  auto image_refresh_default_mode = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_refresh=60,image_refresh_mode=bad");
  assert(image_refresh_default_mode.options == "");
  assert(image_card_refresh_interval_ms(image_refresh_default_mode) == 0);
  assert(!image_card_timer_only_refresh(image_refresh_default_mode));
  auto image_refresh_invalid = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_refresh=5,image_refresh_mode=timer");
  assert(image_refresh_invalid.options == "");
  assert(image_card_refresh_interval_ms(image_refresh_invalid) == 0);

  auto light_control_default_tabs = parse_cfg("light.kitchen;Kitchen;Lightbulb Outline;Lightbulb;;;light_control;;light_tabs=power%7Cbrightness%7Ctemperature%7Ccolor");
  assert(light_control_default_tabs.type == "light_control");
  assert(light_control_default_tabs.options == "");
  auto light_control_custom_tabs = parse_cfg("light.kitchen;Kitchen;Lightbulb Outline;Lightbulb;;;light_control;;light_tabs=brightness%7Cpower");
  assert(light_control_custom_tabs.options == "light_tabs=brightness%7Cpower");
  assert(cfg_option_value(light_control_custom_tabs.options, "light_tabs") == "brightness|power");
  auto light_control_bad_tabs = parse_cfg("light.kitchen;Kitchen;Lightbulb Outline;Lightbulb;;;light_control;;light_tabs=bad%7Cpower%7Cpower");
  assert(light_control_bad_tabs.options == "light_tabs=power");

  auto cover_default_tabs = parse_cfg("cover.office;Office Blind;Blinds;Blinds Open;modal;;cover;;cover_tabs=position%7Ccontrols%7Ctilt%7Cpresets");
  assert(cover_default_tabs.type == "cover");
  assert(cover_default_tabs.options == "");
  auto cover_custom_tabs = parse_cfg("cover.office;Office Blind;Blinds;Blinds Open;modal;;cover;;cover_tabs=controls%7Cposition");
  assert(cover_custom_tabs.options == "cover_tabs=controls%7Cposition");
  assert(cfg_option_value(cover_custom_tabs.options, "cover_tabs") == "controls|position");
  auto cover_bad_tabs = parse_cfg("cover.office;Office Blind;Blinds;Blinds Open;modal;;cover;;cover_tabs=bad%7Cposition%7Cposition");
  assert(cover_bad_tabs.options == "cover_tabs=position");
  auto cover_non_modal_tabs = parse_cfg("cover.office;Office Blind;Blinds;Blinds Open;toggle;;cover;;cover_tabs=controls%7Cposition");
  assert(cover_non_modal_tabs.options == "");

  set_display_temperature_unit("\u00B0F", "UTC (GMT+0)");
  assert(convert_temperature_value_for_display(10, "\u00B0C") == 50);
  assert(convert_temperature_value_for_display(10, "\u00B0F") == 10);
  assert(convert_temperature_value_for_display_float(10.4f, "\u00B0C") > 50.7f);
  assert(convert_temperature_value_for_display_float(10.4f, "\u00B0C") < 50.8f);
  assert(convert_temperature_value_for_display(50, "\u00B0F") == 50);
  set_display_temperature_unit("\u00B0C", "UTC (GMT+0)");
  assert(convert_temperature_value_for_display(50, "\u00B0F") == 10);
  assert(convert_temperature_value_for_display_float(50.7f, "\u00B0F") > 10.3f);
  assert(convert_temperature_value_for_display_float(50.7f, "\u00B0F") < 10.4f);
  assert(convert_temperature_value_for_display(10, "\u00B0C") == 10);

  auto migrated = parse_cfg("media_player.living:Living:Speaker:Auto:controls::media");
  assert(migrated.type.empty());
  auto media = parse_cfg("media_player.living;Living;Speaker;Auto;controls;;media");
  assert(media.type == "media");
  assert(media.sensor == "play_pause");
  assert(media.icon == "Auto");
  auto playlist = parse_cfg("media_player.living;Morning Mix;Music;Auto;playlist;;media;;playlist_content_id=spotify%3Aplaylist%3A1LG2Lnt9EDQS1DqoE8E2uO");
  assert(playlist.type == "media");
  assert(playlist.sensor == "playlist");
  assert(cfg_option_value(playlist.options, "playlist_content_id") == "spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO");
  auto volume = parse_cfg("media_player.kitchen;Kitchen;Auto;Auto;volume;;media;;volume_max=40");
  assert(volume.type == "media");
  assert(volume.sensor == "volume");
  assert(volume.options == "volume_max=40");
  assert(media_volume_max_percent(volume) == 40);
  auto volume_large = parse_cfg("media_player.kitchen;Kitchen;Auto;Auto;volume;;media;;large_numbers");
  assert(volume_large.options == "large_numbers");
  assert(card_large_numbers_enabled(volume_large));
  auto position_large = parse_cfg("media_player.office;Office;Progress Clock;Auto;position;;media;;large_numbers");
  assert(position_large.options == "large_numbers");
  assert(card_large_numbers_enabled(position_large));
  auto now_playing_large = parse_cfg("media_player.office;;Auto;Auto;now_playing;;media;;large_numbers");
  assert(now_playing_large.options == "");
  assert(!card_large_numbers_enabled(now_playing_large));
  auto media_control_display = parse_cfg("media_player.living;Speaker;Auto;Auto;control_modal;;media;;label_display=status,number_display=volume");
  assert(media_control_display.type == "media");
  assert(media_control_display.sensor == "control_modal");
  assert(media_control_display.options == "number_display=volume");
  assert(media_control_card_show_status_label(media_control_display));
  assert(media_control_card_show_volume_number(media_control_display));
  auto media_control_default_display = parse_cfg("media_player.living;Speaker;Auto;Auto;control_modal;;media;;label_display=label,number_display=icon,large_numbers");
  assert(media_control_default_display.options == "label_display=label");
  assert(!media_control_card_show_status_label(media_control_default_display));
  assert(!media_control_card_show_volume_number(media_control_default_display));
  auto media_control_implicit_display = parse_cfg("media_player.living;Speaker;Auto;Auto;control_modal;;media");
  assert(media_control_implicit_display.options == "");
  assert(media_control_card_show_status_label(media_control_implicit_display));
  auto media_control_custom_icon = parse_cfg("media_player.living;Speaker;Music;Auto;control_modal;;media");
  assert(media_control_custom_icon.sensor == "control_modal");
  assert(media_control_custom_icon.icon == "Music");
  auto volume_uncapped = parse_cfg("media_player.kitchen;Kitchen;Auto;Auto;volume;;media;;volume_max=150");
  assert(volume_uncapped.options == "");
  assert(media_volume_max_percent(volume_uncapped) == 100);

  auto action_large = parse_cfg("script.kitchen_lights;Kitchen Lights;Flash;Auto;script.turn_on;;action;;state_entity=sensor.kitchen_power,state_unit=W,state_precision=1,large_numbers");
  assert(action_large.options == "state_entity=sensor.kitchen_power,state_unit=W,state_precision=1,large_numbers");
  assert(card_large_numbers_enabled(action_large));
  auto action_icon = parse_cfg("script.goodnight;Goodnight;Flash;Check Circle;script.turn_on;;action;;state_entity=input_boolean.goodnight_ready,state_precision=icon");
  assert(action_card_state_icon_mode(action_icon));
  assert(!action_card_state_numeric_mode(action_icon));
  assert(!action_card_state_text_mode(action_icon));
  assert(numeric_state_positive_ref("1"));
  assert(numeric_state_positive_ref("2"));
  assert(numeric_state_positive_ref("3.5"));
  assert(!numeric_state_positive_ref("0"));
  assert(!numeric_state_positive_ref("-1"));
  assert(!numeric_state_positive_ref("unknown"));
  auto action_confirm = parse_cfg("script.goodnight;Goodnight;Script Text Play;Auto;script.turn_on;;action;;confirm_on,confirm_message=Run%20bedtime%3F,confirm_yes=Run,confirm_no=Cancel");
  assert(action_script_confirmation_enabled(action_confirm));
  assert(switch_confirmation_message(action_confirm) == "Run bedtime?");
  assert(switch_confirmation_yes_text(action_confirm) == "Run");
  assert(switch_confirmation_no_text(action_confirm) == "Cancel");
  auto action_confirm_default = parse_cfg("script.goodnight;Goodnight;Script Text Play;Auto;script.turn_on;;action;;confirm_on");
  assert(action_script_confirmation_enabled(action_confirm_default));
  assert(switch_confirmation_message(action_confirm_default) == "Run this script?");
  auto scene_confirm = parse_cfg("scene.goodnight;Goodnight;Movie Open;Auto;scene.turn_on;;action;;confirm_on,confirm_message=Run%20bedtime%3F");
  assert(!action_script_confirmation_enabled(scene_confirm));
  assert(scene_confirm.options == "");

  auto climate_large = parse_cfg("climate.living_room;Living;Thermostat;Auto;;;climate;1;large_numbers");
  assert(climate_large.options == "large_numbers");
  assert(card_large_numbers_enabled(climate_large));
  auto climate_icon_large = parse_cfg("climate.living_room;Living;Thermostat;Radiator;;;climate;1;number_display=icon,large_numbers");
  assert(climate_icon_large.options == "number_display=icon");
  assert(!card_large_numbers_enabled(climate_icon_large));

  auto confirm = parse_cfg("switch.printer;Printer;Printer 3D;Auto;;;;;confirm_off,confirm_message=Stop%20print%3F,confirm_yes=Power%20Down");
  assert(switch_confirmation_enabled(confirm));
  assert(switch_confirmation_message(confirm) == "Stop print?");
  assert(switch_confirmation_yes_text(confirm) == "Power Down");
  assert(switch_confirmation_no_text(confirm) == "No");

  auto switch_large = parse_cfg("switch.washer;Washer;Power Plug;Power;sensor.washer_power;W;;;large_numbers");
  assert(switch_large.options == "large_numbers");
  assert(card_large_numbers_enabled(switch_large));
  auto subpage_large = parse_cfg(";Open Windows;Window Closed;Auto;sensor.open_windows;%;subpage;;large_numbers");
  assert(subpage_large.options == "large_numbers");
  assert(card_large_numbers_enabled(subpage_large));
  auto subpage_lights = parse_cfg("light.living_room;Lighting;Lightbulb;Auto;indicator;;subpage;;subpage_kind=lights,large_numbers");
  assert(subpage_lights.options == "subpage_kind=lights");
  assert(!card_large_numbers_enabled(subpage_lights));
  auto subpage_media = parse_cfg("media_player.living_room;Media;Speaker;Auto;indicator;;subpage;;subpage_kind=media");
  assert(subpage_media.options == "subpage_kind=media");
  auto subpage_climate = parse_cfg("climate.living_room;Climate;Thermostat;Auto;indicator;;subpage;;subpage_kind=climate");
  assert(subpage_climate.options == "subpage_kind=climate");
  auto subpage_presence = parse_cfg("person.jane;Presence;Account;Auto;indicator;;subpage;;subpage_kind=presence");
  assert(subpage_presence.options == "subpage_kind=presence");
  auto subpage_alarm = parse_cfg("alarm_control_panel.home;Alarm;Security;Auto;indicator;;subpage;;subpage_kind=alarm");
  assert(subpage_alarm.options == "subpage_kind=alarm");
  auto subpage_vacuum = parse_cfg("vacuum.downstairs;Vacuum;Robot Vacuum;Auto;indicator;;subpage;;subpage_kind=vacuum");
  assert(subpage_vacuum.options == "subpage_kind=vacuum");
  auto subpage_lawn_mower = parse_cfg("lawn_mower.backyard;Lawn Mower;Robot Mower;Auto;indicator;;subpage;;subpage_kind=lawn_mower");
  assert(subpage_lawn_mower.options == "subpage_kind=lawn_mower");
  auto subpage_weather = parse_cfg("weather.home;Weather;Weather Partly Cloudy;Auto;indicator;;subpage;;subpage_kind=weather");
  assert(subpage_weather.options == "subpage_kind=weather");
  auto subpage_bad_kind = parse_cfg("media_player.bad;Bad;Speaker;Auto;indicator;;subpage;;subpage_kind=audio");
  assert(subpage_bad_kind.options == "");

  auto todo = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo");
  assert(todo.entity == "todo.shopping");
  assert(todo.label == "Shopping");
  assert(todo.icon == "Check");
  assert(todo.icon_on == "Auto");
  assert(todo.type == "todo");
  assert(todo.options == "");
  assert(todo_card_show_count(todo));
  auto todo_icon_display = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo;;count_display=icon");
  assert(todo_icon_display.options == "count_display=icon");
  assert(!todo_card_show_count(todo_icon_display));
  assert(!card_large_numbers_supported(todo_icon_display));
  auto todo_large = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo;;large_numbers");
  assert(todo_large.options == "large_numbers");
  assert(todo_card_show_count(todo_large));
  assert(card_large_numbers_enabled(todo_large));
  auto todo_icon_large = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo;;count_display=icon,large_numbers");
  assert(todo_icon_large.options == "count_display=icon");
  assert(!card_large_numbers_enabled(todo_icon_large));
  auto todo_legacy_options = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo;;count_display=top_task,label_display=count,completed_display=hide,large_numbers");
  assert(todo_legacy_options.options == "large_numbers");
  assert(todo_card_show_count(todo_legacy_options));
  assert(!todo_card_shows_top_task(todo_legacy_options));
  assert(!todo_card_label_shows_count(todo_legacy_options));
  assert(!todo_card_shows_completed_items(todo_legacy_options));
  assert(card_large_numbers_enabled(todo_legacy_options));

  assert(cfg_option_token_present("large_numbers,active_color", "active_color"));
  assert(cfg_option_value("state_entity=sensor.room%2Ctemp,state_unit=%25", "state_entity") == "sensor.room,temp");
  assert(cfg_option_value("state_entity=sensor.room%2Ctemp,state_unit=%25", "state_unit") == "%");

  bool valid = false;
  assert(parse_hex_color("FF8C00", valid) == 0xFF8C00 && valid);
  assert(parse_hex_color("BAD", valid) == 0 && !valid);
  assert(!ha_entity_state_unavailable_ref("button.test", "unknown"));
  assert(!ha_entity_state_unavailable_ref("input_button.test", "unknown"));
  assert(ha_entity_state_unavailable_ref("button.test", "unavailable"));
  assert(ha_entity_state_unavailable_ref("button.test", ""));
  assert(ha_entity_state_unavailable_ref("sensor.test", "unknown"));
  assert(ha_entity_state_unavailable_ref("light.test", "unknown"));
  assert(is_entity_on_ref("playing"));
  assert(normalize_width_compensation_percent(0) == 100);
  assert(normalize_width_compensation_percent(25) == 50);
  assert(normalize_width_compensation_percent(175) == 150);
  assert(width_compensation_scale(100) == 256);
  assert(clamp_percent_value(-1) == 0);
  assert(clamp_percent_value(101) == 100);
  int brightness_pct = -1;
  assert(light_brightness_to_percent(0.0f, brightness_pct) && brightness_pct == 0);
  assert(light_brightness_to_percent(1.0f, brightness_pct) && brightness_pct == 1);
  assert(light_brightness_to_percent(128.0f, brightness_pct) && brightness_pct == 50);
  assert(light_brightness_to_percent(255.0f, brightness_pct) && brightness_pct == 100);
  assert(!light_brightness_to_percent(NAN, brightness_pct));
  int hour = 0;
  int minute = 0;
  assert(parse_time_of_day("05:30", hour, minute) && hour == 5 && minute == 30);
  assert(parse_time_of_day("5:30", hour, minute) && hour == 5 && minute == 30);
  assert(!parse_time_of_day("24:00", hour, minute));
  int rise_h = 0;
  int rise_m = 0;
  int set_h = 0;
  int set_m = 0;
  assert(brightness_schedule_times(true, true, 7, 15, 20, 45, "06:00", "18:00", rise_h, rise_m, set_h, set_m));
  assert(rise_h == 7 && rise_m == 15 && set_h == 20 && set_m == 45);
  assert(brightness_schedule_times(false, true, 7, 15, 20, 45, "06:30", "21:05", rise_h, rise_m, set_h, set_m));
  assert(rise_h == 6 && rise_m == 30 && set_h == 21 && set_m == 5);
  assert(!brightness_schedule_times(false, true, 7, 15, 20, 45, "bad", "25:00", rise_h, rise_m, set_h, set_m));
  assert(rise_h == 6 && rise_m == 0 && set_h == 18 && set_m == 0);

  OrderResult parsed;
  parse_order_string("1,2d,3w,4b,5t,6x,99", 9, parsed);
  assert(parsed.positions[0] == 1);
  assert(parsed.positions[1] == 2);
  assert(parsed.row_span[1] == 2 && parsed.col_span[1] == 1);
  assert(parsed.row_span[2] == 1 && parsed.col_span[2] == 2);
  assert(parsed.row_span[3] == 2 && parsed.col_span[3] == 2);
  assert(parsed.row_span[4] == 3 && parsed.col_span[4] == 1);
  assert(parsed.row_span[5] == 1 && parsed.col_span[5] == 3);

  OrderResult overlap;
  parse_order_string("1b,2,3,4,5,6", 9, overlap);
  OrderResult cleared;
  clear_spanned_cells(overlap, 9, 3, cleared);
  assert(cleared.positions[1] == 0);
  assert(cleared.positions[3] == 0);
  assert(cleared.positions[4] == 0);

  return 0;
}
'''


def compiler() -> str | None:
    for name in ("c++", "g++", "clang++"):
        found = shutil.which(name)
        if found:
            return found
    return None


def check_clock_bar_visual_gaps() -> None:
    expected = {
        "esp32-p4-86": '"12"',
        "guition-esp32-p4-jc1060p470": '"10"',
        "guition-esp32-p4-jc4880p443": '"12"',
        "guition-esp32-p4-jc8012p4a1": '"10"',
        "guition-esp32-s3-4848s040": '"8"',
    }
    for device, value in expected.items():
        packages = DEVICES_DIR / device / "packages.yaml"
        text = packages.read_text(encoding="utf-8")
        needle = f"clock_bar_visual_gap: {value}"
        if needle not in text:
            raise RuntimeError(f"{packages} must define {needle}")


def cpp_string(value: str) -> str:
    return json.dumps(value)


def generated_fixture_assertions(fixtures: list[dict], comment: str, prefix: str) -> str:
    lines = [f"  // {comment}"]
    for fixture in fixtures:
        name = fixture["name"]
        expected = fixture["expected"]
        var_name = prefix + "".join(ch if ch.isalnum() else "_" for ch in name.lower())
        cases = [("input", fixture["input"])]
        if "canonical" in fixture:
            cases.append(("canonical", fixture["canonical"]))
        for case_name, input_value in cases:
            case_var_name = var_name if case_name == "input" else f"{var_name}_{case_name}"
            lines.append(f"  auto {case_var_name} = parse_cfg({cpp_string(input_value)});")
            for field in ("entity", "label", "icon", "icon_on", "sensor", "unit", "type", "precision", "options"):
                lines.append(f"  assert({case_var_name}.{field} == {cpp_string(expected[field])});")
    return "\n".join(lines) + "\n"


def remove_suffix(value: str, suffix: str) -> str:
    return value[: -len(suffix)] if suffix and value.endswith(suffix) else value


def generated_card_normalization_assertions() -> str:
    shared_fixtures = json.loads(CARD_NORMALIZATION_FIXTURES.read_text(encoding="utf-8"))
    chunks = []
    for label, fixtures in sorted(shared_fixtures.items()):
        prefix = "fixture_" + "".join(ch if ch.isalnum() else "_" for ch in label.lower()) + "_"
        chunks.append(generated_fixture_assertions(fixtures, f"Shared {label} saved-card normalization fixtures.", prefix))
    for path in sorted(CONFIG_DIR.glob("*_card_normalization_fixtures.json")):
        label = remove_suffix(path.name, "_card_normalization_fixtures.json").replace("_", " ")
        prefix = "fixture_" + remove_suffix(path.stem, "_card_normalization_fixtures") + "_"
        fixtures = json.loads(path.read_text(encoding="utf-8"))
        chunks.append(generated_fixture_assertions(fixtures, f"Shared {label} saved-card normalization fixtures.", prefix))
    return "".join(chunks)


def check_button_grid_facade() -> None:
    """Keep the YAML compatibility entry point free of behaviour code."""
    for line_number, line in enumerate(BUTTON_GRID_FACADE.read_text(encoding="utf-8").splitlines(), 1):
        stripped = line.strip()
        if not stripped or stripped.startswith("//") or stripped.startswith("#"):
            continue
        raise RuntimeError(
            f"{BUTTON_GRID_FACADE}:{line_number}: button_grid.h is an include-only compatibility facade"
        )


def main() -> int:
    check_button_grid_facade()
    check_clock_bar_visual_gaps()
    cxx = compiler()
    if not cxx:
        print("::error::No C++ compiler found for firmware parser checks", file=sys.stderr)
        return 1
    with TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        shutil.copy2(PARSER_HEADER, tmp_path / "button_grid_config_parser.h")
        shutil.copy2(ROOT / "components" / "espcontrol" / "temperature_unit.h", tmp_path / "temperature_unit.h")
        shutil.copy2(ROOT / "components" / "espcontrol" / "sun_calc.h", tmp_path / "sun_calc.h")
        shutil.copy2(DISPLAY_COLOR_HEADER, tmp_path / "display_color.h")
        shutil.copy2(SCREEN_LOCK_STATE_HEADER, tmp_path / "screen_lock_state.h")
        shutil.copy2(CONTRACT_HEADER, tmp_path / "button_grid_contract_generated.h")
        shutil.copy2(CARD_RUNTIME_HEADER, tmp_path / "button_grid_card_runtime.h")
        shutil.copy2(SAVED_CONFIG_VACUUM_HEADER, tmp_path / "button_grid_saved_config_vacuum_generated.h")
        shutil.copy2(SAVED_CONFIG_SENSOR_HEADER, tmp_path / "button_grid_saved_config_sensor_generated.h")
        shutil.copy2(SAVED_CONFIG_ACTION_HEADER, tmp_path / "button_grid_saved_config_action_generated.h")
        shutil.copy2(SAVED_CONFIG_MEDIA_HEADER, tmp_path / "button_grid_saved_config_media_generated.h")
        shutil.copy2(SAVED_CONFIG_STATIC_HEADER, tmp_path / "button_grid_saved_config_static_generated.h")
        shutil.copy2(SAVED_CONFIG_FAN_HEADER, tmp_path / "button_grid_saved_config_fan_generated.h")
        shutil.copy2(SAVED_CONFIG_DATE_TIME_HEADER, tmp_path / "button_grid_saved_config_date_time_generated.h")
        shutil.copy2(SAVED_CONFIG_MOWER_HEADER, tmp_path / "button_grid_saved_config_mower_generated.h")
        shutil.copy2(SAVED_CONFIG_OCCUPANCY_HEADER, tmp_path / "button_grid_saved_config_occupancy_generated.h")
        shutil.copy2(SAVED_CONFIG_ACCESS_HEADER, tmp_path / "button_grid_saved_config_access_generated.h")
        shutil.copy2(SAVED_CONFIG_SECURITY_HEADER, tmp_path / "button_grid_saved_config_security_generated.h")
        shutil.copy2(SAVED_CONFIG_WEATHER_HEADER, tmp_path / "button_grid_saved_config_weather_generated.h")
        shutil.copy2(SAVED_CONFIG_IMAGE_HEADER, tmp_path / "button_grid_saved_config_image_generated.h")
        shutil.copy2(SAVED_CONFIG_CLIMATE_HEADER, tmp_path / "button_grid_saved_config_climate_generated.h")
        shutil.copy2(SAVED_CONFIG_LIGHT_CONTROL_HEADER, tmp_path / "button_grid_saved_config_light_control_generated.h")
        shutil.copy2(SAVED_CONFIG_WEBHOOK_HEADER, tmp_path / "button_grid_saved_config_webhook_generated.h")
        shutil.copy2(SAVED_CONFIG_SUBPAGE_HEADER, tmp_path / "button_grid_saved_config_subpage_generated.h")
        shutil.copy2(SAVED_CONFIG_SWITCH_HEADER, tmp_path / "button_grid_saved_config_switch_generated.h")
        shutil.copy2(CLOCK_BAR_HEADER, tmp_path / "clock_bar.h")
        shutil.copy2(BACKLIGHT_HEADER, tmp_path / "backlight.h")
        shutil.copy2(DISPLAY_MODE_CONTROLLER_HEADER, tmp_path / "display_mode_controller.h")
        shutil.copy2(LAYOUT_HEADER, tmp_path / "button_grid_layout.h")
        shutil.copy2(LIMITS_HEADER, tmp_path / "button_grid_limits.h")
        shutil.copy2(STRING_HEADER, tmp_path / "button_grid_string.h")
        lvgl_stub = tmp_path / "esphome" / "components" / "lvgl" / "lvgl_esphome.h"
        lvgl_stub.parent.mkdir(parents=True, exist_ok=True)
        lvgl_stub.write_text("", encoding="utf-8")
        app_stub = tmp_path / "esphome" / "core" / "application.h"
        app_stub.parent.mkdir(parents=True, exist_ok=True)
        app_stub.write_text(
            "namespace esphome { struct AppClass { bool is_setup_complete() const { return true; } }; inline AppClass App; }\n",
            encoding="utf-8",
        )
        defines_stub = tmp_path / "esphome" / "core" / "defines.h"
        defines_stub.write_text("", encoding="utf-8")
        string_ref_stub = tmp_path / "esphome" / "core" / "string_ref.h"
        string_ref_stub.write_text("", encoding="utf-8")
        log_stub = tmp_path / "esphome" / "core" / "log.h"
        log_stub.write_text("", encoding="utf-8")
        network_stub = tmp_path / "esphome" / "components" / "network" / "util.h"
        network_stub.parent.mkdir(parents=True, exist_ok=True)
        network_stub.write_text(
            "#include <vector>\nnamespace esphome { namespace network { inline std::vector<int> get_ip_addresses() { return {1}; } } }\n",
            encoding="utf-8",
        )
        source = tmp_path / "check_firmware_parser.cpp"
        binary = tmp_path / "check_firmware_parser"
        source.write_text(
            CPP_SOURCE.replace("  return 0;\n}", generated_card_normalization_assertions() + "\n  return 0;\n}"),
            encoding="utf-8",
        )
        subprocess.run([cxx, "-std=c++17", "-Wall", "-Wextra", str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("Firmware parser checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
