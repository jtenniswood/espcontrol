#pragma once

#include <src/libs/qrcode/qrcodegen.h>

#include "wifi_qr_layout.h"

namespace espcontrol::cards {
inline bool wifi_qr_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::WIFI_QR;
}

inline bool wifi_qr_driver_uses_tile_qr(const ParsedCfg &config) {
  return config.type == "wifi_qr_card";
}

inline lv_result_t wifi_qr_driver_update_compact(
    lv_obj_t *qr, const std::string &payload) {
  if (!qr || payload.size() > qrcodegen_BUFFER_LEN_MAX)
    return LV_RESULT_INVALID;
  lv_draw_buf_t *draw_buf = lv_canvas_get_draw_buf(qr);
  if (!draw_buf) return LV_RESULT_INVALID;

  const int version = qrcodegen_getMinFitVersion(
    qrcodegen_Ecc_LOW, payload.size());
  const int module_count = qrcodegen_version2size(version);
  const int scale = wifi_qr_compact_scale(
    draw_buf->header.w, module_count);
  if (version <= 0 || scale <= 0) return LV_RESULT_INVALID;

  const size_t buffer_size = qrcodegen_BUFFER_LEN_FOR_VERSION(version);
  uint8_t *encoded = static_cast<uint8_t *>(lv_malloc(buffer_size));
  uint8_t *data = static_cast<uint8_t *>(lv_malloc(buffer_size));
  if (!encoded || !data) {
    if (encoded) lv_free(encoded);
    if (data) lv_free(data);
    return LV_RESULT_INVALID;
  }
  lv_memcpy(data, payload.data(), payload.size());
  const bool encoded_ok = qrcodegen_encodeBinary(
    data, payload.size(), encoded, qrcodegen_Ecc_LOW, version, version,
    qrcodegen_Mask_AUTO, true);
  if (!encoded_ok) {
    lv_free(encoded);
    lv_free(data);
    return LV_RESULT_INVALID;
  }

  lv_draw_buf_clear(draw_buf, nullptr);
  lv_canvas_set_palette(qr, 0, lv_color_to_32(lv_color_white(), LV_OPA_COVER));
  lv_canvas_set_palette(qr, 1, lv_color_to_32(lv_color_black(), LV_OPA_COVER));
  lv_display_enable_invalidation(lv_obj_get_display(qr), false);
  const int rendered_side = module_count * scale;
  const int margin = (draw_buf->header.w - rendered_side) / 2;
  const lv_color_t dark = lv_color_hex(1);
  for (int module_y = 0; module_y < module_count; module_y++) {
    for (int module_x = 0; module_x < module_count; module_x++) {
      if (!qrcodegen_getModule(encoded, module_x, module_y)) continue;
      const int start_x = margin + module_x * scale;
      const int start_y = margin + module_y * scale;
      for (int y = 0; y < scale; y++) {
        for (int x = 0; x < scale; x++) {
          lv_canvas_set_px(qr, start_x + x, start_y + y, dark, LV_OPA_COVER);
        }
      }
    }
  }
  lv_display_enable_invalidation(lv_obj_get_display(qr), true);
  lv_obj_invalidate(qr);
  lv_free(encoded);
  lv_free(data);
  return LV_RESULT_OK;
}

inline lv_coord_t wifi_qr_driver_tile_side(
    lv_obj_t *button, int row_span, int col_span) {
  if (!button) return 0;
  lv_obj_update_layout(button);
  const lv_coord_t vertical_inset =
    static_cast<lv_coord_t>(wifi_qr_tile_vertical_inset(row_span, col_span));
  const lv_coord_t available = std::min<lv_coord_t>(
    lv_obj_get_width(button), lv_obj_get_height(button) - vertical_inset * 2);
  return available >= 48 ? available : 0;
}

inline bool wifi_qr_driver_render_tile(
    BtnSlot &slot, const ParsedCfg &config, int row_span, int col_span) {
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
    // The QR standard requires four blank modules outside the encoded matrix.
    lv_qrcode_set_quiet_zone(qr, true);
    lv_obj_set_style_border_width(qr, 0, LV_PART_MAIN);
    if (slot.sensor_container) lv_obj_set_user_data(slot.sensor_container, qr);
  }

  std::string payload;
  std::string ssid;
  if (!wifi_qr_payload_from_config(config, &payload, &ssid, nullptr)) return true;
  const lv_coord_t side = wifi_qr_driver_tile_side(slot.btn, row_span, col_span);
  if (side <= 0) return true;
  if (lv_obj_get_width(qr) != side) lv_qrcode_set_size(qr, side);
  const bool compact_tile = row_span == 1 && col_span == 1;
  const std::string rendered_payload = compact_tile
    ? wifi_qr_compact_payload(payload) : payload;
  const lv_result_t update_result = compact_tile
    ? wifi_qr_driver_update_compact(qr, rendered_payload)
    : lv_qrcode_update(qr, rendered_payload.c_str(), rendered_payload.size());
  if (update_result != LV_RESULT_OK) {
    if (slot.sensor_container) lv_obj_set_user_data(slot.sensor_container, nullptr);
    lv_obj_del(qr);
    return true;
  }
  lv_obj_center(qr);
  return true;
}

inline bool wifi_qr_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    int row_span = 1, int col_span = 1) {
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
  wifi_qr_driver_render_tile(slot, config, row_span, col_span);
  apply_push_button_transition(slot.btn);
  return true;
}
inline bool wifi_qr_driver_cleanup(BtnSlot &, const ParsedCfg &, const Context &) { return false; }
inline bool wifi_qr_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    int row_span = 1, int col_span = 1) {
  if (!wifi_qr_driver_matches(context) || !wifi_qr_driver_uses_tile_qr(config))
    return false;
  return wifi_qr_driver_render_tile(slot, config, row_span, col_span);
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
