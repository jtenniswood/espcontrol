#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

struct VacuumCardCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  std::string entity_id;
  std::string mode;
  std::string state;
  std::string label;
  std::string area_id;
  bool status_card = false;
};

struct VacuumCardTextRef {
  lv_obj_t *text_lbl = nullptr;
  VacuumCardCtx *ctx = nullptr;
  ParsedCfg cfg;
};

inline std::vector<VacuumCardTextRef> &subpage_vacuum_card_text_refs() {
  static std::vector<VacuumCardTextRef> refs;
  return refs;
}

inline void clear_subpage_vacuum_card_text_refs() {
  subpage_vacuum_card_text_refs().clear();
}

inline std::string vacuum_card_mode(const std::string &mode) {
  return card_runtime_vacuum_mode(mode);
}

inline bool vacuum_card_mode_needs_state(const std::string &mode) {
  return card_runtime_vacuum_state_mode(mode);
}

inline bool vacuum_card_read_only(const ParsedCfg &p) {
  return p.type == "vacuum" && vacuum_card_mode(p.sensor) == "status";
}

inline const char *vacuum_card_default_icon_name(const std::string &mode) {
  return card_runtime_vacuum_default_icon_name(mode);
}

inline const char *vacuum_card_mode_label(const std::string &mode) {
  std::string normalized = vacuum_card_mode(mode);
  if (normalized == "status") return "Vacuum";
  if (normalized == "dock") return "Dock";
  if (normalized == "pause_resume") return "Pause";
  if (normalized == "clean_spot") return "Spot Clean";
  if (normalized == "locate") return "Locate";
  if (normalized == "clean_area") return "Clean Area";
  return "Start";
}

inline const char *vacuum_state_icon_name(const std::string &state) {
  if (state == "error") return "Robot Vacuum Alert";
  if (state == "unavailable" || state == "unknown") return "Robot Vacuum Off";
  if (state == "docked" || state == "returning") return "Robot Vacuum Variant";
  return "Robot Vacuum";
}

inline std::string vacuum_state_label(const std::string &state,
                                      const std::string &fallback) {
  if (state == "cleaning") return espcontrol_i18n(std::string("Cleaning"));
  if (state == "docked") return espcontrol_i18n(std::string("Docked"));
  if (state == "error") return espcontrol_i18n(std::string("Error"));
  if (state == "idle") return espcontrol_i18n(std::string("Idle"));
  if (state == "paused") return espcontrol_i18n(std::string("Paused"));
  if (state == "returning") return espcontrol_i18n(std::string("Returning"));
  if (state == "unavailable") return espcontrol_i18n(std::string("Unavailable"));
  if (state == "unknown") return espcontrol_i18n(std::string("Unknown"));
  return fallback;
}

inline const char *vacuum_card_icon(const ParsedCfg &p) {
  return (!p.icon.empty() && p.icon != "Auto")
    ? find_icon(p.icon.c_str())
    : find_icon(vacuum_card_default_icon_name(p.sensor));
}

inline void setup_vacuum_card(BtnSlot &s, const ParsedCfg &p) {
  std::string label = p.label.empty()
    ? espcontrol_i18n(std::string(vacuum_card_mode_label(p.sensor)))
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  lv_label_set_text(s.icon_lbl, vacuum_card_icon(p));
  if (vacuum_card_read_only(p)) {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
    return;
  }
  apply_push_button_transition(s.btn);
}

inline VacuumCardCtx *create_vacuum_card_context(const BtnSlot &s,
                                                 const ParsedCfg &p) {
  VacuumCardCtx *ctx = new VacuumCardCtx();
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->text_lbl = s.text_lbl;
  ctx->entity_id = p.entity;
  ctx->mode = vacuum_card_mode(p.sensor);
  ctx->label = p.label.empty()
    ? espcontrol_i18n(std::string(vacuum_card_mode_label(ctx->mode)))
    : p.label;
  ctx->area_id = p.unit;
  ctx->status_card = ctx->mode == "status";
  return ctx;
}

inline void apply_vacuum_card_state(VacuumCardCtx *ctx,
                                    esphome::StringRef state,
                                    bool unavailable) {
  if (!ctx) return;
  ctx->state = unavailable ? "unavailable" : std::string(state.c_str(), state.size());
  if (ctx->icon_lbl) {
    lv_label_set_text(ctx->icon_lbl, find_icon(vacuum_state_icon_name(ctx->state)));
  }
  if (ctx->text_lbl) {
    std::string label = ctx->status_card
      ? vacuum_state_label(ctx->state, ctx->label)
      : ctx->label;
    set_wrapped_button_label_text(ctx->text_lbl, label);
  }
  if (ctx->btn && ctx->mode == "start_stop") {
    set_card_checked_state(ctx->btn, ctx->state == "cleaning");
  }
  if (ctx->btn) {
    apply_control_availability(ctx->btn, ctx->btn, !unavailable, !ctx->status_card);
  }
}

inline void refresh_vacuum_card_translated_text(lv_obj_t *text_lbl,
                                                VacuumCardCtx *ctx,
                                                const ParsedCfg &p) {
  if (!text_lbl) return;
  std::string label = p.label.empty()
    ? espcontrol_i18n(std::string(vacuum_card_mode_label(p.sensor)))
    : p.label;
  if (ctx && p.label.empty()) {
    ctx->label = espcontrol_i18n(std::string(vacuum_card_mode_label(ctx->mode)));
  }
  if (ctx && ctx->status_card && !ctx->state.empty()) {
    label = vacuum_state_label(ctx->state, ctx->label);
  } else if (ctx) {
    label = ctx->label;
  }
  set_wrapped_button_label_text(text_lbl, label);
}

inline void register_subpage_vacuum_card_text(lv_obj_t *text_lbl,
                                              VacuumCardCtx *ctx,
                                              const ParsedCfg &p) {
  if (!text_lbl) return;
  subpage_vacuum_card_text_refs().push_back({text_lbl, ctx, p});
}

inline void refresh_subpage_vacuum_card_translated_text() {
  for (auto &ref : subpage_vacuum_card_text_refs()) {
    refresh_vacuum_card_translated_text(ref.text_lbl, ref.ctx, ref.cfg);
  }
}

inline void subscribe_vacuum_card_state(VacuumCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn, !ctx->status_card);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      bool unavailable = ha_entity_state_unavailable_ref(ctx->entity_id, state);
      apply_vacuum_card_state(ctx, state, unavailable);
    })
  );
}

inline const char *vacuum_service_for_card(const VacuumCardCtx *ctx) {
  if (!ctx) return nullptr;
  if (ctx->mode == "start_stop") {
    return ctx->state == "cleaning" ? "vacuum.stop" : "vacuum.start";
  }
  if (ctx->mode == "pause_resume") {
    if (ctx->state == "cleaning") return "vacuum.pause";
    if (ctx->state == "paused") return "vacuum.start";
    return "vacuum.start_pause";
  }
  if (ctx->mode == "dock") return "vacuum.return_to_base";
  if (ctx->mode == "clean_spot") return "vacuum.clean_spot";
  if (ctx->mode == "locate") return "vacuum.locate";
  if (ctx->mode == "clean_area") return "vacuum.clean_area";
  return nullptr;
}

inline void send_vacuum_card_action(VacuumCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || ctx->status_card) return;
  const char *service = vacuum_service_for_card(ctx);
  if (!service) return;
  bool has_area = ctx->mode == "clean_area";
  if (has_area && ctx->area_id.empty()) return;

  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, has_area ? 2 : 1)) return;
  ha_action_add_entity(req, ctx->entity_id);
  if (has_area) ha_action_add_data(req, "cleaning_area_id", ctx->area_id.c_str());
  ha_action_send(req);
}
