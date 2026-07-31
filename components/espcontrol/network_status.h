// =============================================================================
// NETWORK STATUS - Clock-bar network icon and device information modal
// =============================================================================
#pragma once

#include <cmath>
#include <cstdlib>
#include <string>
#include "esphome/components/network/ip_address.h"
#include "esphome/components/network/util.h"
#include "i18n_generated.h"

constexpr const char *NETWORK_ICON_WIFI_OUTLINE = "\U000F092F";
constexpr const char *NETWORK_ICON_WIFI_1 = "\U000F091F";
constexpr const char *NETWORK_ICON_WIFI_2 = "\U000F0922";
constexpr const char *NETWORK_ICON_WIFI_3 = "\U000F0925";
constexpr const char *NETWORK_ICON_WIFI_4 = "\U000F0928";
constexpr const char *NETWORK_ICON_WIFI_OFF_OUTLINE = "\U000F092E";
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

inline const char *network_status_wifi_icon(float pct) {
  if (!std::isfinite(pct) || pct <= 0.0f) return NETWORK_ICON_WIFI_OUTLINE;
  if (pct < 25.0f) return NETWORK_ICON_WIFI_1;
  if (pct < 50.0f) return NETWORK_ICON_WIFI_2;
  if (pct < 75.0f) return NETWORK_ICON_WIFI_3;
  return NETWORK_ICON_WIFI_4;
}

inline void network_status_set_wifi_icon(lv_obj_t *label, float pct, bool connected) {
  if (!label) return;
  if (!connected) {
    lv_label_set_text(label, NETWORK_ICON_WIFI_OFF_OUTLINE);
    return;
  }
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
  if (clock_bar_enabled && network_status_enabled &&
      clock_bar_active_on_button_grid_page(main_page_obj)) {
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
  return espcontrol_i18n(std::string("Not available"));
}

inline std::string network_status_trim_copy(const std::string &value) {
  const size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

inline bool network_status_is_specific_firmware_version(const std::string &version) {
  std::string trimmed = network_status_trim_copy(version);
  const size_t len = trimmed.size();
  if (len < 6 || (trimmed[0] != 'v' && trimmed[0] != 'V')) return false;

  size_t pos = 1;
  auto read_number = [&]() -> bool {
    if (pos >= len || !std::isdigit(static_cast<unsigned char>(trimmed[pos]))) return false;
    while (pos < len && std::isdigit(static_cast<unsigned char>(trimmed[pos]))) ++pos;
    return true;
  };

  if (!read_number()) return false;
  for (int part = 0; part < 2; ++part) {
    if (pos >= len || trimmed[pos] != '.') return false;
    ++pos;
    if (!read_number()) return false;
  }
  if (pos == len) return true;
  if (trimmed[pos] != '-' && trimmed[pos] != '+') return false;
  ++pos;
  if (pos == len) return false;
  while (pos < len) {
    unsigned char c = static_cast<unsigned char>(trimmed[pos]);
    if (!std::isalnum(c) && trimmed[pos] != '.' && trimmed[pos] != '-') return false;
    ++pos;
  }
  return true;
}

inline std::string network_status_firmware_label(const std::string &version) {
  std::string trimmed = network_status_trim_copy(version);
  if (trimmed.empty()) return espcontrol_i18n(std::string("Version unknown"));
  if (trimmed == "Version unknown") return espcontrol_i18n(std::string("Version unknown"));
  if (network_status_is_specific_firmware_version(trimmed)) return trimmed;
  return espcontrol_i18n(std::string("Dev build"));
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
  control_modal_clear_active(ControlModalKind::NETWORK_STATUS);
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
                                      const lv_font_t *icon_font) {
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::NETWORK_STATUS, nullptr, 100, icon_font,
    network_status_hide_modal);
  NetworkStatusModalUi &ui = network_status_modal_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;

  ControlModalLayout &layout = shell.layout;
  lv_coord_t content_w = shell.content_w;
  lv_obj_t *close_label = lv_obj_get_child(ui.close_btn, 0);
  if (close_label) lv_obj_set_style_text_color(close_label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);

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
    device_name.empty() ? espcontrol_i18n("Not available") : device_name.c_str(),
    text_font,
    content_w,
    DARK_TEXT_PRIMARY);
  ui.ip_lbl = network_status_add_center_label(
    ui.content,
    ip_address.empty() ? espcontrol_i18n("Not available") : ip_address.c_str(),
    text_font,
    content_w,
    DARK_TEXT_MUTED);
  std::string firmware_label = network_status_firmware_label(firmware_version);
  ui.firmware_lbl = network_status_add_center_label(
    ui.content,
    firmware_label.c_str(),
    text_font,
    content_w,
    DARK_TEXT_MUTED);
  lv_obj_update_layout(ui.content);
  lv_obj_align(ui.content, LV_ALIGN_CENTER, 0, 0);

  lv_obj_move_foreground(ui.close_btn);
  lv_obj_move_foreground(ui.overlay);
}
