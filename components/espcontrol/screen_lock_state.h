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
  const lv_font_t *text_font = nullptr;
  const lv_font_t *icon_font = nullptr;
};

inline bool &screen_lock_enabled() {
  static bool locked = false;
  return locked;
}

// ── Configurable lock options (populated by the last registered lock card) ──
// These live in spare ParsedCfg base fields set by the card contract:
//   sensor -> lock display mode ("screensaver" | "screen_off")
//   unit   -> unlock method     ("immediate" | "gesture" | "pin")
//   entity -> PIN digits (only meaningful when unlock method == "pin")

inline std::string &screen_lock_display_mode_ref() {
  static std::string mode = "screensaver";
  return mode;
}

inline std::string &screen_lock_unlock_method_ref() {
  static std::string method = "immediate";
  return method;
}

inline std::string &screen_lock_pin_ref() {
  static std::string pin;
  return pin;
}

inline const std::string &screen_lock_display_mode() { return screen_lock_display_mode_ref(); }
inline const std::string &screen_lock_unlock_method() { return screen_lock_unlock_method_ref(); }
inline const std::string &screen_lock_pin() { return screen_lock_pin_ref(); }

inline std::string screen_lock_normalize_display_mode(const std::string &value) {
  return value == "screen_off" ? std::string("screen_off") : std::string("screensaver");
}

inline std::string screen_lock_normalize_unlock_method(const std::string &value) {
  if (value == "gesture") return "gesture";
  if (value == "pin") return "pin";
  return "immediate";
}

inline void screen_lock_set_options(const std::string &display_mode,
                                    const std::string &unlock_method,
                                    const std::string &pin) {
  screen_lock_display_mode_ref() = screen_lock_normalize_display_mode(display_mode);
  screen_lock_unlock_method_ref() = screen_lock_normalize_unlock_method(unlock_method);
  screen_lock_pin_ref() = pin;
}

inline void screen_lock_reset_options() {
  screen_lock_display_mode_ref() = "screensaver";
  screen_lock_unlock_method_ref() = "immediate";
  screen_lock_pin_ref().clear();
}

// ── Display takeover callback (screensaver / screen-off / wake) ───────────
// Mirrors backlight_display_takeover_callback: C++ raises the intent, a YAML
// lambda runs the existing screensaver/backlight scripts.
enum class ScreenLockDisplayAction { SCREENSAVER, SCREEN_OFF, WAKE };
using ScreenLockDisplayCallback = void (*)(ScreenLockDisplayAction action);

inline ScreenLockDisplayCallback &screen_lock_display_callback() {
  static ScreenLockDisplayCallback callback = nullptr;
  return callback;
}

inline void set_screen_lock_display_callback(ScreenLockDisplayCallback callback) {
  screen_lock_display_callback() = callback;
}

inline void screen_lock_invoke_display(ScreenLockDisplayAction action) {
  ScreenLockDisplayCallback callback = screen_lock_display_callback();
  if (callback) callback(action);
}

// ── Publish callback (keeps HA-exposed entities in sync with device state) ──
using ScreenLockPublishCallback = void (*)(bool locked);

inline ScreenLockPublishCallback &screen_lock_publish_callback() {
  static ScreenLockPublishCallback callback = nullptr;
  return callback;
}

inline void set_screen_lock_publish_callback(ScreenLockPublishCallback callback) {
  screen_lock_publish_callback() = callback;
}

inline bool &screen_lock_published_state() {
  static bool state = false;
  return state;
}

inline bool &screen_lock_published_valid() {
  static bool valid = false;
  return valid;
}

// Defined in button_grid_screen_lock.h once the modal helpers are available.
inline void screen_lock_handle_tap(lv_obj_t *btn);
// Dismisses an open slide/PIN unlock modal (no-op if none). Defined alongside
// the modals; declared here so the state transitions below can call it.
inline void screen_lock_close_unlock_modal();

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
  // A config refresh may remove/retype the lock card while its unlock modal is
  // open; tear it down first so it can't survive on the top layer covering the
  // rebuilt grid (and leave dangling static UI pointers).
  screen_lock_close_unlock_modal();
  screen_lock_controlled_buttons().clear();
  screen_lock_card_refs().clear();
  screen_lock_clickable_objects().clear();
  screen_lock_reset_options();
}

inline bool screen_lock_button_is_lock_card(lv_obj_t *btn) {
  for (const auto &ref : screen_lock_card_refs()) {
    if (ref.btn == btn) return true;
  }
  return false;
}

inline const ScreenLockCardRef *screen_lock_card_ref_for(lv_obj_t *btn) {
  for (const auto &ref : screen_lock_card_refs()) {
    if (ref.btn == btn) return &ref;
  }
  return screen_lock_card_refs().empty() ? nullptr : &screen_lock_card_refs().front();
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
  bool locked = screen_lock_enabled();
  if (screen_lock_card_refs().empty()) {
    locked = false;
    screen_lock_enabled() = false;
  }

  auto &clickable = screen_lock_clickable_objects();
  for (lv_obj_t *btn : screen_lock_controlled_buttons()) {
    if (!btn || screen_lock_button_is_lock_card(btn)) continue;
    if (locked) {
      screen_lock_clear_clickable_tree(btn);
    }
  }
  if (!locked) {
    for (lv_obj_t *obj : clickable) {
      if (obj) lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    clickable.clear();
  }

  for (const auto &ref : screen_lock_card_refs()) {
    if (!ref.btn) continue;
    set_card_checked_state(ref.btn, locked);
    lv_obj_add_flag(ref.btn, LV_OBJ_FLAG_CLICKABLE);
    if (ref.icon_lbl) {
      const char *icon = locked ? ref.locked_icon : ref.unlocked_icon;
      lv_label_set_text(ref.icon_lbl, icon ? icon : "");
    }
    if (ref.text_lbl) {
      lv_label_set_text(ref.text_lbl,
        locked ? espcontrol_i18n("Screen Locked") : espcontrol_i18n("Screen Unlocked"));
    }
  }

  // Keep HA-exposed entities in sync with the on-device state. Guarded so an
  // HA-driven turn_on/turn_off does not bounce back into a feedback loop.
  ScreenLockPublishCallback publish = screen_lock_publish_callback();
  if (publish && (!screen_lock_published_valid() || screen_lock_published_state() != locked)) {
    screen_lock_published_state() = locked;
    screen_lock_published_valid() = true;
    publish(locked);
  }
}

inline void screen_lock_set_enabled(bool locked) {
  bool was_locked = screen_lock_enabled();
  screen_lock_enabled() = locked;
  screen_lock_apply();  // may clamp to false when no lock card is registered
  bool now_locked = screen_lock_enabled();
  if (now_locked == was_locked) return;
  if (now_locked) {
    screen_lock_invoke_display(screen_lock_display_mode() == "screen_off"
      ? ScreenLockDisplayAction::SCREEN_OFF
      : ScreenLockDisplayAction::SCREENSAVER);
  } else {
    // A remote (HA) unlock can happen while a slide/PIN unlock modal is open;
    // dismiss it so it does not keep covering the now-unlocked grid.
    screen_lock_close_unlock_modal();
    screen_lock_invoke_display(ScreenLockDisplayAction::WAKE);
  }
}

inline void screen_lock_toggle() {
  screen_lock_set_enabled(!screen_lock_enabled());
}
