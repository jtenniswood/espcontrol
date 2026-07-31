#pragma once

// Shared lifecycle driver for Fan Control cards. The specialised power,
// speed, preset, oscillation, direction, Home Assistant, and modal helpers
// remain in button_grid_fan.h; this driver owns the grid/subpage boundary.
// Contract coverage marker: "fan_control".

namespace espcontrol::cards {

inline bool fan_control_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::FAN_CONTROL;
}

inline bool fan_control_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!fan_control_driver_matches(context)) return false;
  setup_fan_control_card(slot, config);
  return true;
}

inline bool fan_control_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return fan_control_driver_matches(context);
}

inline bool fan_control_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return fan_control_driver_matches(context);
}

inline bool fan_control_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!fan_control_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

inline FanCardCtx *fan_control_driver_track(
    const Context &context, lv_obj_t *owner, FanCardCtx *fan) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_fan_card_with_owner(owner, fan)
    : grid_track_fan_card_runtime(owner, fan);
}

struct FanControlDriverEnvironment {
  uint32_t on_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  std::function<void(const std::string &)> add_parent_indicator;
};

inline FanControlDriverEnvironment fan_control_driver_environment(
    const CardPalette &palette, const DisplayProfile &display,
    const BtnSlot &slot) {
  FanControlDriverEnvironment environment;
  environment.on_color = palette.has_on
    ? palette.on_val : DEFAULT_SLIDER_COLOR;
  environment.off_color = palette.has_off
    ? palette.off_val : SECONDARY_GREY;
  environment.tertiary_color = palette.has_sensor_color
    ? palette.sensor_val : TERTIARY_GREY;
  environment.label_font = slot.text_lbl
    ? lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN) : nullptr;
  environment.icon_font = display_icon_font(display);
  environment.width_compensation_percent =
    display_main_width_percent(display);
  return environment;
}

inline FanCardCtx *fan_control_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const FanControlDriverEnvironment &environment) {
  if (!fan_control_driver_matches(context) || config.entity.empty()) {
    return nullptr;
  }
  FanCardCtx *fan = fan_control_driver_track(
    context, slot.btn,
    create_fan_card_context(
      slot, config, environment.on_color, environment.off_color,
      environment.tertiary_color, environment.label_font,
      environment.icon_font, environment.width_compensation_percent));
  subscribe_fan_card_state(fan);
  return fan;
}

inline bool fan_control_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const FanControlDriverEnvironment &environment) {
  if (!fan_control_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  fan_control_driver_bind_data(slot, config, context, environment);
  return true;
}

inline bool fan_control_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const FanControlDriverEnvironment &environment) {
  if (!fan_control_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  FanCardCtx *fan = fan_control_driver_bind_data(
    slot, config, context, environment);
  if (!fan) return true;
  if (environment.add_parent_indicator) {
    environment.add_parent_indicator(config.entity);
  }
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    FanCardCtx *context = static_cast<FanCardCtx *>(
      lv_event_get_user_data(event));
    if (context) fan_control_open_modal(context);
  }, LV_EVENT_CLICKED, fan);
  return true;
}

inline bool fan_control_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!fan_control_driver_matches(context)) return false;
  FanCardCtx *fan = button
    ? static_cast<FanCardCtx *>(lv_obj_get_user_data(button)) : nullptr;
  if (fan) fan_control_open_modal(fan);
  return true;
}

}  // namespace espcontrol::cards
