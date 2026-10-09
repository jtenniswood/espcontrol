#!/usr/bin/env python3
"""Exercise production power routing and shared state ownership at the HA boundary."""
from pathlib import Path
import os
import subprocess
import tempfile
from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
media = (root / 'components/espcontrol/button_grid_media.h').read_text()
source = r'''
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include "media_power_capability.h"
struct MediaControlCtx {
  std::string speaker_group_entity;
  std::string entity_id, power_entity, state_text = "unknown", power_state_text = "unknown";
  bool supported_features_known = false, state_known = false, available = false;
  bool power_state_known = false, power_available = false;
  int supported_features = 0;
};
struct MediaPlaybackState {
  std::string entity_id, state_text = "unknown";
  bool has_state = false, available = false, state_subscribed = false;
  bool metadata_subscribed = false, volume_subscribed = false;
  std::vector<MediaControlCtx*> controls, power_controls;
};
constexpr size_t MEDIA_PLAYBACK_STATE_CONSUMERS_MAX = 8;
std::vector<MediaPlaybackState*> states;
std::vector<MediaPlaybackState*> &media_playback_states() { return states; }
std::string sent_entity, sent_service;
int refreshes = 0;
void send_media_player_action(const std::string &entity, const char *service) {
  sent_entity = entity; sent_service = service;
}
void media_control_refresh_power(MediaControlCtx*) { ++refreshes; }
struct MediaControlModalUi { MediaControlCtx *active = nullptr; };
MediaControlModalUi &media_control_modal_ui() { static MediaControlModalUi ui; return ui; }
void media_control_refresh_open_modal(MediaControlCtx*) { ++refreshes; }
void media_playback_refresh_progress_timer(MediaPlaybackState*) {}
MediaPlaybackState *media_playback_ensure_state(const std::string &entity) {
  for (auto *state : states) if (state->entity_id == entity) return state;
  return nullptr;
}
void media_playback_attach_control(MediaPlaybackState *state, MediaControlCtx *ctx) {
  if (std::find(state->controls.begin(), state->controls.end(), ctx) == state->controls.end())
    state->controls.push_back(ctx);
}
void media_playback_subscribe_playback_state(MediaPlaybackState *state) { state->state_subscribed = true; }
void media_playback_subscribe_content(MediaPlaybackState*) {}
void media_playback_subscribe_metadata(MediaPlaybackState *state) { state->metadata_subscribed = true; }
void media_playback_subscribe_volume(MediaPlaybackState *state) { state->volume_subscribed = true; }
void media_playback_subscribe_modes(MediaPlaybackState*) {}
void media_playback_subscribe_grouping(MediaPlaybackState*) {}
void media_playback_subscribe_speaker_discovery(MediaPlaybackState*, const std::string&) {}
void media_playback_subscribe_progress(MediaPlaybackState*) {}
void media_playback_subscribe_friendly_name(MediaPlaybackState*) {}
#define ESP_LOGW(...) ((void)0)
template<typename T>
void media_playback_erase_consumer(std::vector<T*> &items, T *item) {
  items.erase(std::remove(items.begin(), items.end(), item), items.end());
}
'''
for name in ('media_control_power_supported', 'media_control_power_command',
             'media_control_send_power_action', 'media_playback_apply_state_to_power_control',
             'media_playback_attach_power_control', 'media_playback_detach_control',
             'subscribe_media_control_state'):
    line, body = definition(media, name)
    source += f'\n#line {line} "button_grid_media.h"\n{body}\n'
source += r'''
int main() {
  using namespace espcontrol::media;
  MediaControlCtx ctx;
  ctx.entity_id = "media_player.sonos";
  ctx.state_known = true; ctx.available = true; ctx.state_text = "playing";
  ctx.supported_features_known = true; ctx.supported_features = 0;
  assert(!media_control_power_supported(&ctx));
  assert(media_control_power_command(&ctx) == PowerCommand::NONE);
  ctx.power_entity = "media_player.tv";
  assert(media_control_power_supported(&ctx)); // Sonos has no power capability.
  assert(media_control_power_command(&ctx) == PowerCommand::NONE);
  media_control_send_power_action(&ctx);
  assert(sent_entity.empty());
  MediaPlaybackState tv;
  tv.entity_id = ctx.power_entity;
  MediaPlaybackState sonos; sonos.entity_id = ctx.entity_id;
  states = {&tv, &sonos};
  tv.has_state = true; tv.available = true; tv.state_text = "off";
  subscribe_media_control_state(&ctx);
  subscribe_media_control_state(&ctx);
  assert(tv.state_subscribed && !tv.metadata_subscribed && !tv.volume_subscribed);
  assert(sonos.metadata_subscribed && sonos.volume_subscribed);
  assert(tv.power_controls.size() == 1); // Shared subscriptions have one consumer.
  assert(media_control_power_command(&ctx) == PowerCommand::TURN_ON);
  media_control_send_power_action(&ctx);
  assert(sent_entity == "media_player.tv" && sent_service == "homeassistant.turn_on");
  // TV state changes update only power; playback remains on the Sonos.
  tv.state_text = "on";
  media_playback_apply_state_to_power_control(&tv, &ctx);
  assert(ctx.entity_id == "media_player.sonos" && ctx.state_text == "playing");
  media_control_send_power_action(&ctx);
  assert(sent_entity == "media_player.tv" && sent_service == "homeassistant.turn_off");
  // The TV remains usable even when the soundbar is unavailable.
  ctx.available = false;
  assert(media_control_power_command(&ctx) == PowerCommand::TURN_OFF);
  for (const auto &state : {"unknown", "unavailable", ""}) {
    tv.state_text = state;
    media_playback_apply_state_to_power_control(&tv, &ctx);
    assert(media_control_power_command(&ctx) == PowerCommand::NONE);
  }
  tv.state_text = "off"; tv.available = false;
  media_playback_apply_state_to_power_control(&tv, &ctx);
  assert(media_control_power_command(&ctx) == PowerCommand::NONE);
  tv.available = true; tv.has_state = false;
  media_playback_apply_state_to_power_control(&tv, &ctx);
  assert(media_control_power_command(&ctx) == PowerCommand::NONE);
  // Generic on/off entities use the same power route.
  ctx.power_entity = "switch.tv"; tv.has_state = true;
  media_playback_apply_state_to_power_control(&tv, &ctx);
  media_control_send_power_action(&ctx);
  assert(sent_entity == "switch.tv" && sent_service == "homeassistant.turn_on");
  media_playback_detach_control(&ctx);
  assert(sonos.controls.empty() && tv.power_controls.empty());
  // Without an optional entity, native power capability must not enable the tab or actions.
  ctx.power_entity.clear(); ctx.available = true; ctx.state_text = "off";
  ctx.supported_features = SUPPORT_TURN_ON | SUPPORT_TURN_OFF;
  sent_entity.clear(); sent_service.clear();
  assert(!media_control_power_supported(&ctx));
  assert(media_control_power_command(&ctx) == PowerCommand::NONE);
  media_control_send_power_action(&ctx);
  assert(sent_entity.empty() && sent_service.empty());
  assert(refreshes > 0);
}
'''
with tempfile.TemporaryDirectory(prefix='media-power-override-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(source)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Production media power routing and lifecycle passed')
