#pragma once

// Shared lifecycle driver for numeric and selectable cards. The same visual,
// binding, interaction, layout, and cleanup entry points serve main-grid and
// subpage cards while retaining the established slider, fan, and modal helpers.
// Contract coverage markers: "slider", "light_brightness", "light_temperature",
// "fan_speed", "fan_oscillate", "fan_direction", "fan_preset", "option_select",
// plus Action option-select compatibility.

namespace espcontrol::cards {

inline bool numeric_selectable_driver_option_select(
    const Context &context, const ParsedCfg &config) {
  using Driver = card_runtime::CardDriverId;
  return context.runtime.driver == Driver::OPTION_SELECT ||
         (context.runtime.driver == Driver::ACTION &&
          action_card_option_select(config));
}

inline bool numeric_selectable_driver_slider(
    const Context &context) {
  using Driver = card_runtime::CardDriverId;
  using Type = card_runtime::CardTypeId;
  return context.runtime.driver == Driver::NUMERIC ||
         context.runtime.driver == Driver::LIGHT_TEMPERATURE ||
         context.runtime.type == Type::FAN_SPEED;
}

inline bool numeric_selectable_driver_fan_action(
    const Context &context) {
  using Driver = card_runtime::CardDriverId;
  using Type = card_runtime::CardTypeId;
  return context.runtime.driver == Driver::FAN &&
         context.runtime.type != Type::FAN_SPEED &&
         context.runtime.type != Type::FAN_SWITCH;
}

inline bool numeric_selectable_driver_matches(
    const Context &context, const ParsedCfg &config) {
  if (context.legacy_dispatch) return false;
  return numeric_selectable_driver_option_select(context, config) ||
         numeric_selectable_driver_slider(context) ||
         numeric_selectable_driver_fan_action(context);
}

inline void numeric_selectable_driver_track_slider_cleanup(BtnSlot &slot) {
  lv_obj_t *slider = slot.sensor_container
    ? static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container))
    : nullptr;
  SliderCtx *state = slider
    ? static_cast<SliderCtx *>(lv_obj_get_user_data(slider))
    : nullptr;
  if (!slider || !state) return;
  lv_obj_add_event_cb(slider, [](lv_event_t *event) {
    delete static_cast<SliderCtx *>(lv_event_get_user_data(event));
  }, LV_EVENT_DELETE, state);
}

inline bool numeric_selectable_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display) {
  using Driver = card_runtime::CardDriverId;
  if (!numeric_selectable_driver_matches(context, config)) return false;
  if (numeric_selectable_driver_option_select(context, config)) {
    setup_option_select_card(
      slot, config, palette.has_sensor_color, palette.sensor_val,
      display_option_select_value_font_or(
        display, slot.text_lbl
          ? lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN)
          : nullptr));
  } else if (context.runtime.driver == Driver::LIGHT_TEMPERATURE) {
    setup_light_temp_visual(
      slot, config,
      palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    numeric_selectable_driver_track_slider_cleanup(slot);
  } else if (numeric_selectable_driver_fan_action(context)) {
    setup_fan_card(slot, config);
  } else {
    setup_slider_visual(
      slot, config,
      palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    numeric_selectable_driver_track_slider_cleanup(slot);
  }
  return true;
}

inline bool numeric_selectable_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &config, const Context &context) {
  return numeric_selectable_driver_matches(context, config);
}

inline bool numeric_selectable_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!numeric_selectable_driver_matches(context, config)) return false;
  if (numeric_selectable_driver_slider(context)) {
    refresh_slider_card_layout(slot);
  }
  return true;
}

inline bool numeric_selectable_driver_cleanup(
    BtnSlot &, const ParsedCfg &config, const Context &context) {
  // Slider state is deleted with its LVGL slider. Main-grid data allocations
  // use the runtime registry; subpage allocations are tied to their owner.
  return numeric_selectable_driver_matches(context, config);
}

template<typename T>
inline T *numeric_selectable_driver_track(
    const Context &context, lv_obj_t *owner, T *ptr) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_with_owner(owner, ptr)
    : grid_track_runtime_allocation(owner, ptr);
}

inline OptionSelectCtx *numeric_selectable_driver_bind_option_select(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display) {
  if (config.entity.empty()) return nullptr;
  OptionSelectCtx *select = numeric_selectable_driver_track(
    context, slot.btn, create_option_select_context(
      slot, config,
      palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR,
      palette.off_val, palette.sensor_val,
      display_main_width_percent(display)));
  subscribe_option_select_state(select);
  subscribe_option_select_friendly_name(select);
  return select;
}

inline FanCardCtx *numeric_selectable_driver_bind_fan_action(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display) {
  if (config.entity.empty()) return nullptr;
  FanCardCtx *fan = create_fan_card_context(
    slot, config,
    palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR,
    palette.off_val, palette.sensor_val,
    lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN),
    display_icon_font(display), display_main_width_percent(display));
  if (context.surface == Surface::SUBPAGE) {
    grid_delete_fan_card_with_owner(slot.btn, fan);
  } else {
    grid_track_fan_card_runtime(slot.btn, fan);
  }
  subscribe_fan_card_state(fan);
  return fan;
}

inline void numeric_selectable_driver_bind_slider(
    BtnSlot &slot, const ParsedCfg &config) {
  if (config.entity.empty()) return;
  lv_obj_t *slider = slot.sensor_container
    ? static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container))
    : nullptr;
  const bool has_icon_on = slider_has_alt_icon(config.type, config.icon_on);
  const char *icon_on = has_icon_on
    ? slider_icon_on(config.type, config.entity, config.icon, config.icon_on)
    : nullptr;
  const char *icon_off = has_icon_on
    ? slider_icon_off(config.type, config.entity, config.icon)
    : nullptr;
  subscribe_slider_state(
    slot.btn, slot.icon_lbl, slider, has_icon_on, icon_off, icon_on,
    config.entity, false);
  if (config.label.empty()) {
    subscribe_friendly_name(slot.text_lbl, config.entity);
  }
}

inline void numeric_selectable_driver_bind_light_temperature(
    BtnSlot &slot, const ParsedCfg &config) {
  if (config.entity.empty()) return;
  lv_obj_t *slider = slot.sensor_container
    ? static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container))
    : nullptr;
  if (slider) {
    int min_kelvin = 2000;
    int max_kelvin = 6500;
    parse_kelvin_range(config.unit, min_kelvin, max_kelvin);
    subscribe_light_temp_state(
      slot.btn, slider, config.entity, min_kelvin, max_kelvin,
      config.precision == "color");
  }
  if (config.label.empty()) {
    SliderCtx *state = slider
      ? static_cast<SliderCtx *>(lv_obj_get_user_data(slider))
      : nullptr;
    subscribe_friendly_name_for_light_temp(
      slot.text_lbl, state, config.entity);
  }
}

struct NumericSelectableBindings {
  OptionSelectCtx *select = nullptr;
  FanCardCtx *fan = nullptr;
};

inline NumericSelectableBindings numeric_selectable_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display) {
  using Driver = card_runtime::CardDriverId;
  NumericSelectableBindings bindings;
  if (numeric_selectable_driver_option_select(context, config)) {
    bindings.select = numeric_selectable_driver_bind_option_select(
      slot, config, context, palette, display);
  } else if (context.runtime.driver == Driver::LIGHT_TEMPERATURE) {
    numeric_selectable_driver_bind_light_temperature(slot, config);
  } else if (numeric_selectable_driver_fan_action(context)) {
    bindings.fan = numeric_selectable_driver_bind_fan_action(
      slot, config, context, palette, display);
  } else {
    numeric_selectable_driver_bind_slider(slot, config);
  }
  return bindings;
}

inline bool numeric_selectable_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display) {
  if (!numeric_selectable_driver_matches(context, config)) return false;
  numeric_selectable_driver_bind_data(
    slot, config, context, palette, display);
  return true;
}

struct NumericSelectableSubpageEnvironment {
  CardPalette palette;
  DisplayProfile display;
  std::function<void(const std::string &)> add_parent_indicator;
};

inline bool numeric_selectable_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const NumericSelectableSubpageEnvironment &environment) {
  using Driver = card_runtime::CardDriverId;
  if (!numeric_selectable_driver_matches(context, config)) return false;
  NumericSelectableBindings bindings = numeric_selectable_driver_bind_data(
    slot, config, context, environment.palette, environment.display);

  if (bindings.select) {
    lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
      OptionSelectCtx *select = static_cast<OptionSelectCtx *>(
        lv_event_get_user_data(event));
      if (select) option_select_open_modal(select);
    }, LV_EVENT_CLICKED, bindings.select);
  } else if (bindings.fan) {
    if (environment.add_parent_indicator) {
      environment.add_parent_indicator(config.entity);
    }
    lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
      FanCardCtx *fan = static_cast<FanCardCtx *>(
        lv_event_get_user_data(event));
      if (fan) fan_card_handle_click(fan);
    }, LV_EVENT_CLICKED, bindings.fan);
  } else if (context.runtime.driver == Driver::LIGHT_TEMPERATURE) {
    if (!config.entity.empty() && environment.add_parent_indicator) {
      environment.add_parent_indicator(config.entity);
    }
  } else if (!config.entity.empty()) {
    if (environment.add_parent_indicator) {
      environment.add_parent_indicator(config.entity);
    }
    std::string *entity = grid_delete_with_owner(
      slot.btn, new std::string(config.entity));
    lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
      std::string *value = static_cast<std::string *>(
        lv_event_get_user_data(event));
      if (value && !value->empty()) send_slider_action(*value, -1);
    }, LV_EVENT_CLICKED, entity);
  }
  return true;
}

inline bool numeric_selectable_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  using Driver = card_runtime::CardDriverId;
  if (!numeric_selectable_driver_matches(context, config)) return false;
  if (numeric_selectable_driver_option_select(context, config)) {
    OptionSelectCtx *select = static_cast<OptionSelectCtx *>(
      lv_obj_get_user_data(button));
    if (select) option_select_open_modal(select);
  } else if (numeric_selectable_driver_fan_action(context)) {
    FanCardCtx *fan = static_cast<FanCardCtx *>(lv_obj_get_user_data(button));
    if (fan) fan_card_handle_click(fan);
  } else if (context.runtime.driver == Driver::NUMERIC ||
             context.runtime.type == card_runtime::CardTypeId::FAN_SPEED) {
    if (!config.entity.empty()) send_slider_action(config.entity, -1);
  }
  // Light Temperature sends only from the slider release callback.
  return true;
}

}  // namespace espcontrol::cards
