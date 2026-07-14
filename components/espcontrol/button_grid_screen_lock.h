#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Screen Lock v2 unlock modals. Locking (from unlocked) is always an immediate
// tap; unlocking (from locked) is gated by the configured unlock method:
//   - "immediate": toggle right away (legacy behaviour)
//   - "gesture":   slide-to-unlock modal (vertical track + draggable handle)
//   - "pin":       numeric keypad modal (reuses the alarm keypad building blocks)
// The PIN keypad deliberately reuses alarm_create_key_button and the alarm
// keypad geometry helpers rather than duplicating them.

#include <cstring>
#include <string>

// ── Slide-to-unlock gesture modal ─────────────────────────────────────────

struct ScreenLockGestureModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *slider = nullptr;
  lv_obj_t *hint_lbl = nullptr;
};

inline ScreenLockGestureModalUi &screen_lock_gesture_modal_ui() {
  static ScreenLockGestureModalUi ui;
  return ui;
}

inline void screen_lock_gesture_hide_modal() {
  ScreenLockGestureModalUi &ui = screen_lock_gesture_modal_ui();
  control_modal_delete_overlay(ControlModalKind::SCREEN_LOCK_GESTURE, ui.overlay);
  ui = ScreenLockGestureModalUi();
}

inline void screen_lock_gesture_finish_cb(lv_timer_t *timer) {
  lv_timer_del(timer);
  screen_lock_gesture_hide_modal();
  screen_lock_set_enabled(false);
}

inline void screen_lock_gesture_released_cb(lv_event_t *e) {
  lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
  if (!slider) return;
  int32_t value = lv_slider_get_value(slider);
  if (value >= 95) {
    // Defer the unlock+close so we do not delete the overlay from inside the
    // slider's own event callback.
    lv_timer_create(screen_lock_gesture_finish_cb, 1, nullptr);
  } else {
    lv_slider_set_value(slider, 0, LV_ANIM_ON);
  }
}

inline void screen_lock_gesture_open_modal(lv_obj_t *btn) {
  const ScreenLockCardRef *ref = screen_lock_card_ref_for(btn);
  const lv_font_t *button_font = btn ? lv_obj_get_style_text_font(btn, LV_PART_MAIN) : nullptr;
  const lv_font_t *text_font = ref && ref->text_font ? ref->text_font : button_font;
  const lv_font_t *icon_font = ref && ref->icon_font ? ref->icon_font : text_font;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::SCREEN_LOCK_GESTURE, btn, 100, icon_font,
    "\U000F0156", false, screen_lock_gesture_hide_modal);
  if (!shell.overlay || !shell.panel) return;

  ScreenLockGestureModalUi &ui = screen_lock_gesture_modal_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;

  ControlModalLayout &layout = shell.layout;
  uint32_t accent = current_button_primary_color();

  ui.hint_lbl = lv_label_create(ui.panel);
  lv_label_set_text(ui.hint_lbl, espcontrol_i18n("Slide to Unlock"));
  lv_label_set_long_mode(ui.hint_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(ui.hint_lbl, layout.panel_w - layout.inset * 2);
  lv_obj_set_style_text_color(ui.hint_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.hint_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (text_font) lv_obj_set_style_text_font(ui.hint_lbl, text_font, LV_PART_MAIN);
  lv_coord_t hint_y = layout.inset + layout.back_size + control_modal_scaled_px(12, layout.short_side);
  lv_obj_align(ui.hint_lbl, LV_ALIGN_TOP_MID, 0, hint_y);
  lv_obj_update_layout(ui.hint_lbl);

  lv_coord_t track_top = hint_y + lv_obj_get_height(ui.hint_lbl) +
                         control_modal_scaled_px(24, layout.short_side);
  lv_coord_t track_bottom = layout.panel_h - layout.inset;
  lv_coord_t track_h = track_bottom - track_top;
  if (track_h < control_modal_scaled_px(120, layout.short_side)) {
    track_h = control_modal_scaled_px(120, layout.short_side);
  }
  lv_coord_t track_w = control_modal_scaled_px(96, layout.short_side);
  if (track_w < 56) track_w = 56;
  if (track_w > layout.panel_w - layout.inset * 2) track_w = layout.panel_w - layout.inset * 2;

  ui.slider = lv_slider_create(ui.panel);
  // A taller-than-wide slider renders vertically; the value grows upward, so
  // dragging the handle from the bottom to the top completes the unlock.
  lv_obj_set_size(ui.slider, track_w, track_h);
  lv_slider_set_range(ui.slider, 0, 100);
  lv_slider_set_value(ui.slider, 0, LV_ANIM_OFF);
  lv_obj_set_style_radius(ui.slider, track_w / 2, LV_PART_MAIN);
  lv_obj_set_style_radius(ui.slider, track_w / 2, LV_PART_INDICATOR);
  lv_obj_set_style_radius(ui.slider, track_w / 2, LV_PART_KNOB);
  lv_obj_set_style_bg_color(ui.slider, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui.slider, lv_color_hex(accent), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(ui.slider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui.slider, lv_color_hex(accent), LV_PART_KNOB);
  lv_obj_set_style_pad_all(ui.slider, track_w / 8, LV_PART_KNOB);
  lv_obj_align(ui.slider, LV_ALIGN_TOP_MID, 0, track_top);
  lv_obj_add_event_cb(ui.slider, screen_lock_gesture_released_cb, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.slider, [](lv_event_t *e) {
    // Snap back if the drag is abandoned (pointer leaves the slider).
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (slider) lv_slider_set_value(slider, 0, LV_ANIM_ON);
  }, LV_EVENT_PRESS_LOST, nullptr);

  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
}

// ── PIN keypad modal (reuses the alarm keypad) ─────────────────────────────

struct ScreenLockPinModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *pin_lbl = nullptr;
  std::string entry;
  bool showing_error = false;
};

inline ScreenLockPinModalUi &screen_lock_pin_modal_ui() {
  static ScreenLockPinModalUi ui;
  return ui;
}

inline void screen_lock_pin_hide_modal() {
  ScreenLockPinModalUi &ui = screen_lock_pin_modal_ui();
  ui.entry.clear();
  control_modal_delete_overlay(ControlModalKind::SCREEN_LOCK_PIN, ui.overlay);
  ui = ScreenLockPinModalUi();
}

inline void screen_lock_pin_update_display() {
  ScreenLockPinModalUi &ui = screen_lock_pin_modal_ui();
  if (!ui.pin_lbl) return;
  uint32_t color = ui.showing_error ? ALARM_TRIGGERED_COLOR : DARK_TEXT_PRIMARY;
  lv_obj_set_style_text_color(ui.pin_lbl, lv_color_hex(color), LV_PART_MAIN);
  if (ui.showing_error) {
    lv_label_set_text(ui.pin_lbl, espcontrol_i18n("Wrong PIN"));
    return;
  }
  if (ui.entry.empty()) {
    lv_label_set_text(ui.pin_lbl, espcontrol_i18n("Enter Pin"));
    return;
  }
  std::string masked(ui.entry.size(), '*');
  lv_label_set_text(ui.pin_lbl, masked.c_str());
}

inline void screen_lock_pin_unlock_cb(lv_timer_t *timer) {
  lv_timer_del(timer);
  screen_lock_pin_hide_modal();
  screen_lock_set_enabled(false);
}

inline void screen_lock_pin_submit() {
  ScreenLockPinModalUi &ui = screen_lock_pin_modal_ui();
  if (ui.entry.empty()) return;
  if (ui.entry == screen_lock_pin()) {
    // Defer so we do not delete the overlay from inside a key's event callback.
    lv_timer_create(screen_lock_pin_unlock_cb, 1, nullptr);
    return;
  }
  // Wrong PIN: clear the entry and flash a red hint (resets on next digit).
  ui.entry.clear();
  ui.showing_error = true;
  screen_lock_pin_update_display();
}

inline void screen_lock_pin_key_cb(lv_event_t *e) {
  const char *key = static_cast<const char *>(lv_event_get_user_data(e));
  if (!key) return;
  ScreenLockPinModalUi &ui = screen_lock_pin_modal_ui();
  if (strcmp(key, "back") == 0) {
    ui.entry.clear();
    ui.showing_error = false;
    screen_lock_pin_update_display();
    return;
  }
  if (strcmp(key, "submit") == 0) {
    screen_lock_pin_submit();
    return;
  }
  if (ui.entry.size() < 16 && key[0] >= '0' && key[0] <= '9' && key[1] == '\0') {
    ui.showing_error = false;
    ui.entry.push_back(key[0]);
    screen_lock_pin_update_display();
  }
}

inline void screen_lock_pin_open_modal(lv_obj_t *btn) {
  const ScreenLockCardRef *ref = screen_lock_card_ref_for(btn);
  const lv_font_t *button_font = btn ? lv_obj_get_style_text_font(btn, LV_PART_MAIN) : nullptr;
  const lv_font_t *text_font = ref && ref->text_font ? ref->text_font : button_font;
  const lv_font_t *icon_font = ref && ref->icon_font ? ref->icon_font : text_font;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::SCREEN_LOCK_PIN, btn, 100, icon_font,
    "\U000F0156", false, screen_lock_pin_hide_modal);
  if (!shell.overlay || !shell.panel) return;

  ScreenLockPinModalUi &ui = screen_lock_pin_modal_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.entry.clear();
  ui.showing_error = false;

  ControlModalLayout &layout = shell.layout;

  ui.pin_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.pin_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.pin_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (text_font) lv_obj_set_style_text_font(ui.pin_lbl, text_font, LV_PART_MAIN);
  lv_coord_t pin_w = layout.panel_w - (layout.inset + layout.back_size) * 2;
  if (pin_w < 60) pin_w = layout.panel_w - layout.inset * 2;
  lv_obj_set_width(ui.pin_lbl, pin_w);
  screen_lock_pin_update_display();
  lv_obj_update_layout(ui.pin_lbl);
  lv_coord_t pin_h = lv_obj_get_height(ui.pin_lbl);
  lv_coord_t pin_y = alarm_pin_label_y(layout, pin_h);
  lv_obj_align(ui.pin_lbl, LV_ALIGN_TOP_MID, 0, pin_y);

  lv_coord_t pin_button_gap = control_modal_scaled_px(24, layout.short_side);
  if (pin_button_gap < 10) pin_button_gap = 10;
  lv_coord_t keypad_top = layout.inset + layout.back_size + pin_button_gap;
  lv_coord_t keypad_bottom = layout.panel_h - layout.inset;
  lv_coord_t keypad_w = layout.panel_w - layout.inset * 2;
  lv_coord_t keypad_h = keypad_bottom - keypad_top;
  lv_coord_t gap = alarm_pin_keypad_gap(keypad_w, keypad_h, layout.short_side);
  lv_coord_t key_size = alarm_pin_key_size(keypad_w, keypad_h, gap, layout.short_side);
  lv_coord_t total_w = key_size * 3 + gap * 2;
  lv_coord_t total_h = key_size * 4 + gap * 3;
  lv_coord_t start_x = (layout.panel_w - total_w) / 2;
  lv_coord_t start_y = keypad_top + (keypad_h - total_h) / 2;
  if (start_y < keypad_top) start_y = keypad_top;

  static const char *key_data[12] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9",
    "back", "0", "submit",
  };

  for (int i = 0; i < 12; i++) {
    const char *text = key_data[i];
    const lv_font_t *key_font = text_font;
    uint16_t key_zoom = 256;
    if (strcmp(text, "back") == 0) {
      text = "\U000F0156";
      key_font = icon_font;
      key_zoom = 170;
    } else if (strcmp(text, "submit") == 0) {
      text = find_icon("Check");
      key_font = icon_font;
      key_zoom = 170;
    }

    lv_obj_t *key_btn = alarm_create_key_button(
      ui.panel, key_size, key_size, text, key_font, 100, key_zoom);
    if (strcmp(key_data[i], "submit") == 0) {
      lv_obj_set_style_bg_color(key_btn, lv_color_hex(DEFAULT_SLIDER_COLOR), LV_PART_MAIN);
      lv_obj_set_style_border_color(key_btn, lv_color_hex(DEFAULT_SLIDER_COLOR), LV_PART_MAIN);
      lv_obj_t *key_lbl = lv_obj_get_child(key_btn, 0);
      if (key_lbl) lv_obj_set_style_text_color(key_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    }
    int row = i / 3;
    int col = i % 3;
    lv_coord_t x = start_x + col * (key_size + gap);
    lv_coord_t y = start_y + row * (key_size + gap);
    lv_obj_set_pos(key_btn, x, y);
    lv_obj_add_event_cb(key_btn, screen_lock_pin_key_cb, LV_EVENT_CLICKED,
      const_cast<char *>(key_data[i]));
  }

  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
}

// Dismiss an open slide/PIN unlock modal (no-op if none is showing). Used by
// the state transitions in screen_lock_state.h (remote unlock, registry rebuild)
// to stop a modal surviving on the top layer over an unlocked/rebuilt grid.
inline void screen_lock_close_unlock_modal() {
  if (screen_lock_gesture_modal_ui().overlay) screen_lock_gesture_hide_modal();
  if (screen_lock_pin_modal_ui().overlay) screen_lock_pin_hide_modal();
}

// A stored PIN is only usable if the on-screen keypad can reproduce it: digits
// only, within the 16-char entry cap. An imported / hand-written config with a
// longer or non-numeric PIN would otherwise trap the user (the keypad could
// never match it), so treat such a PIN as "no PIN" and fall back to immediate.
inline bool screen_lock_pin_is_usable() {
  const std::string &pin = screen_lock_pin();
  if (pin.empty() || pin.size() > 16) return false;
  for (char c : pin) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

// ── Shared tap handler (both main-grid and subpage tap intercepts) ─────────

inline void screen_lock_handle_tap(lv_obj_t *btn) {
  if (!screen_lock_enabled()) {
    // Currently unlocked: locking is always an immediate tap.
    screen_lock_set_enabled(true);
    return;
  }
  // Currently locked: unlocking is gated by the configured method.
  const std::string &method = screen_lock_unlock_method();
  if (method == "gesture" || (method == "pin" && screen_lock_pin_is_usable())) {
    // Wake the display before showing the unlock modal. With lock display mode
    // "screen_off" the backlight is off, so the slide/PIN modal would otherwise
    // be drawn on a dark screen and the user could only unlock from Home
    // Assistant. WAKE just turns the backlight on; the lock state is unchanged.
    screen_lock_invoke_display(ScreenLockDisplayAction::WAKE);
    if (method == "gesture") {
      screen_lock_gesture_open_modal(btn);
    } else {
      screen_lock_pin_open_modal(btn);
    }
    return;
  }
  // "immediate", or a "pin" with no usable stored code: do not trap the user.
  screen_lock_set_enabled(false);
}
