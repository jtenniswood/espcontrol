#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

struct PlantCardCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *sensor_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  std::string entity_id;
  std::string mode;
  std::string label;
  std::string state = "unknown";
  std::string problem;
  std::string value;
  std::string unit;
  std::string friendly_name_label;
  bool available = false;
  bool use_friendly_name_label = false;
};

inline std::string plant_card_mode(const std::string &mode) {
  return normalize_plant_card_mode(mode);
}

inline bool plant_card_metric_mode(const std::string &mode) {
  return plant_card_mode(mode) != "status";
}

inline const char *plant_metric_fallback_unit(const std::string &mode) {
  std::string normalized = plant_card_mode(mode);
  if (normalized == "moisture" || normalized == "battery") return "%";
  if (normalized == "temperature") return display_temperature_unit_symbol();
  if (normalized == "conductivity") return "\xC2\xB5S/cm";
  if (normalized == "brightness") return "lx";
  return "";
}

inline const char *plant_metric_label(const std::string &mode) {
  std::string normalized = plant_card_mode(mode);
  if (normalized == "moisture") return "Moisture";
  if (normalized == "battery") return "Battery";
  if (normalized == "temperature") return "Temperature";
  if (normalized == "conductivity") return "Conductivity";
  if (normalized == "brightness") return "Brightness";
  return "Status";
}

inline void setup_plant_card(BtnSlot &s, const ParsedCfg &p,
                             bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  std::string mode = plant_card_mode(p.precision);
  if (plant_card_metric_mode(mode)) {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s.sensor_lbl, "--");
    lv_label_set_text(s.unit_lbl, plant_metric_fallback_unit(mode));
    std::string label = p.label.empty() ? espcontrol_i18n(std::string(plant_metric_label(mode))) : p.label;
    lv_label_set_text(s.text_lbl, label.c_str());
    return;
  }
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.icon_lbl, find_icon("Leaf"));
  std::string label = p.label.empty() ? espcontrol_i18n(std::string("Plant")) : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
}

inline PlantCardCtx *create_plant_card_context(const BtnSlot &s,
                                               const ParsedCfg &p) {
  PlantCardCtx *ctx = new PlantCardCtx();
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->sensor_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->text_lbl = s.text_lbl;
  ctx->entity_id = p.entity;
  ctx->mode = plant_card_mode(p.precision);
  ctx->label = p.label.empty()
    ? espcontrol_i18n(std::string(plant_card_metric_mode(ctx->mode) ? plant_metric_label(ctx->mode) : "Plant"))
    : p.label;
  ctx->unit = plant_metric_fallback_unit(ctx->mode);
  ctx->use_friendly_name_label = p.label.empty() && plant_card_metric_mode(ctx->mode);
  return ctx;
}

inline std::string plant_status_display_text(const PlantCardCtx *ctx) {
  if (!ctx) return "Unknown";
  if (!ctx->available) {
    if (ctx->state == "unavailable") return "Unavailable";
    return "Unknown";
  }
  if (ctx->state == "problem") {
    return ctx->problem.empty() ? std::string("Problem") : ctx->problem;
  }
  if (ctx->state == "ok") return ctx->label.empty() ? std::string("OK") : std::string("OK");
  return ctx->state.empty() ? ctx->label : ctx->state;
}

inline void apply_plant_status_card(PlantCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->btn) apply_control_availability(ctx->btn, ctx->btn, ctx->available);
  if (ctx->icon_lbl) lv_label_set_text(ctx->icon_lbl, find_icon("Leaf"));
  if (ctx->text_lbl) set_wrapped_button_label_text(ctx->text_lbl, plant_status_display_text(ctx));
  notify_dashboard_content_changed();
}

inline void apply_plant_metric_card(PlantCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->btn) apply_control_availability(ctx->btn, ctx->btn, ctx->available);
  if (ctx->sensor_lbl) lv_label_set_text(ctx->sensor_lbl, ctx->available && !ctx->value.empty() ? ctx->value.c_str() : "--");
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, ctx->available ? ctx->unit.c_str() : plant_metric_fallback_unit(ctx->mode));
  if (ctx->text_lbl) {
    const std::string &label = ctx->use_friendly_name_label && !ctx->friendly_name_label.empty()
      ? ctx->friendly_name_label
      : ctx->label;
    set_wrapped_button_label_text(ctx->text_lbl, label);
  }
  notify_dashboard_content_changed();
}

inline void subscribe_plant_metric_friendly_name(PlantCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ha_subscribe_attribute(
    ctx->entity_id,
    std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      ctx->label = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      ctx->friendly_name_label = ctx->label;
      apply_plant_metric_card(ctx);
    })
  );
}

inline std::string plant_metric_unit_from_dict(const std::string &dict,
                                               const std::string &mode) {
  std::string key = "\"" + mode + "\"";
  size_t key_pos = dict.find(key);
  if (key_pos == std::string::npos) key_pos = dict.find("'" + mode + "'");
  if (key_pos == std::string::npos) return "";
  size_t colon = dict.find(':', key_pos);
  if (colon == std::string::npos) return "";
  size_t quote = dict.find_first_of("\"'", colon + 1);
  if (quote == std::string::npos) return "";
  char quote_ch = dict[quote];
  size_t end = dict.find(quote_ch, quote + 1);
  if (end == std::string::npos || end <= quote + 1) return "";
  return trim_display_unit(dict.substr(quote + 1, end - quote - 1));
}

inline void subscribe_plant_card(PlantCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn);
  apply_control_availability(ctx->btn, ctx->btn, false);
  if (!plant_card_metric_mode(ctx->mode)) {
    ha_subscribe_state(
      ctx->entity_id,
      std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
        ctx->state = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->available = !ha_state_unavailable_ref(state);
        apply_plant_status_card(ctx);
      })
    );
    ha_subscribe_attribute(
      ctx->entity_id,
      "problem",
      std::function<void(esphome::StringRef)>([ctx](esphome::StringRef problem) {
        ctx->problem = string_ref_limited(problem, HA_STATE_TEXT_MAX_LEN);
        apply_plant_status_card(ctx);
      })
    );
    return;
  }
  ha_subscribe_attribute(
    ctx->entity_id,
    ctx->mode,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      bool unavailable = ha_state_unavailable_ref(value);
      float parsed = 0.0f;
      ctx->available = !unavailable && parse_float_ref(value, parsed) && std::isfinite(parsed);
      if (ctx->available) {
        char buf[16];
        format_fixed_decimal(buf, sizeof(buf), parsed, 0);
        ctx->value = buf;
      } else {
        ctx->value.clear();
      }
      apply_plant_metric_card(ctx);
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id,
    "unit_of_measurement_dict",
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef units) {
      std::string parsed = plant_metric_unit_from_dict(string_ref_limited(units, 256), ctx->mode);
      ctx->unit = parsed.empty() ? plant_metric_fallback_unit(ctx->mode) : parsed;
      apply_plant_metric_card(ctx);
    })
  );
  if (ctx->use_friendly_name_label) subscribe_plant_metric_friendly_name(ctx);
}
