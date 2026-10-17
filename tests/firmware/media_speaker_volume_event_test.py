#!/usr/bin/env python3
"""Exercise the production speaker buttons with direct and bubbled LVGL taps."""
from pathlib import Path
import os
import subprocess
import tempfile

from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
media = (root / "components/espcontrol/button_grid_media.h").read_text()
group = (root / "components/espcontrol/button_grid_media_group.h").read_text()
source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using lv_coord_t = int;
constexpr int LV_OBJ_FLAG_CLICKABLE = 1, LV_OBJ_FLAG_EVENT_BUBBLE = 2;
constexpr int LV_PART_MAIN = 0, LV_OPA_COVER = 255, LV_EVENT_CLICKED = 1;
struct lv_event_t;
struct lv_obj_t {
  void *user_data = nullptr;
  lv_obj_t *parent = nullptr;
  int flags = LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE;
  void (*callback)(lv_event_t *) = nullptr;
  void *callback_data = nullptr;
};
struct lv_event_t {
  lv_obj_t *target;
  lv_obj_t *current_target;
  void *user_data;
};
std::vector<std::unique_ptr<lv_obj_t>> widgets;
lv_obj_t *lv_btn_create(lv_obj_t *parent) {
  widgets.push_back(std::make_unique<lv_obj_t>());
  auto *obj = widgets.back().get(); obj->parent = parent; return obj;
}
lv_obj_t *lv_label_create(lv_obj_t *parent) { return lv_btn_create(parent); }
void lv_obj_clear_flag(lv_obj_t *obj, int flag) { obj->flags &= ~flag; }
void lv_obj_set_user_data(lv_obj_t *obj, void *data) { obj->user_data = data; }
void *lv_obj_get_user_data(lv_obj_t *obj) { return obj->user_data; }
[[maybe_unused]] void *lv_event_get_target(lv_event_t *event) { return event->target; }
void *lv_event_get_current_target(lv_event_t *event) { return event->current_target; }
void *lv_event_get_user_data(lv_event_t *event) { return event->user_data; }
void lv_obj_add_event_cb(lv_obj_t *obj, void (*cb)(lv_event_t *), int, void *data) {
  obj->callback = cb; obj->callback_data = data;
}
void click(lv_obj_t *button, lv_obj_t *target = nullptr) {
  lv_event_t event{target ? target : button, button, button->callback_data};
  button->callback(&event);
}
// Drawing does not affect event ownership or volume dispatch.
template<class... T> void lv_obj_set_size(T...) {}
template<class... T> void lv_obj_set_style_radius(T...) {}
template<class... T> void theme_set_primary_foreground_fill(T...) {}
template<class... T> void lv_obj_set_style_bg_color(T...) {}
template<class... T> void lv_obj_set_style_bg_opa(T...) {}
template<class... T> void lv_obj_set_style_border_width(T...) {}
template<class... T> void lv_obj_set_style_shadow_width(T...) {}
template<class... T> void lv_obj_set_style_pad_all(T...) {}
template<class... T> void control_modal_apply_pressed_fill(T...) {}
template<class... T> void lv_label_set_display_text(T...) {}
template<class... T> void lv_obj_set_style_text_font(T...) {}
template<class... T> void theme_set_content_foreground(T...) {}
template<class... T> void lv_obj_set_style_text_color(T...) {}
template<class... T> void lv_obj_center(T...) {}
int lv_color_hex(int color) { return color; }
struct Theme { int text_primary = 0; };
Theme current_theme() { return {}; }

struct MediaGroupVolumeState {
  std::string entity_id;
  int volume_pct = 0;
  bool volume_known = false, available = false;
};
struct MediaSpeakerRowState {
  std::string entity_id;
  int volume_pct = 0;
  bool volume_known = true, available = true;
};
struct MediaControlCtx {
  int accent_color = 0, max_pct = 50;
  void *icon_font = nullptr;
  std::vector<MediaGroupVolumeState> group_volume_states;
};
struct MediaControlModalUi {
  MediaControlCtx *active = nullptr;
  std::vector<MediaSpeakerRowState *> speaker_rows;
};
MediaControlModalUi ui;
MediaControlModalUi &media_control_modal_ui() { return ui; }
int media_control_volume_max_pct(MediaControlCtx *ctx) { return ctx->max_pct; }
struct Command { std::string entity; int volume; };
std::vector<Command> commands;
int row_refreshes = 0, group_refreshes = 0;
void send_media_volume_action(const std::string &entity, int volume) {
  commands.push_back({entity, volume});
}
void media_control_refresh_speaker_row(MediaControlCtx *, MediaSpeakerRowState *) {
  ++row_refreshes;
}
void media_control_refresh_group_volume(MediaControlCtx *) { ++group_refreshes; }
'''
for text, name in (
    (group, "media_group_step_volume"),
    (media, "media_control_create_speaker_volume_button"),
):
    line, body = definition(text, name)
    source += f'\n#line {line} "{name}"\n{body}\n'
source += r'''
int main() {
  MediaControlCtx ctx;
  MediaSpeakerRowState office{"media_player.office", 25};
  MediaSpeakerRowState kitchen{"media_player.kitchen", 35};
  MediaSpeakerRowState unrelated{"media_player.unrelated", 90};
  ui.active = &ctx; ui.speaker_rows = {&office, &kitchen};
  lv_obj_t parent;
  auto *plus = media_control_create_speaker_volume_button(&ctx, &office, &parent, 46, "+", true);
  auto *icon = widgets.back().get();
  auto *minus = media_control_create_speaker_volume_button(&ctx, &kitchen, &parent, 46, "-", false);
  click(plus);
  assert(office.volume_pct == 26 && commands.back().entity == office.entity_id);
  click(minus);
  assert(kitchen.volume_pct == 34 && commands.back().entity == kitchen.entity_id);
  // A child target must never determine which speaker receives the command.
  icon->user_data = &unrelated;
  click(plus, icon);
  assert(office.volume_pct == 27 && unrelated.volume_pct == 90);
  assert(commands.back().entity == office.entity_id && commands.back().volume == 27);
  icon->user_data = reinterpret_cast<void *>(uintptr_t{1});
  click(plus, icon); // Non-speaker child data must not be dereferenced.
  assert(office.volume_pct == 28);
  assert(!(icon->flags & LV_OBJ_FLAG_CLICKABLE));
  assert(!(plus->flags & LV_OBJ_FLAG_EVENT_BUBBLE));
  assert(!(minus->flags & LV_OBJ_FLAG_EVENT_BUBBLE));
  assert(ctx.group_volume_states.size() == 2);
  assert(ctx.group_volume_states[0].volume_pct == 28);
  assert(ctx.group_volume_states[1].volume_pct == 34);
  assert(row_refreshes == 4 && group_refreshes == 4);
  // Preserve the limit behavior and route repeated taps to the same speaker.
  office.volume_pct = 49;
  click(plus); click(plus);
  assert(office.volume_pct == 50 && commands.back().volume == 50);
  kitchen.volume_pct = 80; click(minus);
  assert(kitchen.volume_pct == 79 && commands.back().volume == 79);
  kitchen.volume_pct = 0; click(minus);
  assert(kitchen.volume_pct == 0 && commands.back().volume == 0);
  const auto sent = commands.size();
  office.available = false; click(plus);
  office.available = true; office.volume_known = false; click(plus);
  office.volume_known = true; ui.active = nullptr; click(plus);
  ui.active = &ctx; ui.speaker_rows = {&kitchen}; click(plus);
  // Reject metadata from a removed row before reading through its pointer.
  plus->user_data = reinterpret_cast<void *>(uintptr_t{1}); click(plus);
  assert(commands.size() == sent);
}
'''
with tempfile.TemporaryDirectory(prefix="media-speaker-volume-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(source)
    subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
        str(path / "test.cpp"), "-o", str(path / "test"),
    ], check=True)
    subprocess.run([str(path / "test")], check=True)
print("Production speaker volume event checks passed")
