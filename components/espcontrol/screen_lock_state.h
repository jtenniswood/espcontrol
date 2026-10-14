#pragma once

#include "display_text.h"
#include "screensaver_pin.h"
#include <algorithm>
#include <functional>
#include <vector>

// Screen Lock cards display the shared touchscreen PIN lock state. Touch
// interception belongs to the PIN overlay; never disable the card/widget tree.
struct ScreenLockCardRef {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  const char *locked_icon = nullptr;
  const char *unlocked_icon = nullptr;
  bool initialized = false;
  bool rendered_locked = false;
};

inline bool screen_lock_enabled() {
  return espcontrol::screensaver_pin().locked();
}

inline std::vector<ScreenLockCardRef> &screen_lock_card_refs() {
  static std::vector<ScreenLockCardRef> refs;
  return refs;
}

inline void screen_lock_reset_registry() {
  screen_lock_card_refs().clear();
}

inline void screen_lock_apply();

inline bool screen_lock_object_is_in_tree(lv_obj_t *obj, lv_obj_t *root) {
  for (lv_obj_t *current = obj; current != nullptr;
       current = lv_obj_get_parent(current)) {
    if (current == root) return true;
  }
  return false;
}

inline void screen_lock_unregister_tree(lv_obj_t *root) {
  if (!root) return;
  auto &refs = screen_lock_card_refs();
  refs.erase(std::remove_if(refs.begin(), refs.end(),
    [root](const ScreenLockCardRef &ref) { return screen_lock_object_is_in_tree(ref.btn, root); }),
    refs.end());
}

inline void screen_lock_register_card(const BtnSlot &s, const ParsedCfg &p);

inline void screen_lock_apply() {
  const bool locked = screen_lock_enabled();
  for (auto &ref : screen_lock_card_refs()) {
    if (!ref.btn || (ref.initialized && ref.rendered_locked == locked)) continue;
    ref.initialized = true;
    ref.rendered_locked = locked;
    set_card_checked_state(ref.btn, locked);
    if (ref.icon_lbl) {
      const char *icon = locked ? ref.locked_icon : ref.unlocked_icon;
      lv_label_set_display_text(ref.icon_lbl, icon ? icon : "");
    }
    if (ref.text_lbl) {
      lv_label_set_display_text(ref.text_lbl,
        locked ? espcontrol_i18n("Screen Locked") : espcontrol_i18n("Screen Unlocked"));
    }
  }
}

inline std::function<void()> &show_screen_lock_pin() {
  static std::function<void()> action;
  return action;
}

inline std::function<void()> &force_screensaver_action() {
  static std::function<void()> action;
  return action;
}

inline void screen_lock_activate(bool screensaver_on_lock = false) {
  if (!espcontrol::screensaver_pin().lock(true)) return;
  screen_lock_apply();
  if (show_screen_lock_pin()) show_screen_lock_pin()();
  if (screensaver_on_lock && force_screensaver_action()) force_screensaver_action()();
}
