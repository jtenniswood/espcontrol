#pragma once

inline void weather_reading_position_value(lv_obj_t *value, lv_obj_t *sublabel) {
  lv_obj_align(value, LV_ALIGN_BOTTOM_LEFT, 0, -lv_obj_get_height(sublabel) - 2);
}

inline void weather_reading_sublabel_resized(lv_event_t *event) {
  auto *value = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
  auto *sublabel = static_cast<lv_obj_t *>(lv_event_get_current_target(event));
  weather_reading_position_value(value, sublabel);
}

inline void weather_reading_layout(BtnSlot &slot, const DisplayProfile &display) {
  capture_card_padding(slot.btn);
  const lv_font_t *font = display_small_text_font(
    display, lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN));
  lv_obj_set_style_text_font(slot.sensor_lbl, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(slot.sensor_lbl,
    lv_obj_get_style_text_color(slot.btn, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_text_opa(slot.sensor_lbl, LV_OPA_70, LV_PART_MAIN);
  lv_obj_align(slot.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_align(slot.sensor_container, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_remove_event_cb_with_user_data(
    slot.sensor_container, weather_reading_sublabel_resized, slot.text_lbl);
  lv_obj_add_event_cb(slot.sensor_container, weather_reading_sublabel_resized,
    LV_EVENT_SIZE_CHANGED, slot.text_lbl);
  weather_reading_position_value(slot.text_lbl, slot.sensor_container);
}
