#!/usr/bin/env python3
"""Exercise production mute subscription, propagation, action, and UI policy."""
import os
import re
import subprocess
import tempfile
from pathlib import Path

from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
media = (root / 'components/espcontrol/button_grid_media.h').read_text()
actions = (root / 'components/espcontrol/button_grid_actions.h').read_text()
modal = (root / 'components/espcontrol/button_grid_modal.h').read_text()
source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "media_volume_capability.h"
#include "media_control_tabs.h"

namespace esphome {
uint32_t now = 1000;
uint32_t millis() { return now; }
class StringRef {
 public:
  explicit StringRef(const char *value) : value_(value) {}
  const char *c_str() const { return value_; }
 private:
  const char *value_;
};
}

std::string string_ref_limited(esphome::StringRef value, size_t) {
  return value.c_str();
}
constexpr size_t HA_SHORT_STATE_MAX_LEN = 64;

struct lv_obj_t;
struct MediaControlCtx {
  std::string entity_id;
  std::string power_entity = "media_player.separate_power";
  bool available = true;
  bool state_known = true;
  bool supported_features_known = false;
  int supported_features = 0;
  bool mute_known = false;
  bool muted = false;
  bool mute_pending = false;
  bool mute_pending_target = false;
  uint32_t mute_pending_until_ms = 0;
  uint32_t accent_color = 0x336699;
  lv_obj_t *icon_lbl = nullptr;
  const char *icon_glyph = nullptr;
  const char *muted_icon_glyph = nullptr;
  bool top_shows_volume = false;
  bool cover_art_mode = false;
};

struct MediaPlaybackState {
  uint32_t generation = 1;
  std::string entity_id;
  bool supported_features_known = false;
  int supported_features = 0;
  bool mute_subscribed = false;
  bool mute_known = false;
  bool muted = false;
  std::vector<MediaControlCtx *> controls;
};

uint32_t ha_subscription_generation() { return 1; }
bool media_playback_generation_valid(MediaPlaybackState *state,
                                     uint32_t generation) {
  return state && state->generation == generation;
}

struct AttributeSubscription {
  std::string entity_id;
  std::string attribute;
  std::function<void(esphome::StringRef)> callback;
};
std::vector<AttributeSubscription> subscriptions;
int reannounce_count = 0;

void ha_subscribe_attribute(
    const std::string &entity_id,
    const std::string &attribute,
    std::function<void(esphome::StringRef)> callback) {
  subscriptions.push_back({entity_id, attribute, std::move(callback)});
}
void ha_reannounce_state_subscriptions() { reannounce_count++; }

using lv_coord_t = int;
using lv_color_t = uint32_t;
constexpr int LV_OBJ_FLAG_HIDDEN = 1, LV_OBJ_FLAG_CLICKABLE = 2;
constexpr int LV_STATE_DISABLED = 1, LV_PART_MAIN = 0;
constexpr int LV_OPA_COVER = 255, LV_OPA_50 = 128, LV_LAYOUT_NONE = 0;
constexpr int LV_ALIGN_CENTER = 1, LV_ALIGN_BOTTOM_MID = 2;
struct lv_obj_t {
  std::string text;
  int flags = LV_OBJ_FLAG_CLICKABLE, states = 0, opacity = 255;
  uint32_t background = 0;
  lv_obj_t *label = nullptr;
  int width = 0, height = 0, x = 0, y = 0, alignment = 0, zoom = 256;
};
struct lv_timer_t {
  void (*callback)(lv_timer_t *);
  uint32_t period;
  bool deleted = false;
};
std::vector<std::unique_ptr<lv_obj_t>> objects;
std::vector<std::unique_ptr<lv_timer_t>> timers;
int timer_create_count = 0, timer_delete_count = 0;
lv_obj_t *make_test_button() {
  auto label = std::make_unique<lv_obj_t>();
  auto button = std::make_unique<lv_obj_t>();
  button->label = label.get();
  auto *result = button.get();
  objects.push_back(std::move(label));
  objects.push_back(std::move(button));
  return result;
}
lv_obj_t *lv_obj_get_child(lv_obj_t *object, int) { return object->label; }
lv_obj_t *control_modal_icon_label(lv_obj_t *object) { return object->label; }
void lv_label_set_display_text(lv_obj_t *label, const char *text) { label->text = text; }
void lv_obj_add_flag(lv_obj_t *object, int flag) { object->flags |= flag; }
void lv_obj_clear_flag(lv_obj_t *object, int flag) { object->flags &= ~flag; }
void lv_obj_add_state(lv_obj_t *object, int state) { object->states |= state; }
void lv_obj_clear_state(lv_obj_t *object, int state) { object->states &= ~state; }
void lv_obj_set_style_opa(lv_obj_t *object, int opacity, int) { object->opacity = opacity; }
void lv_obj_set_style_bg_color(lv_obj_t *object, uint32_t color, int) {
  object->background = color;
}
void lv_obj_set_style_bg_opa(lv_obj_t *, int, int) {}
void lv_obj_set_style_text_color(lv_obj_t *, uint32_t, int) {}
void lv_obj_set_style_radius(lv_obj_t *, int, int) {}
void lv_obj_set_style_transform_zoom(lv_obj_t *object, int zoom, int) {
  object->zoom = zoom;
}
void lv_obj_set_size(lv_obj_t *object, int width, int height) {
  object->width = width; object->height = height;
}
void lv_obj_align(lv_obj_t *object, int alignment, int x, int y) {
  object->alignment = alignment; object->x = x; object->y = y;
}
void control_modal_center_tab_icon(lv_obj_t *) {}
void lv_obj_clean(lv_obj_t *) {}
void lv_obj_set_layout(lv_obj_t *, int) {}
void lv_obj_set_style_pad_row(lv_obj_t *, int, int) {}
lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), uint32_t period, void *) {
  auto timer = std::make_unique<lv_timer_t>(lv_timer_t{callback, period});
  auto *result = timer.get();
  timers.push_back(std::move(timer));
  ++timer_create_count;
  return result;
}
void lv_timer_del(lv_timer_t *timer) {
  assert(timer && !timer->deleted);
  timer->deleted = true;
  ++timer_delete_count;
}
struct Theme { uint32_t surface_primary = 0x111111, text_primary = 0xffffff; };
Theme &current_theme() { static Theme theme; return theme; }
uint32_t theme_display_color(uint32_t color) { return color; }
uint32_t lv_color_hex(uint32_t color) { return color; }
uint32_t readable_text_color_for_bg(uint32_t) { return 0xffffff; }
void theme_set_content_background(lv_obj_t *, bool) {}
const char *find_icon(const char *name) { return name; }
std::string button_label(lv_obj_t *button) { return button->label->text; }
bool button_hidden(lv_obj_t *button) { return button->flags & LV_OBJ_FLAG_HIDDEN; }
bool button_enabled(lv_obj_t *button) {
  return !(button->states & LV_STATE_DISABLED) &&
         (button->flags & LV_OBJ_FLAG_CLICKABLE);
}
bool button_accented(lv_obj_t *button) { return button->background == 0x336699; }
struct MediaSpeakerRowState {};
void media_control_cancel_speaker_action(MediaSpeakerRowState *, const char *) {}

std::string sent_entity, sent_service, sent_key, sent_value;
int action_send_count = 0;
void send_media_player_action(const std::string &entity, const char *service,
                              const char *key, const char *value) {
  sent_entity = entity; sent_service = service;
  sent_key = key; sent_value = value;
  ++action_send_count;
}

struct ModalProfile { int base_touch_target = 48; bool compact = false; };
struct ControlModalLayout {
  int btn_size = 96, short_side = 400, controls_gap = 12, controls_center_y = 88;
  ModalProfile profile;
};
bool display_modal_uses_compact_landscape_controls(const ModalProfile &profile) {
  return profile.compact;
}
int control_modal_scaled_px(int value, int short_side) { return value * short_side / 400; }
'''

start = media.index('struct MediaControlModalUi {')
end = media.index('\n};', start) + len('\n};')
source += media[start:end]
source += '\ninline void media_control_refresh_mute(MediaControlCtx *ctx);\n'
source += '\ninline void media_control_mute_pending_timer_cb(lv_timer_t *timer);\n'

timeout = re.search(r'constexpr uint32_t MEDIA_MUTE_PENDING_TIMEOUT_MS = \d+;', media)
assert timeout, 'Missing production mute confirmation timeout'
source += timeout.group() + '\n'
line, body = definition(actions, 'send_media_mute_action')
source += f'\n#line {line} "button_grid_actions.h"\n{body}\n'
for name in (
    'media_control_modal_ui',
    'media_control_apply_availability',
    'media_control_style_playback_mode_button',
    'media_control_mute_supported',
    'media_control_mute_pending_active',
    'media_control_refresh_mute',
    'media_control_mute_pending_timer_cb',
    'media_control_send_mute_action',
    'media_control_refresh_card_mute_icon',
    'media_playback_apply_mute_to_control',
    'media_playback_apply_mute_to_controls',
    'media_playback_update_mute_state',
    'media_playback_subscribe_mute',
    'media_control_clear_tab_content',
):
    line, body = definition(media, name)
    source += f'\n#line {line} "button_grid_media.h"\n{body}\n'

# Exercise the unchanged production row block against the existing two-button
# layout contract. The rest of the popup layout needs hardware/display fixtures.
line, body = definition(modal, 'control_modal_apply_step_buttons_layout')
source += f'\n#line {line} "button_grid_modal.h"\n{body}\n'
_, layout_body = definition(media, 'media_control_layout_modal')
row_start = layout_body.index('lv_obj_t *volume_buttons[3]')
row_end = layout_body.index('if (ui.volume_pct_lbl)', row_start)
row = layout_body[row_start:row_end]
source += '''\nvoid layout_volume_buttons(ControlModalLayout volume_buttons_layout) {
  MediaControlModalUi &ui = media_control_modal_ui();
  const ControlModalLayout &layout = volume_buttons_layout;
  const bool compact_landscape = layout.profile.compact;
'''
source += row + '\n}\n'

source += r'''
int main() {
MediaPlaybackState state;
state.generation = ha_subscription_generation();
state.entity_id = "media_player.primary";
MediaControlCtx first;
MediaControlCtx second;
first.entity_id = second.entity_id = state.entity_id;
state.controls = {&first, &second};

state.supported_features_known = false;
assert(!media_playback_subscribe_mute(&state));
assert(subscriptions.empty());

state.supported_features_known = true;
state.supported_features = espcontrol::media::SUPPORT_VOLUME_MUTE;
if (media_playback_subscribe_mute(&state)) ha_reannounce_state_subscriptions();
if (media_playback_subscribe_mute(&state)) ha_reannounce_state_subscriptions();
assert(state.mute_subscribed);
assert(std::count_if(
  subscriptions.begin(), subscriptions.end(),
  [](const AttributeSubscription &item) {
    return item.entity_id == "media_player.primary" &&
           item.attribute == "is_volume_muted";
  }) == 1);
assert(reannounce_count == 1);

subscriptions.back().callback(esphome::StringRef("on"));
assert(state.mute_known && state.muted);
assert(first.mute_known && first.muted);
assert(second.mute_known && second.muted);

subscriptions.back().callback(esphome::StringRef("unknown"));
assert(!state.mute_known && state.muted);
assert(!first.mute_known && first.muted);
assert(!second.mute_known && second.muted);

first.mute_pending = true;
first.mute_pending_target = false;
first.mute_pending_until_ms = 2000;
subscriptions.back().callback(esphome::StringRef("on"));
assert(first.mute_pending && first.mute_pending_until_ms == 2000);
subscriptions.back().callback(esphome::StringRef("unknown"));
assert(first.mute_pending && first.mute_pending_until_ms == 2000);
subscriptions.back().callback(esphome::StringRef("off"));
assert(first.mute_known && !first.muted);
assert(!first.mute_pending && first.mute_pending_until_ms == 0);
assert(second.mute_known && !second.muted);

MediaControlCtx ctx;
ctx.entity_id = "media_player.primary";
ctx.available = true;
ctx.supported_features_known = true;
ctx.supported_features = espcontrol::media::SUPPORT_VOLUME_MUTE;
ctx.mute_known = true;
ctx.muted = false;
MediaControlModalUi &ui = media_control_modal_ui();
ui.active = &ctx;
ui.volume_tab = make_test_button();
ui.volume_mute_btn = make_test_button();
state.controls = {&ctx};

media_control_refresh_mute(&ctx);
assert(button_label(ui.volume_tab) == find_icon("Volume High"));
assert(!button_hidden(ui.volume_mute_btn));
assert(button_enabled(ui.volume_mute_btn));
assert(!button_accented(ui.volume_mute_btn));
// The mute button stays grey; its icon shows the confirmed state, like the tab icon.
assert(button_label(ui.volume_mute_btn) == find_icon("Volume High"));

media_control_send_mute_action(&ctx);
assert(sent_entity == "media_player.primary");
assert(sent_entity != ctx.power_entity);
assert(sent_service == "media_player.volume_mute");
assert(sent_key == "is_volume_muted" && sent_value == "true");
assert(ctx.mute_pending && ctx.mute_pending_target);
assert(ctx.mute_pending_until_ms == esphome::now + 1500);
assert(ui.mute_pending_timer && ui.mute_pending_timer->period == 1500);
assert(!button_enabled(ui.volume_mute_btn));
assert(button_label(ui.volume_tab) == find_icon("Volume High"));
assert(!button_accented(ui.volume_mute_btn));
assert(!ctx.muted); // Wait for Home Assistant; do not optimistically mute.

const int sends_while_pending = action_send_count;
const int timers_while_pending = timer_create_count;
media_control_send_mute_action(&ctx);
assert(action_send_count == sends_while_pending);
assert(timer_create_count == timers_while_pending);

// A real subscription callback updates confirmed UI state without rebuilding.
lv_obj_t *mute_button = ui.volume_mute_btn;
lv_timer_t *pending_timer = ui.mute_pending_timer;
subscriptions.back().callback(esphome::StringRef("off"));
assert(ctx.mute_pending && ui.mute_pending_timer == pending_timer);
assert(button_label(ui.volume_tab) == find_icon("Volume High"));
assert(!button_accented(ui.volume_mute_btn));
assert(!button_enabled(ui.volume_mute_btn));
subscriptions.back().callback(esphome::StringRef("on"));
assert(ctx.mute_known && ctx.muted);
assert(!ctx.mute_pending && ctx.mute_pending_until_ms == 0);
assert(ui.mute_pending_timer == nullptr && pending_timer->deleted);
assert(ui.volume_mute_btn == mute_button);
assert(button_label(ui.volume_tab) == find_icon("Volume Off"));
assert(!button_accented(ui.volume_mute_btn));
assert(button_enabled(ui.volume_mute_btn));
assert(button_label(ui.volume_mute_btn) == find_icon("Volume Off"));

ctx.muted = true;
ctx.mute_pending = false;
ctx.mute_pending_until_ms = 0;
media_control_refresh_mute(&ctx);
assert(button_label(ui.volume_tab) == find_icon("Volume Off"));
assert(!button_accented(ui.volume_mute_btn));

ctx.mute_known = false;
media_control_refresh_mute(&ctx);
assert(button_label(ui.volume_tab) == find_icon("Volume High"));
assert(!button_enabled(ui.volume_mute_btn));
media_control_send_mute_action(&ctx);
assert(action_send_count == sends_while_pending);

ctx.supported_features = 0;
media_control_refresh_mute(&ctx);
assert(button_hidden(ui.volume_mute_btn));
assert(button_label(ui.volume_tab) == find_icon("Volume High"));
media_control_send_mute_action(&ctx);
assert(action_send_count == sends_while_pending);

ctx.supported_features = espcontrol::media::SUPPORT_VOLUME_MUTE;
ctx.supported_features_known = false;
media_control_refresh_mute(&ctx);
assert(button_hidden(ui.volume_mute_btn));
assert(!media_control_mute_supported(&ctx));
assert(!media_control_mute_supported(nullptr));
ctx.supported_features_known = true;
ctx.mute_known = true;
ctx.muted = true;
ctx.mute_pending = true;
ctx.mute_pending_target = false;
ctx.mute_pending_until_ms = esphome::now + 1500;
media_control_refresh_mute(&ctx);
assert(button_label(ui.volume_tab) == find_icon("Volume Off"));
assert(!button_accented(ui.volume_mute_btn));
assert(!button_enabled(ui.volume_mute_btn));

esphome::now += 1500;
pending_timer = ui.mute_pending_timer;
media_control_mute_pending_timer_cb(ui.mute_pending_timer);
assert(!ctx.mute_pending && ctx.mute_pending_until_ms == 0);
assert(ui.mute_pending_timer == nullptr && pending_timer->deleted);
assert(button_enabled(ui.volume_mute_btn));
assert(ctx.muted && !button_accented(ui.volume_mute_btn));

// Unmute sends explicit false and retains the confirmed muted appearance.
media_control_send_mute_action(&ctx);
assert(sent_value == "false" && !ctx.mute_pending_target);
assert(ctx.muted && !button_accented(ui.volume_mute_btn));
pending_timer = ui.mute_pending_timer;
subscriptions.back().callback(esphome::StringRef("on"));
assert(ctx.mute_pending && ui.mute_pending_timer == pending_timer);
assert(button_label(ui.volume_tab) == find_icon("Volume Off"));
assert(!button_accented(ui.volume_mute_btn));
subscriptions.back().callback(esphome::StringRef("unknown"));
assert(ctx.mute_pending && !ctx.mute_known);
assert(button_label(ui.volume_tab) == find_icon("Volume High"));
assert(!button_accented(ui.volume_mute_btn));
assert(!button_enabled(ui.volume_mute_btn));
subscriptions.back().callback(esphome::StringRef("off"));
assert(!ctx.mute_pending && ui.mute_pending_timer == nullptr);
assert(pending_timer->deleted && !ctx.muted);
assert(button_enabled(ui.volume_mute_btn));
assert(button_label(ui.volume_mute_btn) == find_icon("Volume High"));

const int sends_before_unavailable = action_send_count;
ctx.available = false;
media_control_refresh_mute(&ctx);
assert(!button_enabled(ui.volume_mute_btn));
media_control_send_mute_action(&ctx);
ctx.available = true;
ctx.state_known = false;
media_control_refresh_mute(&ctx);
assert(!button_enabled(ui.volume_mute_btn));
media_control_send_mute_action(&ctx);
media_control_send_mute_action(nullptr);
assert(action_send_count == sends_before_unavailable);
ctx.state_known = true;

// Tab cleanup owns the timer and widgets, while the command deadline survives.
media_control_send_mute_action(&ctx);
pending_timer = ui.mute_pending_timer;
const uint32_t deadline = ctx.mute_pending_until_ms;
media_control_clear_tab_content();
assert(pending_timer->deleted && ui.mute_pending_timer == nullptr);
assert(ui.volume_mute_btn == nullptr);
assert(ctx.mute_pending && ctx.mute_pending_until_ms == deadline);
esphome::now += 500;
ui.volume_mute_btn = make_test_button();
media_control_refresh_mute(&ctx);
assert(!button_enabled(ui.volume_mute_btn));
assert(ui.mute_pending_timer && ui.mute_pending_timer->period == 1000);
assert(ctx.mute_pending_until_ms == deadline);
esphome::now += 1000;
ui.mute_pending_timer->callback(ui.mute_pending_timer);
assert(!ctx.mute_pending && button_enabled(ui.volume_mute_btn));

media_control_send_mute_action(&ctx);
media_control_clear_tab_content();
esphome::now += 1500;
ui.volume_mute_btn = make_test_button();
media_control_refresh_mute(&ctx);
assert(!ctx.mute_pending && ui.mute_pending_timer == nullptr);
assert(button_enabled(ui.volume_mute_btn));

// Ordinary callbacks refresh the tab icon even with no Volume content mounted.
media_control_clear_tab_content();
subscriptions.back().callback(esphome::StringRef("on"));
assert(button_label(ui.volume_tab) == find_icon("Volume Off"));
subscriptions.back().callback(esphome::StringRef("off"));
assert(button_label(ui.volume_tab) == find_icon("Volume High"));

// Compare unsupported geometry to the existing two-button contract, including
// compact landscape's bottom alignment, minimum size, and icon scale.
for (bool compact : {false, true}) {
  for (int short_side : {320, 400, 720}) {
    for (int size : {54, 55, 57, 96}) {
      ControlModalLayout layout;
      layout.profile.compact = compact;
      layout.short_side = short_side;
      layout.btn_size = size;
      ui.volume_minus_btn = make_test_button();
      ui.volume_plus_btn = make_test_button();
      ui.volume_mute_btn = nullptr;
      auto *old_minus = make_test_button();
      auto *old_plus = make_test_button();
      control_modal_apply_step_buttons_layout(
        old_minus, old_plus, layout, compact,
        control_modal_scaled_px(4, short_side), 256);
      layout_volume_buttons(layout);
      for (const auto &pair : {std::make_pair(ui.volume_minus_btn, old_minus),
                              std::make_pair(ui.volume_plus_btn, old_plus)}) {
        assert(pair.first->width == pair.second->width);
        assert(pair.first->height == pair.second->height);
        assert(pair.first->alignment == pair.second->alignment);
        assert(pair.first->x == pair.second->x && pair.first->y == pair.second->y);
        assert(pair.first->label->zoom == pair.second->label->zoom);
      }
      ui.volume_mute_btn = make_test_button();
      layout_volume_buttons(layout);
      assert(ui.volume_mute_btn->x == 0);
      assert(ui.volume_minus_btn->x == -ui.volume_plus_btn->x);
      assert(ui.volume_plus_btn->x == ui.volume_plus_btn->width + layout.controls_gap);
      assert(ui.volume_minus_btn->width == ui.volume_mute_btn->width);
      assert(ui.volume_mute_btn->width == ui.volume_plus_btn->width);
      assert(ui.volume_mute_btn->y == old_minus->y);
      assert(ui.volume_mute_btn->alignment == old_minus->alignment);
      assert(ui.volume_mute_btn->label->zoom == old_minus->label->zoom);
    }
  }
}

// A real action can wrap its deadline exactly to zero; the pending flag owns validity.
esphome::now = UINT32_MAX - 1499;
const int sends_before_zero_deadline = action_send_count;
media_control_send_mute_action(&ctx);
assert(action_send_count == sends_before_zero_deadline + 1);
assert(ctx.mute_pending && ctx.mute_pending_until_ms == 0);
assert(media_control_mute_pending_active(&ctx));
pending_timer = ui.mute_pending_timer;
assert(pending_timer && pending_timer->period == 1500);
const int timers_at_zero_deadline = timer_create_count;
esphome::now += 1499;
assert(esphome::now == UINT32_MAX);
assert(media_control_mute_pending_active(&ctx));
media_control_send_mute_action(&ctx);
assert(action_send_count == sends_before_zero_deadline + 1);
assert(timer_create_count == timers_at_zero_deadline);
assert(!button_enabled(ui.volume_mute_btn));
esphome::now += 1;
assert(esphome::now == 0);
pending_timer->callback(pending_timer);
assert(!ctx.mute_pending && ctx.mute_pending_until_ms == 0);
assert(!media_control_mute_pending_active(&ctx));
assert(ui.mute_pending_timer == nullptr && pending_timer->deleted);
assert(button_enabled(ui.volume_mute_btn));

assert(!media_control_mute_pending_active(nullptr));
ctx.mute_pending = true;
ctx.mute_pending_until_ms = 0;
assert(!media_control_mute_pending_active(&ctx) && !ctx.mute_pending);
esphome::now = UINT32_MAX - 1000;
ctx.mute_pending = true;
ctx.mute_pending_until_ms = esphome::now + 1500;
assert(media_control_mute_pending_active(&ctx));
esphome::now += 1500;
assert(!media_control_mute_pending_active(&ctx));
assert(ctx.mute_pending_until_ms == 0);

// The main-screen card swaps to its muted icon only for a confirmed mute.
state.controls = {&first};
lv_obj_t *card_icon = make_test_button()->label;
card_icon->text = find_icon("Speaker");
first.icon_lbl = card_icon;
first.icon_glyph = find_icon("Speaker");
first.muted_icon_glyph = find_icon("Speaker Off");
first.supported_features_known = true;
first.supported_features = espcontrol::media::SUPPORT_VOLUME_MUTE;
subscriptions.back().callback(esphome::StringRef("on"));
assert(card_icon->text == find_icon("Speaker Off"));
subscriptions.back().callback(esphome::StringRef("unknown"));
assert(card_icon->text == find_icon("Speaker"));
subscriptions.back().callback(esphome::StringRef("on"));
subscriptions.back().callback(esphome::StringRef("off"));
assert(card_icon->text == find_icon("Speaker"));
first.top_shows_volume = true;
subscriptions.back().callback(esphome::StringRef("on"));
assert(card_icon->text == find_icon("Speaker"));
first.top_shows_volume = false;
first.cover_art_mode = true;
subscriptions.back().callback(esphome::StringRef("off"));
subscriptions.back().callback(esphome::StringRef("on"));
assert(card_icon->text == find_icon("Speaker"));
first.cover_art_mode = false;
first.supported_features = 0;
subscriptions.back().callback(esphome::StringRef("off"));
subscriptions.back().callback(esphome::StringRef("on"));
assert(card_icon->text == find_icon("Speaker"));
first.icon_lbl = nullptr;
subscriptions.back().callback(esphome::StringRef("off"));

assert(timer_create_count == timer_delete_count);
return 0;
}
'''

assert 'state->mute_subscribed = false;' in media
assert 'state->mute_known = false;' in media
assert 'state->muted = false;' in media
assert 'if (media_playback_subscribe_mute(state))' in media
assert 'media_playback_apply_mute_to_control(state, ctx);' in media
assert 'ui.volume_mute_btn = control_modal_create_round_button(' in media
assert 'find_icon("Volume Off")' in media
assert 'ui.volume_minus_btn, ui.volume_mute_btn, ui.volume_plus_btn' in media
assert 'lv_timer_del(ui.mute_pending_timer)' in media
assert 'ui.volume_mute_btn = nullptr;' in media
_, creator = definition(media, 'media_control_create_volume_tab_content')
assert 'if (media_control_mute_supported(ctx))' in creator
assert 'media_control_send_mute_action(media_control_modal_ui().active);' in creator
assert 'media_control_refresh_mute(ctx);' in creator
_, parent = definition(media, 'media_control_refresh_parent_card')
assert 'media_control_refresh_card_mute_icon(ctx);' in parent
_, card_ctx = definition(media, 'create_media_control_context')
assert 'ctx->icon_glyph = ' in card_ctx and 'ctx->muted_icon_glyph = ' in card_ctx
_, refresh = definition(media, 'media_control_refresh_modal')
assert 'media_control_refresh_mute(ctx);' in refresh
_, apply_state = definition(media, 'media_playback_apply_state_to_control')
assert 'const bool mute_was_pending = ctx->mute_pending;' in apply_state
assert 'const bool previous_mute_supported = media_control_mute_supported(ctx);' in apply_state
assert 'previous_mute_supported != mute_supported;' in apply_state
assert 'mute_capability_changed && ui.tab == MediaControlTab::VOLUME' in apply_state
assert 'media_control_refresh_mute(ctx);' in apply_state
assert 'mute_was_pending && !ctx->mute_pending' in apply_state

with tempfile.TemporaryDirectory(prefix='media-mute-control-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(source)
    subprocess.run([
        os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
        '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'),
        '-o', str(path / 'test')
    ], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Production media mute subscriptions, confirmed UI, actions, timeout, cleanup, and layout passed')
