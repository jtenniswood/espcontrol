#pragma once
#include "screensaver_pin.h"
#include "screen_lock_state.h"
#include "pin_keypad.h"

struct ScreensaverPinUi {
  lv_obj_t *overlay{nullptr};
  lv_obj_t *label{nullptr};
  lv_coord_t screen_w{0};
  lv_coord_t screen_h{0};
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
inline void screensaver_pin_update_entry_label() {
  auto &ui = screensaver_pin_ui();
  std::string masked;
  for (size_t i = 0; i < ui.entry.size(); ++i) masked += "\u2022";
  lv_label_set_display_text(ui.label, ui.entry.empty() ? espcontrol_i18n("Enter Pin") : masked.c_str());
}
inline std::function<void()> &return_screen_lock_view() {
  static std::function<void()> action;
  return action;
}
inline bool screensaver_pin_restore_previous_mode(espcontrol::DisplayModeController &controller) {
  const auto mode = espcontrol::screensaver_pin().return_mode();
  if (mode == espcontrol::DisplayMode::ACTIVE || mode == espcontrol::DisplayMode::SETUP_DIMMED) return false;
  if (mode == espcontrol::DisplayMode::COVER_ART) {
    controller.clear(espcontrol::DisplayRequestSource::USER_WAKE);
    controller.request(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK, mode);
  } else {
    controller.request(espcontrol::DisplayRequestSource::MANUAL_SLEEP, mode);
  }
  return true;
}
inline void screensaver_pin_key_cb(lv_event_t *event) {
  auto &ui = screensaver_pin_ui();
  const char *key = static_cast<const char *>(lv_event_get_user_data(event));
  if (!key || !ui.label || ui.touch_blocked) return;
  if (std::strcmp(key, "submit") == 0) {
    if (espcontrol::screensaver_pin().submit(ui.entry)) {
      screen_lock_apply();
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
  if (std::strcmp(key, "back") == 0) {
    if (ui.entry.empty()) {
      espcontrol::screensaver_pin().dismiss_keypad();
      // The originating X click must finish before the keypad is removed.
      lv_async_call([](void *) {
        if (espcontrol::screensaver_pin().locked() && !espcontrol::screensaver_pin().keypad_requested()) {
          if (return_screen_lock_view()) return_screen_lock_view()();
          else if (show_screen_lock_pin()) show_screen_lock_pin()();
        }
      }, nullptr);
      return;
    }
    ui.entry.clear();
  }
  else if (ui.entry.size() < 16 && key[0] >= '0' && key[0] <= '9') ui.entry += key[0];
  screensaver_pin_update_entry_label();
}
inline void screensaver_pin_guard_cb(lv_event_t *) {
  if (screensaver_pin_ui().touch_blocked) return;
  espcontrol::screensaver_pin().request_keypad();
  // Consume the whole gesture before replacing the guard with keys, so this
  // first touch cannot also become a digit or activate an underlying control.
  lv_async_call([](void *) {
    if (show_screen_lock_pin()) show_screen_lock_pin()();
  }, nullptr);
}
inline void screensaver_pin_refresh(bool active, const lv_font_t *label_font,
                                    const lv_font_t *key_font, const lv_font_t *icon_font,
                                    bool touch_blocked, const lv_font_t *title_font = nullptr) {
  if (active) espcontrol::screensaver_pin().wake();
  screen_lock_apply();
  if (!active || !espcontrol::screensaver_pin().locked()) {
    screensaver_pin_hide();
    return;
  }
  auto &ui = screensaver_pin_ui();
  // The tap that woke a dark screen must finish before it can enter a digit.
  ui.touch_blocked = touch_blocked;
  const bool keypad_requested = espcontrol::screensaver_pin().keypad_requested();
  auto *display = lv_disp_get_default();
  const lv_coord_t screen_w = display ? lv_disp_get_hor_res(display) : 480;
  const lv_coord_t screen_h = display ? lv_disp_get_ver_res(display) : 480;
  if (ui.overlay && (ui.label != nullptr) == keypad_requested &&
      ui.screen_w == screen_w && ui.screen_h == screen_h) return;
  std::string entry = ui.label && keypad_requested ? ui.entry : "";
  if (ui.overlay) screensaver_pin_hide();
  ui.entry = std::move(entry);
  ui.screen_w = screen_w;
  ui.screen_h = screen_h;
  ui.touch_blocked = touch_blocked;
  // The system layer is above clock, artwork, status icons, and all control
  // modals. Its full-screen clickable surface consumes every underlying touch.
  // It is independent of the modal service; returning to a previous view
  // keeps PIN protection active.
  ui.overlay = lv_obj_create(lv_layer_sys());
  control_modal_style_overlay(ui.overlay);
  lv_obj_add_flag(ui.overlay, LV_OBJ_FLAG_CLICKABLE);
  if (!keypad_requested) {
    lv_obj_add_event_cb(ui.overlay, screensaver_pin_guard_cb, LV_EVENT_RELEASED, nullptr);
    return;
  }
  const auto layout = control_modal_calc_layout(100);
  auto *panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(panel, control_modal_card_radius(nullptr));
  control_modal_apply_panel_layout(ui.overlay, panel, layout, 0);
  auto *title = control_modal_create_title(panel, espcontrol_i18n("Screen Lock"),
                                          layout.panel_w - layout.inset * 2, title_font, 100);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);
  lv_obj_update_layout(title);
  const lv_coord_t prompt_top = layout.inset * 2 + lv_obj_get_height(title);
  ui.label = pin_keypad_create(panel, layout, label_font, key_font, icon_font,
                              100, screensaver_pin_key_cb, espcontrol::screensaver_pin().randomize_keypad(), true,
                              prompt_top);
  screensaver_pin_update_entry_label();
}
