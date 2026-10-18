#!/usr/bin/env python3
"""Exercise speaker taps, volume synchronization, caps, and grouping failures."""
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
#include <cmath>
#include <cstring>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

using lv_coord_t = int;
struct lv_timer_t {};
constexpr int LV_OBJ_FLAG_CLICKABLE = 1, LV_OBJ_FLAG_EVENT_BUBBLE = 2;
constexpr int LV_PART_MAIN = 0, LV_OPA_COVER = 255, LV_EVENT_CLICKED = 1;
constexpr int LV_OBJ_FLAG_HIDDEN = 4;
#define ESP_LOGW(...) ((void)0)
namespace esphome {
uint32_t now = 100;
uint32_t millis() { return now; }
namespace api {
struct ActionResponse {
  bool success;
  bool is_success() const { return success; }
  std::string get_error_message() const { return "rejected"; }
};
}
}
struct lv_event_t;
struct lv_obj_t {
  std::string text;
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
void lv_obj_add_flag(lv_obj_t *obj, int flag) { obj->flags |= flag; }
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
void lv_label_set_display_text(lv_obj_t *obj, const char *text) { obj->text = text; }
template<class... T> void lv_obj_set_style_text_font(T...) {}
template<class... T> void theme_set_content_foreground(T...) {}
template<class... T> void lv_obj_set_style_text_color(T...) {}
template<class... T> void lv_obj_center(T...) {}
int lv_color_hex(int color) { return color; }
struct Theme { int text_primary = 0, text_muted = 0; };
const char *espcontrol_i18n(const char *text) { return text; }
Theme current_theme() { return {}; }

struct MediaGroupVolumeState {
  std::string entity_id;
  int volume_pct = 0;
  bool volume_known = false, available = true;
  uint32_t pending_until_ms = 0;
};
struct MediaSpeakerRowState {
  std::string entity_id, friendly_name;
  int volume_pct = 0;
  bool volume_known = true, available = true;
  bool selected = false, pending = false, previous_selected = false;
  uint32_t call_id = 0, pending_until_ms = 0;
};
struct MediaGroupDiscoveryItem {
  std::string entity_id, friendly_name;
  int volume_pct = 0;
  bool volume_known = false, available = false;
};
struct MediaControlCtx {
  std::string entity_id = "media_player.office";
  std::vector<std::string> group_members, speaker_helper_members;
  std::vector<MediaGroupDiscoveryItem> speaker_discovery;
  int current_pct = 0, pending_pct = -1;
  uint32_t pending_until_ms = 0;
  bool volume_known = true, available = true;
  int reported_volume_pct = 20;
  bool reported_volume_known = true;
  int accent_color = 0, max_pct = 50;
  void *icon_font = nullptr;
  std::vector<MediaGroupVolumeState> group_volume_states;
};
struct MediaControlModalUi {
  MediaControlCtx *active = nullptr;
  std::vector<MediaSpeakerRowState *> speaker_rows;
  lv_obj_t *speakers_box = nullptr, *speakers_status_lbl = nullptr, *volume_pct_lbl = nullptr;
  bool speaker_status_error = false;
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
void media_control_refresh_volume(MediaControlCtx *) {}
bool ha_cancel_action_response_callback(uint32_t, const char *) { return true; }
void media_control_sync_speaker_candidates(MediaControlCtx *, const std::vector<std::string> &) {}
int media_control_clamp_volume(MediaControlCtx *ctx, int pct) {
  return std::max(0, std::min(ctx->max_pct, pct));
}
'''
for text, name in (
    (group, "media_group_trim"),
    (group, "media_group_valid_entity_id"),
    (group, "media_group_append_unique"),
    (group, "media_group_merge_candidates"),
    (group, "media_group_step_volume"),
    (group, "media_group_mean_volume"),
    (group, "media_group_delta_volumes"),
    (media, "media_control_store_group_volume"),
    (media, "media_control_set_group_volume_target"),
    (media, "media_control_copy_group_volume_to_row"),
    (media, "media_control_find_speaker_row"),
    (media, "media_control_group_size"),
    (media, "media_control_group_contains"),
    (media, "media_control_set_speaker_status"),
    (media, "media_control_current_group_volumes"),
    (media, "media_control_refresh_group_member_volumes"),
    (media, "media_control_refresh_speakers"),
    (media, "media_control_apply_group_volume_percent"),
    (media, "media_control_speaker_action_timer_cb"),
    (media, "media_control_group_action_result"),
    (media, "media_control_create_speaker_volume_button"),
):
    line, body = definition(text, name)
    source += f'\n#line {line} "{name}"\n{body}\n'
source += r'''
int main() {
  MediaControlCtx ctx;
  MediaSpeakerRowState office; office.entity_id = "media_player.office"; office.volume_pct = 25;
  MediaSpeakerRowState kitchen; kitchen.entity_id = "media_player.kitchen"; kitchen.volume_pct = 35;
  MediaSpeakerRowState unrelated; unrelated.entity_id = "media_player.unrelated"; unrelated.volume_pct = 90;
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

  // Cached Home Assistant discovery must not undo repeated volume taps.
  ctx.max_pct = 100;
  office.volume_known = kitchen.volume_known = true;
  office.available = kitchen.available = true;
  ctx.group_members = {office.entity_id, kitchen.entity_id};
  ctx.speaker_discovery = {{office.entity_id, "Office", 20, true, true},
                           {kitchen.entity_id, "Kitchen", 40, true, true}};
  ctx.group_volume_states.clear();
  media_control_store_group_volume(&ctx, office.entity_id, 20, true, true);
  media_control_refresh_group_member_volumes(&ctx);
  ui.speaker_rows = {&office, &kitchen};
  lv_obj_t speaker_box, status;
  ui.speakers_box = &speaker_box; ui.speakers_status_lbl = &status;
  media_control_refresh_speakers(&ctx);
  int mean = 0;
  assert(media_group_mean_volume(media_control_current_group_volumes(&ctx), &mean) && mean == 30);
  commands.clear();
  media_control_apply_group_volume_percent(&ctx, 31, true);
  assert(office.volume_pct == 21 && kitchen.volume_pct == 41);
  media_control_store_group_volume(&ctx, office.entity_id, 20, true, true);
  media_control_refresh_speakers(&ctx); // old discovery and unrelated playback updates
  assert(media_group_mean_volume(media_control_current_group_volumes(&ctx), &mean) && mean == 31);
  media_control_apply_group_volume_percent(&ctx, 32, true);
  assert(commands.size() == 4 && commands[0].volume == 21 && commands[1].volume == 41);
  assert(commands[2].volume == 22 && commands[3].volume == 42);
  // Acknowledged levels resume following external changes immediately.
  ctx.reported_volume_pct = 22;
  media_control_store_group_volume(&ctx, office.entity_id, 22, true, true);
  ctx.speaker_discovery[1].volume_pct = 42;
  media_control_refresh_speakers(&ctx);
  assert(ctx.group_volume_states[0].pending_until_ms == 0);
  assert(ctx.group_volume_states[1].pending_until_ms == 0);
  ctx.reported_volume_pct = 24;
  media_control_store_group_volume(&ctx, office.entity_id, 24, true, true);
  ctx.speaker_discovery[1].volume_pct = 44;
  media_control_refresh_speakers(&ctx);
  assert(office.volume_pct == 24 && kitchen.volume_pct == 44);
  // Missing acknowledgements stop masking actual volumes after 1.5 seconds.
  media_control_apply_group_volume_percent(&ctx, 35, true);
  esphome::now += 1501;
  ctx.reported_volume_pct = 24;
  media_control_store_group_volume(&ctx, office.entity_id, 24, true, true);
  media_control_refresh_speakers(&ctx);
  assert(office.volume_pct == 24 && kitchen.volume_pct == 44);
  // Periodic UI refresh must expire a command even if no direct volume
  // subscription ever acknowledges it or emits another event.
  media_control_apply_group_volume_percent(&ctx, 35, true);
  esphome::now += 1501;
  media_control_speaker_action_timer_cb(nullptr);
  assert(office.volume_pct == 24 && kitchen.volume_pct == 44);
  assert(ctx.group_volume_states[0].pending_until_ms == 0);
  // If the primary only has a discovery reading, optimistic volume_known
  // cannot prevent that reading from restoring the actual level on expiry.
  ctx.reported_volume_known = false;
  ctx.speaker_discovery[0].volume_pct = 24;
  media_control_apply_group_volume_percent(&ctx, 35, true);
  esphome::now += 1501;
  media_control_speaker_action_timer_cb(nullptr);
  assert(office.volume_pct == 24 && kitchen.volume_pct == 44);
  ctx.reported_volume_known = true;
  // The Volume tab uses the same cache even when speaker rows do not exist.
  ui.speaker_rows.clear(); ui.speakers_box = nullptr;
  media_control_apply_group_volume_percent(&ctx, 35, true);
  ctx.reported_volume_pct = 24;
  media_control_store_group_volume(&ctx, office.entity_id, 24, true, true);
  media_control_refresh_group_member_volumes(&ctx);
  assert(media_group_mean_volume(media_control_current_group_volumes(&ctx), &mean) && mean == 35);
  ui.speaker_rows = {&office, &kitchen}; ui.speakers_box = &speaker_box;
  media_control_refresh_speakers(&ctx);
  // Discovery's old primary level must never override the direct subscription.
  assert(office.volume_pct == 25 && kitchen.volume_pct == 45);
  // Individual taps get the same protection, including the primary display value.
  plus->user_data = &office;
  click(plus);
  media_control_refresh_speakers(&ctx);
  assert(office.volume_pct == 26 && ctx.current_pct == 26 && ctx.pending_pct == 26);
  assert(ctx.pending_until_ms > esphome::now);
  // Unavailable or unknown members must disable volume rather than retain targets.
  media_control_store_group_volume(&ctx, office.entity_id, 26, true, false);
  assert(!ctx.group_volume_states[0].available);
  ctx.available = false;
  ctx.speaker_discovery[1].volume_known = false;
  media_control_refresh_speakers(&ctx);
  assert(!kitchen.volume_known);
  assert(!media_group_mean_volume(media_control_current_group_volumes(&ctx), &mean));

  // Opening Speakers preserves a directly known primary volume even when the
  // discovery list omits that speaker (or its volume is not known there).
  ctx.speaker_discovery = {{kitchen.entity_id, "Kitchen", 40, true, true}};
  ctx.available = true; ctx.reported_volume_pct = 20;
  media_control_store_group_volume(&ctx, office.entity_id, 20, true, true);
  office.volume_known = false;
  media_control_refresh_speakers(&ctx);
  assert(office.volume_known && office.volume_pct == 20);
  assert(media_group_mean_volume(media_control_current_group_volumes(&ctx), &mean) && mean == 30);
  ctx.speaker_discovery.insert(ctx.speaker_discovery.begin(), {office.entity_id, "Office", 0, false, true});
  media_control_refresh_speakers(&ctx);
  assert(office.volume_known && office.volume_pct == 20);

  // Discovery may provide a fallback primary volume, but its availability
  // can lag behind the direct player state. Never revive an offline primary
  // or send it a volume command because the helper still calls it available.
  ctx.reported_volume_known = false;
  ctx.available = false;
  ctx.speaker_discovery[0].volume_pct = 20;
  ctx.speaker_discovery[0].volume_known = true;
  ctx.speaker_discovery[0].available = true;
  ctx.group_members = {office.entity_id, kitchen.entity_id};
  media_control_store_group_volume(&ctx, office.entity_id, 0, false, false);
  media_control_refresh_speakers(&ctx);
  assert(office.volume_known && office.volume_pct == 20 && !office.available);
  const auto offline_members = media_control_current_group_volumes(&ctx);
  assert(!offline_members[0].available && offline_members[1].available);
  commands.clear();
  click(plus);
  assert(commands.empty());
  media_control_apply_group_volume_percent(&ctx, 41, true);
  assert(commands.size() == 1 && commands[0].entity == kitchen.entity_id);
  // Recovery follows the direct state even if the helper still says offline.
  ctx.available = true;
  ctx.speaker_discovery[0].available = false;
  media_control_refresh_speakers(&ctx);
  assert(office.available && office.volume_known && office.volume_pct == 20);
  // Other speakers still use the discovery helper's own availability.
  ctx.speaker_discovery[1].available = false;
  media_control_refresh_speakers(&ctx);
  assert(!kitchen.available);
  ctx.speaker_discovery[1].available = true;
  ctx.reported_volume_known = true;
  media_control_refresh_speakers(&ctx);

  // A rejected join remains visible across normal list refreshes and clears
  // explicitly when a new action starts (the toggle handler does this).
  kitchen.call_id = 123; kitchen.pending = true;
  media_control_group_action_result(&ctx, kitchen.entity_id, 123, {false});
  assert(status.text == "Grouping failed" && !(status.flags & LV_OBJ_FLAG_HIDDEN));
  media_control_refresh_speakers(&ctx);
  assert(!(status.flags & LV_OBJ_FLAG_HIDDEN));
  media_control_set_speaker_status(nullptr);
  assert(status.flags & LV_OBJ_FLAG_HIDDEN);
  kitchen.call_id = 124; kitchen.pending = true;
  media_control_group_action_result(&ctx, kitchen.entity_id, 124, {true});
  assert(!ui.speaker_status_error && (status.flags & LV_OBJ_FLAG_HIDDEN));

  // The group minus button's requested mean must remain a one-point decrease,
  // including a group already above Maximum Volume.
  ui.speakers_box = nullptr; ui.speaker_rows.clear();
  ctx.speaker_discovery.clear(); ctx.max_pct = 50;
  ctx.group_members = {office.entity_id, kitchen.entity_id};
  ctx.group_volume_states = {{office.entity_id, 80, true, true},
                             {kitchen.entity_id, 80, true, true}};
  commands.clear();
  media_control_apply_group_volume_percent(&ctx, 79, true);
  assert(commands.size() == 2 && commands[0].volume == 79 && commands[1].volume == 79);
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
print("Production speaker events, volume synchronization, caps, and grouping failures passed")
