#pragma once

#include <memory>
#include "wind_direction.h"

inline void weather_wind_layout(BtnSlot &slot, const DisplayProfile &display) {
  capture_card_padding(slot.btn);
  lv_obj_align(slot.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  const lv_font_t *font = display_small_text_font(
    display, lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN));
  lv_obj_set_style_text_font(slot.sensor_lbl, font, LV_PART_MAIN);
  // Match forecast temperatures: card foreground blended at 70% opacity.
  lv_obj_set_style_text_color(slot.sensor_lbl,
    lv_obj_get_style_text_color(slot.btn, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_text_opa(slot.sensor_lbl, LV_OPA_70, LV_PART_MAIN);
  lv_obj_add_flag(slot.unit_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align(slot.sensor_container, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_align(slot.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, -(font->line_height + 2));
}

inline void subscribe_weather_wind(BtnSlot &slot, const std::string &entity,
                                   const std::string &speed_sensor = "", bool sensors = false) {
  struct WindState {
    lv_obj_t *icon, *label, *speed;
    uint32_t generation;
    int direction = -1;
    std::string value = "--", unit;
    bool available = true;
    void render() const {
      if (generation != ha_subscription_generation() || !lv_obj_is_valid(icon) ||
          !lv_obj_is_valid(label) || !lv_obj_is_valid(speed)) return;
      const int point = available ? direction : -1;
      lv_label_set_display_text(icon, find_icon(espcontrol::wind::icon(point)));
      lv_label_set_display_text(label, espcontrol_i18n(espcontrol::wind::label(point)));
      const std::string text = available && value != "--"
        ? value + (unit.empty() ? "" : " " + unit) : "--";
      lv_label_set_display_text(speed, text.c_str());
      notify_dashboard_content_changed();
    }
  };
  auto state = std::make_shared<WindState>();
  state->icon = slot.icon_lbl;
  state->label = slot.text_lbl;
  state->speed = slot.sensor_lbl;
  state->generation = ha_subscription_generation();
  state->render();
  if (sensors) {
    if (!entity.empty()) ha_subscribe_state(entity, [state](esphome::StringRef value) {
      state->direction = espcontrol::wind::direction(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN));
      state->render();
    });
    if (!speed_sensor.empty()) {
      ha_subscribe_state(speed_sensor, [state](esphome::StringRef value) {
        state->value = espcontrol::wind::speed(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN));
        state->render();
      });
      ha_subscribe_attribute(speed_sensor, "unit_of_measurement", [state](esphome::StringRef value) {
        state->unit = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
        if (state->unit == "unknown" || state->unit == "unavailable") state->unit.clear();
        state->render();
      });
    }
    return;
  }
  if (entity.empty()) return;
  ha_subscribe_state(entity, [state](esphome::StringRef value) {
    const std::string text = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
    state->available = !text.empty() && text != "unknown" && text != "unavailable";
    state->render();
  });
  ha_subscribe_attribute(entity, "wind_bearing", [state](esphome::StringRef value) {
    state->direction = espcontrol::wind::direction(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN));
    state->render();
  });
  ha_subscribe_attribute(entity, "wind_speed", [state](esphome::StringRef value) {
    state->value = espcontrol::wind::speed(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN));
    state->render();
  });
  ha_subscribe_attribute(entity, "wind_speed_unit", [state](esphome::StringRef value) {
    state->unit = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
    if (state->unit == "unknown" || state->unit == "unavailable") state->unit.clear();
    state->render();
  });
}
