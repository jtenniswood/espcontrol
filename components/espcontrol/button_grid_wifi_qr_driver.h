#pragma once

namespace espcontrol::cards {
inline bool wifi_qr_driver_matches(const Context &context) {
  return !context.legacy_dispatch && context.runtime.driver == card_runtime::CardDriverId::WIFI_QR;
}

inline bool wifi_qr_driver_uses_tile_qr(const ParsedCfg &config) {
  return config.type == "wifi_qr_card";
}

inline lv_coord_t wifi_qr_driver_tile_side(lv_obj_t *button) {
  if (!button) return 0;
  lv_obj_update_layout(button);
  const lv_coord_t shortest =
    std::min<lv_coord_t>(lv_obj_get_width(button), lv_obj_get_height(button));
  const lv_coord_t inset = std::max<lv_coord_t>(2, shortest / 64);
  const lv_coord_t available = shortest - inset * 2;
  return available >= 48 ? available : 0;
}

inline bool wifi_qr_driver_render_tile(
    BtnSlot &slot, const ParsedCfg &config) {
  if (!slot.btn || !wifi_qr_driver_uses_tile_qr(config)) return false;
  if (slot.icon_lbl) lv_obj_add_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (slot.text_lbl) lv_obj_add_flag(slot.text_lbl, LV_OBJ_FLAG_HIDDEN);
  if (slot.sensor_container)
    lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(slot.btn, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_bg_color(
    slot.btn, lv_color_white(),
    static_cast<lv_style_selector_t>(static_cast<uint32_t>(LV_PART_MAIN) |
                                     static_cast<uint32_t>(LV_STATE_PRESSED)));
  lv_obj_set_style_bg_color(
    slot.btn, lv_color_white(),
    static_cast<lv_style_selector_t>(static_cast<uint32_t>(LV_PART_MAIN) |
                                     static_cast<uint32_t>(LV_STATE_CHECKED)));

  lv_obj_t *qr = slot.sensor_container
    ? static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container))
    : nullptr;
  if (!qr) {
    qr = lv_qrcode_create(slot.btn);
    if (!qr) return true;
    lv_obj_clear_flag(qr, LV_OBJ_FLAG_CLICKABLE);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    // The white tile and responsive inset provide the visible separation here.
    // LVGL's four-module quiet zone made the code unnecessarily small on cards.
    lv_qrcode_set_quiet_zone(qr, false);
    lv_obj_set_style_border_width(qr, 0, LV_PART_MAIN);
    if (slot.sensor_container) lv_obj_set_user_data(slot.sensor_container, qr);
  }

  std::string payload;
  std::string ssid;
  if (!wifi_qr_payload_from_config(config, &payload, &ssid, nullptr)) return true;
  const lv_coord_t side = wifi_qr_driver_tile_side(slot.btn);
  if (side <= 0) return true;
  if (lv_obj_get_width(qr) != side) lv_qrcode_set_size(qr, side);
  if (lv_qrcode_update(qr, payload.c_str(), payload.size()) != LV_RESULT_OK) {
    if (slot.sensor_container) lv_obj_set_user_data(slot.sensor_container, nullptr);
    lv_obj_del(qr);
    return true;
  }
  lv_obj_center(qr);
  return true;
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
  wifi_qr_driver_render_tile(slot, config);
  apply_push_button_transition(slot.btn);
  return true;
}
inline bool wifi_qr_driver_cleanup(BtnSlot &, const ParsedCfg &, const Context &) { return false; }
inline bool wifi_qr_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!wifi_qr_driver_matches(context) || !wifi_qr_driver_uses_tile_qr(config))
    return false;
  return wifi_qr_driver_render_tile(slot, config);
}
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
