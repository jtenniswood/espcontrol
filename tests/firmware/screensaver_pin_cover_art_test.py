#!/usr/bin/env python3
"""Run production artwork touch/action lambdas against the real PIN policy."""
from pathlib import Path
import subprocess
import sys
import tempfile
import yaml

ROOT = Path(__file__).resolve().parents[2]
class Loader(yaml.SafeLoader):
    pass
Loader.add_constructor('!lambda', lambda loader, node: loader.construct_scalar(node))
text = (ROOT / 'common/device/screen_cover_art.yaml').read_text()
scripts = {script['id']: script for script in yaml.load(text[text.index('script:\n'):], Loader)['script']}
wake = scripts['cover_art_handle_touch']['then'][0]['if']['condition']['lambda']
toggle = scripts['cover_art_toggle_playback']['then'][0]['lambda'].replace('${cover_art_square_overlay}', 'square_panel')
refresh = scripts['cover_art_update_playback_control']['then'][-1]['lambda'].replace('${cover_art_square_overlay}', 'square_panel')
source = r'''
#include <cassert>
#include <cstdio>
#include <string>
#include "screensaver_pin.h"
#include "display_mode_controller.h"
#define id(value) value
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
constexpr int LV_OPA_50 = 128, LV_OPA_COVER = 255;
struct Widget { bool hidden = false; } widget, button, icon;
Widget *cover_art_screensaver = &widget;
Widget *cover_art_playback_button = &button, *cover_art_playback_icon = &icon;
bool lv_obj_has_flag(Widget *obj, int) { return obj->hidden; }
void lv_obj_add_flag(Widget *obj, int) { obj->hidden = true; }
void lv_obj_clear_flag(Widget *obj, int) { obj->hidden = false; }
void lv_label_set_text(Widget *, const char *) {}
void lv_obj_set_style_opa(Widget *, int, int) {}
bool square_panel = false;
bool screensaver_wake_touch_guard_active = false;
struct Setting { bool state = true; } cover_art_playback_control_enabled;
struct App { espcontrol::DisplayModeController controller; auto &display() { return controller; } } espcontrol_app;
std::string cover_art_active_media_player_entity = "media_player.room";
std::string cover_art_last_playback_state = "playing";
bool connected = true;
bool ha_api_state_connected() { return connected; }
uint32_t millis() { return 1234; }
namespace espcontrol::cover_art { enum class PlaybackCommand { NONE, PAUSE, PLAY }; }
int sent = 0;
std::string last_service;
bool ha_send_entity_action(const std::string &, const char *service) { ++sent; last_service = service; return true; }
struct Control {
  auto begin(const std::string &, const std::string &state, uint32_t) {
    return state == "paused" ? espcontrol::cover_art::PlaybackCommand::PLAY : espcontrol::cover_art::PlaybackCommand::PAUSE;
  }
  void cancel_pending() {}
  void expire(uint32_t) {}
  bool pending() { return false; }
} cover_art_playback_control;
struct Script { void execute() {} } cover_art_update_playback_control;
bool uses_wake_path() { WAKE_LAMBDA }
void toggle_playback() { TOGGLE_LAMBDA }
void refresh_playback() { REFRESH_LAMBDA }
int main() {
  auto &pin = espcontrol::screensaver_pin();
  pin.setup();
  std::string code = "0042";
  assert(pin.save(true, &code));
  pin.arm();
  espcontrol_app.display().request(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK,
                                  espcontrol::DisplayMode::COVER_ART);
  toggle_playback();
  if (!uses_wake_path() || sent != 0) {
    std::fprintf(stderr, "Protected cover art bypass: wake=%d, commands=%d\n", uses_wake_path(), sent);
    return 1;
  }
  refresh_playback();
  assert(button.hidden); // Locked artwork must not advertise an unavailable action.
  bool allow = true;
  assert(pin.save(true, nullptr, nullptr, &allow));
  refresh_playback();
  assert(pin.locked() && !button.hidden && !uses_wake_path());
  toggle_playback();
  assert(sent == 1 && last_service == "media_player.media_pause" && pin.locked());
  cover_art_last_playback_state = "paused";
  toggle_playback();
  assert(sent == 2 && last_service == "media_player.media_play" && pin.locked());
  screensaver_wake_touch_guard_active = true;
  assert(uses_wake_path());
  toggle_playback();
  assert(sent == 2); // The original dark-screen wake tap still cannot control music.
  screensaver_wake_touch_guard_active = false;
  connected = false;
  toggle_playback();
  assert(sent == 2);
  connected = true;
  widget.hidden = true;
  assert(uses_wake_path());
  toggle_playback();
  assert(sent == 2); // No actions through a hidden artwork view.
  widget.hidden = false;
  espcontrol_app.display().clear(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK);
  assert(uses_wake_path());
  toggle_playback();
  assert(sent == 2); // The exception applies only to artwork, never normal controls.
  espcontrol_app.display().request(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK,
                                  espcontrol::DisplayMode::COVER_ART);
  allow = false;
  assert(pin.save(true, nullptr, nullptr, &allow));
  refresh_playback();
  assert(button.hidden && uses_wake_path() && pin.locked());
  toggle_playback();
  assert(sent == 2); // Turning the exception off protects playback immediately.
  sent = 0;
  assert(pin.submit(code));
  refresh_playback();
  assert(!button.hidden); // Unlocking restores the control without changing layout.
  pin.arm();
  refresh_playback();
  assert(button.hidden); // A later sleep hides it again.
  assert(pin.save(false, nullptr));
  refresh_playback();
  assert(!button.hidden); // Disabling protection restores the control too.
  assert(!uses_wake_path());
  toggle_playback();
  assert(sent == 1); // Unprotected playback keeps its existing direct action.
  std::string empty;
  assert(pin.save(true, &empty));
  pin.arm();
  assert(!uses_wake_path());
  toggle_playback();
  assert(sent == 2); // No PIN preserves existing screensaver behavior too.
  refresh_playback();
  assert(!button.hidden);
  for (bool square : {false, true}) {
    square_panel = square;
    cover_art_playback_control_enabled.state = false;
    refresh_playback();
    assert(button.hidden == square); // Only square panels allow hiding it in settings.
    cover_art_playback_control_enabled.state = true;
    refresh_playback();
    assert(!button.hidden);
    assert(pin.save(false, &code));
    assert(pin.lock()); // Manual screen lock also applies with automatic protection off.
    refresh_playback();
    assert(button.hidden);
    allow = true;
    assert(pin.save(false, nullptr, nullptr, &allow));
    refresh_playback();
    assert(!button.hidden && pin.locked() && !uses_wake_path());
    const int before = sent;
    toggle_playback();
    assert(sent == before + 1 && pin.locked());
    cover_art_playback_control_enabled.state = false;
    refresh_playback();
    assert(button.hidden == square);
    cover_art_playback_control_enabled.state = true;
    allow = false;
    assert(pin.save(false, nullptr, nullptr, &allow));
    assert(pin.submit(code));
    refresh_playback();
    assert(!button.hidden);
  }
}
'''.replace('WAKE_LAMBDA', wake).replace('TOGGLE_LAMBDA', toggle).replace('REFRESH_LAMBDA', refresh)
with tempfile.TemporaryDirectory() as directory:
    cpp, binary = Path(directory) / 'test.cpp', Path(directory) / 'test'
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'c++', '-std=c++17', '-pthread',
                    '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'components/espcontrol'),
                    '-I', str(ROOT / 'tests/firmware/screensaver_pin_stubs'), str(cpp),
                    str(ROOT / 'components/espcontrol/screensaver_pin.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Artwork playback respects PIN policy, opt-in control, wake guards and square-panel settings')
