#pragma once

#include "button_grid_weather_reading.h"

// Shared main-grid/subpage lifecycle. The grid releases tracked allocations
// before rebinding; FlightsCardCtx releases its subscriptions.
template<typename T>
inline T *grid_track_runtime_allocation(lv_obj_t *owner, T *ptr);

namespace espcontrol::cards {
inline bool flights_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::FLIGHTS;
}

inline std::string flights_driver_configured_title(const ParsedCfg &config) {
  if (config.label.empty() && cfg_option_value(config.options, "title_source") == "sensor") return {};
  const auto area = cfg_option_value(config.options, "area");
  const char *key = area == "entered" ? "flights_entered_area"
    : area == "exited" ? "flights_exited_area" : "flights_current_in_area";
  return config.label.empty() || config.label == espcontrol_i18n_key_en(key)
    ? espcontrol_i18n_key(key) : config.label;
}

inline bool flights_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!flights_driver_matches(context)) return false;
  lv_obj_set_user_data(slot.btn, nullptr);
  const auto area = cfg_option_value(config.options, "area");
  const char *icon = "Travel";
  if (area == "entered") icon = "Connecting Airports";
  if (area == "exited") icon = "Airplane Mode Inactive";
  if (!config.icon.empty() && config.icon != "Auto" &&
      cfg_option_value(config.options, "icon_source") == "custom") icon = config.icon.c_str();
  lv_label_set_display_text(slot.icon_lbl, find_icon(icon));
  lv_label_set_display_text(slot.text_lbl, "--");
  lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);

  // A separate label can ellipsize long sensor names without changing the
  // geometry of the numeric labels reused by other card types.
  lv_obj_t *name = lv_label_create(slot.btn);
  lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
  lv_obj_set_width(name, lv_pct(100));
  const std::string title = flights_driver_configured_title(config);
  lv_label_set_display_text(name, title.empty() ? espcontrol_i18n("Flights") : title.c_str());
  apply_text_width_compensation(name);
  lv_obj_set_user_data(slot.sensor_container, name);
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  return true;
}

inline bool flights_driver_refresh_layout(
    BtnSlot &slot, const Context &context, const DisplayProfile &display) {
  if (!flights_driver_matches(context)) return false;
  auto *name = static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container));
  if (name) {
    BtnSlot reading = slot;
    reading.sensor_lbl = name;
    reading.sensor_container = name;
    weather_reading_layout(reading, display);
    const auto *font = lv_obj_get_style_text_font(name, LV_PART_MAIN);
    lv_obj_set_style_max_height(name,
      2 * font->line_height + lv_obj_get_style_text_line_space(name, LV_PART_MAIN),
      LV_PART_MAIN);
  }
  return true;
}

inline bool flights_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!flights_driver_matches(context)) return false;
  lv_obj_set_user_data(slot.btn, nullptr);
  if (config.entity.compare(0, 7, "sensor.") != 0) return true;
  auto *flights = grid_track_runtime_allocation(slot.btn, new FlightsCardCtx());
  flights->entity_id = config.entity;
  flights->configured_label = flights_driver_configured_title(config);
  flights->count_lbl = slot.text_lbl;
  flights->name_lbl = static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container));
  subscribe_flights_card(flights);
  return true;
}

}  // namespace espcontrol::cards
