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
  bool turn_on = false;
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
  control_modal_delete_overlay(ControlModalKind::SWITCH_CONFIRMATION, ui.overlay);
  ui = SwitchConfirmationModalUi();
}

inline void switch_confirmation_confirm() {
  SwitchConfirmationModalUi &ui = switch_confirmation_modal_ui();
  if (action_script_confirmation_enabled(ui.cfg)) {
    send_action_card_action(ui.cfg);
  } else if (!ui.cfg.entity.empty()) {
    if (ui.turn_on) send_turn_on_action(ui.cfg.entity);
    else send_turn_off_action(ui.cfg.entity);
  }
  if (ui.btn_obj && !action_script_confirmation_enabled(ui.cfg)) {
    if (ui.turn_on) lv_obj_add_state(ui.btn_obj, LV_STATE_CHECKED);
    else lv_obj_clear_state(ui.btn_obj, LV_STATE_CHECKED);
  }
  switch_confirmation_hide_modal();
}

inline void switch_confirmation_open_modal(const ParsedCfg &p, lv_obj_t *btn_obj, bool turn_on) {
  if (p.entity.empty()) return;
  const lv_font_t *button_font = btn_obj
    ? lv_obj_get_style_text_font(btn_obj, LV_PART_MAIN)
    : nullptr;
  const lv_font_t *message_font = switch_confirmation_message_font(button_font);
  const lv_font_t *icon_font = switch_confirmation_icon_font(button_font);

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::SWITCH_CONFIRMATION, btn_obj, 100, icon_font,
    switch_confirmation_hide_modal);
  SwitchConfirmationModalUi &ui = switch_confirmation_modal_ui();
  ui.cfg = p;
  ui.btn_obj = btn_obj;
  ui.turn_on = turn_on;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;

  ControlModalLayout &layout = shell.layout;
  lv_coord_t content_w = shell.content_w;
  lv_coord_t button_gap = control_modal_scaled_px(12, layout.short_side);
  if (button_gap < 8) button_gap = 8;
  lv_coord_t message_button_gap = control_modal_scaled_px(28, layout.short_side);
  if (message_button_gap < 18) message_button_gap = 18;
  lv_coord_t button_h = control_modal_scaled_px(52, layout.short_side);
  if (button_h < 36) button_h = 36;
  lv_coord_t button_max_w = (content_w - button_gap) / 2;
  if (button_max_w < 48) button_max_w = content_w;
  lv_coord_t button_min_w = button_h + control_modal_scaled_px(16, layout.short_side);
  if (button_min_w < 56) button_min_w = 56;
  if (button_min_w > button_max_w) button_min_w = button_max_w;

  ui.message_lbl = lv_label_create(ui.panel);
  std::string message = switch_confirmation_message(p);
  lv_label_set_text(ui.message_lbl, message.c_str());
  lv_label_set_long_mode(ui.message_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(ui.message_lbl, content_w);
  lv_obj_set_style_text_color(ui.message_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.message_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (message_font) lv_obj_set_style_text_font(ui.message_lbl, message_font, LV_PART_MAIN);

  ui.no_btn = control_modal_create_text_button(
    ui.panel, switch_confirmation_no_text(p), button_max_w, button_min_w, button_h,
    button_h / 2, DARK_BORDER, button_font);
  uint32_t confirm_color = current_button_primary_color();
  ui.confirm_btn = control_modal_create_text_button(
    ui.panel, switch_confirmation_yes_text(p), button_max_w, button_min_w, button_h,
    button_h / 2, confirm_color, button_font);
  lv_obj_t *confirm_label = lv_obj_get_child(ui.confirm_btn, 0);
  if (confirm_label) {
    lv_obj_set_style_text_color(
      confirm_label, lv_color_hex(readable_text_color_for_bg(confirm_color)), LV_PART_MAIN);
  }

  lv_obj_update_layout(ui.message_lbl);
  lv_obj_update_layout(ui.no_btn);
  lv_obj_update_layout(ui.confirm_btn);
  lv_coord_t message_h = lv_obj_get_height(ui.message_lbl);
  lv_coord_t no_w = lv_obj_get_width(ui.no_btn);
  lv_coord_t no_h = lv_obj_get_height(ui.no_btn);
  lv_coord_t confirm_w = lv_obj_get_width(ui.confirm_btn);
  lv_coord_t confirm_h = lv_obj_get_height(ui.confirm_btn);
  lv_coord_t action_h = no_h > confirm_h ? no_h : confirm_h;
  lv_coord_t group_h = message_h + message_button_gap + action_h;
  lv_coord_t group_top = (layout.panel_h - group_h) / 2;
  lv_coord_t top_limit = layout.inset + layout.back_size + message_button_gap;
  if (group_top < top_limit) group_top = top_limit;
  lv_coord_t bottom_limit = layout.panel_h - layout.inset;
  if (group_top + group_h > bottom_limit) group_top = bottom_limit - group_h;
  if (group_top < layout.inset) group_top = layout.inset;

  lv_coord_t message_y = group_top + message_h / 2 - layout.panel_h / 2;
  lv_coord_t action_y = group_top + message_h + message_button_gap + action_h / 2 - layout.panel_h / 2;
  lv_coord_t action_w = no_w + button_gap + confirm_w;
  lv_coord_t no_x = -action_w / 2 + no_w / 2;
  lv_coord_t confirm_x = action_w / 2 - confirm_w / 2;
  lv_obj_align(ui.message_lbl, LV_ALIGN_CENTER, 0, message_y);
  lv_obj_align(ui.no_btn, LV_ALIGN_CENTER, no_x, action_y);
  lv_obj_align(ui.confirm_btn, LV_ALIGN_CENTER, confirm_x, action_y);

  lv_obj_add_event_cb(ui.no_btn, [](lv_event_t *) {
    switch_confirmation_hide_modal();
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.confirm_btn, [](lv_event_t *) {
    switch_confirmation_confirm();
  }, LV_EVENT_CLICKED, nullptr);

  lv_obj_move_foreground(ui.overlay);
}
