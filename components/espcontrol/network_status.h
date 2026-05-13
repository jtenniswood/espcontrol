// =============================================================================
// NETWORK STATUS - Clock-bar network icon and device information modal
// =============================================================================
#pragma once

#include <cmath>
#include <cstdlib>
#include <string>
#include "esphome/components/network/ip_address.h"
#include "esphome/components/network/util.h"

constexpr const char *NETWORK_ICON_WIFI_1 = "\U000F091F";
constexpr const char *NETWORK_ICON_WIFI_2 = "\U000F0922";
constexpr const char *NETWORK_ICON_WIFI_3 = "\U000F0925";
constexpr const char *NETWORK_ICON_WIFI_4 = "\U000F0928";
constexpr const char *NETWORK_ICON_ETHERNET = "\U000F0200";

struct NetworkStatusModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *content = nullptr;
  lv_obj_t *device_name_lbl = nullptr;
  lv_obj_t *ip_lbl = nullptr;
  lv_obj_t *firmware_lbl = nullptr;
};

inline NetworkStatusModalUi &network_status_modal_ui() {
  static NetworkStatusModalUi ui;
  return ui;
}

inline uint32_t network_status_parse_color(const std::string &hex, uint32_t fallback) {
  if (hex.length() != 6) return fallback;
  char *end = nullptr;
  uint32_t value = strtoul(hex.c_str(), &end, 16);
  return end && *end == '\0' ? value : fallback;
}

inline const char *network_status_wifi_icon(float pct) {
  if (!std::isfinite(pct) || pct < 25.0f) return NETWORK_ICON_WIFI_1;
  if (pct < 50.0f) return NETWORK_ICON_WIFI_2;
  if (pct < 75.0f) return NETWORK_ICON_WIFI_3;
  return NETWORK_ICON_WIFI_4;
}

inline void network_status_set_wifi_icon(lv_obj_t *label, float pct) {
  if (!label) return;
  lv_label_set_text(label, network_status_wifi_icon(pct));
}

inline void network_status_set_ethernet_icon(lv_obj_t *label) {
  if (!label) return;
  lv_label_set_text(label, NETWORK_ICON_ETHERNET);
}

inline void network_status_update_visibility(lv_obj_t *button, lv_obj_t *main_page_obj,
                                             bool clock_bar_enabled,
                                             bool network_status_enabled) {
  if (!button) return;
  if (clock_bar_enabled && network_status_enabled && lv_scr_act() == main_page_obj) {
    lv_obj_clear_flag(button, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(button, LV_OBJ_FLAG_HIDDEN);
  }
}

inline std::string network_status_ip_address() {
  auto ips = esphome::network::get_ip_addresses();
  if (!ips.empty()) {
    char ip_buf[esphome::network::IP_ADDRESS_BUFFER_SIZE];
    ips[0].str_to(ip_buf);
    return ip_buf;
  }
  return "Not available";
}

inline std::string network_status_trim_copy(const std::string &value) {
  const size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

inline std::string network_status_firmware_label(const std::string &version) {
  std::string trimmed = network_status_trim_copy(version);
  if (trimmed.empty()) return "Version unavailable";
  if (trimmed == "dev" || trimmed == "Dev" || trimmed == "0.0.0") return "Dev build";
  return trimmed;
}

inline void network_status_clean_obj(lv_obj_t *obj) {
  if (!obj) return;
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

inline void network_status_hide_modal() {
  NetworkStatusModalUi &ui = network_status_modal_ui();
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui.overlay = nullptr;
  ui.panel = nullptr;
  ui.close_btn = nullptr;
  ui.content = nullptr;
  ui.device_name_lbl = nullptr;
  ui.ip_lbl = nullptr;
  ui.firmware_lbl = nullptr;
}

inline lv_obj_t *network_status_add_center_label(lv_obj_t *parent,
                                                 const char *text,
                                                 const lv_font_t *font,
                                                 lv_coord_t width,
                                                 uint32_t color) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text ? text : "");
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, width);
  lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  return label;
}

inline void network_status_open_modal(const std::string &device_name,
                                      const std::string &ip_address,
                                      const std::string &firmware_version,
                                      const lv_font_t *text_font,
                                      const lv_font_t *icon_font,
                                      const std::string &panel_color_hex) {
  media_volume_hide_modal();
  climate_control_hide_modal();
  switch_confirmation_hide_modal();
  network_status_hide_modal();
  NetworkStatusModalUi &ui = network_status_modal_ui();

  ControlModalLayout layout = control_modal_calc_layout(100);
  lv_coord_t radius = control_modal_card_radius(nullptr);
  lv_coord_t content_w = layout.panel_w - layout.inset * 2;
  if (content_w < 120) content_w = layout.panel_w;

  ui.overlay = lv_obj_create(lv_layer_top());
  control_modal_style_overlay(ui.overlay);

  uint32_t panel_color = network_status_parse_color(panel_color_hex, 0x212121);
  ui.panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(ui.panel, panel_color, radius);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, radius);

  ui.close_btn = control_modal_create_round_button(ui.panel, 32, "\U000F0156",
    icon_font, 0x454545, panel_color, 100);
  lv_obj_set_style_bg_opa(ui.close_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.close_btn, 0, LV_PART_MAIN);
  lv_obj_t *close_label = lv_obj_get_child(ui.close_btn, 0);
  if (close_label) lv_obj_set_style_text_color(close_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_add_event_cb(ui.close_btn, [](lv_event_t *) {
    network_status_hide_modal();
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_set_size(ui.close_btn, layout.back_size, layout.back_size);
  lv_obj_set_style_radius(ui.close_btn, layout.back_size / 2, LV_PART_MAIN);
  lv_obj_align(ui.close_btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);

  ui.content = lv_obj_create(ui.panel);
  network_status_clean_obj(ui.content);
  lv_obj_set_width(ui.content, content_w);
  lv_obj_set_height(ui.content, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_row(ui.content, control_modal_scaled_px(12, layout.short_side), LV_PART_MAIN);
  lv_obj_set_layout(ui.content, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.content, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.content, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.content, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  ui.device_name_lbl = network_status_add_center_label(
    ui.content,
    device_name.empty() ? "Not available" : device_name.c_str(),
    text_font,
    content_w,
    0xFFFFFF);
  ui.ip_lbl = network_status_add_center_label(
    ui.content,
    ip_address.empty() ? "Not available" : ip_address.c_str(),
    text_font,
    content_w,
    0xA0A0A0);
  std::string firmware_label = network_status_firmware_label(firmware_version);
  ui.firmware_lbl = network_status_add_center_label(
    ui.content,
    firmware_label.c_str(),
    text_font,
    content_w,
    0xA0A0A0);
  lv_obj_update_layout(ui.content);
  lv_obj_align(ui.content, LV_ALIGN_CENTER, 0, 0);

  lv_obj_move_foreground(ui.close_btn);
  lv_obj_move_foreground(ui.overlay);
}
