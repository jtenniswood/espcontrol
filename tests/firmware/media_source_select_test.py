#!/usr/bin/env python3
"""Exercise production media source-list subscription, parsing, and propagation."""
import os
import subprocess
import tempfile
from pathlib import Path

from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
media = (root / 'components/espcontrol/button_grid_media.h').read_text()
option_select = (root / 'components/espcontrol/button_grid_option_select.h').read_text()
config_parser = (root / 'components/espcontrol/button_grid_config_parser.h').read_text()
source = r'''
#include <cassert>
#include <cctype>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "media_control_tabs.h"
#include "media_playback_modes.h"

namespace esphome {
class StringRef {
 public:
  explicit StringRef(const char *value) : value_(value) {}
  const char *c_str() const { return value_; }
 private:
  const char *value_;
};
}

constexpr int OPTION_SELECT_MAX_OPTIONS = 64;
constexpr size_t OPTION_SELECT_OPTION_MAX_LEN = 96;
constexpr size_t OPTION_SELECT_OPTIONS_TEXT_MAX_LEN =
  OPTION_SELECT_MAX_OPTIONS * (OPTION_SELECT_OPTION_MAX_LEN + 4);
std::string string_ref_limited(esphome::StringRef value, size_t limit) {
  std::string text = value.c_str();
  return text.size() > limit ? text.substr(0, limit) : text;
}

struct MediaControlCtx {
  std::vector<std::string> source_list;
  std::string selected_source;
  bool selected_source_known = false;
};

struct MediaPlaybackState {
  uint32_t generation = 1;
  std::string entity_id = "media_player.amp_zone";
  bool supported_features_known = false;
  int supported_features = 0;
  bool source_list_subscribed = false;
  std::vector<std::string> source_list;
  std::string selected_source;
  bool selected_source_known = false;
  std::vector<MediaControlCtx *> controls;
};

struct MediaControlModalUi {
  MediaControlCtx *active = nullptr;
  MediaControlTab tab = MediaControlTab::CONTROLS;
};
MediaControlModalUi ui;
MediaControlModalUi &media_control_modal_ui() { return ui; }
int cleared = 0, rebuilt = 0, refreshed = 0;
void media_control_clear_tab_content() { ++cleared; }
void media_control_refresh_open_modal(MediaControlCtx *, bool layout_needed) {
  assert(layout_needed);
  ++rebuilt;
}
void media_control_refresh_sources(MediaControlCtx *) { ++refreshed; }
bool media_playback_generation_valid(MediaPlaybackState *state, uint32_t generation) {
  return state && state->generation == generation;
}

struct AttributeSubscription {
  std::string entity_id;
  std::string attribute;
  std::function<void(esphome::StringRef)> callback;
};
std::vector<AttributeSubscription> subscriptions;
void ha_subscribe_attribute(const std::string &entity_id, const std::string &attribute,
                            std::function<void(esphome::StringRef)> callback) {
  subscriptions.push_back({entity_id, attribute, std::move(callback)});
}
'''

line, body = definition(config_parser, 'trim_display_unit')
source += f'\n#line {line} "button_grid_config_parser.h"\n{body}\n'
for name in ('option_select_trim', 'option_select_add_option',
             'option_select_parse_delimited_options', 'option_select_parse_list_options',
             'option_select_parse_options'):
    line, body = definition(option_select, name)
    source += f'\n#line {line} "button_grid_option_select.h"\n{body}\n'
for name in ('media_playback_copy_sources_to_control',
             'media_playback_apply_sources_to_controls',
             'media_playback_update_selected_source',
             'media_playback_update_source_list',
             'media_playback_subscribe_source_list'):
    line, body = definition(media, name)
    source += f'\n#line {line} "button_grid_media.h"\n{body}\n'

source += r'''
int main() {
  using espcontrol::media::SUPPORT_SELECT_SOURCE;
  MediaPlaybackState state;
  MediaControlCtx first, second;
  state.controls = {&first, &second};

  // No subscription until Home Assistant confirms source selection support.
  assert(!media_playback_subscribe_source_list(&state));
  state.supported_features_known = true;
  state.supported_features = 659340 & ~SUPPORT_SELECT_SOURCE;
  assert(!media_playback_subscribe_source_list(&state));
  assert(subscriptions.empty());
  state.supported_features = 659340;
  assert(media_playback_subscribe_source_list(&state));
  assert(!media_playback_subscribe_source_list(&state));
  assert(subscriptions.size() == 1);
  assert(subscriptions[0].entity_id == "media_player.amp_zone");
  assert(subscriptions[0].attribute == "source_list");

  // Home Assistant forwards the list as Python text; keep names exactly,
  // including leading spaces, because select_source needs the same value.
  subscriptions[0].callback(esphome::StringRef(
    "['None', 'Sonos Port', ' CAPITAL', \"Kid's Radio\", 'BBC Radio 2']"));
  const std::vector<std::string> expected = {
    "None", "Sonos Port", " CAPITAL", "Kid's Radio", "BBC Radio 2"};
  assert(state.source_list == expected);
  assert(first.source_list == expected && second.source_list == expected);

  // A truncated list is ignored rather than turned into wrong source names.
  subscriptions[0].callback(esphome::StringRef("['None', 'Sonos Po"));
  assert(state.source_list.empty() && first.source_list.empty());
  subscriptions[0].callback(esphome::StringRef(
    "['None', 'Sonos Port', ' CAPITAL', \"Kid's Radio\", 'BBC Radio 2']"));

  // The selected source is the exact Home Assistant value.
  media_playback_update_selected_source(&state, " CAPITAL");
  assert(first.selected_source_known && first.selected_source == " CAPITAL");
  media_playback_update_selected_source(&state, "unavailable");
  assert(!first.selected_source_known && first.selected_source.empty());
  media_playback_update_selected_source(&state, "BBC Radio 2");
  assert(second.selected_source_known && second.selected_source == "BBC Radio 2");

  // An open Source tab only re-highlights for a new selection, and rebuilds
  // its rows when the list itself changes.
  ui.active = &first;
  ui.tab = MediaControlTab::SOURCE;
  refreshed = cleared = rebuilt = 0;
  media_playback_update_selected_source(&state, "Sonos Port");
  assert(refreshed == 1 && cleared == 0 && rebuilt == 0);
  media_playback_update_selected_source(&state, "Sonos Port");
  assert(refreshed == 1); // Unchanged values do not refresh.
  subscriptions[0].callback(esphome::StringRef("['Sonos Port', 'BBC Radio 4']"));
  assert(cleared == 1 && rebuilt == 1);
  assert(first.source_list.size() == 2 && first.source_list[1] == "BBC Radio 4");
  ui.tab = MediaControlTab::VOLUME;
  subscriptions[0].callback(esphome::StringRef("['Sonos Port']"));
  assert(cleared == 1 && rebuilt == 1 && refreshed == 2);

  // Stale callbacks from a previous subscription generation are ignored.
  state.generation = 2;
  subscriptions[0].callback(esphome::StringRef("['Other']"));
  assert(state.source_list.size() == 1 && state.source_list[0] == "Sonos Port");
  return 0;
}
'''

# The production row tap must send the exact list value for the tapped row.
_, content = definition(media, 'media_control_create_source_tab_content')
assert 'send_media_source_action(active->entity_id, active->source_list[index]);' in content
assert 'option_select_trim(ctx->source_list[i])' in content
_, refresh = definition(media, 'media_control_refresh_sources')
assert 'ctx->source_list[i] == ctx->selected_source' in refresh
_, handler = definition(media, 'media_playback_subscribe_source')
assert 'media_playback_update_selected_source(' in handler
_, modes = definition(media, 'media_playback_subscribe_modes')
assert 'media_playback_subscribe_source_list(state)' in modes

with tempfile.TemporaryDirectory(prefix='media-source-select-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(source)
    subprocess.run([
        os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
        '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'),
        '-o', str(path / 'test')
    ], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Production media source list subscription, parsing, and propagation passed')
