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
  bool status_card = false;
};

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
                                                        const ParsedCfg &p) {
  LawnMowerCardCtx *ctx = new LawnMowerCardCtx();
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->text_lbl = s.text_lbl;
  ctx->entity_id = p.entity;
  ctx->mode = lawn_mower_card_mode(p.sensor);
  ctx->label = p.label.empty()
    ? espcontrol_i18n(std::string(lawn_mower_card_mode_label(ctx->mode)))
    : p.label;
  ctx->status_card = ctx->mode == "status";
  return ctx;
}

inline void apply_lawn_mower_card_state(LawnMowerCardCtx *ctx,
                                        esphome::StringRef state,
                                        bool unavailable) {
  if (!ctx) return;
  ctx->state = unavailable ? "unavailable" : std::string(state.c_str(), state.size());
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
    bool available = !(ctx->state == "unavailable" || ctx->state == "unknown");
    apply_control_availability(ctx->btn, ctx->btn, available, !ctx->status_card);
  }
}

inline void subscribe_lawn_mower_card_state(LawnMowerCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn, !ctx->status_card);
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
