"""Run production confirmation dispatch with fake Home Assistant and LVGL."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
firmware = root / "components/espcontrol"


def function(filename, name):
    source = (firmware / filename).read_text()
    start = source.rfind("\ninline ", 0, source.index(name + "(")) + 1
    end = source.index("\n}", start) + 2
    return source[start:end]


harness = r'''
#include <cassert>
#include <string>
#include <vector>
struct ParsedCfg { std::string type, entity, sensor, options; };
struct lv_obj_t { bool checked = false; };
constexpr int LV_STATE_CHECKED = 1;
void lv_obj_add_state(lv_obj_t *button, int) { button->checked = true; }
void lv_obj_clear_state(lv_obj_t *button, int) { button->checked = false; }
void set_card_checked_state(lv_obj_t *button, bool checked) { button->checked = checked; }
struct SwitchConfirmationModalUi {
  void *overlay = nullptr;
  lv_obj_t *btn_obj = nullptr;
  ParsedCfg cfg;
  bool turn_on = false;
};
SwitchConfirmationModalUi ui;
SwitchConfirmationModalUi &switch_confirmation_modal_ui() { return ui; }
enum class ControlModalKind { SWITCH_CONFIRMATION };
int dismissed = 0;
void control_modal_delete_overlay(ControlModalKind, void *) { ++dismissed; }
std::vector<std::string> actions;
void ha_send_entity_action(const std::string &entity, const char *service) {
  actions.push_back(std::string(service) + ":" + entity);
}
bool action_script_confirmation_enabled(const ParsedCfg &cfg) {
  return cfg.type == "action" && cfg.sensor == "script.turn_on";
}
bool garage_command_mode(const std::string &mode) {
  return mode == "open" || mode == "close" || mode == "stop";
}
void send_action_card_action(const ParsedCfg &cfg) {
  ha_send_entity_action(cfg.entity, cfg.sensor.c_str());
}
void send_cover_command_action(const ParsedCfg &cfg) {
  ha_send_entity_action(cfg.entity, ("cover." + cfg.sensor + "_cover").c_str());
}
void send_cover_command_action(const std::string &entity, const std::string &mode) {
  ParsedCfg cfg;
  cfg.entity = entity;
  cfg.sensor = mode;
  send_cover_command_action(cfg);
}
'''
for name in ("send_turn_on_action", "send_turn_off_action", "is_cover_entity"):
    harness += function("button_grid_actions.h", name) + "\n"
for name in ("switch_confirmation_hide_modal", "switch_confirmation_confirm"):
    harness += function("button_grid_confirm.h", name) + "\n"
harness += r'''
void check(const std::string &type, const std::string &entity,
           const std::string &mode, bool pending_on, const std::string &service,
           bool state_changes = true) {
  lv_obj_t button{!pending_on};
  ui.cfg = {type, entity, mode, "confirm_on=1;confirm_off=1"};
  ui.btn_obj = &button;
  ui.overlay = &button;
  ui.turn_on = pending_on;
  actions.clear();
  int before = dismissed;
  switch_confirmation_confirm();
  assert(actions.size() == 1);
  assert(actions.front() == service + ":" + entity);
  assert(button.checked == (state_changes ? pending_on : !pending_on));
  assert(dismissed == before + 1 && !ui.overlay && !ui.btn_obj);
}
int main() {
  // Both main-grid and subpage confirmations share this dispatch. The pending
  // direction must remain fixed even if HA state changes while the dialog is open.
  for (const auto &type : {"garage", ""}) {
    check(type, "cover.garage", "", true, "cover.open_cover");
    check(type, "cover.garage", "", false, "cover.close_cover");
  }
  check("garage", "cover.garage", "open", true, "cover.open_cover", false);
  check("garage", "cover.garage", "close", false, "cover.close_cover", false);
  check("", "switch.fan", "", true, "homeassistant.turn_on");
  check("", "switch.fan", "", false, "homeassistant.turn_off");
  check("action", "script.evening", "script.turn_on", false, "script.turn_on", false);
  // Cancelling/backing out uses the same hide function and must send no command.
  actions.clear();
  ui.cfg = {"garage", "cover.garage", "", "confirm_on=1"};
  ui.turn_on = true;
  switch_confirmation_hide_modal();
  assert(actions.empty());
  // A stale second confirm after dismissal cannot resend the previous action.
  switch_confirmation_confirm();
  assert(actions.empty());
}
'''
with tempfile.TemporaryDirectory(prefix="cover-confirmation-") as directory:
    cpp = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    cpp.write_text(harness)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17",
                    "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Cover confirmation checks passed.")
