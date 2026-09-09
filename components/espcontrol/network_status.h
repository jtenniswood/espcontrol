// =============================================================================
// NETWORK STATUS - Clock-bar network icon and card-grid Settings page
// =============================================================================
#pragma once

#include "button_grid_limits.h"
#include "display_text.h"
#include "settings_backlight.h"
#include "network_status_layout.h"

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
  lv_obj_t *ip_lbl = nullptr;
  lv_obj_t *brightness_slider = nullptr;
  lv_obj_t *brightness_label = nullptr;
  lv_obj_t *brightness_icon = nullptr;
  SliderCtx brightness_context;
  bool brightness_dragging = false;
  SettingsBacklightLevel brightness_level = SettingsBacklightLevel::MANUAL;
  lv_timer_t *refresh_timer = nullptr;
  lv_coord_t columns[MAX_GRID_SLOTS + 1]{};
  lv_coord_t rows[MAX_GRID_SLOTS + 1]{};
};

inline const lv_font_t *&network_status_card_icon_font() {
  static const lv_font_t *font = nullptr;
  return font;
}

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

inline void network_status_set_wifi_icon(lv_obj_t *label, float pct,
                                         bool connected) {
  if (!label) return;
  if (!connected) {
    lv_label_set_display_text(label, NETWORK_ICON_WIFI_OFF_OUTLINE);
    return;
  }
  lv_label_set_display_text(label, network_status_wifi_icon(pct));
}

inline void network_status_set_ethernet_icon(lv_obj_t *label) {
  if (!label) return;
  lv_label_set_display_text(label, NETWORK_ICON_ETHERNET);
}

inline void network_status_update_visibility(lv_obj_t *button,
                                             lv_obj_t *main_page_obj,
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

inline bool network_status_is_specific_firmware_version(
    const std::string &version) {
  std::string trimmed = network_status_trim_copy(version);
  const size_t len = trimmed.size();
  if (len < 6 || (trimmed[0] != 'v' && trimmed[0] != 'V')) return false;

  size_t pos = 1;
  auto read_number = [&]() -> bool {
    if (pos >= len ||
        !std::isdigit(static_cast<unsigned char>(trimmed[pos]))) return false;
    while (pos < len &&
           std::isdigit(static_cast<unsigned char>(trimmed[pos]))) ++pos;
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
    if (!std::isalnum(c) && trimmed[pos] != '.' && trimmed[pos] != '-') {
      return false;
    }
    ++pos;
  }
  return true;
}

inline std::string network_status_firmware_label(const std::string &version) {
  std::string trimmed = network_status_trim_copy(version);
  if (trimmed.empty() || trimmed == "Version unknown") {
    return espcontrol_i18n(std::string("Version unknown"));
  }
  if (network_status_is_specific_firmware_version(trimmed)) return trimmed;
  return espcontrol_i18n(std::string("Dev build"));
}

inline void network_status_hide_modal() {
  NetworkStatusModalUi &ui = network_status_modal_ui();
  if (ui.refresh_timer) lv_timer_del(ui.refresh_timer);
  lv_obj_t *overlay = ui.overlay;
  ui = NetworkStatusModalUi{};
  control_modal_clear_active(ControlModalKind::NETWORK_STATUS);
  set_clock_bar_subpage_label("");
  if (overlay) {
    screen_lock_unregister_tree(overlay);
    lv_obj_del(overlay);
    lv_obj_invalidate(lv_layer_top());
  }
}

inline void network_status_refresh_backlight() {
  auto &ui = network_status_modal_ui();
  auto &service = settings_backlight_service();
  if (!ui.brightness_slider) return;

  const auto state = service.read ? service.read() : SettingsBacklightState{};
  const bool enabled =
      state.available && service.write && !screen_lock_enabled();
  ui.brightness_context.available = state.available;
  ui.brightness_context.interactive = enabled;
  if (enabled) lv_obj_add_flag(ui.brightness_slider, LV_OBJ_FLAG_CLICKABLE);
  else lv_obj_clear_flag(ui.brightness_slider, LV_OBJ_FLAG_CLICKABLE);

  if (!ui.brightness_dragging) {
    ui.brightness_level = state.level;
    lv_slider_set_value(
        ui.brightness_slider,
        settings_backlight_percent(state.level, state.percent), LV_ANIM_OFF);
  }

  lv_label_set_display_text(ui.brightness_label,
                            espcontrol_i18n("Backlight"));
  lv_label_set_display_text(
      ui.brightness_icon,
      ui.brightness_level == SettingsBacklightLevel::NIGHTTIME
          ? "\U000F1A4D"
          : "\U000F0336");

  auto *button = lv_obj_get_parent(ui.brightness_slider);
  lv_obj_set_width(ui.brightness_label,
                   lv_obj_get_width(button) -
                       ui.brightness_context.label_pad_left * 2);
  slider_fit_to_button(ui.brightness_slider, button, false);
  slider_update_fill(ui.brightness_context.fill, button,
                     lv_slider_get_value(ui.brightness_slider), false, false,
                     ui.brightness_context.radius);
}

inline void network_status_refresh_page() {
  auto &ui = network_status_modal_ui();
  if (!ui.overlay) return;
  if (ui.ip_lbl) {
    const std::string ip_address = network_status_ip_address();
    lv_label_set_display_text(ui.ip_lbl, ip_address.c_str());
  }
  network_status_refresh_backlight();
}

inline void network_status_backlight_event(lv_event_t *event) {
  auto &ui = network_status_modal_ui();
  const auto code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED) {
    network_status_refresh_backlight();
    if (!ui.brightness_context.interactive) return;
    ui.brightness_dragging = true;
    slider_apply_vertical_pointer_value(ui.brightness_slider);
  } else if (code == LV_EVENT_PRESSING) {
    if (ui.brightness_dragging) {
      slider_apply_vertical_pointer_value(ui.brightness_slider);
    }
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    if (!ui.brightness_dragging) return;
    lv_slider_set_value(
        ui.brightness_slider,
        settings_backlight_percent(ui.brightness_level,
                                   lv_slider_get_value(ui.brightness_slider)),
        LV_ANIM_OFF);
    network_status_refresh_backlight();
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    settings_backlight_commit(
        ui.brightness_level, lv_slider_get_value(ui.brightness_slider),
        code == LV_EVENT_RELEASED && ui.brightness_dragging &&
            !screen_lock_enabled());
    ui.brightness_dragging = false;
    network_status_refresh_backlight();
  }
}

inline void network_status_open_modal(const std::string &device_name,
                                      const std::string &ip_address,
                                      const std::string &firmware_version,
                                      const lv_font_t *text_font,
                                      const lv_font_t *icon_font) {
  (void) device_name;
  network_status_hide_modal();

  auto &metrics = control_modal_grid_metrics();
  lv_obj_t *page = metrics.page ? metrics.page : lv_scr_act();
  lv_obj_t *reference = metrics.first_card;
  if (!page) return;

  auto &ui = network_status_modal_ui();
  const lv_font_t *label_font = reference
                                    ? lv_obj_get_style_text_font(reference,
                                                                 LV_PART_MAIN)
                                    : text_font;
  const lv_font_t *card_icon_font = network_status_card_icon_font();
  if (!card_icon_font) card_icon_font = icon_font;
  const lv_color_t text_color = reference
                                    ? lv_obj_get_style_text_color(reference,
                                                                  LV_PART_MAIN)
                                    : lv_color_hex(DARK_TEXT_PRIMARY);
  const lv_coord_t radius = control_modal_card_radius(reference);
  const lv_coord_t card_pad = reference
                                  ? lv_obj_get_style_pad_top(reference,
                                                             LV_PART_MAIN)
                                  : 8;

  ui.overlay = lv_obj_create(lv_layer_top());
  lv_obj_update_layout(page);
  const lv_coord_t top = lv_obj_get_style_pad_top(page, LV_PART_MAIN);
  lv_obj_set_size(ui.overlay, lv_pct(100), lv_obj_get_height(page) - top);
  lv_obj_set_pos(ui.overlay, 0, top);
  lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(
      ui.overlay, lv_obj_get_style_bg_color(page, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_pad_top(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(
      ui.overlay, lv_obj_get_style_pad_bottom(page, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_pad_left(
      ui.overlay, lv_obj_get_style_pad_left(page, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_pad_right(
      ui.overlay, lv_obj_get_style_pad_right(page, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_pad_row(
      ui.overlay, lv_obj_get_style_pad_row(page, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_pad_column(
      ui.overlay, lv_obj_get_style_pad_column(page, LV_PART_MAIN), LV_PART_MAIN);

  const int cols = std::max(1, std::min(metrics.cols, MAX_GRID_SLOTS));
  const int rows = std::max(1, std::min(metrics.rows, MAX_GRID_SLOTS));
  for (int i = 0; i < cols; ++i) ui.columns[i] = LV_GRID_FR(1);
  for (int i = 0; i < rows; ++i) ui.rows[i] = LV_GRID_FR(1);
  ui.columns[cols] = LV_GRID_TEMPLATE_LAST;
  ui.rows[rows] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_layout(ui.overlay, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(ui.overlay, ui.columns, ui.rows);

  const char *labels[] = {espcontrol_i18n("Back"), ip_address.c_str(), "", ""};
  const char *icons[] = {"\U000F0141", "\U000F0200", "\U000F035B", "\U000F0336"};

  for (int i = 0; i < 4; ++i) {
    auto *button = create_grid_card_button(ui.overlay, radius, card_pad,
                                           label_font, text_color);
    apply_button_colors(button, false, DEFAULT_SLIDER_COLOR, true,
                        DEFAULT_OFF_COLOR);
    // Follow the device's normal card order and let its column count determine
    // where the next row begins.
    const NetworkStatusGridCell cell = network_status_grid_cell(i, cols);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_STRETCH, cell.column, 1,
                         LV_GRID_ALIGN_STRETCH, cell.row, 1);
    BtnSlot slot = create_dynamic_card_slot(button, card_icon_font, label_font,
                                            label_font, text_color);
    apply_width_compensation(slot.icon_lbl,
                             icon_width_compensation_percent());
    apply_text_width_compensation(slot.text_lbl);
    lv_label_set_display_text(slot.icon_lbl, icons[i]);
    lv_label_set_display_text(slot.text_lbl, labels[i]);

    if (i == NETWORK_STATUS_BACK_CARD_INDEX) {
      lv_obj_add_event_cb(
          button, [](lv_event_t *) { network_status_hide_modal(); },
          LV_EVENT_CLICKED, nullptr);
      continue;
    }
    if (i == NETWORK_STATUS_BACKLIGHT_CARD_INDEX) {
      const CardPadding padding = capture_card_padding(button);
      ui.brightness_slider =
          setup_slider_widget(button, DEFAULT_SLIDER_COLOR, false);
      if (!ui.brightness_slider) continue;
      lv_slider_set_range(ui.brightness_slider,
                          SETTINGS_BACKLIGHT_MIN_PERCENT, 100);
      ui.brightness_label = slot.text_lbl;
      ui.brightness_icon = slot.icon_lbl;
      lv_obj_align(slot.icon_lbl, LV_ALIGN_TOP_LEFT, padding.left, padding.top);
      lv_obj_align(slot.text_lbl, LV_ALIGN_BOTTOM_LEFT, padding.left,
                   -padding.bottom);
      ui.brightness_context.label_pad_left = padding.left;
      ui.brightness_context.fill = lv_obj_get_child(button, 0);
      ui.brightness_context.radius = radius;
      lv_obj_set_user_data(ui.brightness_slider, &ui.brightness_context);
      lv_obj_add_event_cb(ui.brightness_slider,
                          network_status_backlight_event, LV_EVENT_ALL,
                          nullptr);
      screen_lock_register_controlled_button(ui.brightness_slider);
      continue;
    }

    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
    if (i == NETWORK_STATUS_IP_CARD_INDEX) {
      ui.ip_lbl = slot.text_lbl;
    } else if (i == NETWORK_STATUS_BUILD_CARD_INDEX) {
      const std::string build_label =
          network_status_firmware_label(firmware_version);
      lv_label_set_display_text(slot.text_lbl, build_label.c_str());
    }
  }

  control_modal_set_active(ControlModalKind::NETWORK_STATUS, ui.overlay,
                           network_status_hide_modal,
                           ControlModalDismissPolicy::DISMISS);
  lv_obj_update_layout(ui.overlay);
  network_status_refresh_page();
  ui.refresh_timer = lv_timer_create(
      [](lv_timer_t *) { network_status_refresh_page(); }, 1000, nullptr);
  lv_obj_move_foreground(ui.overlay);
  set_clock_bar_subpage_label(espcontrol_i18n(std::string("Settings")));
}
