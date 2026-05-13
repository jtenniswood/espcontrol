#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

struct SwitchConfirmationModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *message_lbl = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *no_btn = nullptr;
  lv_obj_t *confirm_btn = nullptr;
  lv_obj_t *btn_obj = nullptr;
  ParsedCfg cfg;
};

inline SwitchConfirmationModalUi &switch_confirmation_modal_ui() {
  static SwitchConfirmationModalUi ui;
  return ui;
}

inline const lv_font_t *&switch_confirmation_message_font_ref() {
  static const lv_font_t *font = nullptr;
  return font;
}

inline const lv_font_t *&switch_confirmation_icon_font_ref() {
  static const lv_font_t *font = nullptr;
  return font;
}

inline void set_switch_confirmation_message_font(const lv_font_t *font) {
  switch_confirmation_message_font_ref() = font;
}

inline void set_switch_confirmation_icon_font(const lv_font_t *font) {
  switch_confirmation_icon_font_ref() = font;
}

inline const lv_font_t *switch_confirmation_message_font(const lv_font_t *fallback) {
  const lv_font_t *font = switch_confirmation_message_font_ref();
  return font ? font : fallback;
}

inline const lv_font_t *switch_confirmation_icon_font(const lv_font_t *fallback) {
  const lv_font_t *font = switch_confirmation_icon_font_ref();
  return font ? font : fallback;
}

inline void switch_confirmation_hide_modal() {
  SwitchConfirmationModalUi &ui = switch_confirmation_modal_ui();
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui = SwitchConfirmationModalUi();
}

inline lv_obj_t *switch_confirmation_create_text_button(
    lv_obj_t *parent,
    const std::string &text,
    lv_coord_t max_width,
    lv_coord_t min_width,
    lv_coord_t min_height,
    lv_coord_t radius,
    uint32_t bg_color,
    const lv_font_t *font) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, min_width, min_height);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text.c_str());
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);

  lv_obj_update_layout(label);
  lv_coord_t pad_x = min_height / 2;
  if (pad_x < 14) pad_x = 14;
  lv_coord_t pad_y = min_height / 5;
  if (pad_y < 8) pad_y = 8;

  lv_coord_t natural_width = lv_obj_get_width(label) + pad_x * 2;
  if (natural_width < min_width) natural_width = min_width;
  if (natural_width > max_width) natural_width = max_width;

  lv_coord_t label_width = natural_width - pad_x * 2;
  if (label_width < 24) label_width = 24;
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, label_width);
  lv_obj_update_layout(label);

  lv_coord_t button_height = lv_obj_get_height(label) + pad_y * 2;
  if (button_height < min_height) button_height = min_height;
  lv_obj_set_size(btn, natural_width, button_height);
  lv_obj_center(label);
  return btn;
}

inline void switch_confirmation_confirm() {
  SwitchConfirmationModalUi &ui = switch_confirmation_modal_ui();
  if (!ui.cfg.entity.empty()) send_turn_off_action(ui.cfg.entity);
  if (ui.btn_obj) lv_obj_clear_state(ui.btn_obj, LV_STATE_CHECKED);
  switch_confirmation_hide_modal();
}

inline void switch_confirmation_open_modal(const ParsedCfg &p, lv_obj_t *btn_obj) {
  if (p.entity.empty()) return;
  media_volume_hide_modal();
  climate_control_hide_modal();
  switch_confirmation_hide_modal();

  SwitchConfirmationModalUi &ui = switch_confirmation_modal_ui();
  ui.cfg = p;
  ui.btn_obj = btn_obj;

  ControlModalLayout layout = control_modal_calc_layout(100);
  lv_coord_t radius = control_modal_card_radius(btn_obj);
  lv_coord_t content_w = layout.panel_w - layout.inset * 2;
  if (content_w < 120) content_w = layout.panel_w;
  lv_coord_t button_gap = control_modal_scaled_px(12, layout.short_side);
  if (button_gap < 8) button_gap = 8;
  lv_coord_t button_h = control_modal_scaled_px(52, layout.short_side);
  if (button_h < 36) button_h = 36;
  lv_coord_t button_max_w = (content_w - button_gap) / 2;
  if (button_max_w < 48) button_max_w = content_w;
  lv_coord_t button_min_w = button_h + control_modal_scaled_px(16, layout.short_side);
  if (button_min_w < 56) button_min_w = 56;
  if (button_min_w > button_max_w) button_min_w = button_max_w;

  const lv_font_t *button_font = btn_obj
    ? lv_obj_get_style_text_font(btn_obj, LV_PART_MAIN)
    : nullptr;
  const lv_font_t *message_font = switch_confirmation_message_font(button_font);
  const lv_font_t *icon_font = switch_confirmation_icon_font(button_font);

  ui.overlay = lv_obj_create(lv_layer_top());
  control_modal_style_overlay(ui.overlay);

  ui.panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(ui.panel, DEFAULT_TERTIARY_COLOR, radius);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, radius);

  ui.close_btn = control_modal_create_round_button(
    ui.panel, 32, "\U000F0156", icon_font,
    0x454545, DEFAULT_TERTIARY_COLOR);
  lv_obj_set_style_bg_opa(ui.close_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.close_btn, 0, LV_PART_MAIN);
  lv_obj_set_size(ui.close_btn, layout.back_size, layout.back_size);
  lv_obj_set_style_radius(ui.close_btn, layout.back_size / 2, LV_PART_MAIN);
  lv_obj_align(ui.close_btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);

  ui.message_lbl = lv_label_create(ui.panel);
  std::string message = switch_confirmation_message(p);
  lv_label_set_text(ui.message_lbl, message.c_str());
  lv_label_set_long_mode(ui.message_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(ui.message_lbl, content_w);
  lv_obj_set_style_text_color(ui.message_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.message_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (message_font) lv_obj_set_style_text_font(ui.message_lbl, message_font, LV_PART_MAIN);

  ui.no_btn = switch_confirmation_create_text_button(
    ui.panel, switch_confirmation_no_text(p), button_max_w, button_min_w, button_h,
    button_h / 2, 0x454545, button_font);
  ui.confirm_btn = switch_confirmation_create_text_button(
    ui.panel, switch_confirmation_yes_text(p), button_max_w, button_min_w, button_h,
    button_h / 2, DEFAULT_SLIDER_COLOR, button_font);

  lv_obj_update_layout(ui.message_lbl);
  lv_obj_update_layout(ui.no_btn);
  lv_obj_update_layout(ui.confirm_btn);
  lv_coord_t message_h = lv_obj_get_height(ui.message_lbl);
  lv_coord_t no_w = lv_obj_get_width(ui.no_btn);
  lv_coord_t no_h = lv_obj_get_height(ui.no_btn);
  lv_coord_t confirm_w = lv_obj_get_width(ui.confirm_btn);
  lv_coord_t confirm_h = lv_obj_get_height(ui.confirm_btn);
  lv_coord_t action_h = no_h > confirm_h ? no_h : confirm_h;
  lv_coord_t group_h = message_h + button_gap + action_h;
  lv_coord_t group_top = (layout.panel_h - group_h) / 2;
  lv_coord_t top_limit = layout.inset + layout.back_size + button_gap;
  if (group_top < top_limit) group_top = top_limit;
  lv_coord_t bottom_limit = layout.panel_h - layout.inset;
  if (group_top + group_h > bottom_limit) group_top = bottom_limit - group_h;
  if (group_top < layout.inset) group_top = layout.inset;

  lv_coord_t message_y = group_top + message_h / 2 - layout.panel_h / 2;
  lv_coord_t action_y = group_top + message_h + button_gap + action_h / 2 - layout.panel_h / 2;
  lv_coord_t action_w = no_w + button_gap + confirm_w;
  lv_coord_t no_x = -action_w / 2 + no_w / 2;
  lv_coord_t confirm_x = action_w / 2 - confirm_w / 2;
  lv_obj_align(ui.message_lbl, LV_ALIGN_CENTER, 0, message_y);
  lv_obj_align(ui.no_btn, LV_ALIGN_CENTER, no_x, action_y);
  lv_obj_align(ui.confirm_btn, LV_ALIGN_CENTER, confirm_x, action_y);

  lv_obj_add_event_cb(ui.close_btn, [](lv_event_t *) {
    switch_confirmation_hide_modal();
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.no_btn, [](lv_event_t *) {
    switch_confirmation_hide_modal();
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.confirm_btn, [](lv_event_t *) {
    switch_confirmation_confirm();
  }, LV_EVENT_CLICKED, nullptr);

  lv_obj_move_foreground(ui.overlay);
}
