#!/usr/bin/env python3
"""Exercise compact dial sizing with ESPHome's software-rotated viewport."""
from pathlib import Path
import subprocess
import sys
import tempfile

from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
display = (root / "components/espcontrol/button_grid_display.h").read_text()
modal = (root / "components/espcontrol/button_grid_modal.h").read_text()
source = r'''
#include <cassert>
#include <initializer_list>
#include "button_grid_modal_layout.h"
using DisplayModalProfile = espcontrol::modal::DeviceProfile;
using DisplayModalLayoutFamily = espcontrol::modal::LayoutFamily;
using lv_coord_t = int;
struct lv_font_t { int line_height; };
struct lv_disp_t { int width = 480, height = 800, rotation = 0; };
lv_disp_t viewport;
lv_disp_t *active_display = &viewport;
lv_disp_t *lv_disp_get_default() { return active_display; }
int lv_disp_get_hor_res(lv_disp_t *d) { return d->width; }
int lv_disp_get_ver_res(lv_disp_t *d) { return d->height; }
struct lv_obj_t { int width = 0, height = 0, zoom = 256, alignment = 0, x = 0, y = 0; lv_obj_t *child = nullptr; };
constexpr int LV_PART_MAIN = 0, LV_ALIGN_CENTER = 0, LV_ALIGN_BOTTOM_MID = 1;
struct ControlModalLayout {
  DisplayModalProfile profile;
  int btn_size = 80, short_side = 480, controls_gap = 24, controls_center_y = 100;
};
int control_modal_scaled_px(int px, int side) { return espcontrol::modal::scaled_px(px, side); }
void lv_obj_set_size(lv_obj_t *o, int w, int h) { o->width = w; o->height = h; }
void lv_obj_set_style_radius(lv_obj_t *, int, int) {}
void lv_obj_align(lv_obj_t *o, int alignment, int x, int y) { o->alignment = alignment; o->x = x; o->y = y; }
lv_obj_t *lv_obj_get_child(lv_obj_t *o, int) { return o->child; }
void lv_obj_set_style_transform_zoom(lv_obj_t *o, int z, int) { o->zoom = z; }
void control_modal_center_tab_icon(lv_obj_t *) {}
'''
for name in (
    "display_active_modal_profile", "display_active_compact_volume_number_font",
    "display_modal_uses_family", "display_is_landscape",
    "display_modal_uses_compact_landscape_controls", "display_modal_volume_number_font",
):
    line, body = definition(display, name)
    source += f'\n#line {line} "button_grid_display.h"\n{body}\n'
line, body = definition(modal, "control_modal_apply_step_buttons_layout")
source += f'\n#line {line} "button_grid_modal.h"\n{body}\n'
source += r'''
int main() {
  // Reuse the 140 px clock font (164 px line height) in landscape.
  lv_font_t compact{164}, large{183};
  display_active_compact_volume_number_font() = &compact;
  auto &profile = display_active_modal_profile();
  profile.layout_family = DisplayModalLayoutFamily::COMPACT_PORTRAIT;
  ControlModalLayout layout;
  layout.profile = profile;
  lv_obj_t minus_icon, plus_icon, minus, plus;
  minus.child = &minus_icon; plus.child = &plus_icon;

  // ESPHome sets a landscape resolution while LVGL rotation stays zero.
  viewport.width = 800; viewport.height = 480;
  assert(viewport.rotation == 0);
  assert(display_is_landscape());
  assert(display_modal_volume_number_font(&large) == &compact);
  control_modal_apply_step_buttons_layout(&minus, &plus, layout, true, 4, 256);
  assert(minus.alignment == LV_ALIGN_BOTTOM_MID && plus.alignment == LV_ALIGN_BOTTOM_MID);
  assert(minus.y == -4 && plus.y == -4);
  assert(minus.width == 56 && minus.height == 56 && plus.width == 56);
  assert(minus_icon.zoom == 200 && plus_icon.zoom == 200);

  // Returning to portrait restores the configured number and icon sizes.
  viewport.width = 480; viewport.height = 800;
  assert(!display_is_landscape());
  assert(display_modal_volume_number_font(&large) == &large);
  control_modal_apply_step_buttons_layout(&minus, &plus, layout, false, 0, 256);
  assert(minus.width == 80 && plus.width == 80);
  assert(minus.alignment == LV_ALIGN_CENTER && minus.y == layout.controls_center_y);
  assert(minus_icon.zoom == 256 && plus_icon.zoom == 256);

  // Climate and other callers keep their own icon sizes on non-target layouts.
  minus_icon.zoom = plus_icon.zoom = 214;
  control_modal_apply_step_buttons_layout(&minus, &plus, layout);
  assert(minus_icon.zoom == 214 && plus_icon.zoom == 214);

  viewport.width = 800; viewport.height = 480;
  control_modal_apply_step_buttons_layout(&minus, &plus, layout);
  assert(minus_icon.zoom == 214 && plus_icon.zoom == 214);

  viewport.width = 800; viewport.height = 480;
  profile.layout_family = DisplayModalLayoutFamily::WIDE_LANDSCAPE;
  layout.profile = profile;
  assert(display_modal_volume_number_font(&large) == &large);
  control_modal_apply_step_buttons_layout(&minus, &plus, layout);
  assert(minus.width == 80);
  profile.layout_family = DisplayModalLayoutFamily::COMPACT_PORTRAIT;
  display_active_compact_volume_number_font() = nullptr;
  assert(display_modal_volume_number_font(&large) == &large);
  active_display = nullptr;
  assert(!display_is_landscape());
}
'''
with tempfile.TemporaryDirectory(prefix="compact-landscape-controls-") as tmp:
    path = Path(tmp)
    (path / "test.cpp").write_text(source)
    subprocess.run([
        sys.argv[1] if len(sys.argv) > 1 else "c++",
        "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-I", str(root / "components/espcontrol"), str(path / "test.cpp"),
        "-o", str(path / "test"),
    ], check=True)
    subprocess.run([str(path / "test")], check=True)
print("Compact landscape dial controls checks passed.")
