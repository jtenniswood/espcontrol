#pragma once

// One shared lifecycle for main-grid and subpage Power cards. The view borrows
// this context; opening it adds no HA subscriptions or timers.
// Contract coverage marker: "power".
namespace espcontrol::cards {
inline bool power_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::POWER;
}
// Power navigates to a modal. Ordinary Subpage markers remain hidden by the
// upstream helper; reuse the slot's native marker only for this card family.
inline void power_driver_show_chevron(BtnSlot &slot, const GridConfig &grid) {
  if (!slot.subpage_lbl) return;
  lv_obj_clear_flag(slot.subpage_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align(slot.subpage_lbl, LV_ALIGN_BOTTOM_RIGHT,
    grid.subpage_chevron_x, grid.subpage_chevron_y);
  lv_obj_move_foreground(slot.subpage_lbl);
  if (slot.text_lbl) {
    configure_button_label_wrap(slot.text_lbl);
    lv_obj_set_width(slot.text_lbl, lv_pct(grid.subpage_chevron_text_width_percent));
  }
}
inline bool power_driver_setup_visual(BtnSlot &slot, const ParsedCfg &config,
                                       const Context &context, const CardPalette &,
                                       const GridConfig &grid, int col_span) {
  if (!power_driver_matches(context)) return false;
  // Borrow sensor value widgets, not the passive sensor surface/interaction.
  lv_obj_add_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_clear_state(slot.btn, LV_STATE_CHECKED);
  power_driver_show_chevron(slot, grid);
  lv_obj_add_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.secondary_lbl, LV_OBJ_FLAG_HIDDEN);
  const auto label = config.label.empty() || config.label == "Power Dashboard"
    ? std::string(espcontrol_i18n("Power Dashboard")) : config.label;
  power_set_text(slot.text_lbl, label);
  power_set_text(slot.sensor_lbl, "--");
  power_set_text(slot.unit_lbl, "");
  power_set_text(slot.secondary_lbl, espcontrol_i18n("Unavailable"));
  if (col_span < 2) {
    // Legacy narrow cards stay in their cells. Keep safe label-only navigation
    // until resized; never claim an occupied neighbor or reset HA bindings.
    lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(slot.secondary_lbl, LV_OBJ_FLAG_HIDDEN);
    ESP_LOGW("power", "Power needs two columns; resize the label-only navigation card in web settings");
  }
  layout_button_label_with_secondary(slot, col_span >= 2, true);
  apply_push_button_transition(slot.btn);
  return true;
}
inline bool power_driver_refresh_layout(BtnSlot &slot, const Context &context,
                                         const GridConfig &grid, int col_span) {
  if (!power_driver_matches(context)) return false;
  if (col_span < 2) {
    lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(slot.secondary_lbl, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
    const auto *ctx = power_card_for_button(slot.btn);
    if (ctx && ctx->data.model.calculation.secondary_label().empty())
      lv_obj_add_flag(slot.secondary_lbl, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(slot.secondary_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  power_driver_show_chevron(slot, grid);
  layout_button_label_with_secondary(slot, col_span >= 2, true);
  return true;
}
inline void power_driver_event(lv_event_t *event) {
  auto *button = static_cast<lv_obj_t *>(lv_event_get_target(event));
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    auto *ctx = power_card_for_button(button);
    if (ctx && !ctx->slot.config) power_flow_open(ctx);
  }
}
inline bool power_driver_bind_data(BtnSlot &slot, const ParsedCfg &config,
                                    const Context &context, const DisplayProfile &display,
                                    const GridConfig &grid) {
  if (!power_driver_matches(context)) return false;
  auto *ctx = new PowerCardCtx();
  ctx->slot = slot;
  ctx->display = display;
  ctx->grid_cols = std::max(1, grid.cols);
  ctx->grid_rows = (std::max(1, grid.num_slots) + ctx->grid_cols - 1) / ctx->grid_cols;
  ctx->continuous_corners = grid.continuous_card_corners;
  auto *page = lv_obj_get_parent(slot.btn);
  ctx->grid_gap = page ? lv_obj_get_style_pad_column(page, LV_PART_MAIN) : 0;
  ctx->precision = parse_precision(config.precision);
  const auto bindings = espcontrol::power::decode_config_v1(config);
  ctx->has_soc = !bindings.battery_soc_entity.empty();
  ctx->data.model.set_polarity({bindings.invert_grid, bindings.invert_battery});
  ctx->next = power_card_contexts();
  power_card_contexts() = ctx;
  if (context.surface == Surface::SUBPAGE) grid_delete_with_owner(slot.btn, ctx);
  else grid_track_runtime_allocation(slot.btn, ctx);
  ha_retained_state_invalidated_handler() = power_cards_disconnected;
  lv_obj_remove_event_cb(slot.btn, power_driver_event);
  // Shared apply_button_colors owns contrast/pressed events; Power only navigates.
  lv_obj_add_event_cb(slot.btn, power_driver_event, LV_EVENT_CLICKED, nullptr);
  // Model order is Solar, Home, Grid, Battery; SOC is a fifth independent input.
  const std::string entities[5] = {bindings.solar_entity, bindings.home_entity,
    bindings.grid_entity, bindings.battery_entity, bindings.battery_soc_entity};
  uint8_t configured = 0;
  for (size_t i = 0; i < 4; ++i) if (!entities[i].empty()) configured |= 1u << i;
  ctx->data.model.set_capabilities({bindings.installed_components, configured});
  HaCallbackOwnerScope owner(ctx);
  for (size_t i = 0; i < 5; ++i) {
    if (entities[i].empty()) continue;
    if (!ctx->data.model.capabilities.includes(static_cast<espcontrol::power::Node>(i < 4 ? i : 3))) continue;
    const bool state = ha_subscribe_state(entities[i], [ctx, i](esphome::StringRef value) {
      ctx->data.state(i, std::string_view(value.c_str(), value.size()));
      power_card_input_changed(ctx, i);
    }, HA_SUBSCRIPTION_SCOPE_DEFAULT, true);
    const bool unit = ha_subscribe_attribute(entities[i], "unit_of_measurement",
      [ctx, i](esphome::StringRef value) {
        ctx->data.unit(i, std::string_view(value.c_str(), value.size()));
        power_card_input_changed(ctx, i);
      }, HA_SUBSCRIPTION_SCOPE_DEFAULT, true);
    if (!state || !unit) ESP_LOGW("power", "Unable to bind Power input %s", entities[i].c_str());
  }
  power_card_update(ctx);
  return true;
}
inline bool power_driver_handle_main_click(const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!power_driver_matches(context)) return false;
  power_flow_open(power_card_for_button(button));
  return true;
}
} // namespace espcontrol::cards
