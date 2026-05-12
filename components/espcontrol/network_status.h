// =============================================================================
// NETWORK STATUS - Clock-bar network icon and device information modal
// =============================================================================
#pragma once

#include <cmath>
#include <cstdio>
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
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *table = nullptr;
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

inline std::string network_status_wifi_strength_text(float pct) {
  if (!std::isfinite(pct)) return "Not available";
  if (pct < 0.0f) pct = 0.0f;
  if (pct > 100.0f) pct = 100.0f;
  char buf[12];
  snprintf(buf, sizeof(buf), "%d%%", (int)(pct + 0.5f));
  return buf;
}

inline std::string network_status_uptime_text(float seconds) {
  if (!std::isfinite(seconds) || seconds < 0.0f) return "Not available";
  uint32_t total = (uint32_t)(seconds + 0.5f);
  uint32_t days = total / 86400U;
  uint32_t hours = (total % 86400U) / 3600U;
  uint32_t minutes = (total % 3600U) / 60U;
  char buf[24];
  if (days > 0) {
    snprintf(buf, sizeof(buf), "%ud %02uh %02um", days, hours, minutes);
  } else if (hours > 0) {
    snprintf(buf, sizeof(buf), "%uh %02um", hours, minutes);
  } else {
    snprintf(buf, sizeof(buf), "%um", minutes);
  }
  return buf;
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
  ui.back_btn = nullptr;
  ui.table = nullptr;
}

inline lv_obj_t *network_status_add_table_label(lv_obj_t *parent,
                                                const char *text,
                                                const lv_font_t *font,
                                                lv_coord_t width,
                                                uint32_t color) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text ? text : "");
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, width);
  lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  return label;
}

inline void network_status_add_table_row(lv_obj_t *table,
                                         const char *name,
                                         const std::string &value,
                                         const lv_font_t *font,
                                         lv_coord_t row_w,
                                         lv_coord_t name_w,
                                         lv_coord_t value_w,
                                         lv_coord_t row_gap) {
  if (!table) return;

  lv_obj_t *row = lv_obj_create(table);
  network_status_clean_obj(row);
  lv_obj_set_width(row, row_w);
  lv_obj_set_height(row, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_column(row, row_gap, LV_PART_MAIN);
  lv_obj_set_layout(row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(row, LV_FLEX_ALIGN_START, LV_PART_MAIN);

  lv_obj_t *name_lbl = network_status_add_table_label(row, name, font, name_w, 0xA0A0A0);
  lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
  lv_obj_t *value_lbl = network_status_add_table_label(row, value.c_str(), font, value_w, 0xFFFFFF);
  lv_obj_set_style_text_align(value_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
}

inline void network_status_add_device_name(lv_obj_t *table,
                                           const std::string &device_name,
                                           const lv_font_t *font,
                                           lv_coord_t width) {
  if (!table) return;
  lv_obj_t *label = network_status_add_table_label(
    table,
    device_name.empty() ? "Not available" : device_name.c_str(),
    font,
    width,
    0xFFFFFF);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
}

inline void network_status_open_modal(const std::string &device_name,
                                      const std::string &ip_address,
                                      const std::string &wifi_strength,
                                      float uptime_seconds,
                                      const std::string &firmware_version,
                                      const lv_font_t *label_font,
                                      const lv_font_t *device_name_font,
                                      const lv_font_t *icon_font,
                                      const std::string &panel_color_hex) {
  media_volume_hide_modal();
  climate_control_hide_modal();
  switch_confirmation_hide_modal();
  network_status_hide_modal();
  NetworkStatusModalUi &ui = network_status_modal_ui();

  ControlModalLayout layout = control_modal_calc_layout(100);
  lv_coord_t radius = control_modal_card_radius(nullptr);
  lv_coord_t table_w = layout.panel_w - layout.inset * 2;
  if (table_w < 120) table_w = layout.panel_w;
  lv_coord_t row_gap = control_modal_scaled_px(18, layout.short_side);
  if (row_gap < 10) row_gap = 10;
  lv_coord_t name_w = table_w * 38 / 100;
  lv_coord_t value_w = table_w - name_w - row_gap;
  if (value_w < table_w / 2) value_w = table_w / 2;
  lv_coord_t table_top = layout.inset + layout.back_size +
    control_modal_scaled_px(12, layout.short_side);
  lv_coord_t table_h = layout.panel_h - table_top - layout.inset;
  if (table_h < 120) table_h = layout.panel_h - layout.inset * 2;

  ui.overlay = lv_obj_create(lv_layer_top());
  control_modal_style_overlay(ui.overlay);

  uint32_t panel_color = network_status_parse_color(panel_color_hex, 0x212121);
  ui.panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(ui.panel, panel_color, radius);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, radius);

  ui.back_btn = control_modal_create_round_button(ui.panel, 32, "\U000F0141",
    icon_font, 0x454545, panel_color, 100);
  lv_obj_set_style_bg_opa(ui.back_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.back_btn, 0, LV_PART_MAIN);
  lv_obj_t *back_label = lv_obj_get_child(ui.back_btn, 0);
  if (back_label) lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_add_event_cb(ui.back_btn, [](lv_event_t *) {
    network_status_hide_modal();
  }, LV_EVENT_CLICKED, nullptr);
  control_modal_apply_back_button_layout(ui.back_btn, layout);

  ui.table = lv_obj_create(ui.panel);
  network_status_clean_obj(ui.table);
  lv_obj_set_size(ui.table, table_w, table_h);
  lv_obj_set_style_pad_row(ui.table, control_modal_scaled_px(14, layout.short_side), LV_PART_MAIN);
  lv_obj_set_layout(ui.table, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.table, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_align(ui.table, LV_ALIGN_TOP_MID, 0, table_top);

  std::string uptime = network_status_uptime_text(uptime_seconds);
  std::string firmware = firmware_version.empty() ? "Not available" : firmware_version;
  network_status_add_device_name(ui.table, device_name,
    device_name_font ? device_name_font : label_font, table_w);
  network_status_add_table_row(ui.table, "IP address", ip_address,
    label_font, table_w, name_w, value_w, row_gap);
  network_status_add_table_row(ui.table, "WiFi strength", wifi_strength,
    label_font, table_w, name_w, value_w, row_gap);
  network_status_add_table_row(ui.table, "Uptime", uptime,
    label_font, table_w, name_w, value_w, row_gap);
  network_status_add_table_row(ui.table, "Firmware version", firmware,
    label_font, table_w, name_w, value_w, row_gap);

  lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
}
