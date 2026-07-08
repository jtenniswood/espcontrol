#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

struct LawnMowerCardCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  std::string entity_id;
  std::string mode;
  std::string state;
  std::string label;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  bool status_card = false;
  bool available = true;
};

struct LawnMowerControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *state_icon_lbl = nullptr;
  lv_obj_t *state_lbl = nullptr;
  lv_obj_t *start_btn = nullptr;
  lv_obj_t *pause_btn = nullptr;
  lv_obj_t *dock_btn = nullptr;
  LawnMowerCardCtx *active = nullptr;
};

inline LawnMowerControlModalUi &lawn_mower_control_modal_ui() {
  static LawnMowerControlModalUi ui;
  return ui;
}

inline void lawn_mower_control_update_modal(LawnMowerCardCtx *ctx);

inline std::string lawn_mower_card_mode(const std::string &mode) {
  return card_runtime_lawn_mower_mode(mode);
}

inline bool lawn_mower_card_mode_needs_state(const std::string &mode) {
  return card_runtime_lawn_mower_state_mode(mode);
}

inline bool lawn_mower_card_read_only(const ParsedCfg &p) {
  return p.type == "lawn_mower" && lawn_mower_card_mode(p.sensor) == "status";
}

inline const char *lawn_mower_card_default_icon_name(const std::string &mode) {
  return card_runtime_lawn_mower_default_icon_name(mode);
}

inline const char *lawn_mower_card_mode_label(const std::string &mode) {
  std::string normalized = lawn_mower_card_mode(mode);
  if (normalized == "status") return "Lawn Mower";
  if (normalized == "control_panel") return "Control Panel";
  if (normalized == "dock") return "Dock";
  if (normalized == "pause_resume") return "Pause";
  return "Start";
}

inline const char *lawn_mower_state_icon_name(const std::string &state) {
  if (state == "docked" || state == "returning" || state == "error" ||
      state == "unavailable" || state == "unknown") {
    return "Robot Mower Outline";
  }
  return "Robot Mower";
}

inline std::string lawn_mower_state_label(const std::string &state,
                                          const std::string &fallback) {
  if (state == "mowing") return espcontrol_i18n(std::string("Mowing"));
  if (state == "docked") return espcontrol_i18n(std::string("Docked"));
  if (state == "paused") return espcontrol_i18n(std::string("Paused"));
  if (state == "returning") return espcontrol_i18n(std::string("Returning"));
  if (state == "error") return espcontrol_i18n(std::string("Error"));
  if (state == "unavailable") return espcontrol_i18n(std::string("Unavailable"));
  if (state == "unknown") return espcontrol_i18n(std::string("Unknown"));
  return fallback;
}

inline bool lawn_mower_state_active_ref(esphome::StringRef state) {
  std::string value = normalized_state_text(state);
  return value == "mowing" || value == "returning";
}

inline const char *lawn_mower_card_icon(const ParsedCfg &p) {
  return (!p.icon.empty() && p.icon != "Auto")
    ? find_icon(p.icon.c_str())
    : find_icon(lawn_mower_card_default_icon_name(p.sensor));
}

inline void setup_lawn_mower_card(BtnSlot &s, const ParsedCfg &p) {
  std::string label = p.label.empty()
    ? espcontrol_i18n(std::string(lawn_mower_card_mode_label(p.sensor)))
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  lv_label_set_text(s.icon_lbl, lawn_mower_card_icon(p));
  if (lawn_mower_card_read_only(p)) {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
    return;
  }
  apply_push_button_transition(s.btn);
}

inline LawnMowerCardCtx *create_lawn_mower_card_context(const BtnSlot &s,
                                                        const ParsedCfg &p,
                                                        const lv_font_t *label_font = nullptr,
                                                        const lv_font_t *icon_font = nullptr,
                                                        int width_compensation_percent = 100) {
  LawnMowerCardCtx *ctx = new LawnMowerCardCtx();
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->text_lbl = s.text_lbl;
  ctx->entity_id = p.entity;
  ctx->mode = lawn_mower_card_mode(p.sensor);
  ctx->label = p.label.empty()
    ? espcontrol_i18n(std::string(lawn_mower_card_mode_label(ctx->mode)))
    : p.label;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = normalize_width_compensation_percent(width_compensation_percent);
  ctx->status_card = ctx->mode == "status";
  return ctx;
}

inline void apply_lawn_mower_card_state(LawnMowerCardCtx *ctx,
                                        esphome::StringRef state,
                                        bool unavailable) {
  if (!ctx) return;
  ctx->state = unavailable ? "unavailable" : std::string(state.c_str(), state.size());
  ctx->available = !(ctx->state == "unavailable" || ctx->state == "unknown");
  if (ctx->icon_lbl) {
    lv_label_set_text(ctx->icon_lbl, find_icon(lawn_mower_state_icon_name(ctx->state)));
  }
  if (ctx->text_lbl) {
    std::string label = ctx->status_card
      ? lawn_mower_state_label(ctx->state, ctx->label)
      : ctx->label;
    set_wrapped_button_label_text(ctx->text_lbl, label);
  }
  if (ctx->btn) {
    apply_control_availability(ctx->btn, ctx->btn, ctx->available);
  }
  lawn_mower_control_update_modal(ctx);
}

inline void subscribe_lawn_mower_card_state(LawnMowerCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      bool unavailable = ha_entity_state_unavailable_ref(ctx->entity_id, state);
      apply_lawn_mower_card_state(ctx, state, unavailable);
    })
  );
}

inline const char *lawn_mower_service_for_card(const LawnMowerCardCtx *ctx) {
  if (!ctx) return nullptr;
  if (ctx->mode == "start_mowing") return "lawn_mower.start_mowing";
  if (ctx->mode == "dock") return "lawn_mower.dock";
  if (ctx->mode == "pause_resume") {
    if (ctx->state == "mowing") return "lawn_mower.pause";
    return "lawn_mower.start_mowing";
  }
  return nullptr;
}

inline void send_lawn_mower_card_action(LawnMowerCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || ctx->status_card) return;
  if (ctx->state == "unavailable" || ctx->state == "unknown") return;
  const char *service = lawn_mower_service_for_card(ctx);
  if (!service) return;

  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, 1)) return;
  ha_action_add_entity(req, ctx->entity_id);
  ha_action_send(req);
}

inline void send_lawn_mower_control_action(LawnMowerCardCtx *ctx, const char *service) {
  if (!ctx || !service || ctx->entity_id.empty() || !ctx->available) return;
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, 1)) return;
  ha_action_add_entity(req, ctx->entity_id);
  ha_action_send(req);
}

inline bool lawn_mower_control_button_active(const std::string &state,
                                             const char *service) {
  if (!service) return false;
  if (std::strcmp(service, "lawn_mower.pause") == 0) return state == "mowing";
  if (std::strcmp(service, "lawn_mower.start_mowing") == 0)
    return state == "paused" || state == "docked";
  if (std::strcmp(service, "lawn_mower.dock") == 0)
    return state == "returning" || state == "docked";
  return false;
}

inline void lawn_mower_control_style_action_button(lv_obj_t *btn,
                                                   LawnMowerCardCtx *ctx,
                                                   const char *service) {
  if (!btn || !ctx) return;
  bool enabled = ctx->available;
  bool active = enabled && lawn_mower_control_button_active(ctx->state, service);
  lv_obj_t *label = control_modal_icon_label(btn);
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? DEFAULT_SLIDER_COLOR : SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, active ? 2 : 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  if (label) {
    lv_obj_set_style_text_color(
      label, lv_color_hex(active ? 0xFFFFFF : DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
  apply_control_availability(btn, btn, enabled);
}

inline lv_obj_t *lawn_mower_control_create_action_button(lv_obj_t *parent,
                                                        LawnMowerCardCtx *ctx,
                                                        const char *icon,
                                                        const char *service) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  control_modal_apply_pressed_fill(btn);

  lv_obj_t *icon_lbl = lv_label_create(btn);
  if (icon_lbl) {
    lv_label_set_text(icon_lbl, icon);
    if (ctx && ctx->icon_font) lv_obj_set_style_text_font(icon_lbl, ctx->icon_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(icon_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(icon_lbl);
  }

  lawn_mower_control_style_action_button(btn, ctx, service);
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    const char *service = static_cast<const char *>(lv_event_get_user_data(e));
    LawnMowerControlModalUi &ui = lawn_mower_control_modal_ui();
    send_lawn_mower_control_action(ui.active, service);
  }, LV_EVENT_CLICKED, const_cast<char *>(service));
  return btn;
}

inline void lawn_mower_control_layout_modal(LawnMowerCardCtx *ctx) {
  LawnMowerControlModalUi &ui = lawn_mower_control_modal_ui();
  if (!ctx || !ui.panel) return;

  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, control_modal_card_radius(ctx->btn));
  control_modal_apply_back_button_layout(ui.back_btn, layout);
  lv_coord_t content_w = layout.panel_w - layout.inset * 2;
  if (content_w < 120) content_w = layout.panel_w;
  lv_coord_t gap = control_modal_scaled_px(12, layout.short_side);
  if (gap < 8) gap = 8;
  lv_coord_t title_y = layout.back_inset_y + layout.back_size / 2 - layout.back_size / 2;
  lv_coord_t chrome_safe_top = layout.back_inset_y + layout.back_size + layout.inset / 2;
  lv_coord_t content_top = chrome_safe_top;
  lv_coord_t content_bottom = layout.panel_h - layout.inset;
  lv_coord_t content_h = content_bottom - content_top;
  if (content_h < 160) {
    content_top = layout.inset * 2;
    content_h = content_bottom - content_top;
  }
  lv_coord_t state_icon_h = control_modal_scaled_px(48, layout.short_side);
  if (state_icon_h < 34) state_icon_h = 34;
  lv_coord_t state_label_h = control_modal_scaled_px(30, layout.short_side);
  if (state_label_h < 24) state_label_h = 24;
  lv_coord_t state_gap = control_modal_scaled_px(6, layout.short_side);
  if (state_gap < 4) state_gap = 4;
  lv_coord_t button_gap = control_modal_scaled_px(10, layout.short_side);
  if (button_gap < gap) button_gap = gap;
  lv_coord_t state_block_h = state_icon_h + state_gap + state_label_h;
  lv_coord_t button_area_h = content_h - state_block_h - gap * 2;
  if (button_area_h < 64) button_area_h = content_h / 2;
  lv_coord_t button_size = (content_w - button_gap * 2) / 3;
  if (button_size > button_area_h) button_size = button_area_h;
  lv_coord_t max_button_size = control_modal_scaled_px(112, layout.short_side);
  if (button_size > max_button_size) button_size = max_button_size;
  if (button_size < 56) button_size = 56;
  lv_coord_t buttons_total_w = button_size * 3 + button_gap * 2;
  lv_coord_t button_start_x = layout.inset + (content_w - buttons_total_w) / 2;
  if (button_start_x < layout.inset) button_start_x = layout.inset;
  lv_coord_t state_top = content_top + (content_h - state_block_h - button_size - gap) / 2;
  if (state_top < content_top) state_top = content_top;
  lv_coord_t button_y = state_top + state_block_h + gap;
  lv_coord_t button_radius = control_modal_card_radius(ctx->btn);

  if (ui.title_lbl) {
    lv_obj_set_width(ui.title_lbl, content_w - layout.back_size - gap);
    apply_width_compensation(ui.title_lbl, ctx->width_compensation_percent);
    lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, title_y);
  }
  if (ui.state_icon_lbl) {
    lv_obj_set_height(ui.state_icon_lbl, state_icon_h);
    lv_obj_align(ui.state_icon_lbl, LV_ALIGN_TOP_MID, 0, state_top);
  }
  if (ui.state_lbl) {
    lv_obj_set_width(ui.state_lbl, content_w);
    lv_obj_set_height(ui.state_lbl, state_label_h);
    apply_width_compensation(ui.state_lbl, ctx->width_compensation_percent);
    lv_obj_align(ui.state_lbl, LV_ALIGN_TOP_MID, 0, state_top + state_icon_h + state_gap);
  }

  lv_obj_t *buttons[] = {ui.start_btn, ui.pause_btn, ui.dock_btn};
  for (int i = 0; i < 3; i++) {
    if (!buttons[i]) continue;
    lv_obj_set_size(buttons[i], button_size, button_size);
    lv_obj_set_style_radius(buttons[i], button_radius, LV_PART_MAIN);
    apply_width_compensation(buttons[i], ctx->width_compensation_percent);
    lv_obj_align(buttons[i], LV_ALIGN_TOP_LEFT,
                 button_start_x + i * (button_size + button_gap), button_y);
    lv_obj_t *label = control_modal_icon_label(buttons[i]);
    if (label) lv_obj_center(label);
  }
  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
}

inline void lawn_mower_control_update_modal(LawnMowerCardCtx *ctx) {
  LawnMowerControlModalUi &ui = lawn_mower_control_modal_ui();
  if (!ctx || ui.active != ctx || !ui.overlay) return;
  std::string fallback = ctx->state.empty() ? ctx->label : ctx->state;
  std::string state_label = lawn_mower_state_label(ctx->state, fallback);
  if (ui.state_icon_lbl) {
    lv_label_set_text(ui.state_icon_lbl, find_icon(lawn_mower_state_icon_name(ctx->state)));
  }
  if (ui.state_lbl) {
    lv_label_set_text(ui.state_lbl, state_label.c_str());
  }
  lawn_mower_control_style_action_button(ui.start_btn, ctx, "lawn_mower.start_mowing");
  lawn_mower_control_style_action_button(ui.pause_btn, ctx, "lawn_mower.pause");
  lawn_mower_control_style_action_button(ui.dock_btn, ctx, "lawn_mower.dock");
  lawn_mower_control_layout_modal(ctx);
}

inline void lawn_mower_control_hide_modal() {
  LawnMowerControlModalUi &ui = lawn_mower_control_modal_ui();
  lv_obj_t *overlay = ui.overlay;
  ui = LawnMowerControlModalUi();
  control_modal_delete_overlay(ControlModalKind::LAWN_MOWER_CONTROL, overlay);
}

inline void lawn_mower_control_open_modal(LawnMowerCardCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::LAWN_MOWER_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, lawn_mower_control_hide_modal);
  LawnMowerControlModalUi &ui = lawn_mower_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  if (!ui.panel) return;

  ui.title_lbl = control_modal_create_title(
    ui.panel, ctx->label.c_str(), shell.content_w, ctx->label_font,
    ctx->width_compensation_percent);

  ui.state_icon_lbl = lv_label_create(ui.panel);
  if (ui.state_icon_lbl) {
    if (ctx->icon_font) lv_obj_set_style_text_font(ui.state_icon_lbl, ctx->icon_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui.state_icon_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(ui.state_icon_lbl, find_icon(lawn_mower_state_icon_name(ctx->state)));
  }

  ui.state_lbl = lv_label_create(ui.panel);
  if (ui.state_lbl) {
    if (ctx->label_font) lv_obj_set_style_text_font(ui.state_lbl, ctx->label_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui.state_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.state_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  }

  ui.start_btn = lawn_mower_control_create_action_button(
    ui.panel, ctx, find_icon("Play"), "lawn_mower.start_mowing");
  ui.pause_btn = lawn_mower_control_create_action_button(
    ui.panel, ctx, find_icon("Pause"), "lawn_mower.pause");
  ui.dock_btn = lawn_mower_control_create_action_button(
    ui.panel, ctx, find_icon("Home"), "lawn_mower.dock");

  lawn_mower_control_update_modal(ctx);
  lv_obj_move_foreground(ui.overlay);
}
