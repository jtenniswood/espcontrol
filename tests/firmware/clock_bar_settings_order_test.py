#!/usr/bin/env python3
"""Exercise production foreground coordination with LVGL-style recursive child-change events."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
runtime = (root / 'components/espcontrol/editable_clock_bar.h').read_text()
clock = (root / 'components/espcontrol/clock_bar.h').read_text()

def function(source, name):
    start = source.index(name)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

program = r'''
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
struct lv_event_t;
struct lv_obj_t { bool hidden = false; };
using Callback = void (*)(lv_event_t *);
struct lv_event_t { lv_obj_t *target; };
constexpr int LV_EVENT_CHILD_CHANGED = 1, LV_OBJ_FLAG_HIDDEN = 1;
std::vector<Callback> callbacks;
std::vector<lv_obj_t *> children;
lv_obj_t layer;
int depth = 0, max_depth = 0, moves = 0;
void dispatch() {
  ++depth; max_depth = std::max(max_depth, depth); assert(depth < 6);
  lv_event_t event{&layer};
  for (auto callback : callbacks) callback(&event);
  --depth;
}
void lv_obj_add_event_cb(lv_obj_t *, Callback callback, int, void *) { callbacks.push_back(callback); }
void *lv_event_get_target(lv_event_t *event) { return event->target; }
bool lv_obj_has_flag(lv_obj_t *object, int) { return object->hidden; }
void lv_obj_move_foreground(lv_obj_t *object) {
  assert(object);
  if (!children.empty() && children.back() == object) return;
  auto found = std::find(children.begin(), children.end(), object);
  assert(found != children.end());
  children.erase(found); children.push_back(object); ++moves;
  dispatch();
}
struct Config { std::string type; };
struct Widget { Config config; lv_obj_t *box; };
struct Runtime {
 bool active = false, rebuilding = false, raising_settings = false;
 lv_obj_t *settings_layer = nullptr;
 std::vector<Widget> widgets;
};
Runtime &runtime() { static Runtime state; return state; }
'''
program += function(clock, 'inline void clock_bar_raise_settings_button(') + '\n'
program += function(runtime, 'inline void raise_settings()') + '\n'
program += function(runtime, 'inline void register_settings_access(') + '\n'
program += r'''
int main() {
  auto &state = runtime();
  lv_obj_t first, second, sensor, modal;
  state.active = true;
  state.widgets = {{{"network"}, &first}, {{"humidity"}, &sensor}, {{"network"}, &second}};
  children = {&first, &sensor, &second, &modal};
  for (int i = 0; i < 20; ++i) register_settings_access(&layer);
  assert(callbacks.size() == 1);
  dispatch();
  assert(children[children.size()-2] == &first && children.back() == &second);
  assert(max_depth <= 2);
  for (int i = 0; i < 100; ++i) raise_settings();
  assert(max_depth <= 2 && callbacks.size() == 1);
  second.hidden = true;
  lv_obj_move_foreground(&modal);
  assert(children.back() == &first);
  state.rebuilding = true;
  state.widgets = {{{"network"}, nullptr}};
  const int before = moves;
  dispatch();
  assert(moves == before);
  state.rebuilding = false;
  state.widgets = {{{"network"}, &second}}; second.hidden = false;
  dispatch(); assert(children.back() == &second && callbacks.size() == 1);
  state.active = false;
  const int inactive_moves = moves;
  dispatch(); assert(moves == inactive_moves);
}
'''
with tempfile.TemporaryDirectory(prefix='clock-bar-settings-') as directory:
    source = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    source.write_text(program)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Duplicate settings shortcuts use one bounded coordinator and remain safe during rebuilds.')
