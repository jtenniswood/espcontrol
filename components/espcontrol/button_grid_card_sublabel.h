#pragma once

constexpr int CARD_SUBLABEL_GAP = 2;

inline void apply_card_sublabel_style(lv_obj_t *label, lv_obj_t *card,
                                      const lv_font_t *font) {
  lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(label,
    lv_obj_get_style_text_color(card, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_text_opa(label, LV_OPA_70, LV_PART_MAIN);
}
