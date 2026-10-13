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
toggle = scripts['cover_art_toggle_playback']['then'][0]['lambda'].replace('${cover_art_square_overlay}', 'false')
source = r'''
#include <cassert>
#include <cstdio>
#include <string>
#include "screensaver_pin.h"
#include "display_mode_controller.h"
#define id(value) value
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
struct Widget {} widget;
Widget *cover_art_screensaver = &widget;
bool lv_obj_has_flag(Widget *, int) { return false; }
bool screensaver_wake_touch_guard_active = false;
struct Setting { bool state = true; } cover_art_playback_control_enabled;
struct App { espcontrol::DisplayModeController controller; auto &display() { return controller; } } espcontrol_app;
std::string cover_art_active_media_player_entity = "media_player.room";
std::string cover_art_last_playback_state = "playing";
bool ha_api_state_connected() { return true; }
uint32_t millis() { return 1234; }
namespace espcontrol::cover_art { enum class PlaybackCommand { NONE, PAUSE, PLAY }; }
int sent = 0;
bool ha_send_entity_action(const std::string &, const char *) { ++sent; return true; }
struct Control {
  auto begin(const std::string &, const std::string &, uint32_t) { return espcontrol::cover_art::PlaybackCommand::PAUSE; }
  void cancel_pending() {}
} cover_art_playback_control;
struct Script { void execute() {} } cover_art_update_playback_control;
bool uses_wake_path() { WAKE_LAMBDA }
void toggle_playback() { TOGGLE_LAMBDA }
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
  assert(pin.save(false, nullptr));
  assert(!uses_wake_path());
  toggle_playback();
  assert(sent == 1); // Unprotected playback keeps its existing direct action.
  std::string empty;
  assert(pin.save(true, &empty));
  pin.arm();
  assert(!uses_wake_path());
  toggle_playback();
  assert(sent == 2); // No PIN preserves existing screensaver behavior too.
}
'''.replace('WAKE_LAMBDA', wake).replace('TOGGLE_LAMBDA', toggle)
with tempfile.TemporaryDirectory() as directory:
    cpp, binary = Path(directory) / 'test.cpp', Path(directory) / 'test'
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'c++', '-std=c++17', '-pthread',
                    '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'components/espcontrol'),
                    '-I', str(ROOT / 'tests/firmware/screensaver_pin_stubs'), str(cpp),
                    str(ROOT / 'components/espcontrol/screensaver_pin.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Protected artwork wakes without playback commands; disabled/empty PIN keeps direct playback')
