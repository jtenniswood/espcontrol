"""Exercise production touch navigation, contrast restoration and press routing."""
from pathlib import Path
import os
import subprocess
import tempfile
from generate_media_lifecycle_integration import definition

ROOT = Path(__file__).resolve().parents[2]
HEADERS = ROOT / "components/espcontrol"


def production(file, name):
    line, code = definition((HEADERS / file).read_text(), name)
    return f'\n#line {line} "{HEADERS / file}"\n{code}\n'


source = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; void *data; };
using lv_event_cb_t = void (*)(lv_event_t *);
struct Event { lv_event_cb_t cb; int filter; void *data; };
struct lv_obj_t { bool pressed = true; int foreground = 1, label = 2; std::vector<Event> events; };
constexpr int LV_STATE_PRESSED = 1, LV_EVENT_PRESSED = 1, LV_EVENT_RELEASED = 2,
              LV_EVENT_CLICKED = 3, LV_EVENT_ALL = 4;
struct lv_indev_t { bool consumed = false; } pointer;
lv_indev_t *input = &pointer;
lv_indev_t *lv_indev_get_act() { return input; }
void lv_indev_wait_release(lv_indev_t *i) { i->consumed = true; }
void lv_obj_clear_state(lv_obj_t *b, int) { b->pressed = false; }
uint32_t lv_obj_get_event_count(lv_obj_t *b) { return b->events.size(); }
Event *lv_obj_get_event_dsc(lv_obj_t *b, unsigned i) { return &b->events[i]; }
lv_event_cb_t lv_event_dsc_get_cb(Event *e) { return e->cb; }
void *lv_event_get_target(lv_event_t *e) { return e->target; }
void *lv_event_get_user_data(lv_event_t *e) { return e->data; }
void sync_card_checked_text_color(lv_obj_t *b) { b->label = b->pressed ? 0 : b->foreground; }
void card_text_contrast_event_cb(lv_event_t *e) { sync_card_checked_text_color(e->target); }
void lv_obj_add_event_cb(lv_obj_t *b, lv_event_cb_t cb, int filter, void *data) {
  b->events.push_back({cb, filter, data});
}
void emit(lv_obj_t *b, int code) {
  for (auto event : b->events) if (event.filter == code || event.filter == LV_EVENT_ALL) {
    lv_event_t e{b, event.data}; event.cb(&e);
  }
}
'''
source += '#include "button_grid_touch.h"\n'
source += r'''
bool locked = false;
namespace espcontrol { struct Pin { bool locked() { return ::locked; } }; Pin screensaver_pin() { return {}; } }
struct ParsedCfg { std::string type, sensor, entity = "media.test"; };
ParsedCfg parse_cfg(const std::string &s) { return {s, s}; }
bool button_press_opens_modal(const ParsedCfg &p, lv_obj_t *) { return p.type == "modal"; }
bool media_fast_press_mode(const std::string &s) { return s == "next" || s == "previous"; }
std::string media_card_mode(const std::string &s) { return s; }
bool slots[26] = {};
bool *media_fast_press_slots() { return slots; }
constexpr int MAX_GRID_SLOTS = 25;
int opened = 0, sent = 0;
void send_media_playback_action(const std::string &, const std::string &) { ++sent; }
void handle_button_click(const std::string &, int, lv_obj_t *b) {
  assert(pointer.consumed && !b->pressed); ++opened;
}
'''
source += production("button_grid_actions.h", "handle_button_press")
source += r'''
int main() {
  // A card's press contrast is restored before replacing its page.
  lv_obj_t card;
  lv_obj_add_event_cb(&card, card_text_contrast_event_cb, LV_EVENT_ALL, nullptr);
  int actions = 0;
  bind_navigation_button_action(&card, [](lv_event_t *e) {
    auto *b = static_cast<lv_obj_t *>(lv_event_get_target(e));
    assert(pointer.consumed && !b->pressed && b->label == b->foreground);
    ++*static_cast<int *>(lv_event_get_user_data(e));
  }, &actions);
  emit(&card, LV_EVENT_PRESSED);
  assert(actions == 1);
  // Even an explicit release/click cannot call the press action again.
  emit(&card, LV_EVENT_RELEASED); emit(&card, LV_EVENT_CLICKED);
  assert(actions == 1);
  // A fixed modal label keeps its own light-theme foreground.
  lv_obj_t modal;
  consume_navigation_touch(&modal);
  assert(modal.label == 2);
  // Consume before deletion; no helper accesses the control after the action.
  auto *closing = new lv_obj_t;
  bind_navigation_button_action(closing, [](lv_event_t *e) {
    assert(pointer.consumed); delete e->target;
  }, nullptr);
  auto callbacks = closing->events;
  for (auto event : callbacks) { lv_event_t e{closing, event.data}; event.cb(&e); }
  for (const auto &route : {"subpage", "modal"}) {
    pointer.consumed = false; card.pressed = true;
    const int before = opened;
    handle_button_press(route, 1, &card);
    assert(opened == before + 1 && pointer.consumed);
  }
  pointer.consumed = false; card.pressed = true;
  const int before = opened;
  for (const auto &route : {"toggle", "slider", "action"}) handle_button_press(route, 1, &card);
  assert(opened == before && !pointer.consumed && card.pressed);
  locked = true; handle_button_press("subpage", 1, &card); locked = false;
  handle_button_press("subpage", 0, &card);
  handle_button_press("subpage", 26, &card);
  assert(opened == before && !pointer.consumed);
  input = nullptr; consume_navigation_touch(&card); // keyboard/non-pointer activation
  assert(!card.pressed);
}
'''

# Verify the real Back/Close registrations use the tested shared handler.
for file, marker in (("button_grid_grid.h", "bind_navigation_button_action(back_btn"),
                     ("button_grid_alarm.h", "bind_navigation_button_action(back_btn"),
                     ("button_grid_modal.h", "bind_navigation_button_action(shell.close_btn")):
    assert marker in (HEADERS / file).read_text(), file
widget = (ROOT / "common/device/button_widget.yaml").read_text()
assert widget.index("screensaver_wake_touch_guard_active") < widget.index("handle_button_press(")
assert widget.index("DisplayMode::ACTIVE") < widget.index("handle_button_press(")

with tempfile.TemporaryDirectory(prefix="navigation-touch-") as directory:
    cpp, binary = Path(directory) / "test.cpp", Path(directory) / "test"
    cpp.write_text(source)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(HEADERS), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Touch navigation consumes held input, restores card contrast and preserves guards and ordinary actions.")
