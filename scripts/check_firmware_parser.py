#!/usr/bin/env python3
"""Compile and run host-side checks for pure firmware parser helpers."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parent.parent
CONFIG_HEADER = ROOT / "components" / "espcontrol" / "button_grid_config.h"
CONTRACT_HEADER = ROOT / "components" / "espcontrol" / "button_grid_contract_generated.h"
CARD_RUNTIME_HEADER = ROOT / "components" / "espcontrol" / "button_grid_card_runtime.h"
LAYOUT_HEADER = ROOT / "components" / "espcontrol" / "button_grid_layout.h"


CPP_SOURCE = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

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

struct lv_obj_t {};
using lv_coord_t = int;
using lv_style_selector_t = int;
using lv_color_t = int;
inline const char *espcontrol_i18n(const char *text) { return text ? text : ""; }
inline std::string espcontrol_i18n(const std::string &text) { return text; }
constexpr int LV_PART_MAIN = 0;
constexpr int LV_STATE_CHECKED = 1;
constexpr int LV_STATE_PRESSED = 2;
constexpr int LV_STATE_DEFAULT = 0;
constexpr int LV_STATE_DISABLED = 4;
constexpr int LV_LABEL_LONG_WRAP = 0;
constexpr int LV_ALIGN_BOTTOM_LEFT = 0;
constexpr int LV_ALIGN_BOTTOM_RIGHT = 1;
constexpr int LV_OPA_COVER = 255;
constexpr int LV_OPA_50 = 128;
constexpr int LV_OBJ_FLAG_CLICKABLE = 1;
constexpr int LV_OBJ_FLAG_HIDDEN = 2;
constexpr int LV_GRAD_DIR_HOR = 1;
inline int lv_color_hex(uint32_t value) { return static_cast<int>(value); }
inline int lv_pct(int value) { return value; }
inline void lv_obj_set_style_transform_scale_x(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_transform_scale_y(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_bg_color(lv_obj_t *, int, lv_style_selector_t) {}
inline void lv_obj_set_style_bg_grad_color(lv_obj_t *, lv_color_t, lv_style_selector_t) {}
inline void lv_obj_set_style_bg_grad_dir(lv_obj_t *, int, lv_style_selector_t) {}
inline void lv_obj_set_style_text_color(lv_obj_t *, lv_color_t, lv_style_selector_t) {}
inline lv_color_t lv_obj_get_style_text_color(lv_obj_t *, lv_style_selector_t) { return 0; }
inline void lv_obj_set_style_opa(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_text_opa(lv_obj_t *, int, int) {}
inline void lv_obj_add_state(lv_obj_t *, int) {}
inline void lv_obj_clear_state(lv_obj_t *, int) {}
inline bool lv_obj_has_state(lv_obj_t *, int) { return false; }
inline void lv_obj_add_flag(lv_obj_t *, int) {}
inline void lv_obj_clear_flag(lv_obj_t *, int) {}
inline uint32_t lv_obj_get_child_cnt(lv_obj_t *) { return 0; }
inline lv_obj_t *lv_obj_get_child(lv_obj_t *, uint32_t) { return nullptr; }
inline void lv_label_set_long_mode(lv_obj_t *, int) {}
inline void lv_obj_set_width(lv_obj_t *, int) {}
inline void lv_label_set_text(lv_obj_t *, const char *) {}
inline void lv_obj_align(lv_obj_t *, int, int, int) {}
inline void lv_obj_move_foreground(lv_obj_t *) {}

#include "temperature_unit.h"
#include "button_grid_config_pure.h"
#include "button_grid_layout.h"

int main() {
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
  auto image_label = parse_cfg("camera.front_door;Front Door;Auto;Auto;;;image;;image_label");
  assert(image_label.type == "image");
  assert(image_label.label == "Front Door");
  assert(image_label.options == "image_label");
  assert(image_card_label_enabled(image_label));
  auto image_label_refresh = parse_cfg("~camera.front_door,Front%20Door,Auto,Auto,,,image,,image_label%2Cimage_refresh=30%2Cimage_refresh_mode=timer");
  assert(image_label_refresh.label == "Front Door");
  assert(image_label_refresh.options == "image_label,image_refresh=30,image_refresh_mode=timer");
  assert(image_card_refresh_interval_ms(image_label_refresh) == 30000);
  assert(image_card_timer_only_refresh(image_label_refresh));
  auto image_fit_refresh = parse_cfg("~camera.front_door,,Auto,Auto,,,image,,image_modal_mode=fit%2Cimage_refresh=30%2Cimage_refresh_mode=timer");
  assert(image_fit_refresh.options == "image_modal_mode=fit,image_refresh=30,image_refresh_mode=timer");
  assert(image_card_modal_fit_enabled(image_fit_refresh));
  auto image_fill_default = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_modal_mode=fill,image_refresh=30");
  assert(image_fill_default.options == "image_refresh=30");
  assert(!image_card_modal_fit_enabled(image_fill_default));
  auto image_bad_modal = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_modal_mode=stretch,image_refresh=30");
  assert(image_bad_modal.options == "image_refresh=30");
  assert(!image_card_modal_fit_enabled(image_bad_modal));
  auto image_ignored_label = parse_cfg("camera.front_door;Front Door;Auto;Auto;;;image;;");
  assert(image_ignored_label.label == "");
  assert(!image_card_label_enabled(image_ignored_label));
  auto image_refresh = parse_cfg("~camera.front_door,,Auto,Auto,,,image,,image_refresh=30%2Cimage_refresh_mode=timer");
  assert(image_refresh.type == "image");
  assert(image_refresh.options == "image_refresh=30,image_refresh_mode=timer");
  assert(image_card_refresh_interval_ms(image_refresh) == 30000);
  assert(image_card_timer_only_refresh(image_refresh));
  auto image_refresh_default_mode = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_refresh=60,image_refresh_mode=bad");
  assert(image_refresh_default_mode.options == "image_refresh=60");
  assert(image_card_refresh_interval_ms(image_refresh_default_mode) == 60000);
  assert(!image_card_timer_only_refresh(image_refresh_default_mode));
  auto image_refresh_invalid = parse_cfg("camera.front_door;;Auto;Auto;;;image;;image_refresh=5,image_refresh_mode=timer");
  assert(image_refresh_invalid.options == "");
  assert(image_card_refresh_interval_ms(image_refresh_invalid) == 0);

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


def pure_config_header() -> str:
    text = CONFIG_HEADER.read_text(encoding="utf-8")
    marker = "inline const char* weather_icon_for_state"
    index = text.find(marker)
    if index < 0:
        raise RuntimeError(f"Could not find pure parser boundary in {CONFIG_HEADER}")
    return text[:index]


def main() -> int:
    cxx = compiler()
    if not cxx:
        print("::error::No C++ compiler found for firmware parser checks", file=sys.stderr)
        return 1
    with TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        (tmp_path / "button_grid_config_pure.h").write_text(pure_config_header(), encoding="utf-8")
        shutil.copy2(ROOT / "components" / "espcontrol" / "temperature_unit.h", tmp_path / "temperature_unit.h")
        shutil.copy2(CONTRACT_HEADER, tmp_path / "button_grid_contract_generated.h")
        shutil.copy2(CARD_RUNTIME_HEADER, tmp_path / "button_grid_card_runtime.h")
        shutil.copy2(LAYOUT_HEADER, tmp_path / "button_grid_layout.h")
        source = tmp_path / "check_firmware_parser.cpp"
        binary = tmp_path / "check_firmware_parser"
        source.write_text(CPP_SOURCE, encoding="utf-8")
        subprocess.run([cxx, "-std=c++17", "-Wall", "-Wextra", str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("Firmware parser checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
