#!/usr/bin/env python3
"""Exercise the production screensaver refresh and its wall-clock trigger wiring."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import yaml

parser = argparse.ArgumentParser()
parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
root = parser.parse_args().root

class Loader(yaml.SafeLoader):
    pass

Loader.add_constructor('!lambda', lambda loader, node: loader.construct_scalar(node))

def load(name):
    return yaml.load((root / 'common/addon' / name).read_text(), Loader)

time = load('time.yaml')
backlight = load('backlight.yaml')
scripts = {s['id']: s for config in (time, backlight) for s in config['script']}

# Both clock sources must refresh at second zero, regardless of boot/interval phase.
for source in time['time']:
    event = next(e for e in source['on_time'] if e.get('seconds') == 0)
    assert event['minutes'] == '*'
    assert {'script.execute': 'time_update'} in event['then']

refresh = next((a['if'] for a in scripts['time_update']['then'] if 'if' in a and
                {'script.execute': 'clock_screensaver_update_time'} in a['if']['then']), None)
assert refresh is not None, 'Minute-boundary time_update does not refresh the screensaver'
assert 'clock_screensaver_update_time' in str(scripts['show_clock_view'])
assert 'clock_label' not in str(backlight['interval']), 'Clock rendering must not depend on a periodic interval'
assert 'clock_screensaver_update_time' not in str(backlight['interval'])

# Compile the actual rendering lambda and mode guard, with only hardware I/O stubbed.
format_header = (root / 'components/espcontrol/clock_bar.h').read_text()
formatter = format_header[format_header.index('inline void format_clock_time_without_suffix'):format_header.index('inline void format_fixed_decimal')]
time_header = (root / 'components/espcontrol/sun_calc.h').read_text()
fallback = time_header[time_header.index('template <typename TimeT>'):time_header.index('inline std::string trim_ntp_server')]
source = r'''
#include "display_mode_controller.h"
#include <cassert>
#include <cstdio>
#include <string>
using espcontrol::DisplayMode;
struct Time { int hour = 12, minute = 59; bool valid = true;
  bool is_valid() const { return valid; } };
struct Clock { Time value; Time now() const { return value; } } panel_time, homeassistant_time;
struct Widget { std::string text; int writes = 0; } label, overlay;
auto *clock_label = &label;
auto *clock_screensaver = &overlay;
struct Setting { std::string state = "FFFFFF"; } schedule_clock_text_color;
bool clock_format_12h = false;
int drift_minute = -1;
void lv_label_set_text(Widget *widget, const char *text) { widget->text = text; ++widget->writes; }
void apply_clock_screensaver_text_color(Widget *, const std::string &) {}
void position_clock_screensaver_label(Widget *, Widget *, int minute) { drift_minute = minute; }
struct App {
  DisplayMode mode = DisplayMode::CLOCK;
  App &display() { return *this; }
  bool current_mode_is(DisplayMode expected) const { return mode == expected; }
} espcontrol_app;
#define id(x) x
''' + formatter + fallback + '\nvoid render() {\n' + scripts['clock_screensaver_update_time']['then'][0]['lambda'] + '\n}\n' + r'''
void time_update() {
  const bool visible = [] {
''' + refresh['condition']['lambda'] + r'''
  }();
  if (visible) render();
}
int main() {
  render(); // Entry refreshes immediately, before the next minute callback.
  assert(label.text == "12:59" && drift_minute == 59);
  panel_time.value = {13, 0, true};
  time_update(); // :00 arrives before the unrelated :10/:40 interval ticks.
  assert(label.text == "13:00" && drift_minute == 0);
  clock_format_12h = true;
  time_update();
  assert(label.text == "1:00");
  panel_time.value = {0, 0, true};
  time_update();
  assert(label.text == "12:00");
  clock_format_12h = false;
  time_update();
  assert(label.text == "00:00");
  panel_time.value.valid = false;
  homeassistant_time.value = {7, 1, true};
  time_update();
  assert(label.text == "07:01" && drift_minute == 1);
  homeassistant_time.value.valid = false;
  const int writes = label.writes;
  time_update();
  assert(label.writes == writes); // Invalid time does not replace the last valid clock.
  panel_time.value = {8, 2, true};
  for (auto mode : {DisplayMode::ACTIVE, DisplayMode::COVER_ART, DisplayMode::CAMERA,
                    DisplayMode::DIMMED, DisplayMode::SETUP_DIMMED, DisplayMode::DISPLAY_OFF}) {
    espcontrol_app.mode = mode;
    time_update();
    assert(label.writes == writes); // Do not redraw the hidden screensaver.
  }
  espcontrol_app.mode = DisplayMode::CLOCK;
  time_update();
  assert(label.text == "08:02" && drift_minute == 2);
}
'''
with tempfile.TemporaryDirectory(prefix='clock-screensaver-time-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(source)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'components/espcontrol'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Screensaver minute-boundary refresh, entry, format, fallback and visibility checks passed.')
