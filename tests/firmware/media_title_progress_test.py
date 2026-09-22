#!/usr/bin/env python3
"""Exercise read-only Now Playing progress against production functions."""
from pathlib import Path
import subprocess
import sys
import tempfile

from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
media = (root / "components/espcontrol/button_grid_media.h").read_text(encoding="utf-8")
sliders = (root / "components/espcontrol/button_grid_sliders.h").read_text(encoding="utf-8")
slider_context = sliders.split("struct SliderCtx {", 1)[1].split("\n};", 1)[0]
source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
using lv_coord_t = int;
using lv_style_selector_t = int;
struct lv_timer_t {};
struct lv_event_t;
struct lv_obj_t {
  lv_obj_t *parent = nullptr;
  void *data = nullptr;
  int value = 0, width = 200, height = 80, radius = 20;
  bool clickable = true;
  std::vector<lv_obj_t *> children;
  std::vector<std::pair<int, void (*)(lv_event_t *)>> events;
};
struct lv_event_t { lv_obj_t *target; };
constexpr int LV_PART_MAIN = 0, LV_STATE_CHECKED = 1, LV_OBJ_FLAG_CLICKABLE = 1;
constexpr int LV_EVENT_VALUE_CHANGED = 2, LV_ANIM_OFF = 0;
constexpr int LV_ALIGN_LEFT_MID = 0, LV_ALIGN_RIGHT_MID = 1;
constexpr int LV_ALIGN_BOTTOM_MID = 2, LV_ALIGN_TOP_MID = 3;
uint32_t now_ms = 1000;
namespace esphome { uint32_t millis() { return now_ms; } }
uint32_t lv_color_hex(uint32_t color) { return color; }
void lv_obj_set_style_bg_color(lv_obj_t *, uint32_t, int) {}
void lv_obj_set_style_radius(lv_obj_t *o, int radius, int) { o->radius = radius; }
int lv_obj_get_style_radius(lv_obj_t *o, int) { return o->radius; }
int lv_obj_get_width(lv_obj_t *o) { return o->width; }
int lv_obj_get_height(lv_obj_t *o) { return o->height; }
void lv_obj_set_size(lv_obj_t *o, int w, int h) { o->width = w; o->height = h; }
void lv_obj_align(lv_obj_t *, int, int, int) {}
lv_obj_t *lv_obj_get_parent(lv_obj_t *o) { return o->parent; }
lv_obj_t *lv_obj_get_child(lv_obj_t *o, int i) { return o->children.at(i); }
void *lv_obj_get_user_data(lv_obj_t *o) { return o->data; }
void lv_obj_set_user_data(lv_obj_t *o, void *p) { o->data = p; }
void lv_obj_clear_flag(lv_obj_t *o, int) { o->clickable = false; }
void lv_obj_add_event_cb(lv_obj_t *o, void (*cb)(lv_event_t *), int code, void *) {
  o->events.push_back({code, cb});
}
void emit(lv_obj_t *o, int code) {
  lv_event_t e{o};
  for (auto &event : o->events) if (event.first == code) event.second(&e);
}
lv_obj_t *lv_event_get_target(lv_event_t *e) { return e->target; }
int lv_slider_get_value(lv_obj_t *o) { return o->value; }
void lv_slider_set_value(lv_obj_t *o, int value, int) { o->value = value; }
struct CardPadding { int left = 4, right = 4, top = 4, bottom = 4; };
CardPadding capture_card_padding(lv_obj_t *) { return {}; }
lv_obj_t *setup_slider_widget(lv_obj_t *parent, uint32_t, bool) {
  auto *fill = new lv_obj_t;
  auto *slider = new lv_obj_t;
  fill->parent = slider->parent = parent;
  parent->children = {fill, slider};
  return slider;
}
void slider_bind_geometry_refresh(lv_obj_t *, lv_obj_t *) {}
void slider_update_horizontal_track_bg(lv_obj_t *, lv_obj_t *) { assert(false); }
void slider_update_horizontal_track_fill(lv_obj_t *, lv_obj_t *, int) { assert(false); }
void lv_label_set_display_text(lv_obj_t *, const char *) {}
void media_format_time(float, char *, size_t) {}
std::string media_status_text(const std::string &s) { return s; }
'''
source += "struct SliderCtx {" + slider_context + "\n};\n"
source += r'''
struct MediaPlaybackState {
  bool available = true, playing = true, has_position = true;
  bool position_updated_at_known = false;
  float duration = 100, position_seconds = 0;
  uint32_t position_updated_ms = 1000, position_updated_at_ms = 0;
  std::string state_text = "playing";
};
bool media_seek_pending_active(SliderCtx *) { return false; }
constexpr float MEDIA_SEEK_MATCH_TOLERANCE_SECONDS = 1;
void media_schedule_position_refresh(SliderCtx *) {}
'''
for name in ("slider_update_fill", "slider_update_ctx_fill"):
    source += definition(sliders, name)[1] + "\n"
for name in ("media_apply_position", "media_playback_apply_state_to_slider",
             "setup_media_progress_background"):
    source += definition(media, name)[1] + "\n"
source += r'''
int main() {
  lv_obj_t title;
  auto *slider = setup_media_progress_background(
    &title, 0xFF8C00, 0x212121, "media_player.test");
  auto *ctx = static_cast<SliderCtx *>(slider->data);
  assert(!slider->clickable && !ctx->interactive);

  MediaPlaybackState playback;
  media_playback_apply_state_to_slider(&playback, ctx);
  assert(ctx->fill->width == 0);

  now_ms = 6000;
  media_playback_apply_state_to_slider(&playback, ctx);
  assert(slider->value == 5 && ctx->fill->width == 10);

  now_ms = 11000;
  media_playback_apply_state_to_slider(&playback, ctx);
  assert(slider->value == 10 && ctx->fill->width == 20);

  slider->value = 40;
  emit(slider, LV_EVENT_VALUE_CHANGED);
  assert(ctx->fill->width == 80);

  playback.playing = false;
  playback.position_seconds = 10;
  media_playback_apply_state_to_slider(&playback, ctx);
  now_ms = 51000;
  media_playback_apply_state_to_slider(&playback, ctx);
  assert(slider->value == 10);

  playback.duration = 0;
  media_playback_apply_state_to_slider(&playback, ctx);
  assert(slider->value == 0 && ctx->fill->width == 0);

  delete ctx;
  for (auto *child : title.children) delete child;
}
'''
with tempfile.TemporaryDirectory(prefix="media-title-progress-") as tmp:
    path = Path(tmp) / "test.cpp"
    binary = Path(tmp) / "test"
    path.write_text(source, encoding="utf-8")
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "g++", "-std=c++17",
                    "-Wall", "-Wextra", "-Werror", str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Media title progress tests passed.")
