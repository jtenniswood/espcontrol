#pragma once

#include "lvgl.h"

constexpr uint32_t DARK_TEXT_DISABLED = 0x707070;

// Labels include the MDI icon glyphs and nested sensor values. Use a separate
// state style so their normal (including custom) colours survive reconnection.
inline void set_card_content_disabled(lv_obj_t *obj, bool disabled) {
  if (!obj) return;
  if (lv_obj_check_type(obj, &lv_label_class)) {
    lv_obj_set_style_text_color(obj, lv_color_hex(DARK_TEXT_DISABLED),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DISABLED);
    if (disabled) lv_obj_add_state(obj, LV_STATE_DISABLED);
    else lv_obj_clear_state(obj, LV_STATE_DISABLED);
  }
  for (uint32_t i = 0; i < lv_obj_get_child_cnt(obj); ++i) {
    set_card_content_disabled(lv_obj_get_child(obj, i), disabled);
  }
}

inline void set_card_disabled_state(lv_obj_t *btn, bool disabled) {
  if (!btn) return;
  // LVGL's default disabled recolour overlay also alters the card background.
  lv_obj_set_style_recolor_opa(btn, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DISABLED);
  lv_obj_set_style_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  if (disabled) lv_obj_add_state(btn, LV_STATE_DISABLED);
  else lv_obj_clear_state(btn, LV_STATE_DISABLED);
  set_card_content_disabled(btn, disabled);
}
