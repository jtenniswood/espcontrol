#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

struct ScreenLockCardRef {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  const char *locked_icon = nullptr;
  const char *unlocked_icon = nullptr;
};

inline bool &screen_lock_enabled() {
  static bool locked = false;
  return locked;
}

inline bool &screensaver_pin_locked() {
  static bool locked = false;
  return locked;
}

inline std::string &screensaver_pin_unlock_code() {
  static std::string code;
  return code;
}

inline std::string normalize_numeric_pin(const std::string &value) {
  std::string normalized;
  normalized.reserve(value.size());
  for (char ch : value) {
    if (ch >= '0' && ch <= '9') normalized.push_back(ch);
  }
  return normalized;
}

using ScreensaverActivateCallback = void (*)();

inline ScreensaverActivateCallback &screensaver_activate_callback() {
  static ScreensaverActivateCallback callback = nullptr;
  return callback;
}

inline void set_screensaver_activate_callback(ScreensaverActivateCallback callback) {
  screensaver_activate_callback() = callback;
}

inline void activate_configured_screensaver() {
  ScreensaverActivateCallback callback = screensaver_activate_callback();
  if (callback) callback();
}

inline bool screen_interaction_locked() {
  return screen_lock_enabled() || screensaver_pin_locked();
}

inline std::vector<lv_obj_t *> &screen_lock_controlled_buttons() {
  static std::vector<lv_obj_t *> buttons;
  return buttons;
}

inline std::vector<ScreenLockCardRef> &screen_lock_card_refs() {
  static std::vector<ScreenLockCardRef> refs;
  return refs;
}

inline std::vector<lv_obj_t *> &screen_lock_clickable_objects() {
  static std::vector<lv_obj_t *> objects;
  return objects;
}

inline void screen_lock_reset_registry() {
  screen_lock_controlled_buttons().clear();
  screen_lock_card_refs().clear();
  screen_lock_clickable_objects().clear();
}

inline bool screen_lock_button_is_lock_card(lv_obj_t *btn) {
  for (const auto &ref : screen_lock_card_refs()) {
    if (ref.btn == btn) return true;
  }
  return false;
}

inline void screen_lock_register_controlled_button(lv_obj_t *btn) {
  if (!btn) return;
  auto &buttons = screen_lock_controlled_buttons();
  if (std::find(buttons.begin(), buttons.end(), btn) == buttons.end()) {
    buttons.push_back(btn);
  }
}

inline void screen_lock_register_card(const BtnSlot &s, const ParsedCfg &p);

inline void screen_lock_clear_clickable_tree(lv_obj_t *obj) {
  if (!obj) return;
  auto &clickable = screen_lock_clickable_objects();
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    if (std::find(clickable.begin(), clickable.end(), obj) == clickable.end()) {
      clickable.push_back(obj);
    }
  }
  int32_t child_count = static_cast<int32_t>(lv_obj_get_child_cnt(obj));
  for (int32_t i = 0; i < child_count; i++) {
    screen_lock_clear_clickable_tree(lv_obj_get_child(obj, i));
  }
}

inline void screen_lock_apply() {
  bool card_locked = screen_lock_enabled();
  if (screen_lock_card_refs().empty()) {
    card_locked = false;
    screen_lock_enabled() = false;
  }
  const bool pin_locked = screensaver_pin_locked();
  const bool interaction_locked = card_locked || pin_locked;

  auto &clickable = screen_lock_clickable_objects();
  for (lv_obj_t *btn : screen_lock_controlled_buttons()) {
    if (!btn) continue;
    if (screen_lock_button_is_lock_card(btn) && !pin_locked) continue;
    if (interaction_locked) {
      screen_lock_clear_clickable_tree(btn);
    }
  }
  if (!interaction_locked) {
    for (lv_obj_t *obj : clickable) {
      if (obj) lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    clickable.clear();
  }

  for (const auto &ref : screen_lock_card_refs()) {
    if (!ref.btn) continue;
    set_card_checked_state(ref.btn, card_locked);
    if (pin_locked) screen_lock_clear_clickable_tree(ref.btn);
    else lv_obj_add_flag(ref.btn, LV_OBJ_FLAG_CLICKABLE);
    if (ref.icon_lbl) {
      const char *icon = card_locked ? ref.locked_icon : ref.unlocked_icon;
      lv_label_set_text(ref.icon_lbl, icon ? icon : "");
    }
    if (ref.text_lbl) {
      lv_label_set_text(ref.text_lbl,
        card_locked ? espcontrol_i18n("Screen Locked") : espcontrol_i18n("Screen Unlocked"));
    }
  }
}

inline void screen_lock_set_enabled(bool locked) {
  screen_lock_enabled() = locked;
  screen_lock_apply();
}

inline void screen_lock_toggle() {
  screen_lock_set_enabled(!screen_lock_enabled());
}
