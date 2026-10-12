#pragma once

// Shared lifecycle driver for current conditions and all forecast views.
// Forecast cards share one bounded source per entity and the same widget layout.

#include "button_grid_weather_daily_ui.h"
#include "card_modal_target.h"

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

inline void weather_driver_apply_background(
    BtnSlot &slot, const CardPalette &palette) {
  if (!palette.has_sensor_color) return;
  lv_obj_set_style_bg_color(
    slot.btn, lv_color_hex(palette.surface_sensor_val),
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
    lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
    weather_daily_setup(slot, config, display);
    return true;
  }

  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_display_text(slot.icon_lbl, find_icon("Weather Cloudy"));
  lv_label_set_display_text(slot.sensor_lbl, "");
  lv_label_set_display_text(slot.unit_lbl, "");
  lv_label_set_display_text(slot.text_lbl, espcontrol_i18n("Cloudy"));
  return true;
}

inline bool weather_driver_attach_interaction(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!weather_driver_matches(context)) return false;
  if (weather_driver_shows_forecast(config) && weather_daily_card_for(slot.btn)) {
    lv_obj_add_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
    if (context.surface == Surface::SUBPAGE)
      lv_obj_add_event_cb(slot.btn, weather_daily_card_clicked, LV_EVENT_CLICKED, nullptr);
  }
  else lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
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
  (void) display; (void) row_span; (void) col_span;
  if (weather_driver_shows_forecast(config))
    weather_daily_render_card(weather_daily_card_for(slot.btn));
  return true;
}

inline bool weather_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  weather_daily_cleanup(slot.btn);
  // Forecast requests and visual references are reset centrally before each
  // grid rebuild. Current-weather cards own no dynamic allocation.
  return weather_driver_matches(context);
}

inline bool weather_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  if (!weather_driver_matches(context)) return false;
  if (weather_driver_shows_forecast(config)) weather_daily_open_modal(weather_daily_card_for(button));
  return true;
}

inline ModalTarget weather_driver_modal_target(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  if (!weather_driver_matches(context) || !weather_driver_shows_forecast(config)) return {};
  ModalTarget target;
  target.entity = config.entity; target.kind = ControlModalKind::WEATHER_FORECAST;
  auto *card = weather_daily_card_for(button);
  target.available = card && card->source;
  if (card) target.open = [card]() { weather_daily_open_modal(card); };
  return target;
}

}  // namespace espcontrol::cards
