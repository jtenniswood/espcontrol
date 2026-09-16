#pragma once

// Shared lifecycle driver for Notification cards. The colours, marquee
// decision, and modal live in button_grid_notification.h; this driver owns the
// grid and subpage lifecycle boundary.
// Contract coverage marker: "notification".

namespace espcontrol::cards {

inline bool notification_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::NOTIFICATION;
}

inline bool notification_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const DisplayProfile &display) {
  if (!notification_driver_matches(context)) return false;
  setup_notification_card(slot, config, display);
  return true;
}

inline bool notification_driver_attach_interaction(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!notification_driver_matches(context)) return false;
  // The card is information-only but still opens its message on tap.
  lv_obj_add_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  return true;
}

inline bool notification_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &, const Context &context,
    const DisplayProfile &display) {
  if (!notification_driver_matches(context)) return false;
  // The generic layout pass clamps the card label to a wrapped, bottom-aligned
  // block; restore the single centred marquee line after it runs.
  notification_card_apply_card_layout(slot, display);
  return true;
}

inline bool notification_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!notification_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

inline bool notification_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const GridConfig &grid) {
  if (!notification_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  notification_card_bind_runtime(
    slot, config, grid.width_compensation_percent, false);
  return true;
}

inline bool notification_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const GridConfig &grid) {
  if (!notification_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  notification_card_bind_runtime(
    slot, config, grid.width_compensation_percent, true);
  return true;
}

inline bool notification_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!notification_driver_matches(context)) return false;
  NotificationCardCtx *notification = button
    ? static_cast<NotificationCardCtx *>(lv_obj_get_user_data(button)) : nullptr;
  if (notification) notification_card_open_modal(notification);
  return true;
}

}  // namespace espcontrol::cards
