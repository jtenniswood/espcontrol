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
#define LVGL_VERSION_MAJOR 8
using lv_color_t = uint32_t;
using lv_style_selector_t = int;
constexpr int LV_PART_MAIN = 0, LV_STATE_DEFAULT = 0, LV_STATE_CHECKED = 1, LV_STATE_PRESSED = 2;
struct lv_obj_t {
  uint32_t background[3] = {}, text[3] = {};
  std::vector<lv_obj_t *> children;
};
struct BtnSlot { lv_obj_t *btn; };
struct DisplayProfile {
  struct { int red_percent = 100, green_percent = 100, blue_percent = 100; } color;
};
uint32_t lv_color_hex(uint32_t rgb) { return rgb; }
void lv_obj_set_style_bg_color(lv_obj_t *obj, uint32_t rgb, int selector) { obj->background[selector] = rgb; }
void lv_obj_set_style_text_color(lv_obj_t *obj, uint32_t rgb, int selector) { obj->text[selector] = rgb; }
uint32_t lv_obj_get_style_text_color(lv_obj_t *obj, int selector) { return obj->text[selector]; }
bool lv_obj_has_state(lv_obj_t *, int) { return false; }
size_t lv_obj_get_child_cnt(lv_obj_t *obj) { return obj->children.size(); }
lv_obj_t *lv_obj_get_child(lv_obj_t *obj, size_t index) { return obj->children[index]; }
'''
grid = (headers / "button_grid_grid.h").read_text()
source += grid[grid.index("struct CardPalette {"):grid.index("inline CardPalette card_palette_for_config")]
groups = (
    ("button_grid_display.h", "", ("display_correct_color",)),
    ("button_grid_layout.h", "", ("parse_hex_color", "apply_button_colors", "apply_card_descendant_text_color", "sync_card_checked_text_color")),
    ("button_grid_grid.h", "", ("card_palette_for_config",)),
    ("button_grid_sensor_driver.h", "espcontrol::cards", ("sensor_driver_apply_background",)),
    ("button_grid_weather_driver.h", "espcontrol::cards", ("weather_driver_apply_background",)),
    ("button_grid_subscriptions.h", "", ("apply_sensor_active_color",)),
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
    {"FF8C00", 0xFF8C00, 0xFFAF4D, 0xFFFFFF},
    {"FFEC16", 0xFFEC16, 0xFFF25C, 0x212121},
    {"000000", 0x000000, 0x4D4D4D, 0xFFFFFF},
  };
  for (const auto &colour : colours) {
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
