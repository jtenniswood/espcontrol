#!/usr/bin/env python3
"""Exercise production PIN rendering and event handling with modeled LVGL I/O."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ui = (ROOT / 'components/espcontrol/screensaver_pin_ui.h').read_text()
ui = ui[ui.index('struct ScreensaverPinUi'):]
lock = (ROOT / 'components/espcontrol/screen_lock_state.h').read_text()
lock = lock[lock.index('struct ScreenLockCardRef'):]
navigation = (ROOT / 'components/espcontrol/button_grid_navigation.h').read_text()
navigation = navigation[navigation.index('inline bool navigation_return_home'):navigation.index('inline void navigation_clear_home_targets')]
stubs = r'''
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include "screensaver_pin.h"
using lv_coord_t = int;
struct lv_font_t {};
struct lv_event_t { const char *key; };
struct lv_obj_t { lv_obj_t *parent{}; bool full_screen{}; unsigned flags{}; std::string text; void (*on_release)(lv_event_t *){}; const lv_font_t *font{}; int align{}, text_align{}, x{}, y{}; };
struct ControlModalLayout { int panel_w{720}; int inset{18}; };
constexpr unsigned LV_OBJ_FLAG_CLICKABLE = 1;
constexpr unsigned LV_EVENT_RELEASED = 2;
constexpr int LV_TEXT_ALIGN_RIGHT = 3, LV_ALIGN_TOP_RIGHT = 4, LV_PART_MAIN = 0;
lv_obj_t system_layer;
lv_obj_t *active_page;
constexpr int LV_SCR_LOAD_ANIM_NONE = 0;
#define ESP_LOGW(...) ((void)0)
void navigation_hide_modals() {}
lv_obj_t *lv_scr_act() { return active_page; }
void lv_scr_load_anim(lv_obj_t *page, int, int, int, bool) { active_page = page; }
int deleted = 0;
std::vector<lv_obj_t *> objects;
std::vector<void (*)(void *)> deferred;
bool rendered_randomize = true;
bool rendered_center_prompt = false;
int rendered_prompt_top = 0;
lv_obj_t *rendered_title = nullptr;
lv_obj_t *lv_layer_sys() { return &system_layer; }
lv_obj_t *lv_obj_create(lv_obj_t *parent) {
  auto *obj = new lv_obj_t; obj->parent = parent; objects.push_back(obj); return obj;
}
void lv_obj_del(lv_obj_t *root) {
  for (auto *obj : objects) delete obj;
  objects.clear(); (void) root; ++deleted;
}
void lv_obj_add_flag(lv_obj_t *obj, unsigned flag) { obj->flags |= flag; }
void lv_obj_add_event_cb(lv_obj_t *obj, void (*callback)(lv_event_t *), unsigned code, void *) {
  assert(code == LV_EVENT_RELEASED); obj->on_release = callback;
}
void control_modal_style_overlay(lv_obj_t *obj) { obj->full_screen = true; }
ControlModalLayout control_modal_calc_layout(int) { return {}; }
int control_modal_card_radius(lv_obj_t *) { return 0; }
void control_modal_style_panel(lv_obj_t *, int) {}
void control_modal_apply_panel_layout(lv_obj_t *, lv_obj_t *, const ControlModalLayout &, int) {}
lv_obj_t *control_modal_create_title(lv_obj_t *parent, const std::string &text, int,
                                   const lv_font_t *font, int) {
  rendered_title = lv_obj_create(parent);
  rendered_title->text = text; rendered_title->font = font;
  return rendered_title;
}
void lv_obj_set_style_text_align(lv_obj_t *obj, int align, int) { obj->text_align = align; }
void lv_obj_align(lv_obj_t *obj, int align, int x, int y) { obj->align = align; obj->x = x; obj->y = y; }
void lv_obj_update_layout(lv_obj_t *) {}
int lv_obj_get_height(lv_obj_t *) { return 30; }
void *lv_event_get_user_data(lv_event_t *event) { return const_cast<char *>(event->key); }
const char *espcontrol_i18n(const char *text) { return text; }
struct BtnSlot {};
struct ParsedCfg {};
lv_obj_t *lv_obj_get_parent(lv_obj_t *obj) { return obj->parent; }
void set_card_checked_state(lv_obj_t *, bool) {}
void lv_label_set_display_text(lv_obj_t *obj, const char *text) { obj->text = text; }
void lv_async_call(void (*callback)(void *), void *) { deferred.push_back(callback); }
lv_obj_t *pin_keypad_create(lv_obj_t *panel, const ControlModalLayout &, const lv_font_t *,
                           const lv_font_t *, const lv_font_t *, int, void (*)(lv_event_t *), bool randomize,
                           bool center_prompt, int prompt_top) {
  rendered_randomize = randomize;
  rendered_center_prompt = center_prompt;
  rendered_prompt_top = prompt_top;
  return lv_obj_create(panel);
}
'''
checks = r'''
int main() {
  auto &pin = espcontrol::screensaver_pin();
  pin.setup();
  std::string code = "04";
  assert(pin.save(true, &code));
  pin.arm();
  lv_font_t header_font;
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, true, &header_font);
  auto &ui = screensaver_pin_ui();
  assert(ui.overlay && ui.overlay->parent == &system_layer);
  assert(!rendered_randomize);
  assert(rendered_center_prompt);
  assert(rendered_title && rendered_title->text == "Screen Lock" && rendered_title->font == &header_font);
  assert(rendered_title->align == LV_ALIGN_TOP_RIGHT && rendered_title->text_align == LV_TEXT_ALIGN_RIGHT);
  assert(rendered_title->x < 0 && rendered_title->y > 0);
  assert(rendered_prompt_top > rendered_title->y + lv_obj_get_height(rendered_title));
  assert(ui.overlay->full_screen && (ui.overlay->flags & LV_OBJ_FLAG_CLICKABLE));
  // A press on a card/status/modal behind this full-screen system-layer object
  // is intercepted before LVGL searches the top layer and active page.
  lv_event_t zero{"0"};
  screensaver_pin_key_cb(&zero);
  assert(ui.entry.empty());  // Original wake tap does not become the first digit.
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  screensaver_pin_key_cb(&zero);
  assert(ui.entry == "0" && ui.label->text == "\u2022");
  lv_event_t submit{"submit"};
  screensaver_pin_key_cb(&submit);
  assert(pin.locked() && ui.overlay && ui.entry.empty());
  assert(ui.label->text == "Incorrect PIN");
  screensaver_pin_key_cb(&zero);
  lv_event_t clear{"back"};
  screensaver_pin_key_cb(&clear);
  assert(ui.entry.empty() && ui.label->text == "Enter Pin");
  assert(deferred.empty() && ui.label); // X with digits only clears the entry.
  screensaver_pin_key_cb(&zero);
  lv_event_t four{"4"};
  screensaver_pin_key_cb(&four);
  screensaver_pin_key_cb(&submit);
  assert(!pin.locked() && ui.overlay && deferred.size() == 1 && deleted == 0);
  deferred.front()(nullptr); deferred.clear();
  assert(!ui.overlay && deleted == 1);
  pin.arm();
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  screensaver_pin_refresh(false, nullptr, nullptr, nullptr, false);
  assert(pin.locked() && !ui.overlay); // Sleep hides the entry without unlocking.
  bool randomize = false;
  assert(pin.save(true, nullptr, &randomize));
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  assert(ui.overlay && ui.entry.empty());
  assert(!rendered_randomize);
  assert(pin.save(false, nullptr));
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  assert(!ui.overlay);
  // A manual card lock preserves the page and intercepts every gesture with a
  // transparent guard; its opening touch must not enter a keypad digit.
  lv_obj_t card, icon, label, other_card;
  card.flags = other_card.flags = LV_OBJ_FLAG_CLICKABLE;
  screen_lock_card_refs().push_back({&card, &icon, &label, "locked", "unlocked"});
  show_screen_lock_pin() = [] { screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false); };
  espcontrol::DisplayModeController controller;
  return_screen_lock_view() = [&] {
    if (screensaver_pin_restore_previous_mode(controller)) {
      pin.arm(pin.return_mode());
      screensaver_pin_refresh(false, nullptr, nullptr, nullptr, false);
    } else screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  };
  screen_lock_activate();
  assert(ui.overlay && !ui.label && screen_lock_enabled() && label.text == "Screen Locked");
  assert(card.flags == LV_OBJ_FLAG_CLICKABLE && other_card.flags == LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t home, subpage;
  active_page = &subpage;
  auto *guard = ui.overlay;
  assert(navigation_return_home(&home)); // The unchanged idle timer's navigation path.
  assert(active_page == &home && ui.overlay == guard && !ui.label && pin.locked());
  lv_event_t released{""};
  ui.touch_blocked = true;
  ui.overlay->on_release(&released);
  assert(!pin.keypad_requested() && deferred.empty());
  ui.touch_blocked = false;
  ui.overlay->on_release(&released);
  assert(pin.keypad_requested() && !ui.label && deferred.size() == 1);
  deferred.front()(nullptr); deferred.clear();
  assert(ui.label && ui.entry.empty());
  auto *keypad = ui.overlay;
  screensaver_pin_key_cb(&clear);
  assert(pin.locked() && !pin.keypad_requested() && ui.overlay == keypad && deferred.size() == 1);
  deferred.front()(nullptr); deferred.clear();
  assert(ui.overlay && !ui.label && pin.locked()); // Empty X returns to locked cards.
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  assert(!ui.label); // Periodic refresh must not reopen the dismissed keypad.
  ui.overlay->on_release(&released);
  deferred.front()(nullptr); deferred.clear();
  assert(ui.label && ui.entry.empty());
  screensaver_pin_key_cb(&zero);
  screensaver_pin_key_cb(&submit);
  assert(screen_lock_enabled() && ui.overlay);
  pin.arm();
  screensaver_pin_refresh(false, nullptr, nullptr, nullptr, false);
  assert(screen_lock_enabled() && !ui.overlay);
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  screensaver_pin_key_cb(&zero);
  screensaver_pin_key_cb(&four);
  screensaver_pin_key_cb(&submit);
  assert(!screen_lock_enabled() && label.text == "Screen Unlocked" && icon.text == "unlocked");
  deferred.front()(nullptr); deferred.clear();
  assert(!ui.overlay && other_card.flags == LV_OBJ_FLAG_CLICKABLE);
  int screensaver_calls = 0;
  force_screensaver_action() = [&] {
    ++screensaver_calls;
    pin.arm();
    screensaver_pin_refresh(false, nullptr, nullptr, nullptr, false);
  };
  screen_lock_activate(true);
  assert(screensaver_calls == 1 && pin.locked() && !ui.overlay);
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  assert(ui.label); // Wake from the immediate screensaver uses the shared keypad.
  screensaver_pin_key_cb(&zero);
  screensaver_pin_key_cb(&four);
  screensaver_pin_key_cb(&submit);
  deferred.front()(nullptr); deferred.clear();
  assert(!pin.locked() && !ui.overlay);
  // Cancel restores the actual previous mode, rather than the current
  // configured screensaver preference. Unlocking is still required on wake.
  for (auto mode : {espcontrol::DisplayMode::CLOCK, espcontrol::DisplayMode::DIMMED,
                    espcontrol::DisplayMode::CAMERA, espcontrol::DisplayMode::DISPLAY_OFF,
                    espcontrol::DisplayMode::COVER_ART}) {
    assert(pin.lock());
    pin.arm(mode);
    controller.request(espcontrol::DisplayRequestSource::USER_WAKE, espcontrol::DisplayMode::ACTIVE);
    screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
    assert(ui.label && pin.return_mode() == mode);
    screensaver_pin_key_cb(&clear);
    assert(pin.locked() && deferred.size() == 1);
    deferred.front()(nullptr); deferred.clear();
    assert(!ui.overlay && pin.locked() && controller.target_mode() == mode);
    controller.clear(espcontrol::DisplayRequestSource::MANUAL_SLEEP);
    controller.clear(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK);
    screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
    assert(ui.label && ui.entry.empty());
    screensaver_pin_key_cb(&zero);
    screensaver_pin_key_cb(&four);
    screensaver_pin_key_cb(&submit);
    deferred.front()(nullptr); deferred.clear();
    assert(!pin.locked() && !ui.overlay);
  }
  screen_lock_activate();
  assert(pin.locked());
  screen_lock_reset_registry();
  assert(pin.locked()); // Rebuilding/removing cards must not bypass the shared lock.
  std::string empty;
  assert(pin.save(false, &empty));
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  screen_lock_activate();
  assert(!screen_lock_enabled() && !ui.overlay);
  screen_lock_activate(true);
  assert(screensaver_calls == 1); // A card without a PIN cannot lock or start sleep.
}
'''
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    source.write_text(stubs + lock + ui + navigation + checks)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'c++', '-std=c++17', '-pthread',
                    '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'components/espcontrol'),
                    '-I', str(ROOT / 'tests/firmware/screensaver_pin_stubs'), str(source),
                    str(ROOT / 'components/espcontrol/screensaver_pin.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PIN blocking, wake-tap guard, masking, rejection, deferred unlock and empty-X return views passed')
