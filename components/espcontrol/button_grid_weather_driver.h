#pragma once

// Shared lifecycle driver for current-weather cards and the supported forecast
// compatibility form. Forecast data keeps its specialised request/registry
// implementation while sharing visual, interaction, layout, and binding wiring.

namespace espcontrol::cards {

inline bool weather_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::WEATHER;
}

inline bool weather_driver_shows_forecast(const ParsedCfg &config) {
  return card_runtime_weather_forecast_supported() &&
    (config.type == "weather_forecast" ||
     (config.type == "weather" &&
      card_runtime_weather_forecast_precision(config.precision)));
}

inline std::string weather_driver_forecast_day(const ParsedCfg &config) {
  return config.precision == "today" ? "today" : "tomorrow";
}

inline void weather_driver_apply_background(
    BtnSlot &slot, const CardPalette &palette) {
  if (!palette.has_sensor_color) return;
  lv_obj_set_style_bg_color(
    slot.btn, lv_color_hex(palette.sensor_val),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
      static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
}

inline bool weather_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display) {
  if (!weather_driver_matches(context)) return false;
  weather_driver_apply_background(slot, palette);
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);

  if (weather_driver_shows_forecast(config)) {
    lv_obj_add_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(slot.sensor_lbl, "--/--");
    lv_label_set_text(slot.unit_lbl, display_temperature_unit_symbol());
    const std::string day = weather_driver_forecast_day(config);
    const std::string label = config.label.empty()
      ? (day == "today"
          ? espcontrol_i18n(std::string("Today"))
          : espcontrol_i18n(std::string("Tomorrow")))
      : config.label;
    lv_label_set_text(slot.text_lbl, label.c_str());
    apply_width_compensation(
      slot.sensor_container, display_main_width_percent(display));
    apply_width_compensation(
      slot.text_lbl, display_main_width_percent(display));
    register_weather_forecast_card(
      slot.btn, slot.sensor_lbl, slot.unit_lbl, slot.text_lbl,
      config.entity, day, config.label);
    return true;
  }

  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(slot.icon_lbl, find_icon("Weather Cloudy"));
  lv_label_set_text(slot.sensor_lbl, "");
  lv_label_set_text(slot.unit_lbl, "");
  lv_label_set_text(slot.text_lbl, espcontrol_i18n("Cloudy"));
  return true;
}

inline bool weather_driver_attach_interaction(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!weather_driver_matches(context)) return false;
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  return true;
}

inline bool weather_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!weather_driver_matches(context)) return false;
  if (weather_driver_shows_forecast(config)) return true;
  if (!config.entity.empty()) {
    subscribe_weather_state(slot.icon_lbl, slot.text_lbl, config.entity);
  }
  return true;
}

inline bool weather_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const DisplayProfile &display, int row_span, int col_span) {
  if (!weather_driver_matches(context)) return false;
  if (weather_driver_shows_forecast(config) &&
      large_number_square_card_layout(row_span, col_span) &&
      card_large_numbers_active_for_layout(config, row_span, col_span) &&
      display_large_sensor_font(display)) {
    apply_large_sensor_number_style(
      slot, display_large_sensor_font(display),
      display_large_sensor_unit_offset_percent(display));
  }
  return true;
}

inline bool weather_driver_cleanup(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  // Forecast requests and visual references are reset centrally before each
  // grid rebuild. Current-weather cards own no dynamic allocation.
  return weather_driver_matches(context);
}

}  // namespace espcontrol::cards
