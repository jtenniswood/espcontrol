#pragma once

// Shared lifecycle driver for Garage, Gate, Lock, and non-modal Cover cards.
// Cover modal behaviour deliberately remains with the later rich-card stage.
// Contract coverage markers: "garage", "gate", "lock", "cover".

namespace espcontrol::cards {

inline bool access_cover_driver_access(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::ACCESS;
}

inline bool access_cover_driver_cover(const Context &context) {
  using Driver = card_runtime::CardDriverId;
  return context.runtime.driver == Driver::COVER_COMMAND ||
         context.runtime.driver == Driver::COVER_TOGGLE ||
         context.runtime.driver == Driver::COVER_POSITION ||
         context.runtime.driver == Driver::COVER_TILT;
}

inline bool access_cover_driver_slider(const Context &context) {
  using Driver = card_runtime::CardDriverId;
  return context.runtime.driver == Driver::COVER_POSITION ||
         context.runtime.driver == Driver::COVER_TILT;
}

inline bool access_cover_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         (access_cover_driver_access(context) ||
          access_cover_driver_cover(context));
}

inline void access_cover_driver_track_slider_cleanup(BtnSlot &slot) {
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

inline bool access_cover_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette) {
  using Driver = card_runtime::CardDriverId;
  using Type = card_runtime::CardTypeId;
  if (!access_cover_driver_matches(context)) return false;

  if (access_cover_driver_access(context)) {
    switch (context.runtime.type) {
      case Type::GARAGE: setup_garage_card(slot, config); break;
      case Type::GATE: setup_gate_card(slot, config); break;
      case Type::LOCK: setup_lock_card(slot, config); break;
      default: return false;
    }
  } else if (context.runtime.driver == Driver::COVER_COMMAND) {
    setup_cover_command_card(slot, config);
  } else if (context.runtime.driver == Driver::COVER_TOGGLE) {
    setup_cover_toggle_card(slot, config);
  } else {
    setup_slider_visual(
      slot, config,
      palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    access_cover_driver_track_slider_cleanup(slot);
  }
  return true;
}

inline bool access_cover_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return access_cover_driver_matches(context);
}

inline bool access_cover_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!access_cover_driver_matches(context)) return false;
  if (access_cover_driver_slider(context)) refresh_slider_card_layout(slot);
  return true;
}

inline bool access_cover_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!access_cover_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

template<typename T>
inline T *access_cover_driver_track(
    const Context &context, lv_obj_t *owner, T *ptr) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_with_owner(owner, ptr)
    : grid_track_runtime_allocation(owner, ptr);
}

inline TransientStatusLabel *access_cover_driver_track_status(
    const Context &context, lv_obj_t *owner, TransientStatusLabel *status) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_transient_status_label_with_owner(owner, status)
    : grid_track_transient_status_label_runtime(owner, status);
}

struct AccessCoverDriverBindings {
  CoverCommandCtx *cover_command = nullptr;
  LockCardCtx *lock = nullptr;
  TransientStatusLabel *status = nullptr;
  bool state_bound = false;
};

inline void access_cover_driver_bind_slider(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
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
    config.entity,
    context.runtime.driver == card_runtime::CardDriverId::COVER_TILT);
  if (config.label.empty()) {
    subscribe_friendly_name(slot.text_lbl, config.entity);
  }
}

inline AccessCoverDriverBindings access_cover_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  using Driver = card_runtime::CardDriverId;
  using Type = card_runtime::CardTypeId;
  AccessCoverDriverBindings bindings;
  if (!access_cover_driver_matches(context)) return bindings;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  if (config.entity.empty()) return bindings;

  if (access_cover_driver_access(context)) {
    if (context.runtime.type == Type::GARAGE) {
      const bool command = garage_command_mode(config.sensor);
      if (!command || garage_card_show_status(config)) {
        bindings.state_bound = bind_garage_status_card(
          slot, config, &bindings.status);
      }
    } else if (context.runtime.type == Type::GATE) {
      const bool command = gate_command_mode(config.sensor);
      if (command) {
        subscribe_control_availability(slot.btn, slot.btn, config.entity);
      }
      if (!command || gate_card_show_status(config)) {
        bindings.state_bound = bind_gate_status_card(
          slot, config, &bindings.status);
      }
    } else if (context.runtime.type == Type::LOCK &&
               !lock_command_mode(config.sensor)) {
      bindings.lock = access_cover_driver_track(
        context, slot.btn,
        bind_lock_status_card(slot, config, &bindings.status));
      bindings.state_bound = bindings.lock != nullptr;
    }
    if (bindings.status) {
      access_cover_driver_track_status(context, slot.btn, bindings.status);
    }
  } else if (context.runtime.driver == Driver::COVER_COMMAND) {
    if (config.label.empty()) {
      subscribe_friendly_name(slot.text_lbl, config.entity);
    }
    bindings.cover_command = access_cover_driver_track(
      context, slot.btn, create_cover_command_context(config));
    lv_obj_set_user_data(slot.btn, bindings.cover_command);
    subscribe_cover_command_features(bindings.cover_command);
  } else if (context.runtime.driver == Driver::COVER_TOGGLE) {
    bindings.status = create_transient_status_label(
      slot.text_lbl,
      config.label.empty() ? espcontrol_i18n(std::string("Cover")) : config.label);
    access_cover_driver_track_status(context, slot.btn, bindings.status);
    subscribe_cover_toggle_state(
      slot.btn, slot.icon_lbl, bindings.status,
      slider_icon_off(config.type, config.entity, config.icon),
      slider_icon_on(config.type, config.entity, config.icon, config.icon_on),
      config.entity);
    if (config.label.empty()) {
      subscribe_friendly_name(bindings.status, config.entity);
    }
    bindings.state_bound = true;
  } else {
    access_cover_driver_bind_slider(slot, config, context);
    bindings.state_bound = true;
  }
  return bindings;
}

inline bool access_cover_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!access_cover_driver_matches(context)) return false;
  access_cover_driver_bind_data(slot, config, context);
  return true;
}

struct AccessCoverDriverSubpageEnvironment {
  std::function<void(const std::string &)> add_parent_indicator;
  std::function<void(lv_obj_t *, const std::string &, bool)> add_toggle_click;
};

struct AccessCoverDriverSliderClick {
  std::string entity;
  bool tilt = false;
};

inline void access_cover_driver_add_config_click(
    lv_obj_t *button, const ParsedCfg &config, bool lock_action) {
  ParsedCfg *saved = grid_delete_with_owner(button, new ParsedCfg(config));
  if (lock_action) {
    lv_obj_add_event_cb(button, [](lv_event_t *event) {
      ParsedCfg *value = static_cast<ParsedCfg *>(
        lv_event_get_user_data(event));
      if (value) send_lock_command_action(*value);
    }, LV_EVENT_CLICKED, saved);
  } else {
    lv_obj_add_event_cb(button, [](lv_event_t *event) {
      ParsedCfg *value = static_cast<ParsedCfg *>(
        lv_event_get_user_data(event));
      if (value) send_cover_command_action(*value);
    }, LV_EVENT_CLICKED, saved);
  }
}

inline bool access_cover_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const AccessCoverDriverSubpageEnvironment &environment) {
  using Driver = card_runtime::CardDriverId;
  using Type = card_runtime::CardTypeId;
  if (!access_cover_driver_matches(context)) return false;
  AccessCoverDriverBindings bindings =
    access_cover_driver_bind_data(slot, config, context);
  if (config.entity.empty()) return true;

  if (access_cover_driver_access(context)) {
    if (context.runtime.type == Type::GARAGE ||
        context.runtime.type == Type::GATE) {
      const bool command = context.runtime.type == Type::GARAGE
        ? garage_command_mode(config.sensor)
        : gate_command_mode(config.sensor);
      if (command) {
        if (bindings.state_bound && environment.add_parent_indicator) {
          environment.add_parent_indicator(config.entity);
        }
        access_cover_driver_add_config_click(slot.btn, config, false);
      } else if (bindings.state_bound) {
        if (environment.add_parent_indicator) {
          environment.add_parent_indicator(config.entity);
        }
        if (environment.add_toggle_click) {
          environment.add_toggle_click(slot.btn, config.entity, true);
        }
      }
    } else if (context.runtime.type == Type::LOCK) {
      if (lock_command_mode(config.sensor)) {
        access_cover_driver_add_config_click(slot.btn, config, true);
      } else if (bindings.lock) {
        if (environment.add_parent_indicator) {
          environment.add_parent_indicator(config.entity);
        }
        lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
          LockCardCtx *lock = static_cast<LockCardCtx *>(
            lv_event_get_user_data(event));
          if (lock) send_lock_action(lock);
        }, LV_EVENT_CLICKED, bindings.lock);
      }
    }
  } else if (context.runtime.driver == Driver::COVER_COMMAND) {
    if (bindings.cover_command) {
      lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
        CoverCommandCtx *command = static_cast<CoverCommandCtx *>(
          lv_event_get_user_data(event));
        if (command) send_cover_command_action(*command);
      }, LV_EVENT_CLICKED, bindings.cover_command);
    }
  } else if (context.runtime.driver == Driver::COVER_TOGGLE) {
    if (environment.add_parent_indicator) {
      environment.add_parent_indicator(config.entity);
    }
    if (environment.add_toggle_click) {
      environment.add_toggle_click(slot.btn, config.entity, true);
    }
  } else {
    if (environment.add_parent_indicator) {
      environment.add_parent_indicator(config.entity);
    }
    AccessCoverDriverSliderClick *payload = grid_delete_with_owner(
      slot.btn, new AccessCoverDriverSliderClick{
        config.entity,
        context.runtime.driver == Driver::COVER_TILT,
      });
    lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
      auto *payload = static_cast<AccessCoverDriverSliderClick *>(
        lv_event_get_user_data(event));
      if (payload && !payload->entity.empty()) {
        send_slider_action(payload->entity, -1, payload->tilt);
      }
    }, LV_EVENT_CLICKED, payload);
  }
  return true;
}

inline bool access_cover_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  using Driver = card_runtime::CardDriverId;
  using Type = card_runtime::CardTypeId;
  if (!access_cover_driver_matches(context)) return false;

  if (access_cover_driver_access(context)) {
    if (context.runtime.type == Type::GARAGE ||
        context.runtime.type == Type::GATE) {
      const bool command = context.runtime.type == Type::GARAGE
        ? garage_command_mode(config.sensor)
        : gate_command_mode(config.sensor);
      if (command) {
        send_cover_command_action(config);
      } else if (!config.entity.empty()) {
        set_card_checked_state(button, true);
        send_toggle_action(config.entity);
      }
    } else if (context.runtime.type == Type::LOCK) {
      if (lock_command_mode(config.sensor)) {
        send_lock_command_action(config);
      } else {
        LockCardCtx *lock = static_cast<LockCardCtx *>(
          lv_obj_get_user_data(button));
        if (lock) send_lock_action(lock);
        else send_lock_action(config.entity, "");
      }
    }
  } else if (context.runtime.driver == Driver::COVER_COMMAND) {
    CoverCommandCtx *command = static_cast<CoverCommandCtx *>(
      lv_obj_get_user_data(button));
    if (command) send_cover_command_action(*command);
    else send_cover_command_action(config);
  } else if (context.runtime.driver == Driver::COVER_TOGGLE) {
    if (!config.entity.empty()) {
      set_card_checked_state(button, true);
      send_toggle_action(config.entity);
    }
  } else if (!config.entity.empty()) {
    send_slider_action(
      config.entity, -1,
      context.runtime.driver == Driver::COVER_TILT);
  }
  return true;
}

}  // namespace espcontrol::cards
