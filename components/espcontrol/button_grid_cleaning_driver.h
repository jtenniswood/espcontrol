#pragma once

// Shared lifecycle driver for Vacuum and Lawn Mower cards. Main-grid and
// subpage cards use the same visual, binding, interaction, layout, cleanup,
// translated-text, and click-dispatch entry points while retaining the
// established service and state helpers.
// Contract coverage markers: "vacuum", "lawn_mower".

namespace espcontrol::cards {

inline bool cleaning_driver_matches(const Context &context) {
  using Driver = card_runtime::CardDriverId;
  if (context.legacy_dispatch) return false;
  return context.runtime.driver == Driver::VACUUM ||
         context.runtime.driver == Driver::LAWN_MOWER;
}

inline bool cleaning_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  using Driver = card_runtime::CardDriverId;
  if (!cleaning_driver_matches(context)) return false;
  if (context.runtime.driver == Driver::VACUUM) {
    setup_vacuum_card(slot, config);
  } else {
    setup_lawn_mower_card(slot, config);
  }
  return true;
}

inline bool cleaning_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return cleaning_driver_matches(context);
}

inline bool cleaning_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return cleaning_driver_matches(context);
}

inline bool cleaning_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!cleaning_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

template<typename T>
inline T *cleaning_driver_track(
    const Context &context, lv_obj_t *owner, T *ptr) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_with_owner(owner, ptr)
    : grid_track_runtime_allocation(owner, ptr);
}

struct CleaningDriverBindings {
  VacuumCardCtx *vacuum = nullptr;
  LawnMowerCardCtx *mower = nullptr;
};

inline CleaningDriverBindings cleaning_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  using Driver = card_runtime::CardDriverId;
  CleaningDriverBindings bindings;
  if (!cleaning_driver_matches(context)) return bindings;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  if (config.entity.empty()) return bindings;

  if (context.runtime.driver == Driver::VACUUM) {
    const bool needs_state = vacuum_card_mode_needs_state(config.sensor);
    if (needs_state || context.surface == Surface::SUBPAGE) {
      bindings.vacuum = cleaning_driver_track(
        context, slot.btn, create_vacuum_card_context(slot, config));
      if (needs_state) subscribe_vacuum_card_state(bindings.vacuum);
      lv_obj_set_user_data(slot.btn, bindings.vacuum);
    }
  } else {
    const bool needs_state = lawn_mower_card_mode_needs_state(config.sensor);
    if (needs_state || context.surface == Surface::SUBPAGE) {
      bindings.mower = cleaning_driver_track(
        context, slot.btn, create_lawn_mower_card_context(slot, config));
      if (needs_state) subscribe_lawn_mower_card_state(bindings.mower);
      lv_obj_set_user_data(slot.btn, bindings.mower);
    }
  }
  return bindings;
}

inline bool cleaning_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!cleaning_driver_matches(context)) return false;
  cleaning_driver_bind_data(slot, config, context);
  return true;
}

struct CleaningDriverSubpageEnvironment {
  std::function<void(const std::string &)> add_parent_indicator;
  std::function<void(const std::string &)> add_mower_parent_indicator;
};

inline bool cleaning_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CleaningDriverSubpageEnvironment &environment) {
  if (!cleaning_driver_matches(context)) return false;
  CleaningDriverBindings bindings = cleaning_driver_bind_data(
    slot, config, context);

  if (bindings.vacuum) {
    register_subpage_vacuum_card_text(slot.text_lbl, bindings.vacuum, config);
    if (environment.add_parent_indicator) {
      environment.add_parent_indicator(config.entity);
    }
    if (!vacuum_card_read_only(config)) {
      lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
        VacuumCardCtx *vacuum = static_cast<VacuumCardCtx *>(
          lv_event_get_user_data(event));
        send_vacuum_card_action(vacuum);
      }, LV_EVENT_CLICKED, bindings.vacuum);
    }
  } else if (bindings.mower) {
    if (environment.add_mower_parent_indicator) {
      environment.add_mower_parent_indicator(config.entity);
    }
    if (!lawn_mower_card_read_only(config)) {
      lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
        LawnMowerCardCtx *mower = static_cast<LawnMowerCardCtx *>(
          lv_event_get_user_data(event));
        send_lawn_mower_card_action(mower);
      }, LV_EVENT_CLICKED, bindings.mower);
    }
  }
  return true;
}

inline bool cleaning_driver_refresh_translated_text(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!cleaning_driver_matches(context)) return false;
  if (context.runtime.driver == card_runtime::CardDriverId::VACUUM) {
    refresh_vacuum_card_translated_text(
      slot.text_lbl,
      static_cast<VacuumCardCtx *>(lv_obj_get_user_data(slot.btn)), config);
  }
  return true;
}

inline void cleaning_driver_refresh_subpage_translated_text() {
  refresh_subpage_vacuum_card_translated_text();
}

inline bool cleaning_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  using Driver = card_runtime::CardDriverId;
  if (!cleaning_driver_matches(context)) return false;
  if (context.runtime.driver == Driver::VACUUM) {
    VacuumCardCtx *vacuum = static_cast<VacuumCardCtx *>(
      lv_obj_get_user_data(button));
    if (vacuum) {
      send_vacuum_card_action(vacuum);
    } else if (!vacuum_card_read_only(config)) {
      VacuumCardCtx fallback;
      fallback.entity_id = config.entity;
      fallback.mode = vacuum_card_mode(config.sensor);
      fallback.area_id = config.unit;
      send_vacuum_card_action(&fallback);
    }
  } else {
    LawnMowerCardCtx *mower = static_cast<LawnMowerCardCtx *>(
      lv_obj_get_user_data(button));
    if (mower) {
      send_lawn_mower_card_action(mower);
    } else if (!lawn_mower_card_read_only(config)) {
      LawnMowerCardCtx fallback;
      fallback.entity_id = config.entity;
      fallback.mode = lawn_mower_card_mode(config.sensor);
      send_lawn_mower_card_action(&fallback);
    }
  }
  return true;
}

}  // namespace espcontrol::cards
