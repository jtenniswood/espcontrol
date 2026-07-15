#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Home Assistant subscriptions ──────────────────────────────────────

struct ToggleTextSensorCtx {
  lv_obj_t *text_lbl = nullptr;
  std::string steady_text;
  std::string sensor_text = "--";
  bool on = false;
};

struct TimeSensorCtx {
  lv_obj_t *sensor_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  std::string entity_id;
  std::string manual_unit;
  std::string state;
  std::string auto_unit;
  bool has_state = false;
  bool has_auto_unit = false;
  std::string warned_unit;
  bool warned_unit_set = false;
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

inline void apply_sensor_active_color(lv_obj_t *btn, bool active_color,
                                      esphome::StringRef state,
                                      uint32_t on_color, uint32_t sensor_color,
                                      bool unavailable) {
  if (!btn || !active_color) return;
  uint32_t next_color = (!unavailable && is_entity_on_ref(state)) ? on_color : sensor_color;
  lv_obj_set_style_bg_color(btn, lv_color_hex(next_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
}

inline void apply_control_availability(lv_obj_t *visual_obj, lv_obj_t *input_obj,
                                       bool available) {
  if (visual_obj) {
    lv_obj_set_style_opa(visual_obj, available ? LV_OPA_COVER : LV_OPA_50, LV_PART_MAIN);
    if (available) lv_obj_clear_state(visual_obj, LV_STATE_DISABLED);
    else lv_obj_add_state(visual_obj, LV_STATE_DISABLED);
  }
  if (input_obj && input_obj != visual_obj) {
    if (available) lv_obj_clear_state(input_obj, LV_STATE_DISABLED);
    else lv_obj_add_state(input_obj, LV_STATE_DISABLED);
  }
  if (!input_obj) return;
  if (available) lv_obj_add_flag(input_obj, LV_OBJ_FLAG_CLICKABLE);
  else lv_obj_clear_flag(input_obj, LV_OBJ_FLAG_CLICKABLE);
}

inline void register_ha_control_availability(lv_obj_t *visual_obj, lv_obj_t *input_obj) {
  apply_control_availability(visual_obj, input_obj, true);
}

inline void subscribe_control_availability(lv_obj_t *visual_obj, lv_obj_t *input_obj,
                                           const std::string &entity_id) {
  register_ha_control_availability(visual_obj, input_obj);
  if (entity_id.empty()) return;
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [visual_obj, input_obj](esphome::StringRef state) {
        apply_control_availability(visual_obj, input_obj, !ha_state_unavailable_ref(state));
      })
  );
}

// Subscribe to a HA sensor entity and update an LVGL label with its numeric value.
inline void subscribe_sensor_value(lv_obj_t *sensor_lbl, const std::string &sensor_id,
                                   int precision = 0,
                                   lv_obj_t *unit_lbl = nullptr,
                                   const std::string &unit = "",
                                   lv_obj_t *availability_obj = nullptr,
                                   bool active_color = false,
                                   uint32_t on_color = DEFAULT_SLIDER_COLOR,
                                   uint32_t sensor_color = TERTIARY_GREY) {
  std::string display_unit = trim_display_unit(unit);
  ha_subscribe_state(
    sensor_id,
    std::function<void(esphome::StringRef)>(
      [sensor_lbl, precision, unit_lbl, display_unit, availability_obj,
       active_color, on_color, sensor_color](esphome::StringRef state) {
      bool unavailable = ha_state_unavailable_ref(state);
      apply_sensor_active_color(availability_obj, active_color, state,
        on_color, sensor_color, unavailable);

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

inline void apply_time_sensor_value(TimeSensorCtx *ctx) {
  if (!ctx || !ctx->sensor_lbl) return;
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
  if (!ctx->has_state || (ctx->manual_unit.empty() && !ctx->has_auto_unit)) {
    lv_label_set_text(ctx->sensor_lbl, "");
    return;
  }

  const std::string &input_unit = ctx->manual_unit.empty() ? ctx->auto_unit : ctx->manual_unit;
  double multiplier = 0.0;
  if (!duration_unit_seconds_multiplier(input_unit, multiplier)) {
    lv_label_set_text(ctx->sensor_lbl, "");
    if (ctx->manual_unit.empty() &&
        (!ctx->warned_unit_set || ctx->warned_unit != input_unit)) {
      ESP_LOGW("sensors", "Time sensor %s has unsupported or missing unit_of_measurement '%s'",
               ctx->entity_id.c_str(), input_unit.c_str());
      ctx->warned_unit = input_unit;
      ctx->warned_unit_set = true;
    }
    return;
  }
  ctx->warned_unit.clear();
  ctx->warned_unit_set = false;

  char output[48];
  if (!format_duration_sensor_state(output, sizeof(output),
                                    ctx->state, ctx->has_state,
                                    ctx->auto_unit, ctx->has_auto_unit,
                                    ctx->manual_unit)) {
    lv_label_set_text(ctx->sensor_lbl, "");
    return;
  }
  lv_label_set_text(ctx->sensor_lbl, output);
}

inline void subscribe_time_sensor_value(TimeSensorCtx *ctx, const std::string &sensor_id,
                                        const std::string &manual_unit) {
  if (!ctx) return;
  ctx->entity_id = sensor_id;
  ctx->manual_unit = manual_unit;
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
  ha_subscribe_state(
    sensor_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      if (!ctx) return;
      ctx->state.assign(state.c_str(), state.size());
      ctx->has_state = true;
      apply_time_sensor_value(ctx);
    })
  );
  if (!manual_unit.empty()) return;
  ha_subscribe_attribute(
    sensor_id, "unit_of_measurement",
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef unit) {
      if (!ctx) return;
      ctx->auto_unit.assign(unit.c_str(), unit.size());
      ctx->has_auto_unit = true;
      apply_time_sensor_value(ctx);
    })
  );
}

inline void subscribe_toggle_text_sensor_value(ToggleTextSensorCtx *ctx, const std::string &sensor_id) {
  ha_subscribe_state(
    sensor_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      if (!ctx) return;
      ctx->sensor_text = text_sensor_display_text(state);
      apply_toggle_text_sensor_label(ctx);
    })
  );
}

inline void subscribe_text_sensor_value(lv_obj_t *text_lbl, const std::string &sensor_id,
                                        lv_obj_t *availability_obj = nullptr,
                                        bool active_color = false,
                                        uint32_t on_color = DEFAULT_SLIDER_COLOR,
                                        uint32_t sensor_color = TERTIARY_GREY) {
  ha_subscribe_state(
    sensor_id,
    std::function<void(esphome::StringRef)>(
      [text_lbl, availability_obj, active_color, on_color, sensor_color](esphome::StringRef state) {
      bool unavailable = ha_state_unavailable_ref(state);
      apply_sensor_active_color(availability_obj, active_color, state,
        on_color, sensor_color, unavailable);
      set_wrapped_button_label_text(text_lbl, text_sensor_display_text(state));
    })
  );
}

inline void subscribe_sensor_icon_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                        const ParsedCfg &p) {
  if (p.sensor.empty()) return;
  const char *icon_off = (p.icon.empty() || p.icon == "Auto")
    ? find_icon("Auto") : find_icon(p.icon.c_str());
  bool has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
  const char *icon_on = has_icon_on ? find_icon(p.icon_on.c_str()) : icon_off;
  ha_subscribe_state(
    p.sensor,
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, icon_off, icon_on](esphome::StringRef state) {
      bool unavailable = ha_state_unavailable_ref(state);
      if (btn_ptr) {
        set_card_checked_state(btn_ptr, !unavailable && is_entity_on_ref(state));
      }
      lv_label_set_text(icon_lbl, (!unavailable && is_entity_on_ref(state)) ? icon_on : icon_off);
    })
  );
}

inline void subscribe_sensor_text_card_value(lv_obj_t *text_lbl, const ParsedCfg &p,
                                             lv_obj_t *availability_obj = nullptr,
                                             bool active_color = false,
                                             uint32_t on_color = DEFAULT_SLIDER_COLOR,
                                             uint32_t sensor_color = TERTIARY_GREY) {
  if (p.sensor.empty()) return;
  ha_subscribe_state(
    p.sensor,
    std::function<void(esphome::StringRef)>(
      [text_lbl, p, availability_obj, active_color, on_color, sensor_color](esphome::StringRef state) {
      bool unavailable = ha_state_unavailable_ref(state);
      apply_sensor_active_color(availability_obj, active_color, state,
        on_color, sensor_color, unavailable);
      set_wrapped_button_label_text(text_lbl, sensor_state_display_text(p, state));
    })
  );
}

inline void subscribe_weather_state(lv_obj_t *icon_lbl, lv_obj_t *text_lbl, const std::string &entity_id) {
  ESP_LOGI("weather", "Subscribing to current weather state for %s", entity_id.c_str());
  uint32_t generation = ha_subscription_generation();
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>([icon_lbl, text_lbl, entity_id, generation](esphome::StringRef state) {
      if (generation != ha_subscription_generation()) return;
      std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
      ESP_LOGI("weather", "Current weather state for %s: %s", entity_id.c_str(), state_text.c_str());
      lv_label_set_text(icon_lbl, weather_icon_for_state(state_text));
      lv_label_set_text(text_lbl, weather_label_for_state(state_text).c_str());
      notify_dashboard_content_changed();
    })
  );
}

inline void subscribe_garage_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                   TransientStatusLabel *status_label,
                                   const char *closed_icon, const char *open_icon,
                                   const std::string &entity_id,
                                   bool persistent_status = false) {
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon, persistent_status](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool active = garage_state_is_active(state_text);
        set_card_checked_state(btn_ptr, active);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state_text) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state_text),
          persistent_status ? false : garage_state_releases_label(state_text));
      })
  );
}

inline void subscribe_gate_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                 TransientStatusLabel *status_label,
                                 const char *closed_icon, const char *open_icon,
                                 const std::string &entity_id,
                                 bool persistent_status = false) {
  register_ha_control_availability(btn_ptr, btn_ptr);
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon, persistent_status](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool unavailable = ha_state_unavailable_ref(state);
        apply_control_availability(btn_ptr, btn_ptr, !unavailable);
        bool active = garage_state_is_active(state_text);
        set_card_checked_state(btn_ptr, active);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state_text) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state_text),
          persistent_status ? false : garage_state_releases_label(state_text));
      })
  );
}

inline void subscribe_cover_toggle_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                         TransientStatusLabel *status_label,
                                         const char *closed_icon, const char *open_icon,
                                         const std::string &entity_id) {
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool active = cover_toggle_state_is_active(state_text);
        set_card_checked_state(btn_ptr, active);
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
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, locked_icon, unlocked_icon, ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->state = state_text;
        bool active = lock_state_is_active(state_text);
        set_card_checked_state(btn_ptr, active);
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
  ha_subscribe_attribute(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([status_label](esphome::StringRef name) {
      transient_status_label_set_steady(status_label, string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN));
    })
  );
}

inline void subscribe_friendly_name(ToggleTextSensorCtx *ctx,
                                    const std::string &entity_id) {
  ha_subscribe_attribute(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      if (!ctx) return;
      ctx->steady_text = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      if (!ctx->on) apply_toggle_text_sensor_label(ctx);
    })
  );
}

inline void subscribe_friendly_name(lv_obj_t *text_lbl, const std::string &entity_id) {
  ha_subscribe_attribute(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([text_lbl](esphome::StringRef name) {
      set_wrapped_button_label_text(text_lbl, string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN));
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
                                   bool disable_interaction = true,
                                   bool (*is_active_state)(esphome::StringRef) = is_entity_on_ref) {
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, sensor_ctr, slot_has_sensor, slot_sensor_text_mode,
       slot_has_icon_on, slot_icon_off, slot_icon_on, text_sensor_ctx,
       is_active_state](esphome::StringRef state) {
        bool on = is_active_state(state);
        set_card_checked_state(btn_ptr, on);

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

struct ActionCardStateCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  lv_obj_t *sensor_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  bool show_icon_state = false;
  bool show_text_state = false;
  bool show_numeric_state = false;
  bool has_icon_on = false;
  const char *icon_off = nullptr;
  const char *icon_on = nullptr;
  int precision = 0;
  std::string unit;
};

inline ActionCardStateCtx *create_action_card_state_context(const BtnSlot &s,
                                                            const ParsedCfg &p) {
  ActionCardStateCtx *ctx = new ActionCardStateCtx();
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->text_lbl = s.text_lbl;
  ctx->sensor_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->show_icon_state = action_card_state_icon_mode(p);
  ctx->show_text_state = action_card_state_text_mode(p);
  ctx->show_numeric_state = action_card_state_numeric_mode(p);
  ctx->has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
  ctx->icon_off = (p.icon.empty() || p.icon == "Auto") ? find_icon("Flash") : find_icon(p.icon.c_str());
  ctx->icon_on = ctx->has_icon_on ? find_icon(p.icon_on.c_str()) : ctx->icon_off;
  ctx->precision = parse_precision(action_card_state_precision(p));
  ctx->unit = trim_display_unit(action_card_state_unit(p));
  return ctx;
}

inline void apply_action_card_display_value(ActionCardStateCtx *ctx,
                                            esphome::StringRef state,
                                            bool unavailable) {
  if (!ctx) return;
  if (ctx->show_icon_state && ctx->icon_lbl) {
    lv_label_set_text(ctx->icon_lbl, (!unavailable && is_entity_on_ref(state)) ? ctx->icon_on : ctx->icon_off);
    return;
  }
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

inline void subscribe_action_card_display_state(ActionCardStateCtx *ctx,
                                                const std::string &entity_id) {
  if (!ctx || entity_id.empty()) return;
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>([ctx, entity_id](esphome::StringRef state) {
      bool unavailable = ha_entity_state_unavailable_ref(entity_id, state);
      bool active = !unavailable && (ctx->show_numeric_state
        ? numeric_state_positive_ref(state)
        : is_entity_on_ref(state));
      set_card_checked_state(ctx->btn, active);
      apply_action_card_display_value(ctx, state, unavailable);
    })
  );
}
