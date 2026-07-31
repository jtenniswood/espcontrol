#pragma once

// Shared lifecycle driver for the "door_window" and "presence" runtime types.
// Both cards use the same visual, binding, interaction, layout, and cleanup
// stages; only their state interpretation, icons, and default label differ.

namespace espcontrol::cards {

inline bool status_entity_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::STATUS_ENTITY;
}

inline bool status_entity_driver_state_active(
    card_runtime::CardTypeId type, esphome::StringRef state) {
  using Type = card_runtime::CardTypeId;
  switch (type) {
    case Type::DOOR_WINDOW: return is_entity_on_ref(state);
    case Type::PRESENCE: return presence_detected_ref(state);
    default: return false;
  }
}

inline const char *status_entity_driver_inactive_icon(
    const ParsedCfg &config, const Context &context) {
  using Type = card_runtime::CardTypeId;
  switch (context.runtime.type) {
    case Type::DOOR_WINDOW: return door_window_closed_icon(config);
    case Type::PRESENCE: return presence_clear_icon(config);
    default: return find_icon("Auto");
  }
}

inline const char *status_entity_driver_active_icon(
    const ParsedCfg &config, const Context &context) {
  using Type = card_runtime::CardTypeId;
  switch (context.runtime.type) {
    case Type::DOOR_WINDOW: return door_window_open_icon(config);
    case Type::PRESENCE: return presence_detected_icon(config);
    default: return find_icon("Auto");
  }
}

inline std::string status_entity_driver_default_label(
    const ParsedCfg &config, const Context &context) {
  using Type = card_runtime::CardTypeId;
  switch (context.runtime.type) {
    case Type::DOOR_WINDOW:
      return normalize_door_window_subtype(config.precision) == "window"
        ? espcontrol_i18n(std::string("Window"))
        : espcontrol_i18n(std::string("Door"));
    case Type::PRESENCE:
      return espcontrol_i18n(std::string("Presence"));
    default:
      return "";
  }
}

inline bool status_entity_driver_active_color_enabled(
    const ParsedCfg &config, const Context &context) {
  using Type = card_runtime::CardTypeId;
  switch (context.runtime.type) {
    case Type::DOOR_WINDOW: return door_window_active_color_enabled(config);
    case Type::PRESENCE: return presence_active_color_enabled(config);
    default: return false;
  }
}

inline bool status_entity_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette) {
  if (!status_entity_driver_matches(context)) return false;
  if (config.sensor.empty()) return true;

  if (palette.has_sensor_color) {
    lv_obj_set_style_bg_color(
      slot.btn, lv_color_hex(palette.sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
        static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(
    slot.icon_lbl, status_entity_driver_inactive_icon(config, context));
  const std::string label = config.label.empty()
    ? status_entity_driver_default_label(config, context)
    : config.label;
  lv_label_set_text(slot.text_lbl, label.c_str());
  return true;
}

inline bool status_entity_driver_attach_interaction(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!status_entity_driver_matches(context)) return false;
  if (config.sensor.empty()) return true;
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  return true;
}

inline bool status_entity_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context, int, int) {
  // Status cards currently use the standard card layout. Keeping this stage
  // explicit means later family drivers can add layout rules without creating
  // separate main-grid and subpage wiring.
  return status_entity_driver_matches(context);
}

inline bool status_entity_driver_cleanup(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  // This pilot owns no dynamic allocations. The explicit cleanup stage keeps
  // lifecycle ownership consistent with drivers that will need it later.
  return status_entity_driver_matches(context);
}

inline bool status_entity_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette) {
  if (!status_entity_driver_matches(context)) return false;
  if (config.sensor.empty()) return true;

  const auto type = context.runtime.type;
  const char *inactive_icon = status_entity_driver_inactive_icon(config, context);
  const char *active_icon = status_entity_driver_active_icon(config, context);
  const bool active_color =
    status_entity_driver_active_color_enabled(config, context);
  ha_subscribe_state(
    config.sensor,
    std::function<void(esphome::StringRef)>(
      [btn = slot.btn, icon = slot.icon_lbl, type, inactive_icon, active_icon,
       active_color, on_color = palette.on_val,
       sensor_color = palette.sensor_val](esphome::StringRef state) {
        const bool unavailable = ha_state_unavailable_ref(state);
        const bool active = !unavailable &&
          status_entity_driver_state_active(type, state);
        lv_label_set_text(icon, active ? active_icon : inactive_icon);
        if (btn && active_color) {
          lv_obj_set_style_bg_color(
            btn, lv_color_hex(active ? on_color : sensor_color),
            static_cast<lv_style_selector_t>(LV_PART_MAIN) |
              static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
        }
      })
  );
  if (config.label.empty()) {
    subscribe_friendly_name(slot.text_lbl, config.sensor);
  }
  return true;
}

}  // namespace espcontrol::cards
