#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Home Assistant subscriptions ──────────────────────────────────────

struct ToggleTextSensorCtx {
  lv_obj_t *text_lbl = nullptr;
  std::string steady_text;
  std::string sensor_text = "--";
  bool on = false;
};

inline std::string label_text_or_empty(lv_obj_t *label) {
  if (!label) return "";
  const char *text = lv_label_get_text(label);
  return text ? std::string(text) : "";
}

inline void apply_toggle_text_sensor_label(ToggleTextSensorCtx *ctx) {
  if (!ctx || !ctx->text_lbl) return;
  set_wrapped_button_label_text(ctx->text_lbl, ctx->on ? ctx->sensor_text : ctx->steady_text);
}

// Subscribe to a HA sensor entity and update an LVGL label with its numeric value.
inline void subscribe_sensor_value(lv_obj_t *sensor_lbl, const std::string &sensor_id,
                                   int precision = 0,
                                   lv_obj_t *unit_lbl = nullptr,
                                   const std::string &unit = "",
                                   lv_obj_t *availability_obj = nullptr) {
  std::string display_unit = trim_display_unit(unit);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(esphome::StringRef)>(
      [sensor_lbl, precision, unit_lbl, display_unit, availability_obj](esphome::StringRef state) {
      bool unavailable = ha_state_unavailable_ref(state);
      if (availability_obj) {
        apply_control_availability(availability_obj, availability_obj, !unavailable, false);
      }

      float val = 0.0f;
      if (!unavailable && parse_float_ref(state, val) && std::isfinite(val)) {
        char buf[16];
        format_fixed_decimal(buf, sizeof(buf), val, precision);
        lv_label_set_text(sensor_lbl, buf);
        if (unit_lbl) lv_label_set_text(unit_lbl, display_unit.c_str());
      } else {
        lv_label_set_text(sensor_lbl, "");
        if (unit_lbl) lv_label_set_text(unit_lbl, "");
      }
    })
  );
}

inline void subscribe_toggle_text_sensor_value(ToggleTextSensorCtx *ctx, const std::string &sensor_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      if (!ctx) return;
      ctx->sensor_text = text_sensor_display_text(state);
      apply_toggle_text_sensor_label(ctx);
    })
  );
}

inline void subscribe_text_sensor_value(lv_obj_t *text_lbl, const std::string &sensor_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(esphome::StringRef)>([text_lbl](esphome::StringRef state) {
      set_wrapped_button_label_text(text_lbl, text_sensor_display_text(state));
    })
  );
}

inline void subscribe_weather_state(lv_obj_t *icon_lbl, lv_obj_t *text_lbl, const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>([icon_lbl, text_lbl](esphome::StringRef state) {
      std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
      lv_label_set_text(icon_lbl, weather_icon_for_state(state_text));
      lv_label_set_text(text_lbl, weather_label_for_state(state_text).c_str());
    })
  );
}

inline void subscribe_garage_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                   TransientStatusLabel *status_label,
                                   const char *closed_icon, const char *open_icon,
                                   const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool unavailable = ha_state_unavailable_ref(state);
        apply_control_availability(btn_ptr, btn_ptr, !unavailable);
        bool active = garage_state_is_active(state_text);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state_text) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state_text), garage_state_releases_label(state_text));
      })
  );
}

inline void subscribe_cover_toggle_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                         TransientStatusLabel *status_label,
                                         const char *closed_icon, const char *open_icon,
                                         const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool unavailable = ha_state_unavailable_ref(state);
        apply_control_availability(btn_ptr, btn_ptr, !unavailable);
        bool active = cover_toggle_state_is_active(state_text);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state_text) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state_text), garage_state_releases_label(state_text));
      })
  );
}

inline void subscribe_lock_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                 TransientStatusLabel *status_label,
                                 const char *locked_icon, const char *unlocked_icon,
                                 LockCardCtx *ctx) {
  if (!ctx) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, locked_icon, unlocked_icon, ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool unavailable = ha_state_unavailable_ref(state);
        apply_control_availability(btn_ptr, btn_ptr, !unavailable);
        ctx->state = state_text;
        bool active = lock_state_is_active(state_text);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl,
          lock_state_uses_unlocked_icon(state_text) ? unlocked_icon : locked_icon);
        transient_status_label_show_if_changed(
          status_label, lock_state_label(state_text), lock_state_releases_label(state_text));
      })
  );
}

// Subscribe to an entity's friendly_name attribute and use it as the button label
inline void subscribe_friendly_name(TransientStatusLabel *status_label,
                                    const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([status_label](esphome::StringRef name) {
      transient_status_label_set_steady(status_label, string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN));
    })
  );
}

inline void subscribe_friendly_name(ToggleTextSensorCtx *ctx,
                                    const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      if (!ctx) return;
      ctx->steady_text = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      if (!ctx->on) apply_toggle_text_sensor_label(ctx);
    })
  );
}

inline void subscribe_friendly_name(lv_obj_t *text_lbl, const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([text_lbl](esphome::StringRef name) {
      lv_label_set_text_limited(text_lbl, name, HA_FRIENDLY_NAME_MAX_LEN);
    })
  );
}

// Subscribe to a toggle entity's state; updates checked visual, icon swap, sensor overlay
inline void subscribe_toggle_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                   lv_obj_t *sensor_ctr,
                                   bool *slot_has_sensor, bool *slot_sensor_text_mode,
                                   bool *slot_has_icon_on,
                                   const char **slot_icon_off, const char **slot_icon_on,
                                   ToggleTextSensorCtx *text_sensor_ctx,
                                   const std::string &entity_id,
                                   bool disable_interaction = true) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, sensor_ctr, slot_has_sensor, slot_sensor_text_mode,
       slot_has_icon_on, slot_icon_off, slot_icon_on, text_sensor_ctx,
       disable_interaction](esphome::StringRef state) {
        bool unavailable = ha_state_unavailable_ref(state);
        apply_control_availability(btn_ptr, btn_ptr, !unavailable, disable_interaction);
        bool on = is_entity_on_ref(state);
        if (on) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);

        if (text_sensor_ctx) {
          text_sensor_ctx->on = on;
          apply_toggle_text_sensor_label(text_sensor_ctx);
        }

        bool show_numeric_sensor = *slot_has_sensor && !*slot_sensor_text_mode;
        if (show_numeric_sensor && sensor_ctr) {
          if (on) {
            lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          }
        } else {
          if (icon_lbl) lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
          if (sensor_ctr) lv_obj_add_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          if (*slot_has_icon_on)
            lv_label_set_text(icon_lbl, on ? *slot_icon_on : *slot_icon_off);
        }
      })
  );
}

inline void subscribe_control_availability(lv_obj_t *visual_obj, lv_obj_t *input_obj,
                                           const std::string &entity_id,
                                           bool disable_interaction = true) {
  if (entity_id.empty()) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [visual_obj, input_obj, disable_interaction](esphome::StringRef state) {
        apply_control_availability(
          visual_obj, input_obj, !ha_state_unavailable_ref(state), disable_interaction);
      })
  );
}

struct ActionCardStateCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *text_lbl = nullptr;
  lv_obj_t *sensor_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  bool action_available = true;
  bool state_available = true;
  bool has_state_entity = false;
  bool show_text_state = false;
  bool show_numeric_state = false;
  int precision = 0;
  std::string unit;
};

inline ActionCardStateCtx *create_action_card_state_context(const BtnSlot &s,
                                                            const ParsedCfg &p) {
  ActionCardStateCtx *ctx = new ActionCardStateCtx();
  ctx->btn = s.btn;
  ctx->text_lbl = s.text_lbl;
  ctx->sensor_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->show_text_state = action_card_state_text_mode(p);
  ctx->show_numeric_state = action_card_state_numeric_mode(p);
  ctx->precision = parse_precision(action_card_state_precision(p));
  ctx->unit = trim_display_unit(action_card_state_unit(p));
  return ctx;
}

inline void apply_action_card_availability(ActionCardStateCtx *ctx) {
  if (!ctx || !ctx->btn) return;
  bool available = ctx->action_available &&
                   (!ctx->has_state_entity || ctx->state_available);
  apply_control_availability(ctx->btn, ctx->btn, available);
}

inline void apply_action_card_display_value(ActionCardStateCtx *ctx,
                                            esphome::StringRef state,
                                            bool unavailable) {
  if (!ctx) return;
  if (ctx->show_text_state && ctx->text_lbl) {
    set_wrapped_button_label_text(ctx->text_lbl, text_sensor_display_text(state, HA_STATE_TEXT_MAX_LEN));
    return;
  }
  if (!ctx->show_numeric_state || !ctx->sensor_lbl) return;

  float val = 0.0f;
  if (!unavailable && parse_float_ref(state, val) && std::isfinite(val)) {
    char buf[16];
    format_fixed_decimal(buf, sizeof(buf), val, ctx->precision);
    lv_label_set_text(ctx->sensor_lbl, buf);
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, ctx->unit.c_str());
  } else {
    lv_label_set_text(ctx->sensor_lbl, "");
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
  }
}

inline void subscribe_action_card_target_availability(ActionCardStateCtx *ctx,
                                                      const std::string &entity_id) {
  if (!ctx || entity_id.empty()) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      ctx->action_available = !ha_state_unavailable_ref(state);
      apply_action_card_availability(ctx);
    })
  );
}

inline void subscribe_action_card_display_state(ActionCardStateCtx *ctx,
                                                const std::string &entity_id) {
  if (!ctx || entity_id.empty()) return;
  ctx->has_state_entity = true;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      bool unavailable = ha_state_unavailable_ref(state);
      ctx->state_available = !unavailable;
      if (!unavailable && is_entity_on_ref(state)) lv_obj_add_state(ctx->btn, LV_STATE_CHECKED);
      else lv_obj_clear_state(ctx->btn, LV_STATE_CHECKED);
      apply_action_card_display_value(ctx, state, unavailable);
      apply_action_card_availability(ctx);
    })
  );
}
