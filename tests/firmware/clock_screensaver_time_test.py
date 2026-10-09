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
date_header = (root / 'components/espcontrol/button_grid_datetime_cards.h').read_text()
month_formatter = date_header[date_header.index('inline const char *calendar_month_name'):date_header.index('inline void apply_calendar_card_text(const CalendarCardRef &ref,\n                                     const CalendarDateState &state) {')]
source = r'''
#include "display_mode_controller.h"
#include <cassert>
#include <cstdio>
#include <string>
using espcontrol::DisplayMode;
struct Time { int hour = 12, minute = 59; bool valid = true; int day_of_month = 9, month = 10, year = 2026;
  bool is_valid() const { return valid; } };
struct Clock { Time value; Time now() const { return value; } } panel_time, homeassistant_time;
struct Widget { std::string text; int writes = 0; bool hidden = true; } label, overlay, date;
auto *clock_label = &label;
auto *clock_screensaver = &overlay;
auto *clock_date_label = &date;
struct Switch { bool state = false; } clock_screensaver_show_date;
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
void lv_obj_clear_flag(Widget *widget, int) { widget->hidden = false; }
void lv_obj_add_flag(Widget *widget, int) { widget->hidden = true; }
const char *espcontrol_i18n(const char *text) { return text; }
struct Setting { std::string state = "FFFFFF"; } schedule_clock_text_color;
bool clock_format_12h = false;
int drift_minute = -1;
void lv_label_set_text(Widget *widget, const char *text) { widget->text = text; ++widget->writes; }
void lv_label_set_display_text(Widget *widget, const char *text) { lv_label_set_text(widget, text); }
void apply_clock_screensaver_text_color(Widget *, const std::string &) {}
void position_clock_screensaver_label(Widget *, Widget *, int minute, Widget *) { drift_minute = minute; }
struct App {
  DisplayMode mode = DisplayMode::CLOCK;
  App &display() { return *this; }
  bool current_mode_is(DisplayMode expected) const { return mode == expected; }
} espcontrol_app;
#define id(x) x
''' + formatter + month_formatter + fallback + '\nvoid render() {\n' + scripts['clock_screensaver_update_time']['then'][0]['lambda'] + '\n}\n' + r'''
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
  assert(label.text == "00:00" && date.hidden);
  clock_screensaver_show_date.state = true;
  panel_time.value.day_of_month = 31;
  panel_time.value.month = 12;
  time_update();
  assert(!date.hidden && date.text == "31 December 2026");
  panel_time.value = {0, 0, true, 1, 1, 2027};
  time_update();
  assert(date.text == "1 January 2027");
  clock_screensaver_show_date.state = false;
  time_update();
  assert(date.hidden);
  clock_screensaver_show_date.state = true;
  panel_time.value.valid = false;
  homeassistant_time.value = {7, 1, true};
  time_update();
  assert(label.text == "07:01" && drift_minute == 1);
  assert(date.text == "9 October 2026" && !date.hidden);
  homeassistant_time.value.valid = false;
  const int writes = label.writes;
  time_update();
  assert(date.hidden);
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

# Exercise the production positioning helper across panel sizes and rotations.
layout_header = (root / 'components/espcontrol/backlight.h').read_text()
layout_helper = layout_header[layout_header.index('inline void position_clock_screensaver_label'):layout_header.index('inline void position_clock_image_overlay')]
layout_source = r'''
#include <algorithm>
#include <cassert>
using lv_coord_t = int;
struct Widget { int w, h, x = 0, y = 0; Widget *parent = nullptr; };
using lv_obj_t = Widget;
using lv_disp_t = int;
constexpr int LV_SIZE_CONTENT = -1;
void screensaver_fill_screen(Widget *) {}
void lv_obj_update_layout(Widget *) {}
Widget *lv_obj_get_parent(Widget *w) { return w->parent; }
int lv_obj_get_width(Widget *w) { return w->w; }
int lv_obj_get_height(Widget *w) { return w->h; }
lv_disp_t *lv_disp_get_default() { return nullptr; }
int lv_disp_get_hor_res(lv_disp_t *) { return 480; }
int lv_disp_get_ver_res(lv_disp_t *) { return 480; }
void lv_obj_set_pos(Widget *w, int x, int y) { w->x = x; w->y = y; }
void lv_obj_set_width(Widget *w, int width) { if (width != LV_SIZE_CONTENT) w->w = width; }
''' + layout_helper + r'''
int main() {
  for (auto dimensions : {std::pair<int, int>{480, 480}, {800, 480}, {480, 800},
                          {1280, 800}, {800, 1280}, {720, 720}}) {
    Widget overlay{dimensions.first, dimensions.second};
    for (int clock_width : {240, 300}) {
      Widget clock{clock_width, 170};
      Widget date{260, 28};
      for (int minute = 0; minute < 60; ++minute) {
        position_clock_screensaver_label(&overlay, &clock, minute, &date);
        assert(std::abs((2 * clock.x + clock.w) - (2 * date.x + date.w)) <= 1);
        assert(date.y > clock.y + clock.h);
        assert(clock.x >= 0 && date.x >= 0 && clock.y >= 0);
        assert(clock.x + clock.w <= overlay.w && date.x + date.w <= overlay.w);
        assert(date.y + date.h <= overlay.h);
        position_clock_screensaver_label(&overlay, &clock, minute);
        assert(clock.x == overlay.w / 2 + (minute * 7) % 61 - 30 - clock.w / 2);
        assert(clock.y == overlay.h / 2 + (minute * 13) % 41 - 20 - clock.h / 2);
      }
    }
  }
}
'''
with tempfile.TemporaryDirectory(prefix='clock-screensaver-layout-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(layout_source)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Clock/date alignment, spacing, drift and screen bounds checks passed.')
