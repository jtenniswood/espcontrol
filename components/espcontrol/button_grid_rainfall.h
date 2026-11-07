#pragma once

#include <memory>
#include "rainfall_display.h"

#include "button_grid_weather_reading.h"

inline void rainfall_layout(BtnSlot &slot, const DisplayProfile &display) {
  weather_reading_layout(slot, display);
}
inline void rainfall_setup(BtnSlot &slot, const DisplayProfile &display) {
  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.unit_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_display_text(slot.icon_lbl, find_icon("Weather Rainy"));
  lv_label_set_display_text(slot.text_lbl, espcontrol_i18n("Rainfall"));
  lv_label_set_display_text(slot.sensor_lbl, "--");
  rainfall_layout(slot, display);
}
inline void subscribe_rainfall_sensor(BtnSlot &slot, const std::string &entity,
                                     const std::string &unit_override = "") {
  struct State {
    lv_obj_t *label = nullptr;
    uint32_t generation = 0;
    double value = NAN;
    std::string unit;
    void render() const {
      if (generation != ha_subscription_generation() || !lv_obj_is_valid(label)) return;
      lv_label_set_display_text(label, espcontrol::rainfall::amount(value, unit).c_str());
      notify_dashboard_content_changed();
    }
  };
  auto state = std::make_shared<State>();
  state->label = slot.sensor_lbl;
  state->generation = ha_subscription_generation();
  state->unit = unit_override;
  ha_subscribe_state(entity, [state](esphome::StringRef value) {
    state->value = espcontrol::rainfall::value(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN));
    state->render();
  });
  if (unit_override.empty()) {
    ha_subscribe_attribute(entity, "unit_of_measurement", [state](esphome::StringRef unit) {
      state->unit = string_ref_limited(unit, HA_SHORT_STATE_MAX_LEN);
      state->render();
    });
  }
}
