"""Exercise production colour palettes, sensor backgrounds and state updates."""
from pathlib import Path
import subprocess
import sys
import tempfile

from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
headers = root / "components/espcontrol"
source = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>
struct lv_obj_t;
void lv_label_set_text(lv_obj_t *, const char *) {}
std::string espcontrol_i18n(const std::string &text) { return text; }
#include "button_grid_config_parser.h"
#define ESPCONTROL_SUBPAGE_PARSER_ONLY
#include "button_grid_subpages.h"
#include "display_color.h"
#include "button_grid_style.h"
using lv_color_t = uint32_t;
using lv_style_selector_t = int;
constexpr int LV_PART_MAIN = 0, LV_STATE_DEFAULT = 0, LV_STATE_CHECKED = 1, LV_STATE_PRESSED = 2;
constexpr int LV_EVENT_ALL = 0, LV_EVENT_PRESSED = 3, LV_EVENT_RELEASED = 4, LV_EVENT_PRESS_LOST = 5;
constexpr int LV_OPA_COVER = 255, LV_OPA_TRANSP = 0;
struct lv_event_t { lv_obj_t *target; int code; };
using lv_event_cb_t = void (*)(lv_event_t *);
struct lv_obj_t {
  uint32_t background[3] = {}, text[3] = {};
  int state = 0;
  std::vector<lv_event_cb_t> events;
  std::vector<lv_obj_t *> children;
  std::string content;
};
struct BtnSlot { lv_obj_t *btn, *icon_lbl = nullptr, *text_lbl = nullptr; };
struct AlarmCardCtx { lv_obj_t *btn; uint32_t off_color; };
struct DisplayProfile {
  struct { int red_percent = 100, green_percent = 100, blue_percent = 100; } color;
};
uint32_t lv_color_hex(uint32_t rgb) { return rgb; }
void lv_obj_set_style_bg_color(lv_obj_t *obj, uint32_t rgb, int selector) { obj->background[selector] = rgb; }
void lv_obj_set_style_text_color(lv_obj_t *obj, uint32_t rgb, int selector) { obj->text[selector] = rgb; }
void lv_obj_set_style_bg_opa(lv_obj_t *, int, int) {}
void lv_obj_set_style_border_color(lv_obj_t *, uint32_t, int) {}
void lv_obj_set_style_border_width(lv_obj_t *, int, int) {}
void lv_obj_set_style_shadow_width(lv_obj_t *, int, int) {}
uint32_t lv_obj_get_style_text_color(lv_obj_t *obj, int) {
  return obj->text[(obj->state & LV_STATE_PRESSED) ? LV_STATE_PRESSED
    : ((obj->state & LV_STATE_CHECKED) ? LV_STATE_CHECKED : LV_STATE_DEFAULT)];
}
bool lv_obj_has_state(lv_obj_t *obj, int state) { return (obj->state & state) != 0; }
void lv_obj_add_state(lv_obj_t *obj, int state) { obj->state |= state; }
void lv_obj_clear_state(lv_obj_t *obj, int state) { obj->state &= ~state; }
int lv_event_get_code(lv_event_t *event) { return event->code; }
lv_obj_t *lv_event_get_target(lv_event_t *event) { return event->target; }
void lv_obj_remove_event_cb_with_user_data(lv_obj_t *obj, lv_event_cb_t callback, void *) {
  auto &events = obj->events;
  events.erase(std::remove(events.begin(), events.end(), callback), events.end());
}
void lv_obj_add_event_cb(lv_obj_t *obj, lv_event_cb_t callback, int, void *) { obj->events.push_back(callback); }
void send_event(lv_obj_t *obj, int code) {
  lv_event_t event{obj, code};
  for (auto callback : obj->events) callback(&event);
}
inline void bind_card_text_contrast_events(lv_obj_t *);
std::map<std::string, std::function<void(esphome::StringRef)>> subscriptions;
void ha_subscribe_state(const std::string &entity, std::function<void(esphome::StringRef)> callback) {
  subscriptions[entity] = callback;
}
void subscribe_friendly_name(lv_obj_t *, const std::string &) {}
void lv_label_set_display_text(lv_obj_t *label, const char *text) { label->content = text; }
namespace espcontrol::cards {
const char *status_entity_driver_inactive_icon(const ParsedCfg &, const Context &) { return "off"; }
const char *status_entity_driver_active_icon(const ParsedCfg &, const Context &) { return "on"; }
}
size_t lv_obj_get_child_cnt(lv_obj_t *obj) { return obj->children.size(); }
lv_obj_t *lv_obj_get_child(lv_obj_t *obj, size_t index) { return obj->children[index]; }
'''
grid = (headers / "button_grid_grid.h").read_text()
source += grid[grid.index("struct CardPalette {"):grid.index("inline CardPalette card_palette_for_config")]
groups = (
    ("button_grid_display.h", "", ("display_correct_color",)),
    ("button_grid_layout.h", "", ("parse_hex_color", "apply_button_colors", "apply_card_descendant_text_color", "sync_card_checked_text_color", "card_text_contrast_event_cb", "bind_card_text_contrast_events", "set_card_checked_state")),
    ("button_grid_grid.h", "", ("card_palette_for_config",)),
    ("button_grid_sensor_driver.h", "espcontrol::cards", ("sensor_driver_apply_background",)),
    ("button_grid_weather_driver.h", "espcontrol::cards", ("weather_driver_apply_background",)),
    ("button_grid_subscriptions.h", "", ("apply_sensor_active_color",)),
    ("button_grid_status_entity_driver.h", "espcontrol::cards", ("status_entity_driver_matches", "status_entity_driver_state_active", "status_entity_driver_active_color_enabled", "status_entity_driver_bind_data")),
    ("button_grid_alarm.h", "", ("alarm_set_card_state_colors",)),
    ("button_grid_fan.h", "", ("fan_control_style_binary_button",)),
    ("button_grid_climate.h", "", ("climate_style_range_target_button",)),
)
for filename, namespace, names in groups:
    text = (headers / filename).read_text()
    if namespace:
        source += "\nnamespace " + namespace + " {\n"
    for name in names:
        line, code = definition(text, name)
        source += f'\n#line {line} "{filename}"\n{code}\n'
    if namespace:
        source += "\n}\n"
source += r'''
#line 1 "card_colour_rendering_cases"
int main() {
  CardPalette defaults;
  defaults.has_on = defaults.has_off = defaults.has_sensor_color = true;
  defaults.on_val = 0xFF8C00;
  defaults.off_val = 0x313131;
  defaults.sensor_val = 0x212121;
  DisplayProfile display;
  struct Colour { const char *hex; uint32_t base, active, text; };
  const Colour colours[] = {
    {"6633B9", 0x6633B9, 0x9470CE, 0xFFFFFF},
    {"FF0000", 0xFF0000, 0xFF4D4D, 0xFFFFFF},
    {"F6402C", 0xF6402C, 0xF9796B, 0xFFFFFF},
    {"FF8C00", 0xFF8C00, 0xFFAF4D, 0xFFFFFF},
    {"1093F5", 0x1093F5, 0x58B3F8, 0xFFFFFF},
    {"46AF4A", 0x46AF4A, 0x7EC780, 0xFFFFFF},
    {"009687", 0x009687, 0x4DB6AB, 0xFFFFFF},
    {"00BBD5", 0x00BBD5, 0x4DCFE2, 0xFFFFFF},
    {"B2B2B2", 0xB2B2B2, 0xC9C9C9, 0xFFFFFF},
    {"B3B3B3", 0xB3B3B3, 0xCACACA, 0x212121},
    {"88C440", 0x88C440, 0xACD679, 0x212121},
    {"CCDD1E", 0xCCDD1E, 0xDBE762, 0x212121},
    {"FFFFFF", 0xFFFFFF, 0xFFFFFF, 0x212121},
    {"FFEC16", 0xFFEC16, 0xFFF25C, 0x212121},
    {"000000", 0x000000, 0x4D4D4D, 0xFFFFFF},
  };
  for (const auto &colour : colours) {
    lv_obj_t modal_button, modal_label;
    modal_button.children = {&modal_label};
    fan_control_style_binary_button(&modal_button, true, colour.active, SECONDARY_GREY);
    assert(modal_label.text[0] == display_text_color_for_bg(colour.active));
    climate_style_range_target_button(&modal_button, true, colour.active);
    assert(modal_label.text[0] == display_text_color_for_bg(colour.active));
    for (const std::string type : {"sensor", "local_sensor"}) {
      for (const std::string precision : {"0", "text", "icon", "time"}) {
        const std::string options = "card_off_color=" + std::string(colour.hex);
        const std::string main = "sensor.temp;Temperature;Thermometer;Auto;sensor.temp;C;" + type + ";" + precision + ";" + options;
        const std::string legacy = "B,1|sensor.temp:Temperature:Thermometer:Auto:sensor.temp:C:" + type + ":" + precision + ":" + options;
        const std::string compact = "~B,1|" + type + ",sensor.temp,Temperature,Thermometer,,sensor.temp,C," + precision + "," + options;
        const ParsedCfg configs[] = {
          parse_cfg(main),
          parsed_cfg_from_subpage_btn(parse_subpage_config(legacy).at(0)),
          parsed_cfg_from_subpage_btn(parse_subpage_config(compact).at(0)),
        };
        for (const auto &config : configs) {
          const auto palette = card_palette_for_config(defaults, config, display);
          lv_obj_t button, label, value, unit;
          button.children = {&label, &value, &unit};
          BtnSlot slot{&button};
          apply_button_colors(&button, palette.has_on, palette.on_val, palette.has_off, palette.off_val);
          sync_card_checked_text_color(&button);
          espcontrol::cards::sensor_driver_apply_background(slot, palette);
          assert(button.background[LV_STATE_DEFAULT] == colour.base);
          assert(button.background[LV_STATE_CHECKED] == colour.active);
          assert(label.text[0] == colour.text && value.text[0] == colour.text && unit.text[0] == colour.text);
          apply_button_colors(&button, palette.has_on, palette.on_val, palette.has_off, palette.off_val);
          assert(button.events.size() == 1);
          button.state = LV_STATE_PRESSED;
          send_event(&button, LV_EVENT_PRESSED);
          assert(value.text[0] == display_text_color_for_bg(colour.active));
          button.state = 0;
          send_event(&button, LV_EVENT_RELEASED);
          assert(value.text[0] == colour.text);
          button.state = LV_STATE_PRESSED;
          send_event(&button, LV_EVENT_PRESSED);
          button.state = 0;
          send_event(&button, LV_EVENT_PRESS_LOST);
          assert(value.text[0] == colour.text);
          set_card_checked_state(&button, true);
          assert(value.text[0] == display_text_color_for_bg(colour.active));
          set_card_checked_state(&button, false);
          assert(value.text[0] == colour.text);
          for (const char *state : {"42", "0", "42"}) {
            apply_sensor_active_color(&button, true, esphome::StringRef(state), palette.on_val, palette.sensor_val, false, true);
            const auto expected = std::string(state) == "0" ? colour.base : colour.active;
            assert(button.background[0] == expected);
            assert(value.text[0] == display_text_color_for_bg(expected));
            assert(label.text[0] == value.text[0] && unit.text[0] == value.text[0]);
          }
          apply_sensor_active_color(&button, true, esphome::StringRef("unavailable"), palette.on_val, palette.sensor_val, true);
          assert(button.background[0] == colour.base);
          espcontrol::cards::weather_driver_apply_background(slot, palette);
          assert(button.background[0] == colour.base);
        }
      }
    }
    for (const std::string type : {"door_window", "presence"}) {
      const std::string options = "active_color,card_off_color=" + std::string(colour.hex);
      const ParsedCfg configs[] = {
        parse_cfg(";Status;Auto;Auto;binary_sensor.status;;" + type + ";;" + options),
        parsed_cfg_from_subpage_btn(parse_subpage_config("B,1|:Status:Auto:Auto:binary_sensor.status::" + type + "::" + options).at(0)),
        parsed_cfg_from_subpage_btn(parse_subpage_config("~B,1|" + type + ",,Status,,,binary_sensor.status,,,active_color%2Ccard_off_color=" + colour.hex).at(0)),
      };
      for (size_t index = 0; index < 3; ++index) {
        const auto &config = configs[index];
        const auto context = card_runtime_context(config, index == 0
          ? espcontrol::cards::Surface::MAIN_GRID : espcontrol::cards::Surface::SUBPAGE);
        const auto palette = card_palette_for_config(defaults, config, display);
        lv_obj_t button, label, icon;
        button.children = {&label, &icon};
        BtnSlot slot{&button, &icon, &label};
        apply_button_colors(&button, palette.has_on, palette.on_val, palette.has_off, palette.off_val);
        sync_card_checked_text_color(&button);
        subscriptions.clear();
        assert(espcontrol::cards::status_entity_driver_bind_data(slot, config, context, palette));
        assert(subscriptions.count(config.sensor) == 1);
        for (const char *state : {"on", "off", "unavailable", "on"}) {
          subscriptions.at(config.sensor)(esphome::StringRef(state));
          const auto expected = std::string(state) == "on" ? colour.active : colour.base;
          assert(button.background[0] == expected);
          assert(label.text[0] == display_text_color_for_bg(expected));
          assert(icon.text[0] == label.text[0]);
        }
      }
    }
  }
  for (const char *options : {"", "card_off_color=invalid"}) {
    ParsedCfg config;
    config.options = options;
    const auto palette = card_palette_for_config(defaults, config, display);
    assert(palette.on_val == defaults.on_val && palette.off_val == defaults.off_val);
    assert(palette.sensor_val == defaults.sensor_val);
  }
  ParsedCfg config;
  config.options = "card_off_color=FF8C00";
  display.color.red_percent = 50;
  display.color.green_percent = 75;
  const auto corrected = card_palette_for_config(defaults, config, display);
  assert(corrected.sensor_val == 0x7F6900);
  assert(corrected.off_val == corrected.sensor_val);
  assert(corrected.on_val == 0x7F834D);
  assert(defaults.sensor_val == 0x212121);
  const auto explicit_colour = card_palette_for_config(CardPalette{}, config, DisplayProfile{});
  assert(explicit_colour.has_on && explicit_colour.has_off && explicit_colour.has_sensor_color);
  assert(explicit_colour.sensor_val == 0xFF8C00);
  lv_obj_t alarm, alarm_label;
  alarm.children = {&alarm_label};
  AlarmCardCtx alarm_context{&alarm, 0xFFEC16};
  alarm_set_card_state_colors(&alarm_context, 0xFFF25C);
  set_card_checked_state(&alarm, true);
  assert(alarm_label.text[0] == 0x212121);
  alarm_set_card_state_colors(&alarm_context, 0xC62828);
  assert(alarm.background[LV_STATE_CHECKED] == 0xC62828);
  assert(alarm_label.text[0] == 0xFFFFFF);
  alarm_set_card_state_colors(&alarm_context, 0xFFF25C);
  assert(alarm_label.text[0] == 0x212121);
}
'''
with tempfile.TemporaryDirectory(prefix="card-colour-rendering-") as directory:
    cpp = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(root / "tests/firmware/stubs"), "-I" + str(headers), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Card colour rendering checks passed: main/subpage sensor backgrounds, state updates, contrast, defaults and display correction.")
