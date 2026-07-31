#pragma once

// Shared lifecycle driver for Light Control cards. The specialised modal,
// colour, temperature, brightness, and Home Assistant helpers remain in
// button_grid_sliders.h; this driver owns the grid and subpage boundary.
// Contract coverage marker: "light_control".

namespace espcontrol::cards {

inline bool light_control_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::LIGHT_CONTROL;
}

inline bool light_control_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!light_control_driver_matches(context)) return false;
  setup_light_control_card(slot, config);
  return true;
}

inline bool light_control_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return light_control_driver_matches(context);
}

inline bool light_control_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return light_control_driver_matches(context);
}

inline bool light_control_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!light_control_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

template<typename T>
inline T *light_control_driver_track(
    const Context &context, lv_obj_t *owner, T *ptr) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_with_owner(owner, ptr)
    : grid_track_runtime_allocation(owner, ptr);
}

struct LightControlDriverEnvironment {
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  std::function<void(const std::string &)> add_parent_indicator;
};

inline LightControlDriverEnvironment light_control_driver_environment(
    const CardPalette &palette, const DisplayProfile &display,
    const BtnSlot &slot) {
  LightControlDriverEnvironment environment;
  environment.accent_color = palette.has_on
    ? palette.on_val : DEFAULT_SLIDER_COLOR;
  environment.number_font = display_volume_number_font(display);
  environment.label_font = display_volume_label_font(display)
    ? display_volume_label_font(display)
    : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  environment.icon_font = display_icon_font(display);
  environment.width_compensation_percent =
    display_volume_width_percent(display);
  return environment;
}

inline LightControlCtx *light_control_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const LightControlDriverEnvironment &environment) {
  if (!light_control_driver_matches(context) || config.entity.empty()) {
    return nullptr;
  }
  LightControlCtx *light = light_control_driver_track(
    context, slot.btn,
    create_light_control_context(
      slot, config, environment.accent_color, environment.number_font,
      environment.label_font, environment.icon_font,
      environment.width_compensation_percent));
  subscribe_light_control_state(light);
  return light;
}

inline bool light_control_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const LightControlDriverEnvironment &environment) {
  if (!light_control_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  light_control_driver_bind_data(slot, config, context, environment);
  return true;
}

inline bool light_control_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const LightControlDriverEnvironment &environment) {
  if (!light_control_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  LightControlCtx *light = light_control_driver_bind_data(
    slot, config, context, environment);
  if (!light) return true;
  if (environment.add_parent_indicator) {
    environment.add_parent_indicator(config.entity);
  }
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    LightControlCtx *context = static_cast<LightControlCtx *>(
      lv_event_get_user_data(event));
    if (context) light_control_open_modal(context);
  }, LV_EVENT_CLICKED, light);
  return true;
}

inline bool light_control_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!light_control_driver_matches(context)) return false;
  LightControlCtx *light = button
    ? static_cast<LightControlCtx *>(lv_obj_get_user_data(button)) : nullptr;
  if (light) light_control_open_modal(light);
  return true;
}

}  // namespace espcontrol::cards
