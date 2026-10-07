#!/usr/bin/env python3
"""Exercise 7-inch rotation before the delayed grid refresh can update pixel shape."""
import argparse
from pathlib import Path
import subprocess
import tempfile

import yaml

parser = argparse.ArgumentParser()
parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
parser.add_argument("--compiler", default="c++")
args = parser.parse_args()


class Loader(yaml.SafeLoader):
    pass


for tag in ("!lambda", "!include"):
    Loader.add_constructor(tag, lambda loader, node: loader.construct_scalar(node))


def actions(items):
    result = []
    for item in items:
        kind, value = next(iter(item.items()))
        if kind == "lambda":
            result.append(value)
        elif kind == "if":
            result.append("if ([] { " + value["condition"]["lambda"] + " }()) {\n" +
                          actions(value["then"]) + "\n} else {\n" +
                          actions(value.get("else", [])) + "\n}")
        elif kind == "lvgl.display.set_rotation":
            result.append(f"display_rotation = {value};")
        elif kind == "script.execute":
            # The grid refresh queues work three seconds later; it does not run here.
            result.append(f"{value}();")
        else:
            raise AssertionError(f"Unsupported rotation action: {kind}")
    return "\n".join(result)


layout = (args.root / "components/espcontrol/button_grid_layout.h").read_text()
helpers = layout[layout.index("inline int normalize_width_compensation_percent"):
                 layout.index("inline void apply_slot_text_width_compensation")]
screen = yaml.load((args.root / "common/device/screen_cover_art.yaml").read_text(), Loader)
responsive = next(s for s in screen["script"] if s["id"] == "cover_art_apply_responsive_layout")
# Execute the real compensation call, including its position before the geometry cache.
compensation = responsive["then"][0]["lambda"].split("int screen_w", 1)[0]

for slug in ("guition-esp32-p4-jc1060p470", "guition-esp32-p4-jc1060p470-v2"):
    device = yaml.load((args.root / "devices" / slug / "device/device.yaml").read_text(), Loader)
    rotation = next(s for s in device["script"] if s["id"] == "apply_screen_rotation")
    boot = next(b for b in device["esphome"]["on_boot"] if b["priority"] == -100)
    select = next(s for s in device["select"] if s["id"] == "screen_rotation_select")
    source = r'''
#include <cassert>
#include <string>
using lv_coord_t = int;
constexpr int LV_PART_MAIN = 0;
struct lv_obj_t { int x = 256, y = 256; } button, icon, text;
void lv_obj_set_style_transform_scale_x(lv_obj_t *obj, int scale, int) { obj->x = scale; }
void lv_obj_set_style_transform_scale_y(lv_obj_t *obj, int scale, int) { obj->y = scale; }
struct Select { std::string option; std::string current_option() { return option; } } screen_rotation_select;
bool screen_rotation_ready = false;
int display_rotation = -1;
auto *cover_art_playback_button = &button;
#define id(x) x
''' + helpers + "\nvoid cover_art_apply_responsive_layout() {\n" + compensation + r'''
}
void camera_screensaver_request_image() {}
void refresh_button_grid() {} // Queued refresh deliberately has not executed.
void apply_screen_rotation() {
''' + actions(rotation["then"]) + "\n}\nvoid boot() {\n" + actions(boot["then"]) + \
        "\n}\nvoid select_rotation() {\n" + actions(select["on_value"]["then"]) + r'''
}
void check(const std::string &option) {
  const bool portrait = option == "90" || option == "270";
  assert(button.x == (portrait ? 256 : 243));
  assert(button.y == (portrait ? 243 : 256));
  assert(display_rotation == (std::stoi(option) + 180) % 360);
  // Standalone icons use the same rotated axis, preserving their chosen zoom.
  apply_icon_width_compensation(&icon, 180);
  assert(icon.x == (portrait ? 180 : 171));
  assert(icon.y == (portrait ? 171 : 180));
  // Normal 7-inch text keeps its separate 100% setting in every orientation.
  apply_text_width_compensation(&text);
  assert(text.x == 256 && text.y == 256);
  // Text compensation, when configured, also switches axes and clears the old one.
  set_text_width_compensation_percent(95);
  apply_text_width_compensation(&text);
  assert(text.x == (portrait ? 256 : 243));
  assert(text.y == (portrait ? 243 : 256));
  set_text_width_compensation_percent(100);
  apply_text_width_compensation(&text);
  assert(text.x == 256 && text.y == 256);
}
int main() {
  set_icon_width_compensation_percent(95);
  for (const std::string option : {"0", "90", "180", "270"}) {
    screen_rotation_select.option = option;
    // Start with the opposite axis to expose reliance on a stale grid profile.
    set_width_compensation_vertical_axis(option == "0" || option == "180");
    boot(); // Restored rotation must work before the first grid refresh.
    check(option);
  }
  for (const std::string option : {"0", "90", "270", "180", "90", "0"}) {
    screen_rotation_select.option = option;
    select_rotation();
    check(option);
    cover_art_apply_responsive_layout(); // Re-entry keeps the same axis.
    check(option);
  }
}
'''
    with tempfile.TemporaryDirectory(prefix="cover-art-rotation-") as directory:
        cpp = Path(directory) / "test.cpp"
        exe = Path(directory) / "test"
        cpp.write_text(source)
        subprocess.run([args.compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print(f"{slug}: button, icon, and text compensation follow restored and changed rotation")
