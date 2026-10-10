#!/usr/bin/env python3
"""Compile production media tab selection against a simulated modal boundary."""
from pathlib import Path
import os
import subprocess
import tempfile
from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
media = (root / 'components/espcontrol/button_grid_media.h').read_text()
source = r'''
#include <cassert>
#include <string>
#include "media_control_tabs.h"
#include "media_power_capability.h"
struct MediaControlCtx {
  espcontrol::media::ControlTabs control_tabs;
  bool group_only = false, progress = true, power = true;
  bool grouping_supported = true, speaker_discovery_available = true;
  bool available = true, power_state_known = true, power_available = true;
  std::string power_state_text = "on";
  size_t group_size = 1;
};
struct MediaPlaybackState {
  bool has_state = true, available = true;
  std::string state_text = "on";
};
struct MediaControlModalUi {
  MediaControlCtx *active = nullptr;
  espcontrol::media::ControlTabs visible_tabs;
  MediaControlTab tab = MediaControlTab::CONTROLS;
  int speaker_generation = 0;
};
MediaControlModalUi ui;
int cleared = 0, hidden = 0, layouts = 0, refreshed = 0;
MediaControlModalUi &media_control_modal_ui() { return ui; }
void media_control_clear_tab_content() { ++cleared; }
bool media_control_progress_supported(MediaControlCtx *ctx) { return ctx->progress; }
bool media_control_power_supported(MediaControlCtx *ctx) { return ctx->power; }
size_t media_control_group_size(MediaControlCtx *ctx) { return ctx->group_size; }
bool media_group_speaker_tab_available(bool supported, bool discovered, bool grouped) {
  return supported && (discovered || grouped);
}
void media_control_hide_modal() { ++hidden; ui.active = nullptr; }
void media_control_refresh_modal(MediaControlCtx*) { ++refreshed; }
void media_control_refresh_power(MediaControlCtx*) { ++refreshed; }
void media_control_ensure_visible_tab(MediaControlCtx*);
espcontrol::media::ControlTabs media_control_visible_tabs(MediaControlCtx*);
void media_control_layout_modal(MediaControlCtx *ctx) {
  ++layouts;
  media_control_ensure_visible_tab(ctx);
  ui.visible_tabs = media_control_visible_tabs(ctx);
}
'''
for name in ('media_control_power_command', 'media_control_visible_tabs',
             'media_control_tab_layout_changed', 'media_control_can_open_modal',
             'media_control_refresh_open_modal', 'media_control_ensure_visible_tab',
             'media_playback_apply_state_to_power_control'):
    line, body = definition(media, name)
    source += f'\n#line {line} "button_grid_media.h"\n{body}\n'
source += r'''
int main() {
  using namespace espcontrol::media;
  using Tab = MediaControlTab;
  MediaControlCtx ctx;
  ctx.control_tabs = parse_control_tabs("power|volume|controls|speakers|progress", true);
  ui.active = &ctx;
  // Opening All Controls and Cover Art uses the first available configured tab.
  assert(media_control_visible_tabs(&ctx).tabs[0] == Tab::POWER);
  ui.tab = Tab::POWER;
  ctx.power = false;
  media_control_ensure_visible_tab(&ctx);
  assert(ui.tab == Tab::VOLUME && cleared == 1);
  ctx.power = true;
  media_control_ensure_visible_tab(&ctx);
  assert(ui.tab == Tab::VOLUME && cleared == 1); // Do not interrupt an active visible tab.
  ui.tab = Tab::SPEAKERS; ctx.speaker_discovery_available = false;
  media_control_ensure_visible_tab(&ctx);
  assert(ui.tab == Tab::POWER && ui.speaker_generation == 1 && cleared == 2);
  ctx.control_tabs = parse_control_tabs("volume");
  media_control_ensure_visible_tab(&ctx);
  assert(ui.tab == Tab::VOLUME && cleared == 3);
  ctx.control_tabs = parse_control_tabs("progress|speakers|power", true);
  ctx.progress = false; ctx.power = false;
  media_control_ensure_visible_tab(&ctx);
  assert(ui.tab == Tab::CONTROLS && cleared == 4);
  // Reflow on capability changes even when the tab button was created earlier.
  ctx.control_tabs = parse_control_tabs("volume|progress|power", true);
  ctx.progress = false; ctx.power = true;
  ui.visible_tabs = media_control_visible_tabs(&ctx);
  assert(!media_control_tab_layout_changed(&ctx));
  ctx.progress = true;
  assert(media_control_tab_layout_changed(&ctx));
  ui.visible_tabs = media_control_visible_tabs(&ctx);
  assert(!media_control_tab_layout_changed(&ctx));
  ctx.power = false;
  assert(media_control_tab_layout_changed(&ctx));
  ui.visible_tabs = media_control_visible_tabs(&ctx);
  ctx.power = true; ctx.progress = false;
  assert(media_control_tab_layout_changed(&ctx)); // Same count, different available tabs.
  ctx.group_only = true;
  media_control_ensure_visible_tab(&ctx);
  assert(ui.tab == Tab::SPEAKERS); // Standalone speaker groups retain their own screen.
  // An unavailable main player leaves only its usable, enabled separate Power tab.
  ctx.group_only = false;
  ctx.control_tabs = parse_control_tabs("volume|power|controls", true);
  ctx.available = true;
  ui.tab = Tab::VOLUME;
  ui.visible_tabs = media_control_visible_tabs(&ctx);
  ctx.available = false;
  assert(media_control_can_open_modal(&ctx));
  media_control_refresh_open_modal(&ctx);
  assert(ui.active == &ctx && ui.tab == Tab::POWER && layouts > 0);
  assert(ui.visible_tabs.count == 1 && ui.visible_tabs.tabs[0] == Tab::POWER);
  // A later power state loss closes an already-open offline modal.
  MediaPlaybackState power;
  power.available = false; power.state_text = "unavailable";
  media_playback_apply_state_to_power_control(&power, &ctx);
  assert(ui.active == nullptr && hidden == 1 && !media_control_can_open_modal(&ctx));
  // Recovery permits reopening Power, without reviving the offline player's tabs.
  power.available = true; power.state_text = "off";
  media_playback_apply_state_to_power_control(&power, &ctx);
  assert(media_control_can_open_modal(&ctx));
  assert(media_control_visible_tabs(&ctx).tabs[0] == Tab::POWER);
  for (const auto &invalid : {"unknown", "unavailable", ""}) {
    power.state_text = invalid;
    media_playback_apply_state_to_power_control(&power, &ctx);
    assert(!media_control_can_open_modal(&ctx));
  }
  power.state_text = "off"; power.has_state = false;
  media_playback_apply_state_to_power_control(&power, &ctx);
  assert(!media_control_can_open_modal(&ctx));
  power.has_state = true;
  media_playback_apply_state_to_power_control(&power, &ctx);
  ctx.control_tabs = parse_control_tabs("volume|controls", true);
  assert(!media_control_can_open_modal(&ctx)); // Respect manually hidden Power.
  ctx.control_tabs = parse_control_tabs("power", true); ctx.power = false;
  assert(!media_control_can_open_modal(&ctx)); // No optional power entity.
  ctx.power = true; ctx.group_only = true;
  assert(!media_control_can_open_modal(&ctx)); // Standalone speaker group stays offline.
  ctx.group_only = false; ctx.control_tabs = parse_control_tabs("volume|power|controls", true);
  ui.active = &ctx; ui.tab = Tab::POWER; ui.visible_tabs = media_control_visible_tabs(&ctx);
  // Playback recovery restores configured tabs without interrupting active Power.
  ctx.available = true;
  media_control_refresh_open_modal(&ctx);
  assert(ui.visible_tabs.count == 3 && ui.tab == Tab::POWER);
  power.available = false;
  media_playback_apply_state_to_power_control(&power, &ctx);
  assert(ui.active == &ctx && media_control_can_open_modal(&ctx));
  assert(!media_control_can_open_modal(nullptr));
}
'''
# Check that the production opener and context constructor use the tested policy.
_, opener = definition(media, 'media_control_open_modal')
assert 'ui.tab = media_control_visible_tabs(ctx).tabs[0];' in opener
assert 'if (!media_control_can_open_modal(ctx)) return;' in opener
_, playback_update = definition(media, 'media_playback_apply_state_to_control')
assert 'media_control_refresh_open_modal(ctx, layout_needed);' in playback_update
_, constructor = definition(media, 'create_media_control_context')
assert 'ctx->control_tabs = media_config.control_tabs;' in constructor
with tempfile.TemporaryDirectory(prefix='media-tabs-runtime-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(source)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Production media tab selection and capability fallback passed')
