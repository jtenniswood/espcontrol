#pragma once

#include <memory>
#include "button_grid_weather_reading.h"
#include "weather_reading_display.h"

inline void weather_metric_setup(BtnSlot &slot, const std::string &mode,
                                 const DisplayProfile &display) {
  const char *label = mode == "temperature" ? "Currently" : mode == "pressure" ? "Air Pressure" : mode == "humidity" ? "Humidity" : "Visibility";
  const char *icon = mode == "temperature" ? "Thermometer" : mode == "pressure" ? "Gauge" : mode == "humidity" ? "Water Percent" : "Weather Hazy";
  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.unit_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_display_text(slot.icon_lbl, find_icon(icon));
  lv_label_set_display_text(slot.text_lbl, espcontrol_i18n(label));
  lv_label_set_display_text(slot.sensor_lbl, "--");
  weather_reading_layout(slot, display);
}

inline void subscribe_weather_metric(BtnSlot &slot, const std::string &entity,
                                     const std::string &mode, bool forecast) {
  if (entity.empty()) return;
  struct State {
    lv_obj_t *label = nullptr;
    uint32_t generation = 0;
    double value = NAN;
    std::string unit;
    bool available = true;
    bool humidity = false;
    bool temperature = false;
    void render() const {
      if (generation != ha_subscription_generation() || !lv_obj_is_valid(label)) return;
      const double reading = available && (!humidity || value <= 100) ? value : NAN;
      lv_label_set_display_text(label, espcontrol::weather_reading::amount(reading, unit, temperature).c_str());
      notify_dashboard_content_changed();
    }
  };
  auto state = std::make_shared<State>();
  state->label = slot.sensor_lbl;
  state->generation = ha_subscription_generation();
  state->humidity = mode == "humidity";
  state->temperature = mode == "temperature";
  if (state->humidity) state->unit = "%";
  const auto value_changed = [state](esphome::StringRef value) {
    state->value = espcontrol::weather_reading::value(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN));
    state->render();
  };
  const auto unit_changed = [state](esphome::StringRef unit) {
    state->unit = string_ref_limited(unit, HA_SHORT_STATE_MAX_LEN);
    state->render();
  };
  if (!forecast) {
    ha_subscribe_state(entity, value_changed);
    ha_subscribe_attribute(entity, "unit_of_measurement", unit_changed);
    return;
  }
  // The Forecast source uses the provider's live weather attributes, as Wind
  // does. Visibility is not part of Home Assistant's daily forecast payload.
  ha_subscribe_state(entity, [state](esphome::StringRef value) {
    const auto text = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
    state->available = !text.empty() && text != "unknown" && text != "unavailable";
    state->render();
  });
  ha_subscribe_attribute(entity, mode, value_changed);
  if (!state->humidity)
    ha_subscribe_attribute(entity, mode + "_unit", unit_changed);
}
