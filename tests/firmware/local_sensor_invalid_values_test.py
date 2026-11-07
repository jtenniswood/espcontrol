"""Execute production local Sensor binding and updates with non-finite readings."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIRMWARE = ROOT / "components/espcontrol"


def definition(filename, name, kind="inline"):
    source = (FIRMWARE / filename).read_text()
    pattern = (rf"^struct {name} \{{\n.*?^\}};" if kind == "struct" else
               rf"^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}")
    matches = re.findall(pattern, source, re.M | re.S)
    assert len(matches) == 1, name
    return matches[0] + "\n"


code = r'''
#include "page_local_sensor_feed.h"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>
#include "esphome/core/string_ref.h"
#define USE_SENSOR
struct lv_event_t;
struct lv_obj_t {
  std::string text;
  std::vector<void (*)(lv_event_t *)> callbacks{};
};
struct lv_event_t { lv_obj_t *target; };
constexpr int LV_EVENT_DELETE = 1;
void *lv_event_get_target(lv_event_t *event) { return event->target; }
void lv_obj_remove_event_cb(lv_obj_t *owner, void (*callback)(lv_event_t *)) {
  owner->callbacks.erase(std::remove(owner->callbacks.begin(), owner->callbacks.end(), callback), owner->callbacks.end());
}
void lv_obj_add_event_cb(lv_obj_t *owner, void (*callback)(lv_event_t *), int, void *) {
  owner->callbacks.push_back(callback);
}
void lv_label_set_display_text(lv_obj_t *label, const char *text) { label->text = text; }
void set_wrapped_button_label_text(lv_obj_t *label, const std::string &text) { label->text = text; }
struct FakeSensor {
  float state = NAN;
  std::vector<std::function<void(float)>> callbacks;
  esphome::StringRef get_object_id_to(char *) { return esphome::StringRef("room_temp"); }
  void add_on_state_callback(std::function<void(float)> cb) { callbacks.push_back(cb); }
  void publish(float value) { state = value; for (auto &cb : callbacks) cb(value); }
};
namespace esphome {
struct FakeApp { std::vector<FakeSensor *> sensors; const auto &get_sensors() { return sensors; } } App;
}
struct ParsedCfg { std::string entity, precision; };
struct BtnSlot { lv_obj_t *btn, *sensor_lbl, *text_lbl; };
'''
for name in ("LocalSensorControl", "LocalSensorCallbackBinding"):
    code += definition("button_grid_local_controls.h", name, "struct")
for name in ("local_sensor_registry", "local_sensor_unregister_owner", "local_sensor_owner_deleted",
             "local_sensor_callback_bindings", "local_sensor_callback_registered",
             "local_sensor_render_numeric", "local_sensor_apply_value", "local_sensor_apply_text"):
    code += definition("button_grid_local_controls.h", name)
code += definition("button_grid_sensor_driver.h", "sensor_driver_register_local_value")
code += r'''
int main() {
  FakeSensor sensor; esphome::App.sensors = {&sensor};
  lv_obj_t owner, second_owner, value{"old value"}, second_value, text{"text value"};
  BtnSlot slot{&owner, &value, &text}, second{&second_owner, &second_value, &text};
  sensor_driver_register_local_value(slot, {"room_temp", "1"});
  assert(value.text == "--"); // Invalid startup must clear reused labels.
  sensor.publish(23.5f); assert(value.text == "23.5");
  sensor_driver_register_local_value(second, {"room_temp", "2"});
  assert(second_value.text == "23.50" && sensor.callbacks.size() == 1);
  local_sensor_registry().push_back({"room_temp", true, 0, nullptr, &text, nullptr});
  for (float invalid : {NAN, INFINITY, -INFINITY}) {
    sensor.publish(invalid);
    assert(value.text == "--" && second_value.text == "--" && text.text == "text value");
    assert(local_sensor_apply_value("room_temp", invalid));
  }
  assert(!local_sensor_apply_value("missing", NAN));
  sensor.publish(-4.25f); assert(value.text == "-4.2" && second_value.text == "-4.25");
  assert(local_sensor_apply_text("room_temp", "still text") && text.text == "still text");
  lv_obj_t third_owner, initial_infinity{"old"}; BtnSlot third{&third_owner, &initial_infinity, nullptr};
  sensor.state = INFINITY; sensor_driver_register_local_value(third, {"room_temp", ""});
  assert(initial_infinity.text == "--");
  assert(third_owner.callbacks.size() == 1);
  lv_event_t removed{&third_owner};
  for (auto callback : third_owner.callbacks) callback(&removed);
  sensor.publish(7);
  assert(value.text == "7.0" && second_value.text == "7.00" && initial_infinity.text == "--");
  puts("Local Sensor invalid startup, live readings, recovery and multiple consumers passed.");
}
'''
with tempfile.TemporaryDirectory(prefix="local-sensor-invalid-") as temporary:
    source = Path(temporary) / "test.cpp"
    binary = Path(temporary) / "test"
    source.write_text(code)
    compiler = shlex.split(os.environ.get("CXX", sys.argv[1] if len(sys.argv) > 1 else "c++"))
    subprocess.run(compiler + ["-std=c++17","-I",str(FIRMWARE), "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "tests/firmware/stubs"), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
