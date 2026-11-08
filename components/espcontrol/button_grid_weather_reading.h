#pragma once

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
  lv_obj_align(slot.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, -(font->line_height + 2));
}
