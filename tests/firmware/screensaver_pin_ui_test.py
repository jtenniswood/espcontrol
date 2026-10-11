#!/usr/bin/env python3
"""Exercise production PIN rendering and event handling with modeled LVGL I/O."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ui = (ROOT / 'components/espcontrol/screensaver_pin_ui.h').read_text()
ui = ui[ui.index('struct ScreensaverPinUi'):ui.index('inline std::function<void()>')]
stubs = r'''
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include "screensaver_pin.h"
struct lv_font_t {};
struct lv_obj_t { lv_obj_t *parent{}; bool full_screen{}; unsigned flags{}; std::string text; };
struct lv_event_t { const char *key; };
struct ControlModalLayout {};
constexpr unsigned LV_OBJ_FLAG_CLICKABLE = 1;
lv_obj_t system_layer;
int deleted = 0;
std::vector<lv_obj_t *> objects;
std::vector<void (*)(void *)> deferred;
bool rendered_randomize = true;
lv_obj_t *lv_layer_sys() { return &system_layer; }
lv_obj_t *lv_obj_create(lv_obj_t *parent) {
  auto *obj = new lv_obj_t; obj->parent = parent; objects.push_back(obj); return obj;
}
void lv_obj_del(lv_obj_t *root) {
  for (auto *obj : objects) delete obj;
  objects.clear(); (void) root; ++deleted;
}
void lv_obj_add_flag(lv_obj_t *obj, unsigned flag) { obj->flags |= flag; }
void control_modal_style_overlay(lv_obj_t *obj) { obj->full_screen = true; }
ControlModalLayout control_modal_calc_layout(int) { return {}; }
int control_modal_card_radius(lv_obj_t *) { return 0; }
void control_modal_style_panel(lv_obj_t *, int) {}
void control_modal_apply_panel_layout(lv_obj_t *, lv_obj_t *, const ControlModalLayout &, int) {}
void *lv_event_get_user_data(lv_event_t *event) { return const_cast<char *>(event->key); }
const char *espcontrol_i18n(const char *text) { return text; }
void lv_label_set_display_text(lv_obj_t *obj, const char *text) { obj->text = text; }
void lv_async_call(void (*callback)(void *), void *) { deferred.push_back(callback); }
lv_obj_t *pin_keypad_create(lv_obj_t *panel, const ControlModalLayout &, const lv_font_t *,
                           const lv_font_t *, const lv_font_t *, int, void (*)(lv_event_t *), bool randomize) {
  rendered_randomize = randomize;
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
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, true);
  auto &ui = screensaver_pin_ui();
  assert(ui.overlay && ui.overlay->parent == &system_layer);
  assert(rendered_randomize);
  assert(ui.overlay->full_screen && (ui.overlay->flags & LV_OBJ_FLAG_CLICKABLE));
  // A press on a card/status/modal behind this full-screen system-layer object
  // is intercepted before LVGL searches the top layer and active page.
  lv_event_t zero{"0"};
  screensaver_pin_key_cb(&zero);
  assert(ui.entry.empty());  // Original wake tap does not become the first digit.
  screensaver_pin_refresh(true, nullptr, nullptr, nullptr, false);
  screensaver_pin_key_cb(&zero);
  assert(ui.entry == "0" && ui.label->text == "*");
  lv_event_t submit{"submit"};
  screensaver_pin_key_cb(&submit);
  assert(pin.locked() && ui.overlay && ui.entry.empty());
  assert(ui.label->text == "Incorrect PIN");
  screensaver_pin_key_cb(&zero);
  lv_event_t clear{"back"};
  screensaver_pin_key_cb(&clear);
  assert(ui.entry.empty() && ui.label->text == "Enter Pin");
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
}
'''
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    source.write_text(stubs + ui + checks)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'c++', '-std=c++17', '-pthread',
                    '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'components/espcontrol'),
                    '-I', str(ROOT / 'tests/firmware/screensaver_pin_stubs'), str(source),
                    str(ROOT / 'components/espcontrol/screensaver_pin.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PIN system-layer blocking, wake-tap guard, masking, rejection and deferred unlock passed')
