#pragma once

// Shared lifecycle driver for the basic toggle and action-bearing card family.
// The same visual, binding, interaction, layout, and cleanup entry points serve
// main-grid and subpage cards while preserving their established ownership.
// Contract coverage markers: type == "", "action", "alarm_action",
// "fan_switch", "internal", "light_switch", "push", "screen_lock", "webhook".

namespace espcontrol::cards {

inline bool basic_action_driver_matches(const Context &context,
                                        const ParsedCfg &config) {
  using Driver = card_runtime::CardDriverId;
  using Type = card_runtime::CardTypeId;
  if (context.legacy_dispatch) return false;
  switch (context.runtime.driver) {
    case Driver::TOGGLE:
    case Driver::ACTION:
    case Driver::ALARM_ACTION:
    case Driver::INTERNAL:
    case Driver::PUSH:
    case Driver::SCREEN_LOCK:
    case Driver::WEBHOOK:
      return !action_card_option_select(config);
    case Driver::FAN:
      return context.runtime.type == Type::FAN_SWITCH;
    default:
      return false;
  }
}

inline bool basic_action_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  using Driver = card_runtime::CardDriverId;
  if (!basic_action_driver_matches(context, config)) return false;
  switch (context.runtime.driver) {
    case Driver::SCREEN_LOCK:
      setup_screen_lock_card(slot, config);
      break;
    case Driver::ALARM_ACTION:
      setup_alarm_action_card(slot, config);
      break;
    case Driver::FAN:
      setup_fan_card(slot, config);
      break;
    case Driver::INTERNAL:
      setup_internal_relay_card(slot, config);
      break;
    case Driver::ACTION:
      if (config.type == "local" || action_card_local_action(config)) {
        setup_local_action_card(slot, config);
      } else {
        setup_action_card(slot, config);
      }
      break;
    default:
      setup_toggle_visual(slot, config);
      break;
  }
  return true;
}

inline bool basic_action_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &config, const Context &context) {
  return basic_action_driver_matches(context, config);
}

inline bool basic_action_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &config, const Context &context,
    const DisplayProfile &, int, int) {
  return basic_action_driver_matches(context, config);
}

inline bool basic_action_driver_cleanup(
    BtnSlot &, const ParsedCfg &config, const Context &context) {
  // Main-grid allocations are released by grid_phase2; subpage allocations
  // are tied to their LVGL owner. Local registries reset before each rebuild.
  return basic_action_driver_matches(context, config);
}

template<typename T>
inline T *basic_action_driver_track(const Context &context,
                                    lv_obj_t *owner, T *ptr) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_with_owner(owner, ptr)
    : grid_track_runtime_allocation(owner, ptr);
}

struct ToggleDriverState {
  bool *has_sensor = nullptr;
  bool *sensor_text_mode = nullptr;
  bool *has_icon_on = nullptr;
  const char **icon_off = nullptr;
  const char **icon_on = nullptr;
};

inline void basic_action_driver_bind_toggle(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    ToggleDriverState state) {
  if (!state.has_sensor || !state.sensor_text_mode || !state.has_icon_on ||
      !state.icon_off || !state.icon_on || config.entity.empty()) return;

  *state.has_sensor = !config.sensor.empty();
  *state.sensor_text_mode = *state.has_sensor && config.precision == "text";
  *state.has_icon_on = !config.icon_on.empty() && config.icon_on != "Auto";
  *state.icon_on = *state.has_icon_on ? find_icon(config.icon_on.c_str()) : nullptr;
  *state.icon_off = (config.icon.empty() || config.icon == "Auto")
    ? domain_default_icon(config.entity.substr(0, config.entity.find('.')))
    : find_icon(config.icon.c_str());

  ToggleTextSensorCtx *text_context = nullptr;
  if (*state.sensor_text_mode) {
    text_context = basic_action_driver_track(
      context, slot.btn, new ToggleTextSensorCtx());
    text_context->text_lbl = slot.text_lbl;
    text_context->steady_text = label_text_or_empty(slot.text_lbl);
  }

  if (config.label.empty()) {
    if (text_context) subscribe_friendly_name(text_context, config.entity);
    else subscribe_friendly_name(slot.text_lbl, config.entity);
  }

  subscribe_toggle_state(
    slot.btn, slot.icon_lbl, slot.sensor_container,
    state.has_sensor, state.sensor_text_mode, state.has_icon_on,
    state.icon_off, state.icon_on, text_context, config.entity);
  if (*state.has_sensor) {
    if (*state.sensor_text_mode) {
      subscribe_toggle_text_sensor_value(text_context, config.sensor);
    } else {
      subscribe_sensor_value(
        slot.sensor_lbl, config.sensor, parse_precision(config.precision),
        slot.unit_lbl, config.unit);
    }
  }
}

inline AlarmActionCtx *basic_action_driver_bind_alarm_action(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    lv_obj_t *grid_page, const GridConfig &grid_config,
    const CardPalette &palette, const DisplayProfile &display, int grid_cols) {
  if (config.entity.empty()) return nullptr;
  AlarmCardCtx *card = new AlarmCardCtx();
  card->entity_id = config.entity;
  card->label = config.label.empty()
    ? alarm_action_label(config.sensor) : config.label;
  card->options = config.options;
  card->btn = slot.btn;
  card->icon_lbl = slot.icon_lbl;
  card->grid_page = grid_page;
  card->label_font = lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  card->key_label_font = display_media_title_font_or(display, card->label_font);
  card->pin_label_font = card->key_label_font;
  card->icon_font = display_icon_font(display);
  card->arming_title_font = card->key_label_font;
  card->on_color = palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR;
  card->off_color = palette.off_val;
  card->tertiary_color = palette.sensor_val;
  card->width_compensation_percent = display_main_width_percent(display);
  card->grid_cols = grid_cols;
  card->begin_display_takeover = grid_config.begin_display_takeover;
  card->end_display_takeover = grid_config.end_display_takeover;
  card->audio_hooks = grid_config.alarm_delay_audio;
  alarm_set_card_state_colors(card, card->on_color);

  AlarmActionCtx *action = context.surface == Surface::SUBPAGE
    ? grid_delete_alarm_action_with_owner(slot.btn, new AlarmActionCtx())
    : grid_track_alarm_action_runtime(slot.btn, new AlarmActionCtx());
  action->card = card;
  action->mode = alarm_action_valid(config.sensor) ? config.sensor : "away";
  action->requires_pin = alarm_action_requires_pin(card->options, action->mode);
  subscribe_alarm_action_state(card, action->mode);
  lv_obj_set_user_data(slot.btn, action);
  return action;
}

inline FanCardCtx *basic_action_driver_bind_fan_switch(
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

inline void basic_action_driver_bind_action_state(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  const std::string state_entity = action_card_state_entity(config);
  if (state_entity.empty()) return;
  ActionCardStateCtx *state = basic_action_driver_track(
    context, slot.btn, create_action_card_state_context(slot, config));
  subscribe_action_card_display_state(state, state_entity);
}

inline bool basic_action_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const GridConfig &grid_config, const CardPalette &palette,
    const DisplayProfile &display, lv_obj_t *grid_page, int grid_cols,
    ToggleDriverState toggle_state) {
  using Driver = card_runtime::CardDriverId;
  if (!basic_action_driver_matches(context, config)) return false;
  switch (context.runtime.driver) {
    case Driver::TOGGLE:
      basic_action_driver_bind_toggle(slot, config, context, toggle_state);
      break;
    case Driver::ACTION:
      basic_action_driver_bind_action_state(slot, config, context);
      break;
    case Driver::ALARM_ACTION:
      basic_action_driver_bind_alarm_action(
        slot, config, context, grid_page, grid_config, palette, display, grid_cols);
      break;
    case Driver::FAN:
      basic_action_driver_bind_fan_switch(slot, config, context, palette, display);
      break;
    case Driver::INTERNAL:
      if (!config.entity.empty() && !internal_relay_push_mode(config)) {
        const bool has_icon_on =
          !config.icon_on.empty() && config.icon_on != "Auto";
        const char *icon_on = has_icon_on
          ? find_icon(config.icon_on.c_str()) : nullptr;
        watch_internal_relay_state(
          config.entity, slot.btn, slot.icon_lbl, has_icon_on,
          internal_relay_icon(config, false), icon_on);
      }
      break;
    default:
      break;
  }
  return true;
}

struct BasicActionSubpageEnvironment {
  const GridConfig *grid_config = nullptr;
  const ParsedCfg *parent_config = nullptr;
  CardPalette palette;
  DisplayProfile display;
  lv_obj_t *grid_page = nullptr;
  int grid_cols = 1;
  std::function<void(const std::string &)> add_parent_indicator;
  bool parent_indicator_enabled = false;
  int *child_allocation_index = nullptr;
  int child_capacity = 0;
  bool *child_was_on = nullptr;
  lv_obj_t *parent_btn = nullptr;
  lv_obj_t *parent_icon = nullptr;
  int parent_index = 0;
  bool parent_has_icon_on = false;
  const char *parent_icon_off = nullptr;
  const char *parent_icon_on = nullptr;
  int *parent_on_count = nullptr;
};

inline void basic_action_driver_attach_subpage_toggle(
    BtnSlot &slot, const ParsedCfg &config) {
  ParsedCfg *click = grid_delete_with_owner(slot.btn, new ParsedCfg(config));
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    ParsedCfg *card = static_cast<ParsedCfg *>(lv_event_get_user_data(event));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(event));
    if (!card || card->entity.empty()) return;
    const bool currently_on = target && lv_obj_has_state(target, LV_STATE_CHECKED);
    if (switch_confirmation_required(*card, currently_on) && target &&
        !is_button_entity(card->entity)) {
      switch_confirmation_open_modal(*card, target, !currently_on);
    } else {
      send_toggle_action(card->entity);
    }
  }, LV_EVENT_CLICKED, click);
}

inline bool basic_action_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const BasicActionSubpageEnvironment &environment) {
  using Driver = card_runtime::CardDriverId;
  if (!basic_action_driver_matches(context, config)) return false;
  switch (context.runtime.driver) {
    case Driver::SCREEN_LOCK:
      lv_obj_add_event_cb(slot.btn, [](lv_event_t *) {
        screen_lock_toggle();
      }, LV_EVENT_CLICKED, nullptr);
      break;
    case Driver::PUSH: {
      std::string label = config.label.empty()
        ? espcontrol_i18n(std::string("Push")) : config.label;
      std::string *stored_label = grid_delete_with_owner(
        slot.btn, new std::string(label));
      lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
        std::string *value = static_cast<std::string *>(
          lv_event_get_user_data(event));
        esphome::api::HomeassistantActionRequest request;
        if (!value || !ha_action_begin(
              request, "esphome.push_button_pressed", true, 1)) return;
        ha_action_add_data(request, "label", value->c_str());
        ha_action_send(request);
      }, LV_EVENT_CLICKED, stored_label);
      break;
    }
    case Driver::WEBHOOK: {
      ParsedCfg *click = grid_delete_with_owner(slot.btn, new ParsedCfg(config));
      lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
        ParsedCfg *card = static_cast<ParsedCfg *>(lv_event_get_user_data(event));
        if (card) send_webhook_action(*card);
      }, LV_EVENT_CLICKED, click);
      break;
    }
    case Driver::ACTION: {
      basic_action_driver_bind_action_state(slot, config, context);
      if (!config.entity.empty() &&
          (config.type == "local" || !config.sensor.empty())) {
        ParsedCfg *click = grid_delete_with_owner(slot.btn, new ParsedCfg(config));
        lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
          ParsedCfg *card = static_cast<ParsedCfg *>(lv_event_get_user_data(event));
          lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(event));
          if (!card) return;
          if (card->type == "local") {
            send_local_action(card->entity);
          } else if (action_script_confirmation_enabled(*card) && target) {
            switch_confirmation_open_modal(*card, target, false);
          } else {
            send_action_card_action(*card);
          }
        }, LV_EVENT_CLICKED, click);
      }
      break;
    }
    case Driver::ALARM_ACTION: {
      if (environment.grid_config) {
        ParsedCfg effective_config = config;
        if (environment.parent_config) {
          if (effective_config.entity.empty()) {
            effective_config.entity = environment.parent_config->entity;
          }
          if (effective_config.options.empty()) {
            effective_config.options = environment.parent_config->options;
          }
        }
        AlarmActionCtx *action = basic_action_driver_bind_alarm_action(
          slot, effective_config, context, environment.grid_page,
          *environment.grid_config, environment.palette,
          environment.display, environment.grid_cols);
        if (action) {
          lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
            AlarmActionCtx *value = static_cast<AlarmActionCtx *>(
              lv_event_get_user_data(event));
            alarm_action_activate(value);
          }, LV_EVENT_CLICKED, action);
        }
      }
      break;
    }
    case Driver::FAN: {
      FanCardCtx *fan = basic_action_driver_bind_fan_switch(
        slot, config, context, environment.palette, environment.display);
      if (fan) {
        if (environment.add_parent_indicator) {
          environment.add_parent_indicator(config.entity);
        }
        lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
          FanCardCtx *value = static_cast<FanCardCtx *>(
            lv_event_get_user_data(event));
          if (value) fan_card_handle_click(value);
        }, LV_EVENT_CLICKED, fan);
      }
      break;
    }
    case Driver::INTERNAL: {
      const bool push_mode = internal_relay_push_mode(config);
      if (!push_mode && !config.entity.empty()) {
        const bool has_icon_on =
          !config.icon_on.empty() && config.icon_on != "Auto";
        const char *icon_on = has_icon_on
          ? find_icon(config.icon_on.c_str()) : nullptr;
        bool *child_state = nullptr;
        lv_obj_t *parent_btn = nullptr;
        lv_obj_t *parent_icon = nullptr;
        if (environment.parent_indicator_enabled &&
            environment.child_allocation_index && environment.child_was_on) {
          const int child_index = (*environment.child_allocation_index)++;
          if (child_index >= environment.child_capacity) {
            ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s",
              config.entity.c_str());
          } else {
            environment.child_was_on[child_index] = false;
            child_state = &environment.child_was_on[child_index];
            parent_btn = environment.parent_btn;
            parent_icon = environment.parent_icon;
          }
        }
        watch_internal_relay_state(
          config.entity, slot.btn, slot.icon_lbl, has_icon_on,
          internal_relay_icon(config, push_mode), icon_on,
          child_state, parent_btn, parent_icon, environment.parent_index,
          environment.parent_has_icon_on, environment.parent_icon_off,
          environment.parent_icon_on, environment.parent_on_count);
      }
      if (!config.entity.empty()) {
        InternalRelayClickCtx *click = grid_delete_with_owner(
          slot.btn, new InternalRelayClickCtx());
        click->key = config.entity;
        click->push_mode = push_mode;
        lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
          InternalRelayClickCtx *value = static_cast<InternalRelayClickCtx *>(
            lv_event_get_user_data(event));
          if (value && !value->key.empty()) {
            send_internal_relay_action(value->key, value->push_mode);
          }
        }, LV_EVENT_CLICKED, click);
      }
      break;
    }
    case Driver::TOGGLE: {
      ToggleDriverState state;
      state.has_sensor = grid_delete_with_owner(slot.btn, new bool(false));
      state.sensor_text_mode = grid_delete_with_owner(slot.btn, new bool(false));
      state.has_icon_on = grid_delete_with_owner(slot.btn, new bool(false));
      state.icon_off = grid_delete_with_owner(slot.btn, new const char *(nullptr));
      state.icon_on = grid_delete_with_owner(slot.btn, new const char *(nullptr));
      basic_action_driver_bind_toggle(slot, config, context, state);
      if (!config.entity.empty()) {
        if (environment.add_parent_indicator) {
          environment.add_parent_indicator(config.entity);
        }
        basic_action_driver_attach_subpage_toggle(slot, config);
      }
      break;
    }
    default:
      break;
  }
  return true;
}

inline bool basic_action_driver_handle_main_click(
    const Context &context, const ParsedCfg &config,
    int slot_number, lv_obj_t *button) {
  using Driver = card_runtime::CardDriverId;
  if (!basic_action_driver_matches(context, config)) return false;
  switch (context.runtime.driver) {
    case Driver::SCREEN_LOCK:
      screen_lock_toggle();
      break;
    case Driver::PUSH: {
      std::string label = config.label;
      if (label.empty()) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "Push %d", slot_number);
        label = buffer;
      }
      esphome::api::HomeassistantActionRequest request;
      if (!ha_action_begin(
            request, "esphome.push_button_pressed", true, 2)) break;
      ha_action_add_data(request, "label", label.c_str());
      char slot_buffer[8];
      snprintf(slot_buffer, sizeof(slot_buffer), "%d", slot_number);
      ha_action_add_data(request, "slot", slot_buffer);
      ha_action_send(request);
      break;
    }
    case Driver::ALARM_ACTION: {
      AlarmActionCtx *action = static_cast<AlarmActionCtx *>(
        lv_obj_get_user_data(button));
      if (alarm_action_context_valid(action)) alarm_action_activate(action);
      break;
    }
    case Driver::FAN: {
      FanCardCtx *fan = static_cast<FanCardCtx *>(lv_obj_get_user_data(button));
      if (fan) fan_card_handle_click(fan);
      break;
    }
    case Driver::INTERNAL:
      if (!config.entity.empty()) send_internal_relay_action(config);
      break;
    case Driver::ACTION:
      if (config.type == "local") {
        if (!config.entity.empty()) send_local_action(config.entity);
      } else if (action_card_local_action(config)) {
        if (!config.entity.empty()) send_local_action(config.entity);
      } else if (action_script_confirmation_enabled(config) && button) {
        switch_confirmation_open_modal(config, button, false);
      } else {
        send_action_card_action(config);
      }
      break;
    case Driver::WEBHOOK:
      send_webhook_action(config);
      break;
    case Driver::TOGGLE:
      if (!config.entity.empty()) {
        const bool currently_on =
          button && lv_obj_has_state(button, LV_STATE_CHECKED);
        if (switch_confirmation_required(config, currently_on) && button &&
            !is_button_entity(config.entity)) {
          switch_confirmation_open_modal(
            config, button, !currently_on);
        } else {
          send_toggle_action(config.entity);
        }
      }
      break;
    default:
      break;
  }
  return true;
}

}  // namespace espcontrol::cards
