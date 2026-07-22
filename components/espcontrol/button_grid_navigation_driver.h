#pragma once

// Shared lifecycle driver for Subpage navigation cards. The grid still lays
// out child cards; this driver owns the parent visual, parent indicators,
// screen target registration, navigation click, and target cleanup.
// Contract coverage marker: "subpage".

namespace espcontrol::cards {

inline bool navigation_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::SUBPAGE;
}

inline bool navigation_driver_parent_sensor_state_enabled(
    const ParsedCfg &config, const Context &context) {
  return navigation_driver_matches(context) &&
         !config.sensor.empty() && config.sensor != "indicator";
}

inline bool navigation_driver_parent_text_state_enabled(
    const ParsedCfg &config, const Context &context) {
  return navigation_driver_parent_sensor_state_enabled(config, context) &&
         config.precision == "text";
}

inline bool navigation_driver_parent_entity_state_enabled(
    const ParsedCfg &config, const Context &context) {
  return navigation_driver_matches(context) &&
         config.sensor == "indicator" && !config.entity.empty();
}

inline bool navigation_driver_aggregates_child_state(
    const ParsedCfg &config, const Context &context) {
  return navigation_driver_matches(context) &&
         config.sensor == "indicator" && config.entity.empty();
}

inline bool navigation_driver_parent_has_alt_icon(
    const ParsedCfg &config, const Context &context) {
  return navigation_driver_matches(context) &&
         !config.icon_on.empty() && config.icon_on != "Auto";
}

inline const char *navigation_driver_parent_icon_on(
    const ParsedCfg &config, const Context &context) {
  return navigation_driver_parent_has_alt_icon(config, context)
    ? find_icon(config.icon_on.c_str()) : nullptr;
}

inline const char *navigation_driver_parent_icon_off(
    const ParsedCfg &config, const Context &context) {
  if (!navigation_driver_parent_has_alt_icon(config, context)) return nullptr;
  return (config.icon.empty() || config.icon == "Auto")
    ? "\U000F024B" : find_icon(config.icon.c_str());
}

inline bool navigation_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const GridConfig &grid, const DisplayProfile &display) {
  if (!navigation_driver_matches(context)) return false;
  if (navigation_driver_parent_sensor_state_enabled(config, context)) {
    setup_subpage_parent_state_card(
      slot, config, display_sensor_font(display),
      grid.subpage_chevrons_enabled, grid.subpage_chevron_x,
      grid.subpage_chevron_y, grid.subpage_chevron_text_width_percent);
  } else {
    setup_toggle_visual(slot, config);
  }
  return true;
}

inline bool navigation_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return navigation_driver_matches(context);
}

inline bool navigation_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &, const Context &context,
    const GridConfig &grid) {
  if (!navigation_driver_matches(context)) return false;
  set_subpage_chevron_visible(
    slot, grid.subpage_chevrons_enabled, grid.subpage_chevron_x,
    grid.subpage_chevron_y, grid.subpage_chevron_text_width_percent);
  return true;
}

inline bool navigation_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!navigation_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

struct NavigationDriverParentState {
  bool *has_sensor = nullptr;
  bool *sensor_text_mode = nullptr;
  bool *has_icon_on = nullptr;
  const char **icon_off = nullptr;
  const char **icon_on = nullptr;
};

inline bool navigation_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const NavigationDriverParentState &state) {
  if (!navigation_driver_matches(context)) return false;

  if (navigation_driver_parent_sensor_state_enabled(config, context)) {
    if (navigation_driver_parent_text_state_enabled(config, context)) {
      subscribe_text_sensor_value(slot.text_lbl, config.sensor);
    } else {
      subscribe_sensor_value(
        slot.sensor_lbl, config.sensor, parse_precision(config.precision),
        slot.unit_lbl, config.unit);
      if (config.label.empty()) {
        subscribe_friendly_name(slot.text_lbl, config.sensor);
      }
    }
    return true;
  }

  if (!navigation_driver_parent_entity_state_enabled(config, context)) {
    return true;
  }
  if (!state.has_sensor || !state.sensor_text_mode || !state.has_icon_on ||
      !state.icon_off || !state.icon_on) {
    return true;
  }

  *state.has_sensor = false;
  *state.sensor_text_mode = false;
  *state.has_icon_on = !config.icon_on.empty() && config.icon_on != "Auto";
  if (*state.has_icon_on) *state.icon_on = find_icon(config.icon_on.c_str());
  *state.icon_off = (config.icon.empty() || config.icon == "Auto")
    ? domain_default_icon(config.entity.substr(0, config.entity.find('.')))
    : find_icon(config.icon.c_str());

  if (config.label.empty()) {
    subscribe_friendly_name(slot.text_lbl, config.entity);
  }

  const std::string kind = normalize_subpage_kind(
    cfg_option_value(config.options, "subpage_kind"));
  if (kind == "climate") {
    subscribe_climate_subpage_parent_indicator(
      config.entity, slot.btn, slot.icon_lbl, *state.has_icon_on,
      *state.icon_off, *state.icon_on);
  } else if (kind == "lawn_mower") {
    subscribe_toggle_state(
      slot.btn, slot.icon_lbl, slot.sensor_container,
      state.has_sensor, state.sensor_text_mode, state.has_icon_on,
      state.icon_off, state.icon_on, nullptr, config.entity, false,
      lawn_mower_state_active_ref);
  } else {
    subscribe_toggle_state(
      slot.btn, slot.icon_lbl, slot.sensor_container,
      state.has_sensor, state.sensor_text_mode, state.has_icon_on,
      state.icon_off, state.icon_on, nullptr, config.entity, false);
  }
  return true;
}

struct NavigationDriverChildIndicators {
  bool child_was_on[MAX_SUBPAGE_ITEMS] = {};
  int parent_on_count[MAX_GRID_SLOTS] = {};
  int next_child = 0;
};

inline void navigation_driver_reset_child_indicators(
    NavigationDriverChildIndicators &state) {
  memset(state.child_was_on, 0, sizeof(state.child_was_on));
  memset(state.parent_on_count, 0, sizeof(state.parent_on_count));
  state.next_child = 0;
}

inline void navigation_driver_add_child_indicator(
    NavigationDriverChildIndicators &state,
    BtnSlot &parent_slot, int parent_index,
    const ParsedCfg &parent_config, const Context &parent_context,
    const std::string &entity_id,
    bool (*is_active_state)(esphome::StringRef) = is_entity_on_ref) {
  if (!navigation_driver_aggregates_child_state(
        parent_config, parent_context) || entity_id.empty()) {
    return;
  }
  const int child_index = state.next_child++;
  if (child_index >= MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s",
             entity_id.c_str());
    return;
  }

  const bool has_alt_icon = navigation_driver_parent_has_alt_icon(
    parent_config, parent_context);
  const char *on_glyph = navigation_driver_parent_icon_on(
    parent_config, parent_context);
  const char *off_glyph = navigation_driver_parent_icon_off(
    parent_config, parent_context);
  subscribe_subpage_parent_indicator(
    entity_id, parent_slot.btn, parent_slot.icon_lbl, parent_index,
    &state.child_was_on[child_index], has_alt_icon,
    off_glyph, on_glyph, state.parent_on_count, is_active_state);
}

inline bool navigation_driver_own_subpage(
    BtnSlot &parent_slot, const ParsedCfg &config, const Context &context,
    int slot_number, int display_order, lv_obj_t *screen) {
  if (!navigation_driver_matches(context) || !screen) return false;
  navigation_register_subpage(
    slot_number, display_order,
    normalize_subpage_kind(cfg_option_value(config.options, "subpage_kind")),
    screen);
  if (parent_slot.btn) lv_obj_set_user_data(parent_slot.btn, screen);
  return true;
}

inline bool navigation_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!navigation_driver_matches(context)) return false;
  lv_obj_t *screen = button
    ? static_cast<lv_obj_t *>(lv_obj_get_user_data(button)) : nullptr;
  if (screen) {
    button_grid_load_screen(screen);
  }
  return true;
}

}  // namespace espcontrol::cards
