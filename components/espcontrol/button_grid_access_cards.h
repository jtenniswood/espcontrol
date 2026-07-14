#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

inline const char* garage_closed_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Garage") : find_icon(icon.c_str());
}

inline const char* garage_open_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Garage Open") : find_icon(icon_on.c_str());
}

inline bool garage_command_mode(const std::string &sensor) {
  return card_runtime_garage_command_mode(sensor);
}

inline const char *garage_command_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(p.sensor == "open" ? "Garage Open" : "Garage");
}

inline const char *garage_card_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label.c_str();
  if (p.sensor == "open") return espcontrol_i18n("Open");
  if (p.sensor == "close") return espcontrol_i18n("Close");
  return espcontrol_i18n("Garage Door");
}

inline bool garage_card_show_status(const ParsedCfg &p) {
  return normalize_garage_label_display(cfg_option_value(p.options, "label_display")) == "status";
}

inline const char* gate_closed_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Gate") : find_icon(icon.c_str());
}

inline const char* gate_open_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Gate Open") : find_icon(icon_on.c_str());
}

inline bool gate_command_mode(const std::string &sensor) {
  return card_runtime_gate_command_mode(sensor);
}

inline const char *gate_command_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  if (p.sensor == "open") return find_icon("Gate Open");
  if (p.sensor == "stop") return find_icon("Stop");
  return find_icon("Gate");
}

inline const char *gate_card_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label.c_str();
  if (p.sensor == "open") return espcontrol_i18n("Open");
  if (p.sensor == "close") return espcontrol_i18n("Close");
  if (p.sensor == "stop") return espcontrol_i18n("Stop");
  return espcontrol_i18n("Gate");
}

inline bool gate_card_show_status(const ParsedCfg &p) {
  return normalize_gate_label_display(cfg_option_value(p.options, "label_display")) == "status";
}

inline const char* lock_locked_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Lock") : find_icon(icon.c_str());
}

inline const char* lock_unlocked_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Lock Open") : find_icon(icon_on.c_str());
}

inline bool lock_command_mode(const std::string &sensor) {
  return card_runtime_lock_command_mode(sensor);
}

inline const char *lock_command_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(p.sensor == "unlock" ? "Lock Open" : "Lock");
}

inline const char *lock_card_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label.c_str();
  if (p.sensor == "lock") return espcontrol_i18n("Lock");
  if (p.sensor == "unlock") return espcontrol_i18n("Unlock");
  return espcontrol_i18n("Lock");
}

inline std::string garage_state_label(const std::string &state) {
  if (state.empty()) return "--";
  if (state == "open") return espcontrol_i18n_key("state_open");
  return espcontrol_i18n(sentence_cap_text(state));
}

inline bool garage_state_is_active(const std::string &state) {
  return state == "open" || state == "opening" || state == "closing";
}

inline bool cover_toggle_state_is_active(const std::string &state) {
  return state == "closed" || state == "closing";
}

inline bool garage_state_uses_open_icon(const std::string &state) {
  return state == "open" || state == "opening";
}

inline bool garage_state_releases_label(const std::string &state) {
  return state == "open" || state == "closed";
}

struct LockCardCtx {
  std::string entity_id;
  std::string state;
};

inline std::string lock_state_label(const std::string &state) {
  if (state.empty()) return "--";
  return sentence_cap_text(state);
}

inline bool lock_state_is_active(const std::string &state) {
  return state == "unlocked" || state == "unlocking" ||
         state == "open" || state == "opening" ||
         state == "jammed";
}

inline bool lock_state_uses_unlocked_icon(const std::string &state) {
  return lock_state_is_active(state);
}

inline bool lock_state_releases_label(const std::string &state) {
  return state == "locked" || state == "unlocked" || state == "open";
}

// Reusable label helper: show changed status, then optionally return to steady text.
static const uint32_t STATUS_LABEL_STABLE_MS = 3000;

struct TransientStatusLabel {
  lv_obj_t *label = nullptr;
  std::string steady_text;
  std::string last_status_text;
  bool has_status = false;
  bool showing_status = false;
  lv_timer_t *revert_timer = nullptr;
};

inline void transient_status_label_revert_cb(lv_timer_t *timer) {
  TransientStatusLabel *ctx = static_cast<TransientStatusLabel *>(lv_timer_get_user_data(timer));
  if (!ctx) return;
  ctx->showing_status = false;
  if (ctx->label) lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  lv_timer_pause(timer);
}

inline TransientStatusLabel *create_transient_status_label(
    lv_obj_t *label, const std::string &steady_text,
    uint32_t stable_ms = STATUS_LABEL_STABLE_MS) {
  // Intentionally leaked -- lives for the lifetime of the display.
  TransientStatusLabel *ctx = new TransientStatusLabel();
  ctx->label = label;
  ctx->steady_text = steady_text;
  if (ctx->label) lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  ctx->revert_timer = lv_timer_create(transient_status_label_revert_cb, stable_ms, ctx);
  if (ctx->revert_timer) lv_timer_pause(ctx->revert_timer);
  return ctx;
}

inline void transient_status_label_set_steady(TransientStatusLabel *ctx,
                                              const std::string &steady_text) {
  if (!ctx) return;
  ctx->steady_text = steady_text;
  if (!ctx->showing_status && ctx->label) {
    lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  }
}

inline void transient_status_label_show_if_changed(TransientStatusLabel *ctx,
                                                   const std::string &status_text,
                                                   bool release_to_steady = true) {
  if (!ctx) return;
  if (!ctx->has_status) {
    ctx->last_status_text = status_text;
    ctx->has_status = true;
    if (!release_to_steady) {
      ctx->showing_status = true;
      if (ctx->label) lv_label_set_text(ctx->label, status_text.c_str());
      if (ctx->revert_timer) lv_timer_pause(ctx->revert_timer);
    }
    return;
  }
  if (ctx->last_status_text == status_text) return;
  ctx->last_status_text = status_text;
  ctx->showing_status = true;
  if (ctx->label) lv_label_set_text(ctx->label, status_text.c_str());
  if (ctx->revert_timer) {
    if (release_to_steady) {
      lv_timer_reset(ctx->revert_timer);
      lv_timer_resume(ctx->revert_timer);
    } else {
      lv_timer_pause(ctx->revert_timer);
    }
  }
}
