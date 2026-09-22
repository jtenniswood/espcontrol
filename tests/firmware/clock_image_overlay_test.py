#!/usr/bin/env python3
"""Run the production overlay lambdas through camera/media mode changes."""
from pathlib import Path
import subprocess
import tempfile
import textwrap

root = Path(__file__).resolve().parents[2]
yaml = (root / "common/addon/backlight.yaml").read_text()


def script_lambda(name):
    script = yaml.split(f"  - id: {name}\n", 1)[1].split("\n  - id:", 1)[0]
    return textwrap.dedent(script.split("      - lambda: |-\n", 1)[1].split("\n  #", 1)[0])


source = r'''
#include "display_mode_controller.h"
#include <cassert>
#include <string>
using espcontrol::DisplayMode;
struct Widget { bool hidden = true; int raised = 0; };
using lv_obj_t = Widget;
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
Widget overlay, shadow, label, metadata, metadata_shadow;
auto *clock_image_overlay = &overlay;
auto *clock_image_overlay_shadow = &shadow;
auto *clock_image_overlay_label = &label;
auto *photo_metadata_label = &metadata;
auto *photo_metadata_shadow = &metadata_shadow;
void lv_obj_add_flag(Widget *w, int) { w->hidden = true; }
void lv_obj_clear_flag(Widget *w, int) { w->hidden = false; }
bool lv_obj_has_flag(Widget *w, int) { return w->hidden; }
void lv_obj_move_foreground(Widget *w) { ++w->raised; }
void lv_label_set_text(Widget *, const char *) {}
struct Time { int hour = 12, minute = 30; bool is_valid() { return true; } };
struct Clock { Time now() { return {}; } } panel_time, homeassistant_time;
Time panel_time_or_fallback(Time time, Time) { return time; }
struct Toggle { bool state = true; } clock_overlay_enabled, metadata_overlay_enabled;
struct Setting { std::string state; } screensaver_metadata_entity, schedule_clock_text_color;
std::string photo_metadata_value, photo_metadata_subscribed_entity;
unsigned photo_metadata_subscription_generation = 1;
unsigned ha_subscription_generation() { return 1; }
bool ha_api_state_connected() { return true; }
bool clock_format_12h = false;
void format_clock_time_without_suffix(char *b, size_t, int, int, bool) { b[0] = 0; }
void apply_clock_screensaver_text_color(Widget *, const std::string &) {}
void position_clock_image_overlay(Widget *, Widget *, Widget *, Widget *, Widget *, bool, bool) {}
struct App {
  DisplayMode mode = DisplayMode::CAMERA;
  App &display() { return *this; }
  DisplayMode current_mode() { return mode; }
} espcontrol_app;
#define id(x) x
void apply(int target_mode) {
''' + script_lambda("clock_overlay_apply") + r'''
}
void keep_on_top() {
''' + script_lambda("clock_overlay_keep_on_top") + r'''
}
int main() {
  // Camera and image entities both use CAMERA; media playback uses COVER_ART.
  apply(static_cast<int>(DisplayMode::CAMERA));
  assert(!overlay.hidden && !label.hidden);
  keep_on_top();
  assert(!overlay.hidden);
  for (auto mode : {DisplayMode::COVER_ART, DisplayMode::ACTIVE,
                    DisplayMode::CLOCK, DisplayMode::DIMMED,
                    DisplayMode::SETUP_DIMMED, DisplayMode::DISPLAY_OFF}) {
    // Switching modes, toggling the setting, or refreshing time must hide it.
    apply(static_cast<int>(DisplayMode::CAMERA));
    apply(static_cast<int>(mode));
    assert(overlay.hidden);
    // Re-raising a previously visible overlay must also respect the mode.
    overlay.hidden = false;
    espcontrol_app.mode = mode;
    keep_on_top();
    assert(overlay.hidden);
  }
  espcontrol_app.mode = DisplayMode::CAMERA;
  apply(static_cast<int>(DisplayMode::CAMERA));
  assert(!overlay.hidden); // Camera resumes after media playback.
  clock_overlay_enabled.state = false;
  apply(static_cast<int>(DisplayMode::CAMERA));
  assert(overlay.hidden);
  photo_metadata_value = "Photo caption";
  photo_metadata_subscribed_entity = screensaver_metadata_entity.state = "sensor.photo";
  apply(static_cast<int>(DisplayMode::CAMERA));
  assert(!overlay.hidden && label.hidden && !metadata.hidden);
  apply(static_cast<int>(DisplayMode::COVER_ART));
  assert(overlay.hidden); // Metadata must not leak onto media artwork either.
}
'''
with tempfile.TemporaryDirectory(prefix="clock-image-overlay-") as directory:
    cpp = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    cpp.write_text(source)
    subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "components/espcontrol"), str(cpp),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Camera-only clock overlay transitions passed.")
