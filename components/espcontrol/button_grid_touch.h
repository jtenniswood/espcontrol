#pragma once

inline void sync_bound_card_text_color(lv_obj_t *button) {
  if (!button) return;
  for (uint32_t i = 0; i < lv_obj_get_event_count(button); ++i) {
    if (lv_event_dsc_get_cb(lv_obj_get_event_dsc(button, i)) == card_text_contrast_event_cb) {
      sync_card_checked_text_color(button);
      return;
    }
  }
}

// Consume the held touch before navigation can replace or delete the control.
// Its release must neither repeat the action nor reach the destination UI.
inline void consume_navigation_touch(lv_obj_t *button) {
  if (auto *input = lv_indev_get_act()) lv_indev_wait_release(input);
  if (!button) return;
  lv_obj_clear_state(button, LV_STATE_PRESSED);
  // A consumed release cannot restore card contrast through its usual callback.
  // Modal controls can style their child labels independently of the button.
  sync_bound_card_text_color(button);
}

inline void bind_navigation_button_action(lv_obj_t *button, lv_event_cb_t action,
                                          void *user_data) {
  lv_obj_add_event_cb(button, [](lv_event_t *event) {
    consume_navigation_touch(static_cast<lv_obj_t *>(lv_event_get_target(event)));
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(button, action, LV_EVENT_PRESSED, user_data);
}
