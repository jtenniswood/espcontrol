#pragma once

// Local guest-network sharing. The values arrive as base64url option values;
// they are decoded only while building the QR payload and are never logged.

#include "wifi_qr_codec.h"

struct WifiQrModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *qr = nullptr;
};

inline WifiQrModalUi &wifi_qr_modal_ui() {
  static WifiQrModalUi ui;
  return ui;
}

inline const lv_font_t *&wifi_qr_icon_font_ref() {
  static const lv_font_t *font = nullptr;
  return font;
}

inline void set_wifi_qr_icon_font(const lv_font_t *font) {
  wifi_qr_icon_font_ref() = font;
}

inline bool wifi_qr_payload_from_config(const ParsedCfg &config, std::string *payload,
                                        std::string *ssid) {
  return wifi_qr_build_payload(cfg_option_value(config.options, "ssid64"),
    cfg_option_value(config.options, "security"), cfg_option_value(config.options, "pass64"),
    cfg_option_token_present(config.options, "hidden"), payload, ssid);
}

inline void wifi_qr_hide_modal() {
  WifiQrModalUi &ui = wifi_qr_modal_ui();
  control_modal_delete_overlay(ControlModalKind::WIFI_QR, ui.overlay);
  ui = WifiQrModalUi();
}

inline void wifi_qr_open_modal(const ParsedCfg &config, lv_obj_t *owner) {
  std::string payload, ssid;
  if (!wifi_qr_payload_from_config(config, &payload, &ssid)) {
    ESP_LOGW("wifi_qr", "Wi-Fi Share card has invalid credentials");
    return;
  }
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::WIFI_QR, owner, 100, wifi_qr_icon_font_ref(), wifi_qr_hide_modal);
  if (!shell.overlay || !shell.panel || !shell.close_btn) return;
  WifiQrModalUi &ui = wifi_qr_modal_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  lv_obj_set_style_bg_color(ui.panel, lv_color_hex(DARK_OVERLAY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t *title = lv_label_create(ui.panel);
  lv_label_set_text(title, config.label.empty() ? "Guest Wi-Fi" : config.label.c_str());
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, shell.layout.inset + shell.layout.back_size / 4);
  lv_obj_t *subtitle = lv_label_create(ui.panel);
  std::string subtitle_text = "Scan to join " + ssid;
  lv_label_set_text(subtitle, subtitle_text.c_str());
  lv_label_set_long_mode(subtitle, LV_LABEL_LONG_DOT);
  lv_obj_set_width(subtitle, lv_pct(80));
  lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, shell.layout.inset + shell.layout.back_size + 8);
  ui.qr = lv_qrcode_create(ui.panel);
  if (!ui.qr) {
    lv_obj_t *message = lv_label_create(ui.panel);
    lv_label_set_text(message, "Unable to create QR code");
    lv_obj_align(message, LV_ALIGN_CENTER, 0, 0);
    return;
  }
  lv_coord_t side = std::max<lv_coord_t>(120, std::min(shell.layout.panel_w, shell.layout.panel_h) -
    (shell.layout.inset * 2 + shell.layout.back_size * 2));
  lv_qrcode_set_size(ui.qr, side);
  lv_qrcode_set_dark_color(ui.qr, lv_color_black());
  lv_qrcode_set_light_color(ui.qr, lv_color_white());
  // The white border is a reliable quiet zone across the LVGL 9 versions
  // supported by ESPHome (the dedicated quiet-zone API is newer).
  lv_obj_set_style_border_color(ui.qr, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.qr, std::max<lv_coord_t>(4, side / 24), LV_PART_MAIN);
  if (lv_qrcode_update(ui.qr, payload.c_str(), payload.size()) != LV_RESULT_OK) {
    lv_obj_del(ui.qr);
    ui.qr = nullptr;
    lv_obj_t *message = lv_label_create(ui.panel);
    lv_label_set_text(message, "Unable to create QR code");
    lv_obj_align(message, LV_ALIGN_CENTER, 0, 0);
    return;
  }
  lv_obj_align(ui.qr, LV_ALIGN_BOTTOM_MID, 0, -shell.layout.inset);
  lv_obj_move_foreground(shell.close_btn);
}
