#pragma once
#include "screensaver_pin.h"
#include "pin_keypad.h"

struct ScreensaverPinUi {
  lv_obj_t *overlay{nullptr};
  lv_obj_t *label{nullptr};
  std::string entry;
  bool touch_blocked{false};
};
inline ScreensaverPinUi &screensaver_pin_ui() {
  static ScreensaverPinUi ui;
  return ui;
}
inline void screensaver_pin_hide() {
  auto &ui = screensaver_pin_ui();
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui = {};
}
inline void screensaver_pin_key_cb(lv_event_t *event) {
  auto &ui = screensaver_pin_ui();
  const char *key = static_cast<const char *>(lv_event_get_user_data(event));
  if (!key || !ui.label || ui.touch_blocked) return;
  if (std::strcmp(key, "submit") == 0) {
    if (espcontrol::screensaver_pin().submit(ui.entry)) {
      // Defer deletion until LVGL has finished dispatching this click.
      lv_async_call([](void *) {
        if (!espcontrol::screensaver_pin().locked()) screensaver_pin_hide();
      }, nullptr);
      return;
    }
    ui.entry.clear();
    lv_label_set_display_text(ui.label, espcontrol_i18n("Incorrect PIN"));
    return;
  }
  if (std::strcmp(key, "back") == 0) ui.entry.clear();
  else if (ui.entry.size() < 16 && key[0] >= '0' && key[0] <= '9') ui.entry += key[0];
  const std::string masked(ui.entry.size(), '*');
  lv_label_set_display_text(ui.label, ui.entry.empty() ? espcontrol_i18n("Enter Pin") : masked.c_str());
}
inline void screensaver_pin_refresh(bool active, const lv_font_t *label_font,
                                    const lv_font_t *key_font, const lv_font_t *icon_font,
                                    bool touch_blocked) {
  if (active) espcontrol::screensaver_pin().wake();
  if (!active || !espcontrol::screensaver_pin().locked()) {
    screensaver_pin_hide();
    return;
  }
  auto &ui = screensaver_pin_ui();
  // The tap that woke a dark screen must finish before it can enter a digit.
  ui.touch_blocked = touch_blocked;
  if (ui.overlay) return;
  // The system layer is above clock, artwork, status icons, and all control
  // modals. Its full-screen clickable surface consumes every underlying touch.
  // It is independent of the dismissible modal service and has no close button.
  ui.overlay = lv_obj_create(lv_layer_sys());
  control_modal_style_overlay(ui.overlay);
  lv_obj_add_flag(ui.overlay, LV_OBJ_FLAG_CLICKABLE);
  const auto layout = control_modal_calc_layout(100);
  auto *panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(panel, control_modal_card_radius(nullptr));
  control_modal_apply_panel_layout(ui.overlay, panel, layout, 0);
  ui.label = pin_keypad_create(panel, layout, label_font, key_font, icon_font,
                              100, screensaver_pin_key_cb);
}

inline std::function<void()> &force_screensaver_action() {
  static std::function<void()> action;
  return action;
}
