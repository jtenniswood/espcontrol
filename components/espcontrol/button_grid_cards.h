#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Configure a button as a read-only sensor card (non-clickable, shows value + unit)
inline void setup_sensor_card(BtnSlot &s, const ParsedCfg &p,
                              bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  if (p.precision == "icon") {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    const char *icon_cp = (p.icon.empty() || p.icon == "Auto")
      ? find_icon("Auto") : find_icon(p.icon.c_str());
    lv_label_set_text(s.icon_lbl, icon_cp);
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    return;
  }
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (!p.unit.empty()) {
    std::string unit = trim_display_unit(p.unit);
    lv_label_set_text(s.unit_lbl, unit.c_str());
  }
  if (!p.label.empty()) {
    lv_label_set_text(s.text_lbl, p.label.c_str());
  }
}

#include "button_grid_datetime_cards.h"

inline void setup_weather_card(BtnSlot &s, bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.icon_lbl, find_icon("Weather Cloudy"));
  lv_label_set_text(s.sensor_lbl, "");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, espcontrol_i18n("Cloudy"));
}

inline bool weather_card_shows_forecast(const ParsedCfg &p) {
  return card_runtime_weather_forecast_supported() &&
    (p.type == "weather_forecast" ||
     (p.type == "weather" && card_runtime_weather_forecast_precision(p.precision)));
}

inline std::string weather_card_forecast_day(const ParsedCfg &p) {
  return p.precision == "today" ? "today" : "tomorrow";
}

inline void setup_weather_forecast_card(BtnSlot &s, const ParsedCfg &p,
                                        bool has_sensor_color, uint32_t sensor_val,
                                        int width_compensation_percent = 100) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.sensor_lbl, "--/--");
  lv_label_set_text(s.unit_lbl, display_temperature_unit_symbol());
  std::string day = weather_card_forecast_day(p);
  std::string label = p.label.empty()
    ? (day == "today" ? espcontrol_i18n(std::string("Today")) : espcontrol_i18n(std::string("Tomorrow")))
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  apply_width_compensation(s.sensor_container, width_compensation_percent);
  apply_width_compensation(s.text_lbl, width_compensation_percent);
  register_weather_forecast_card(s.btn, s.sensor_lbl, s.unit_lbl, s.text_lbl,
    p.entity, day, p.label);
}

inline void apply_push_button_transition(lv_obj_t *btn);
inline void clear_push_button_transition(lv_obj_t *btn);

inline void setup_garage_card(BtnSlot &s, const ParsedCfg &p) {
  if (garage_command_mode(p.sensor)) {
    lv_label_set_text(s.icon_lbl, garage_command_icon(p));
    lv_label_set_text(s.text_lbl, garage_card_show_status(p) ? "--" : garage_card_label(p));
    apply_push_button_transition(s.btn);
    return;
  }
  lv_label_set_text(s.icon_lbl, garage_closed_icon(p.icon));
  lv_label_set_text(s.text_lbl, garage_card_show_status(p) ? "--" : garage_card_label(p));
}

inline void setup_gate_card(BtnSlot &s, const ParsedCfg &p) {
  if (gate_command_mode(p.sensor)) {
    lv_label_set_text(s.icon_lbl, gate_command_icon(p));
    lv_label_set_text(s.text_lbl, gate_card_show_status(p) ? "--" : gate_card_label(p));
    apply_push_button_transition(s.btn);
    return;
  }
  lv_label_set_text(s.icon_lbl, gate_closed_icon(p.icon));
  lv_label_set_text(s.text_lbl, gate_card_show_status(p) ? "--" : gate_card_label(p));
}

inline void setup_lock_card(BtnSlot &s, const ParsedCfg &p) {
  if (lock_command_mode(p.sensor)) {
    lv_label_set_text(s.icon_lbl, lock_command_icon(p));
    lv_label_set_text(s.text_lbl, lock_card_label(p));
    apply_push_button_transition(s.btn);
    return;
  }
  lv_label_set_text(s.icon_lbl, lock_locked_icon(p.icon));
  lv_label_set_text(s.text_lbl, lock_card_label(p));
}

inline const char *screen_lock_locked_icon(const ParsedCfg &p) {
  (void) p;
  return find_icon("Lock");
}

inline const char *screen_lock_unlocked_icon(const ParsedCfg &p) {
  (void) p;
  return find_icon("Lock Open");
}

inline std::string screen_lock_card_label() {
  return screen_lock_enabled()
    ? espcontrol_i18n(std::string("Screen Locked"))
    : espcontrol_i18n(std::string("Screen Unlocked"));
}

inline void screen_lock_register_card(const BtnSlot &s, const ParsedCfg &p) {
  ScreenLockCardRef ref;
  ref.btn = s.btn;
  ref.icon_lbl = s.icon_lbl;
  ref.text_lbl = s.text_lbl;
  ref.locked_icon = screen_lock_locked_icon(p);
  ref.unlocked_icon = screen_lock_unlocked_icon(p);
  screen_lock_card_refs().push_back(ref);
  screen_lock_register_controlled_button(s.btn);
}

inline void setup_screen_lock_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl,
    screen_lock_enabled() ? screen_lock_locked_icon(p) : screen_lock_unlocked_icon(p));
  std::string label = screen_lock_card_label();
  lv_label_set_text(s.text_lbl, label.c_str());
  screen_lock_register_card(s, p);
  apply_push_button_transition(s.btn);
}

inline void apply_push_button_transition(lv_obj_t *btn) {
  if (!btn) return;
  static const lv_style_prop_t push_props[] = {LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV};
  static lv_style_transition_dsc_t push_trans;
  static bool push_trans_inited = false;
  if (!push_trans_inited) {
    lv_style_transition_dsc_init(&push_trans, push_props, lv_anim_path_ease_out, 400, 0, NULL);
    push_trans_inited = true;
  }
  lv_obj_set_style_transition(btn, &push_trans,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
}

inline void clear_push_button_transition(lv_obj_t *btn) {
  if (!btn) return;
  lv_obj_remove_local_style_prop(btn, LV_STYLE_TRANSITION,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
}

inline void setup_internal_relay_card(BtnSlot &s, const ParsedCfg &p) {
  bool push_mode = internal_relay_push_mode(p);
  std::string label = internal_relay_label(p);
  lv_label_set_text(s.text_lbl, label.c_str());
  const char *icon_off = internal_relay_icon(p, push_mode);
  lv_label_set_text(s.icon_lbl, icon_off);
  if (push_mode) {
    apply_push_button_transition(s.btn);
    return;
  }
  bool has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
  const char *icon_on = has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
  apply_internal_relay_state(s.btn, s.icon_lbl, internal_relay_state(p.entity),
    has_icon_on, icon_off, icon_on);
}

// Set icon and label on a toggle/push button based on its config
inline void setup_toggle_visual(BtnSlot &s, const ParsedCfg &p) {
  if (!p.entity.empty()) {
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    const char* icon_cp = "\U000F0493";
    if (p.icon.empty() || p.icon == "Auto") {
      std::string domain = p.entity.substr(0, p.entity.find('.'));
      icon_cp = domain_default_icon(domain);
    } else {
      icon_cp = find_icon(p.icon.c_str());
    }
    lv_label_set_text(s.icon_lbl, icon_cp);

    if (!p.sensor.empty()) {
      if (!p.unit.empty()) {
        std::string unit = trim_display_unit(p.unit);
        lv_label_set_text(s.unit_lbl, unit.c_str());
      }
    }
  } else {
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    if (!p.icon.empty() && p.icon != "Auto") {
      lv_label_set_text(s.icon_lbl, find_icon(p.icon.c_str()));
    } else if (p.type == "push") {
      lv_label_set_text(s.icon_lbl, "\U000F0741");
      apply_push_button_transition(s.btn);
    }
    if (p.type == "push" && p.label.empty()) {
      lv_label_set_text(s.text_lbl, espcontrol_i18n("Push"));
    }
  }
}

inline void setup_local_action_card(BtnSlot &s, const ParsedCfg &p);

inline void setup_action_card(BtnSlot &s, const ParsedCfg &p) {
  if (action_card_local_action(p)) {
    setup_local_action_card(s, p);
    return;
  }
  std::string action_label = p.label.empty()
    ? (p.entity.empty() ? espcontrol_i18n(std::string("Action")) : p.entity)
    : p.label;
  lv_label_set_text(s.text_lbl, action_label.c_str());
  const char *icon_cp = (p.icon.empty() || p.icon == "Auto") ? find_icon("Flash") : find_icon(p.icon.c_str());
  lv_label_set_text(s.icon_lbl, icon_cp);
  if (action_card_state_icon_mode(p) || action_card_state_text_mode(p)) {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  } else if (action_card_state_numeric_mode(p)) {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s.sensor_lbl, "--");
    std::string unit = trim_display_unit(action_card_state_unit(p));
    lv_label_set_text(s.unit_lbl, unit.c_str());
  }
  apply_push_button_transition(s.btn);
}

inline void setup_local_action_card(BtnSlot &s, const ParsedCfg &p) {
  std::string label = p.label.empty() ? (p.entity.empty() ? "Local Action" : sentence_cap_text(p.entity)) : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  const char *icon_cp = (p.icon.empty() || p.icon == "Auto") ? find_icon("Gesture Tap") : find_icon(p.icon.c_str());
  lv_label_set_text(s.icon_lbl, icon_cp);
  apply_push_button_transition(s.btn);
}

inline void send_local_sensor_update(const std::string &key, float value) {
  for (auto &s : local_sensor_registry()) {
    if (s.key != key || s.is_text) continue;
    char buf[32];
    if (s.precision == 1) snprintf(buf, sizeof(buf), "%.1f", value);
    else if (s.precision == 2) snprintf(buf, sizeof(buf), "%.2f", value);
    else snprintf(buf, sizeof(buf), "%.0f", value);
    if (s.sensor_lbl) lv_label_set_text(s.sensor_lbl, buf);
    return;
  }
  ESP_LOGW("espcontrol", "Local sensor '%s' not registered", key.c_str());
}

inline void send_local_sensor_update(const std::string &key, const char *value) {
  for (auto &s : local_sensor_registry()) {
    if (s.key != key || !s.is_text) continue;
    if (s.text_lbl) set_wrapped_button_label_text(s.text_lbl, value ? value : "--");
    return;
  }
  ESP_LOGW("espcontrol", "Local sensor '%s' not registered", key.c_str());
}

inline void setup_text_sensor_card(BtnSlot &s, const ParsedCfg &p,
                                   bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  setup_toggle_visual(s, p);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  set_wrapped_button_label_text(s.text_lbl, "--");
}

inline void setup_local_sensor_card(BtnSlot &s, const ParsedCfg &p,
                                    bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);

  bool is_text = (p.precision == "text");
  LocalSensorControl ctrl;
  ctrl.key = p.entity;
  ctrl.is_text = is_text;
  ctrl.precision = 0;
  ctrl.sensor_lbl = nullptr;
  ctrl.text_lbl = nullptr;

  if (is_text) {
    setup_toggle_visual(s, p);
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    set_wrapped_button_label_text(s.text_lbl, "--");
    ctrl.text_lbl = s.text_lbl;
  } else {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s.sensor_lbl, "--");
    if (!p.unit.empty()) lv_label_set_text(s.unit_lbl, trim_display_unit(p.unit).c_str());
    if (!p.label.empty()) lv_label_set_text(s.text_lbl, p.label.c_str());
    ctrl.sensor_lbl = s.sensor_lbl;
    if (!p.precision.empty()) ctrl.precision = atoi(p.precision.c_str());
  }

  auto &reg = local_sensor_registry();
  bool found = false;
  for (auto &existing : reg) {
    if (existing.key == ctrl.key) { existing = ctrl; found = true; break; }
  }
  if (!found) reg.push_back(ctrl);

#ifdef USE_SENSOR
  if (!is_text) {
    for (auto *esp_s : esphome::App.get_sensors()) {
      char oid_buf[128];
      if (std::string(esp_s->get_object_id_to(oid_buf).c_str()) != ctrl.key) continue;
      auto *lbl = ctrl.sensor_lbl;
      int prec = ctrl.precision;
      esp_s->add_on_state_callback([lbl, prec](float val) {
        if (!lbl || std::isnan(val)) return;
        char buf[32];
        if (prec == 1) snprintf(buf, sizeof(buf), "%.1f", val);
        else if (prec == 2) snprintf(buf, sizeof(buf), "%.2f", val);
        else snprintf(buf, sizeof(buf), "%.0f", val);
        lv_label_set_text(lbl, buf);
      });
      if (!std::isnan(esp_s->state) && ctrl.sensor_lbl) {
        char buf[32];
        if (ctrl.precision == 1) snprintf(buf, sizeof(buf), "%.1f", esp_s->state);
        else if (ctrl.precision == 2) snprintf(buf, sizeof(buf), "%.2f", esp_s->state);
        else snprintf(buf, sizeof(buf), "%.0f", esp_s->state);
        lv_label_set_text(ctrl.sensor_lbl, buf);
      }
      break;
    }
  }
#endif
#ifdef USE_TEXT_SENSOR
  if (is_text) {
    for (auto *esp_ts : esphome::App.get_text_sensors()) {
      char oid_buf[128];
      if (std::string(esp_ts->get_object_id_to(oid_buf).c_str()) != ctrl.key) continue;
      auto *lbl = ctrl.text_lbl;
      esp_ts->add_on_state_callback([lbl](std::string val) {
        if (!lbl) return;
        set_wrapped_button_label_text(lbl, val);
      });
      if (!esp_ts->state.empty() && ctrl.text_lbl)
        set_wrapped_button_label_text(ctrl.text_lbl, esp_ts->state);
      break;
    }
  }
#endif
}

inline const char *door_window_closed_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(door_window_closed_icon_name(p.precision));
}

inline const char *door_window_open_icon(const ParsedCfg &p) {
  if (!p.icon_on.empty() && p.icon_on != "Auto") return find_icon(p.icon_on.c_str());
  return find_icon(door_window_open_icon_name(p.precision));
}

inline const char *presence_clear_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon("Motion Sensor Off");
}

inline const char *presence_detected_icon(const ParsedCfg &p) {
  if (!p.icon_on.empty() && p.icon_on != "Auto") return find_icon(p.icon_on.c_str());
  return find_icon("Motion Sensor");
}

inline bool subpage_parent_sensor_state_enabled(const ParsedCfg &p) {
  return p.type == "subpage" &&
         !p.sensor.empty() &&
         p.sensor != "indicator";
}

inline bool subpage_parent_text_state_enabled(const ParsedCfg &p) {
  return subpage_parent_sensor_state_enabled(p) &&
         p.precision == "text";
}

inline bool subpage_parent_icon_entity_state_enabled(const ParsedCfg &p) {
  return p.type == "subpage" &&
         p.sensor == "indicator" &&
         !p.entity.empty();
}

inline void setup_subpage_parent_state_card(BtnSlot &s, const ParsedCfg &p,
                                            const lv_font_t *value_font,
                                            bool subpage_chevron_enabled = true,
                                            int subpage_chevron_x = 0,
                                            int subpage_chevron_y = 2,
                                            int subpage_chevron_text_width_percent = 94) {
  setup_toggle_visual(s, p);
  if (p.precision == "text") {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    set_wrapped_button_label_text(s.text_lbl, "--");
    set_subpage_chevron_visible(
      s, subpage_chevron_enabled, subpage_chevron_x, subpage_chevron_y,
      subpage_chevron_text_width_percent);
    return;
  }

  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (value_font) lv_obj_set_style_text_font(s.sensor_lbl, value_font, LV_PART_MAIN);
  lv_label_set_text(s.sensor_lbl, "--");
  std::string unit = trim_display_unit(p.unit);
  lv_label_set_text(s.unit_lbl, unit.c_str());
  std::string subpage_label = p.label.empty() ? espcontrol_i18n(std::string("Subpage")) : p.label;
  lv_label_set_text(s.text_lbl, subpage_label.c_str());
  set_subpage_chevron_visible(
    s, subpage_chevron_enabled, subpage_chevron_x, subpage_chevron_y,
    subpage_chevron_text_width_percent);
}
