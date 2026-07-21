// =============================================================================
// NOTIFICATION POPUP - Top-layer overlay driven by HA notify action
// =============================================================================
// Called from screen_notification_popup.yaml. Manages show/hide, the
// auto-dismiss LVGL timer, and firing acknowledgement events back to HA.
//
// show: replaces any active popup; resets/starts the timeout timer.
// dismiss: cancels the timer, hides the scrim, fires the appropriate event.
// =============================================================================
#pragma once
#include <string>
#include "esphome/components/api/homeassistant_service.h"

// --------------------------------------------------------------------------
// Module-level state (one popup at a time)
// --------------------------------------------------------------------------
static lv_timer_t  *s_notification_timer   = nullptr;
static std::string  s_notification_msg_id;
static lv_obj_t    *s_notification_scrim   = nullptr;

// --------------------------------------------------------------------------
// Fire a HA event with device_name + message_id payload
// --------------------------------------------------------------------------
inline void notification_fire_event(const char *event_name, const std::string &message_id) {
  if (esphome::api::global_api_server == nullptr) return;
  esphome::api::HomeassistantActionRequest req;
  req.service  = decltype(req.service)(event_name);
  req.is_event = true;
  req.data.init(2);
  auto &kv1   = req.data.emplace_back();
  kv1.key     = decltype(kv1.key)("device_name");
  kv1.value   = decltype(kv1.value)(esphome::App.get_name().c_str());
  auto &kv2   = req.data.emplace_back();
  kv2.key     = decltype(kv2.key)("message_id");
  kv2.value   = decltype(kv2.value)(message_id.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);
}

// --------------------------------------------------------------------------
// Auto-dismiss timer callback (fires when timeout elapses unread)
// --------------------------------------------------------------------------
static void notification_timer_cb(lv_timer_t *timer) {
  lv_timer_pause(timer);
  if (s_notification_scrim)
    lv_obj_add_flag(s_notification_scrim, LV_OBJ_FLAG_HIDDEN);
  notification_fire_event("esphome.notification_expired", s_notification_msg_id);
}

// --------------------------------------------------------------------------
// Dismiss the popup (user_tapped = true → acknowledged, false → expired)
// --------------------------------------------------------------------------
inline void notification_popup_dismiss(bool user_tapped) {
  if (s_notification_timer)
    lv_timer_pause(s_notification_timer);
  if (s_notification_scrim)
    lv_obj_add_flag(s_notification_scrim, LV_OBJ_FLAG_HIDDEN);
  notification_fire_event(
    user_tapped ? "esphome.notification_acknowledged" : "esphome.notification_expired",
    s_notification_msg_id);
}

// --------------------------------------------------------------------------
// Show a popup. Call after screensaver_wake to ensure display is on.
// timeout_sec == 0 → manual dismiss only (no auto-dismiss).
// --------------------------------------------------------------------------
inline void notification_popup_show(
    lv_obj_t *scrim, lv_obj_t *title_lbl, lv_obj_t *body_lbl,
    const std::string &title, const std::string &body,
    int timeout_sec, const std::string &message_id) {
  s_notification_scrim  = scrim;
  s_notification_msg_id = message_id;

  if (title.empty()) {
    lv_obj_add_flag(title_lbl, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(title_lbl, title.c_str());
    lv_obj_clear_flag(title_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  lv_label_set_text(body_lbl, body.c_str());
  lv_obj_clear_flag(scrim, LV_OBJ_FLAG_HIDDEN);

  if (timeout_sec > 0) {
    uint32_t period_ms = (uint32_t)timeout_sec * 1000u;
    if (!s_notification_timer) {
      s_notification_timer = lv_timer_create(notification_timer_cb, period_ms, nullptr);
    } else {
      lv_timer_set_period(s_notification_timer, period_ms);
      lv_timer_reset(s_notification_timer);
      lv_timer_resume(s_notification_timer);
    }
  } else {
    // No auto-dismiss; pause any running timer from a previous popup.
    if (s_notification_timer)
      lv_timer_pause(s_notification_timer);
  }
}
