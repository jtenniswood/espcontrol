#pragma once

// Local guest-network sharing. The values arrive as base64url option values;
// they are decoded only while the modal is open and are never logged.

#include "wifi_qr_codec.h"

enum class WifiQrTab : uint8_t {
  QR = 0,
  DETAILS = 1,
};

struct WifiQrModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *tab_row = nullptr;
  lv_obj_t *qr_tab = nullptr;
  lv_obj_t *details_tab = nullptr;
  lv_obj_t *qr_view = nullptr;
  lv_obj_t *details_view = nullptr;
  lv_obj_t *qr = nullptr;
  std::string qr_payload;
  lv_coord_t qr_side = 0;
  std::vector<WifiQrTab> tabs;
  WifiQrTab tab = WifiQrTab::QR;
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

inline const lv_font_t *&wifi_qr_label_font_ref() {
  static const lv_font_t *font = nullptr;
  return font;
}

inline void set_wifi_qr_label_font(const lv_font_t *font) {
  wifi_qr_label_font_ref() = font;
}

inline const lv_font_t *&wifi_qr_heading_font_ref() {
  static const lv_font_t *font = nullptr;
  return font;
}

inline void set_wifi_qr_heading_font(const lv_font_t *font) {
  wifi_qr_heading_font_ref() = font;
}

inline bool wifi_qr_payload_from_config(const ParsedCfg &config, std::string *payload,
                                        std::string *ssid, std::string *password) {
  return wifi_qr_build_payload(cfg_option_value(config.options, "ssid64"),
    cfg_option_value(config.options, "security"), cfg_option_value(config.options, "pass64"),
    cfg_option_token_present(config.options, "hidden"), payload, ssid, password);
}

inline void wifi_qr_hide_modal() {
  WifiQrModalUi &ui = wifi_qr_modal_ui();
  control_modal_delete_overlay(ControlModalKind::WIFI_QR, ui.overlay);
  ui = WifiQrModalUi();
}

inline void wifi_qr_set_visible(lv_obj_t *obj, bool visible) {
  if (!obj) return;
  if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

inline void wifi_qr_style_tab(lv_obj_t *btn, bool active) {
  if (!btn) return;
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? DARK_TEXT_PRIMARY : SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) {
    lv_obj_set_style_text_color(
      label, lv_color_hex(active ? TERTIARY_GREY : DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
}

inline void wifi_qr_apply_tab_visibility() {
  WifiQrModalUi &ui = wifi_qr_modal_ui();
  const bool show_qr = ui.tab == WifiQrTab::QR;
  wifi_qr_set_visible(ui.qr_view, show_qr);
  wifi_qr_set_visible(ui.details_view, !show_qr);
  wifi_qr_style_tab(ui.qr_tab, show_qr);
  wifi_qr_style_tab(ui.details_tab, !show_qr);
}

inline void wifi_qr_layout_modal() {
  WifiQrModalUi &ui = wifi_qr_modal_ui();
  if (!ui.overlay || !ui.panel) return;
  const ControlModalLayout layout = control_modal_calc_layout(100);
  control_modal_apply_panel_layout(
    ui.overlay, ui.panel, layout, control_modal_card_radius(nullptr));
  control_modal_apply_back_button_layout(ui.back_btn, layout);

  const int tab_count = std::max<int>(1, ui.tabs.size());
  const bool show_tab_bar = ui.tabs.size() > 1;
  const ControlModalTabLayout tabs_layout =
    control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);
  control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);
  for (size_t index = 0; index < ui.tabs.size(); ++index) {
    const WifiQrTab tab = ui.tabs[index];
    lv_obj_t *button = tab == WifiQrTab::QR ? ui.qr_tab : ui.details_tab;
    control_modal_layout_tab_button(
      button, layout, tabs_layout, index, ui.tab == tab);
  }

  // With only one enabled tab there is no selector separating the Back button
  // from the content. Reserve the button's full height plus a normal inset so
  // the QR quiet zone and credentials never sit beneath its touch target.
  const lv_coord_t content_safe_top = show_tab_bar
    ? 0
    : layout.back_inset_y + layout.back_size + layout.inset;
  const espcontrol::modal::ContentLayout content =
    control_modal_calc_content_layout(
      layout, tabs_layout, show_tab_bar, 120, content_safe_top);
  for (lv_obj_t *view : {ui.qr_view, ui.details_view}) {
    if (!view) continue;
    lv_obj_set_size(view, content.width, content.height);
    lv_obj_align(view, LV_ALIGN_TOP_MID, 0, content.top);
    lv_obj_set_style_pad_left(view, layout.inset, LV_PART_MAIN);
    lv_obj_set_style_pad_right(view, layout.inset, LV_PART_MAIN);
  }

  if (ui.qr) {
    const lv_coord_t qr_padding = std::max<lv_coord_t>(
      layout.inset, control_modal_scaled_px(24, layout.short_side));
    const lv_coord_t available_side =
      std::min<lv_coord_t>(content.width, content.height) - qr_padding * 2;
    const lv_coord_t side = std::max<lv_coord_t>(120, available_side);
    if (ui.qr_side != side) {
      // Resizing a QR widget replaces and clears its canvas buffer, so the
      // payload must be rendered again after every real size change.
      lv_qrcode_set_size(ui.qr, side);
      ui.qr_side = side;
      if (lv_qrcode_update(ui.qr, ui.qr_payload.c_str(), ui.qr_payload.size()) !=
          LV_RESULT_OK) {
        lv_obj_del(ui.qr);
        ui.qr = nullptr;
      }
    }
  }

  if (ui.qr) {
    const lv_coord_t side = ui.qr_side;
    lv_obj_set_style_border_width(
      ui.qr, std::max<lv_coord_t>(4, side / 24), LV_PART_MAIN);
    lv_obj_center(ui.qr);
  }

  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  if (ui.tab_row) lv_obj_move_foreground(ui.tab_row);
}

inline lv_obj_t *wifi_qr_create_tab_button(lv_obj_t *parent, const char *icon,
                                           WifiQrTab tab) {
  if (!parent) return nullptr;
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *label = lv_label_create(btn);
  if (label) {
    lv_label_set_display_text(label, icon);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (wifi_qr_icon_font_ref())
      lv_obj_set_style_text_font(label, wifi_qr_icon_font_ref(), LV_PART_MAIN);
    control_modal_center_tab_icon(label);
  }
  lv_obj_add_event_cb(btn, [](lv_event_t *event) {
    WifiQrModalUi &ui = wifi_qr_modal_ui();
    ui.tab = static_cast<WifiQrTab>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
    wifi_qr_apply_tab_visibility();
    wifi_qr_layout_modal();
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(tab)));
  return btn;
}

inline lv_obj_t *wifi_qr_create_view(lv_obj_t *parent) {
  lv_obj_t *view = lv_obj_create(parent);
  if (!view) return nullptr;
  lv_obj_set_style_bg_opa(view, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(view, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(view, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_top(view, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(view, 0, LV_PART_MAIN);
  lv_obj_clear_flag(view, LV_OBJ_FLAG_SCROLLABLE);
  return view;
}

inline lv_obj_t *wifi_qr_create_detail_label(lv_obj_t *parent, const char *text,
                                             uint32_t color,
                                             const lv_font_t *font = nullptr) {
  lv_obj_t *label = lv_label_create(parent);
  if (!label) return nullptr;
  lv_label_set_text(label, text);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, lv_pct(100));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
  const lv_font_t *resolved_font = font ? font : wifi_qr_label_font_ref();
  if (resolved_font)
    lv_obj_set_style_text_font(label, resolved_font, LV_PART_MAIN);
  return label;
}

inline void wifi_qr_open_modal(const ParsedCfg &config, lv_obj_t *owner) {
  std::string payload, ssid, password;
  if (!wifi_qr_payload_from_config(config, &payload, &ssid, &password)) {
    ESP_LOGW("wifi_qr", "Wifi Share card has invalid credentials");
    return;
  }

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::WIFI_QR, owner, 100, wifi_qr_icon_font_ref(), wifi_qr_hide_modal);
  if (!shell.overlay || !shell.panel || !shell.close_btn) return;
  WifiQrModalUi &ui = wifi_qr_modal_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.qr_payload = payload;
  const std::vector<std::string> configured_tabs =
    wifi_qr_tabs(cfg_option_value(config.options, "wifi_tabs"));
  for (const std::string &tab : configured_tabs) {
    ui.tabs.push_back(tab == "credentials" ? WifiQrTab::DETAILS : WifiQrTab::QR);
  }
  ui.tab = ui.tabs.front();

  ui.tab_row = control_modal_create_tab_row(ui.panel);
  for (WifiQrTab tab : ui.tabs) {
    lv_obj_t *button = wifi_qr_create_tab_button(ui.tab_row,
      find_icon(tab == WifiQrTab::QR ? "Wifi QR Tab" : "Wifi Password Tab"), tab);
    if (tab == WifiQrTab::QR) ui.qr_tab = button;
    else ui.details_tab = button;
  }

  if (std::find(ui.tabs.begin(), ui.tabs.end(), WifiQrTab::QR) != ui.tabs.end())
    ui.qr_view = wifi_qr_create_view(ui.panel);
  if (std::find(ui.tabs.begin(), ui.tabs.end(), WifiQrTab::DETAILS) != ui.tabs.end())
    ui.details_view = wifi_qr_create_view(ui.panel);
  if (ui.details_view) {
    lv_obj_set_flex_flow(ui.details_view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(ui.details_view, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_flex_cross_place(ui.details_view, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_row(ui.details_view, shell.layout.title_gap, LV_PART_MAIN);
    wifi_qr_create_detail_label(ui.details_view,
      espcontrol_i18n_key("network"), DARK_TEXT_MUTED, wifi_qr_heading_font_ref());
    wifi_qr_create_detail_label(
      ui.details_view, ssid.c_str(), DARK_TEXT_PRIMARY, wifi_qr_heading_font_ref());
    lv_obj_t *spacer = lv_obj_create(ui.details_view);
    if (spacer) {
      lv_obj_set_size(spacer, 1, std::max<lv_coord_t>(16, shell.layout.title_gap * 2));
      lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
      lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_all(spacer, 0, LV_PART_MAIN);
      lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
    }
    wifi_qr_create_detail_label(
      ui.details_view, espcontrol_i18n_key("password"), DARK_TEXT_MUTED,
      wifi_qr_heading_font_ref());
    wifi_qr_create_detail_label(
      ui.details_view,
      password.empty() ? espcontrol_i18n_key("none") : password.c_str(),
      DARK_TEXT_PRIMARY, wifi_qr_heading_font_ref());
  }

  ui.qr = ui.qr_view ? lv_qrcode_create(ui.qr_view) : nullptr;
  if (ui.qr) {
    lv_qrcode_set_dark_color(ui.qr, lv_color_black());
    lv_qrcode_set_light_color(ui.qr, lv_color_white());
    // The white border preserves the scanner quiet zone inside the roomy tab inset.
    lv_obj_set_style_border_color(ui.qr, lv_color_white(), LV_PART_MAIN);
  }

  wifi_qr_apply_tab_visibility();
  wifi_qr_layout_modal();

  if (!ui.qr && ui.qr_view) {
    lv_obj_t *message = wifi_qr_create_detail_label(ui.qr_view,
      espcontrol_i18n_key("unable_to_create_qr_code"), DARK_TEXT_PRIMARY);
    if (message) lv_obj_center(message);
  }
}
