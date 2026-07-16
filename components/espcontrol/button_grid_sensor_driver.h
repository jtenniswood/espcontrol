#pragma once

// Shared lifecycle driver for Home Assistant sensors and the local-sensor
// compatibility form. Numeric, text, icon, duration, and local ESPHome values
// all use the same setup/binding path on the main grid and subpages.

namespace espcontrol::cards {

inline bool sensor_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::SENSOR;
}

inline bool sensor_driver_is_local(
    const ParsedCfg &config, const Context &context) {
  return context.runtime.type == card_runtime::CardTypeId::LOCAL_SENSOR ||
         sensor_card_local_sensor(config);
}

inline bool sensor_driver_is_text(
    const ParsedCfg &config, const Context &context) {
  return sensor_driver_is_local(config, context)
    ? config.precision == "text"
    : is_text_sensor_card(config);
}

inline void sensor_driver_apply_background(
    BtnSlot &slot, const CardPalette &palette) {
  if (!palette.has_sensor_color) return;
  lv_obj_set_style_bg_color(
    slot.btn, lv_color_hex(palette.sensor_val),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
      static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
}

inline bool sensor_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette) {
  if (!sensor_driver_matches(context)) return false;

  const bool local = sensor_driver_is_local(config, context);
  if ((local && config.entity.empty()) || (!local && config.sensor.empty())) {
    return true;
  }

  sensor_driver_apply_background(slot, palette);
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);

  if (sensor_driver_is_text(config, context)) {
    setup_toggle_visual(slot, config);
    lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
    set_wrapped_button_label_text(slot.text_lbl, "--");
    return true;
  }

  if (!local && config.precision == "icon") {
    lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
    const char *icon = (config.icon.empty() || config.icon == "Auto")
      ? find_icon("Auto")
      : find_icon(config.icon.c_str());
    lv_label_set_text(slot.icon_lbl, icon);
    if (!config.label.empty()) lv_label_set_text(slot.text_lbl, config.label.c_str());
    return true;
  }

  lv_obj_add_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (local) lv_label_set_text(slot.sensor_lbl, "--");
  if (config.precision == "time") lv_label_set_text(slot.unit_lbl, "");
  if (!config.unit.empty()) {
    const std::string unit = trim_display_unit(config.unit);
    lv_label_set_text(slot.unit_lbl, unit.c_str());
  }
  if (!config.label.empty()) lv_label_set_text(slot.text_lbl, config.label.c_str());
  return true;
}

inline bool sensor_driver_attach_interaction(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!sensor_driver_matches(context)) return false;
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  return true;
}

inline bool sensor_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const DisplayProfile &display, int row_span, int col_span) {
  if (!sensor_driver_matches(context)) return false;
  if (sensor_driver_is_local(config, context) ||
      sensor_driver_is_text(config, context) || config.precision == "icon") {
    return true;
  }
  if (large_number_square_card_layout(row_span, col_span) &&
      card_large_numbers_active_for_layout(config, row_span, col_span) &&
      display_large_sensor_font(display)) {
    apply_large_sensor_number_style(
      slot, display_large_sensor_font(display),
      display_large_sensor_unit_offset_percent(display));
  }
  return true;
}

inline bool sensor_driver_cleanup(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  // Duration-card allocations are owned by the existing grid allocation
  // tracker. Local sensor callbacks and controls retain their established
  // registry lifetime across dashboard rebuilds.
  return sensor_driver_matches(context);
}

inline void sensor_driver_register_local_value(
    BtnSlot &slot, const ParsedCfg &config) {
  const bool is_text = config.precision == "text";
  LocalSensorControl control;
  control.key = config.entity;
  control.is_text = is_text;
  control.precision = 0;
  control.sensor_lbl = is_text ? nullptr : slot.sensor_lbl;
  control.text_lbl = is_text ? slot.text_lbl : nullptr;
  if (!is_text && !config.precision.empty()) {
    control.precision = atoi(config.precision.c_str());
  }

  auto &registry = local_sensor_registry();
  bool found = false;
  for (auto &existing : registry) {
    if (existing.key != control.key) continue;
    existing = control;
    found = true;
    break;
  }
  if (!found) registry.push_back(control);

#ifdef USE_SENSOR
  if (!is_text) {
    for (auto *esp_sensor : esphome::App.get_sensors()) {
      char object_id[128];
      if (std::string(esp_sensor->get_object_id_to(object_id).c_str()) != control.key) continue;
      auto *label = control.sensor_lbl;
      const int precision = control.precision;
      esp_sensor->add_on_state_callback([label, precision](float value) {
        if (!label || std::isnan(value)) return;
        char buffer[32];
        if (precision == 1) snprintf(buffer, sizeof(buffer), "%.1f", value);
        else if (precision == 2) snprintf(buffer, sizeof(buffer), "%.2f", value);
        else snprintf(buffer, sizeof(buffer), "%.0f", value);
        lv_label_set_text(label, buffer);
      });
      if (!std::isnan(esp_sensor->state) && control.sensor_lbl) {
        char buffer[32];
        if (control.precision == 1) snprintf(buffer, sizeof(buffer), "%.1f", esp_sensor->state);
        else if (control.precision == 2) snprintf(buffer, sizeof(buffer), "%.2f", esp_sensor->state);
        else snprintf(buffer, sizeof(buffer), "%.0f", esp_sensor->state);
        lv_label_set_text(control.sensor_lbl, buffer);
      }
      break;
    }
  }
#endif
#ifdef USE_TEXT_SENSOR
  if (is_text) {
    for (auto *esp_text_sensor : esphome::App.get_text_sensors()) {
      char object_id[128];
      if (std::string(esp_text_sensor->get_object_id_to(object_id).c_str()) != control.key) continue;
      auto *label = control.text_lbl;
      esp_text_sensor->add_on_state_callback([label](std::string value) {
        if (label) set_wrapped_button_label_text(label, value);
      });
      if (!esp_text_sensor->state.empty() && control.text_lbl) {
        set_wrapped_button_label_text(control.text_lbl, esp_text_sensor->state);
      }
      break;
    }
  }
#endif
}

inline bool sensor_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette) {
  if (!sensor_driver_matches(context)) return false;

  if (sensor_driver_is_local(config, context)) {
    if (!config.entity.empty()) sensor_driver_register_local_value(slot, config);
    return true;
  }
  if (is_text_sensor_card(config)) {
    if (!config.sensor.empty()) {
      subscribe_sensor_text_card_value(
        slot.text_lbl, config, slot.btn, sensor_active_color_enabled(config),
        palette.on_val, palette.sensor_val);
    }
    return true;
  }
  if (!config.sensor.empty()) {
    if (config.precision == "icon") {
      subscribe_sensor_icon_state(slot.btn, slot.icon_lbl, config);
    } else if (config.precision == "time") {
      TimeSensorCtx *time = slot.config == nullptr
        ? grid_delete_with_owner(slot.btn, new TimeSensorCtx())
        : grid_track_runtime_allocation(slot.btn, new TimeSensorCtx());
      time->sensor_lbl = slot.sensor_lbl;
      time->unit_lbl = slot.unit_lbl;
      subscribe_time_sensor_value(
        time, config.sensor,
        cfg_option_value(config.options, SENSOR_TIME_UNIT_OPTION));
    } else {
      subscribe_sensor_value(
        slot.sensor_lbl, config.sensor, parse_precision(config.precision),
        slot.unit_lbl, config.unit, slot.btn,
        sensor_active_color_enabled(config), palette.on_val,
        palette.sensor_val);
    }
    if (config.label.empty()) subscribe_friendly_name(slot.text_lbl, config.sensor);
  }
  return true;
}

}  // namespace espcontrol::cards
