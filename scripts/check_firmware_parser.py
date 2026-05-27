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
constexpr int LV_PART_MAIN = 0;
constexpr int LV_STATE_CHECKED = 1;
constexpr int LV_STATE_PRESSED = 2;
constexpr int LV_STATE_DEFAULT = 0;
constexpr int LV_STATE_DISABLED = 4;
constexpr int LV_LABEL_LONG_WRAP = 0;
constexpr int LV_ALIGN_BOTTOM_LEFT = 0;
constexpr int LV_OPA_COVER = 255;
constexpr int LV_OPA_50 = 128;
constexpr int LV_OBJ_FLAG_CLICKABLE = 1;
inline int lv_color_hex(uint32_t value) { return static_cast<int>(value); }
inline int lv_pct(int value) { return value; }
inline void lv_obj_set_style_transform_scale_x(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_transform_scale_y(lv_obj_t *, int, int) {}
inline void lv_obj_set_style_bg_color(lv_obj_t *, int, lv_style_selector_t) {}
inline void lv_obj_set_style_opa(lv_obj_t *, int, int) {}
inline void lv_obj_add_state(lv_obj_t *, int) {}
inline void lv_obj_clear_state(lv_obj_t *, int) {}
inline void lv_obj_add_flag(lv_obj_t *, int) {}
inline void lv_obj_clear_flag(lv_obj_t *, int) {}
inline void lv_label_set_long_mode(lv_obj_t *, int) {}
inline void lv_obj_set_width(lv_obj_t *, int) {}
inline void lv_label_set_text(lv_obj_t *, const char *) {}
inline void lv_obj_align(lv_obj_t *, int, int, int) {}

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
  auto volume_uncapped = parse_cfg("media_player.kitchen;Kitchen;Auto;Auto;volume;;media;;volume_max=150");
  assert(volume_uncapped.options == "");
  assert(media_volume_max_percent(volume_uncapped) == 100);

  auto confirm = parse_cfg("switch.printer;Printer;Printer 3D;Auto;;;;;confirm_off,confirm_message=Stop%20print%3F,confirm_yes=Power%20Down");
  assert(switch_confirmation_enabled(confirm));
  assert(switch_confirmation_message(confirm) == "Stop print?");
  assert(switch_confirmation_yes_text(confirm) == "Power Down");
  assert(switch_confirmation_no_text(confirm) == "No");

  auto todo = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo");
  assert(todo.entity == "todo.shopping");
  assert(todo.label == "Shopping");
  assert(todo.icon == "Check");
  assert(todo.icon_on == "Auto");
  assert(todo.type == "todo");
  assert(todo.options == "");

  auto todo_icon = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo;;count_display=icon");
  assert(todo_icon.options == "count_display=icon");
  assert(!todo_card_show_count(todo_icon));

  auto todo_label_count = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo;;label_display=count");
  assert(todo_label_count.options == "label_display=count");
  assert(todo_card_label_shows_count(todo_label_count));

  auto todo_hide_completed = parse_cfg("todo.shopping;Shopping;Check;Auto;;;todo;;completed_display=hide");
  assert(todo_hide_completed.options == "completed_display=hide");
  assert(!todo_card_shows_completed_items(todo_hide_completed));

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
