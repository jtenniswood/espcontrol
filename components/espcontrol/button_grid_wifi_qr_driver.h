#pragma once

namespace espcontrol::cards {
inline bool wifi_qr_driver_matches(const Context &context) {
  return !context.legacy_dispatch && context.runtime.driver == card_runtime::CardDriverId::WIFI_QR;
}
inline bool wifi_qr_driver_setup_visual(BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!wifi_qr_driver_matches(context)) return false;
  setup_toggle_visual(slot, config);
  if (slot.text_lbl) {
    set_wifi_qr_label_font(lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN));
  }
  if (slot.icon_lbl) {
    lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    const char *icon = (config.icon.empty() || config.icon == "Auto")
      ? find_icon("Wifi") : find_icon(config.icon.c_str());
    lv_label_set_display_text(slot.icon_lbl, icon);
  }
  apply_push_button_transition(slot.btn);
  return true;
}
inline bool wifi_qr_driver_cleanup(BtnSlot &, const ParsedCfg &, const Context &) { return false; }
inline bool wifi_qr_driver_refresh_layout(BtnSlot &, const ParsedCfg &, const Context &) { return false; }
inline bool wifi_qr_driver_bind_main(BtnSlot &, const ParsedCfg &, const Context &context) { return wifi_qr_driver_matches(context); }
inline bool wifi_qr_driver_bind_subpage(BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!wifi_qr_driver_matches(context)) return false;
  ParsedCfg *stored = grid_delete_with_owner(slot.btn, new ParsedCfg(config));
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    ParsedCfg *card = static_cast<ParsedCfg *>(lv_event_get_user_data(event));
    if (card) wifi_qr_open_modal(*card, static_cast<lv_obj_t *>(lv_event_get_target(event)));
  }, LV_EVENT_CLICKED, stored);
  return true;
}
inline bool wifi_qr_driver_handle_main_click(const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  if (!wifi_qr_driver_matches(context)) return false;
  wifi_qr_open_modal(config, button);
  return true;
}
}  // namespace espcontrol::cards
