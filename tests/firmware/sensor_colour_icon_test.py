"""Compile the production icon subscription and colour observer against an LVGL boundary."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIRMWARE = ROOT / "components/espcontrol"


def definition(filename, name):
    text = (FIRMWARE / filename).read_text()
    start = text.rfind("\n", 0, text.index(name + "(")) + 1
    brace = re.search(r"\)\s*\{", text[start:]).end() + start - 1
    depth = 0
    for offset in range(brace, len(text)):
        if text[offset] == "{":
            depth += 1
        elif text[offset] == "}":
            depth -= 1
            if depth == 0:
                return text[start:offset + 1]
    raise ValueError(name)


PREFIX = r'''
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "theme_palette.h"
namespace esphome {
struct StringRef { const char *value; StringRef(const char *v): value(v) {} };
}
struct lv_obj_t {
  bool label = false, checked = false, custom_colour = false;
  uint32_t background = 0, foreground = 0xFFFFFF;
  std::string text;
  std::vector<lv_obj_t *> children;
};
struct ParsedCfg { std::string sensor, icon, icon_on; };
struct SensorColourRules { std::string state; uint32_t colour; bool has_default_colour = false; uint32_t default_colour = 0; };
using lv_style_selector_t = int;
constexpr int LV_PART_MAIN = 0, LV_STATE_DEFAULT = 0, LV_STATE_CHECKED = 1;
constexpr uint32_t TERTIARY_GREY = 0x212121, DARK_TEXT_PRIMARY = 0xFFFFFF;
int lv_label_class = 1;
inline uint32_t current_grid_sensor_surface_color() { return theme_display_color(current_theme().surface_sensor); }
inline void theme_set_content_background(lv_obj_t *obj, bool owned = true) { obj->custom_colour = owned; }
inline uint32_t lv_color_hex(uint32_t colour) { return colour; }
inline bool lv_obj_check_type(lv_obj_t *obj, int *) { return obj->label; }
inline size_t lv_obj_get_child_cnt(lv_obj_t *obj) { return obj->children.size(); }
inline lv_obj_t *lv_obj_get_child(lv_obj_t *obj, size_t i) { return obj->children[i]; }
inline void lv_obj_set_style_text_color(lv_obj_t *obj, uint32_t colour, int) { obj->foreground = colour; }
inline void lv_obj_set_style_bg_color(lv_obj_t *obj, uint32_t colour, int) { obj->background = colour; }
inline void lv_obj_clear_state(lv_obj_t *obj, int) { obj->checked = false; }
inline void sync_default_text(lv_obj_t *obj) {
  if (obj->label) obj->foreground = 0xFFFFFF;
  for (auto *child : obj->children) sync_default_text(child);
}
inline void set_card_checked_state(lv_obj_t *obj, bool checked) {
  obj->checked = checked;
  sync_default_text(obj); // Model the real palette resync which used to erase custom contrast.
}
inline const char *find_icon(const char *icon) { return icon; }
inline void lv_label_set_display_text(lv_obj_t *obj, const char *text) { obj->text = text; }
inline bool ha_state_unavailable_ref(esphome::StringRef state) { return std::string(state.value) == "unavailable"; }
inline bool is_entity_on_ref(esphome::StringRef state) { return std::string(state.value) == "on"; }
inline bool sensor_colour_matches(const SensorColourRules &rules, esphome::StringRef state, uint32_t &colour) {
  if (std::string(state.value) != rules.state) return false;
  colour = rules.colour;
  return true;
}
std::function<void(esphome::StringRef)> subscribed;
inline void ha_subscribe_state(const std::string &, std::function<void(esphome::StringRef)> callback) { subscribed = callback; }
'''

TEST = r'''
int main() {
  lv_obj_t button, icon, label;
  icon.label = label.label = true;
  button.children = {&icon, &label};
  ParsedCfg config{"sensor.running", "off-icon", "on-icon"};
  for (uint32_t colour : {0xFFFFFFu, 0x000000u}) {
    SensorColourRules rules{"on", colour};
    assert(sensor_driver_default_colour(rules) == TERTIARY_GREY);
    rules.has_default_colour = true;
    rules.default_colour = 0xFFFFFF;
    assert(sensor_driver_default_colour(rules) == 0xFFFFFF);
    auto default_observer = sensor_driver_colour_observer(&button, rules);
    default_observer("unavailable");
    assert(button.background == 0xFFFFFF && icon.foreground == 0x212121);
    default_observer("off");
    assert(button.background == 0xFFFFFF && icon.foreground == 0x212121);
    default_observer("on");
    assert(button.background == colour);
    rules.default_colour = 0x000000;
    auto black_default = sensor_driver_colour_observer(&button, rules);
    black_default("off");
    assert(button.background == 0 && icon.foreground == 0xFFFFFF);
    rules.has_default_colour = false;
    auto observer = sensor_driver_colour_observer(&button, rules);
    subscribe_sensor_icon_state(&button, &icon, config, false, observer, true);
    subscribed("on");
    assert(button.background == colour && !button.checked);
    assert(icon.text == "on-icon");
    const uint32_t expected = colour == 0xFFFFFF ? 0x212121 : 0xFFFFFF;
    assert(icon.foreground == expected && label.foreground == expected);
    subscribed("unavailable");
    assert(button.background == 0x212121 && icon.foreground == 0xFFFFFF && label.foreground == 0xFFFFFF);
    subscribed("on");
    assert(icon.foreground == expected && label.foreground == expected);
    // A separate colour source can update first; displayed icon updates must preserve it.
    subscribe_sensor_icon_state(&button, &icon, config, false, {}, true);
    observer("on");
    for (const char *state : {"off", "on", "unavailable", "on"}) {
      subscribed(state);
      assert(button.background == colour && !button.checked);
      assert(icon.foreground == expected && label.foreground == expected);
    }
    observer("unavailable");
    assert(button.background == 0x212121 && icon.foreground == 0xFFFFFF);
  }
  SensorColourRules theme_rules{"on", 0xFFFFFF};
  auto theme_observer = sensor_driver_colour_observer(&button, theme_rules);
  set_active_theme_palette(LIGHT_THEME);
  theme_observer("off");
  assert(button.background == 0xF5F5F5 && icon.foreground == 0x333333 && !button.custom_colour);
  theme_observer("on");
  assert(button.background == 0xFFFFFF && icon.foreground == 0x212121 && button.custom_colour);
  set_active_theme_palette(DARK_THEME);
  theme_observer("off");
  assert(button.background == 0x212121 && icon.foreground == 0xFFFFFF && !button.custom_colour);
  subscribe_sensor_icon_state(&button, &icon, config, true);
  subscribed("on");
  assert(button.checked && icon.foreground == 0xFFFFFF);
  subscribed("off");
  assert(!button.checked && icon.text == "off-icon");
}
'''

code = PREFIX + definition("button_grid_style.h", "readable_text_color_for_bg")
for name in ("sensor_driver_apply_content_colour", "sensor_driver_apply_colour", "sensor_driver_default_colour", "sensor_driver_colour_observer"):
    code += "\n" + definition("button_grid_sensor_driver.h", name)
code += "\n" + definition("button_grid_subscriptions.h", "subscribe_sensor_icon_state") + TEST
with tempfile.TemporaryDirectory() as directory:
    source, binary = Path(directory) / "sensor_icon.cpp", Path(directory) / "sensor_icon"
    source.write_text(code)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I" + str(FIRMWARE), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Sensor icon colour runtime checks passed.")
