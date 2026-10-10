"""Exercise the production fan tile refresh with host LVGL doubles."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
firmware = root / "components/espcontrol"


def definition(filename, name):
    source = (firmware / filename).read_text()
    matches = re.findall(
        rf"^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}", source, re.M | re.S
    )
    assert len(matches) == 1, name
    return matches[0]


source = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
struct lv_obj_t { std::string text; bool checked = false; };
struct lv_timer_t {};
void lv_label_set_display_text(lv_obj_t *label, const char *text) { label->text = text; }
void lv_timer_pause(lv_timer_t *) {}
void lv_timer_reset(lv_timer_t *) {}
void lv_timer_resume(lv_timer_t *) {}
constexpr int LV_STATE_CHECKED = 1;
void set_card_checked_state(lv_obj_t *button, bool checked) { button->checked = checked; }
std::string espcontrol_i18n(const std::string &text) { return text; }
std::string fan_option_label(const std::string &text) { return text; }
bool fan_preset_active(const std::string &text) { return !text.empty() && text != "none"; }
'''
access = (firmware / "button_grid_access_cards.h").read_text()
source += re.search(r"^struct TransientStatusLabel \{.*?^\};", access, re.M | re.S)[0]
for name in ("transient_status_label_set_steady", "transient_status_label_show_if_changed"):
    source += "\n" + definition("button_grid_access_cards.h", name)
source += r'''
struct FanCardCtx {
  std::string type = "fan_control", label, friendly_name, preset_mode, direction;
  std::vector<std::string> preset_modes;
  bool available = true, on = false, percentage_known = false;
  bool oscillating = false, oscillation_known = false, direction_known = false;
  int percentage = 0;
  lv_obj_t *btn = nullptr, *icon_lbl = nullptr;
  const char *icon_on_glyph = "fan-on", *icon_off_glyph = "fan-off";
  TransientStatusLabel *status_label = nullptr;
};
'''
for name in ("fan_apply_card_visual", "fan_status_text", "fan_control_card_title",
             "fan_control_refresh_card"):
    source += "\n" + definition("button_grid_fan.h", name)
source += r'''
int main() {
  lv_obj_t button, icon, label;
  TransientStatusLabel status;
  status.label = &label;
  FanCardCtx fan;
  fan.btn = &button;
  fan.icon_lbl = &icon;
  fan.status_label = &status;
  fan.label = "Bedroom fan";
  fan.friendly_name = "Home Assistant fan";
  fan_control_refresh_card(&fan);
  assert(label.text == "Bedroom fan");
  assert(!button.checked && icon.text == "fan-off");

  // Home Assistant speed updates must not replace the tile's name.
  fan.on = true;
  fan.percentage_known = true;
  for (int percentage : {25, 50, 100}) {
    fan.percentage = percentage;
    fan_control_refresh_card(&fan);
    assert(label.text == "Bedroom fan");
    assert(button.checked && icon.text == "fan-on");
  }
  fan.on = false;
  fan_control_refresh_card(&fan);
  assert(label.text == "Bedroom fan");
  assert(!button.checked && icon.text == "fan-off");
  fan.percentage_known = false;
  fan.preset_mode = "sleep";
  fan_control_refresh_card(&fan);
  assert(label.text == "Bedroom fan");
  fan.available = false;
  fan_control_refresh_card(&fan);
  assert(label.text == "Bedroom fan");
  fan.available = true;
  fan_control_refresh_card(&fan);
  assert(label.text == "Bedroom fan");

  // Label edits and friendly-name updates apply even after state updates.
  fan.label = "Office fan";
  fan_control_refresh_card(&fan);
  assert(label.text == "Office fan");
  fan.label.clear();
  fan_control_refresh_card(&fan);
  assert(label.text == "Home Assistant fan");
  fan.friendly_name = "Renamed fan";
  fan_control_refresh_card(&fan);
  assert(label.text == "Renamed fan");
  fan.friendly_name.clear();
  fan_control_refresh_card(&fan);
  assert(label.text == "Fan");
  fan_control_refresh_card(nullptr);
}
'''
with tempfile.TemporaryDirectory(prefix="fan-control-label-") as temp:
    cpp = Path(temp) / "test.cpp"
    binary = Path(temp) / "test"
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17",
                    "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Fan All Controls labels, title fallbacks and power visuals passed.")
