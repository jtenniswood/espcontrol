#pragma once

// Shared lifecycle driver for Climate Control cards. The specialised
// temperature, HVAC mode, preset, fan, swing, Home Assistant, and modal
// helpers remain in button_grid_climate.h; this driver owns the grid/subpage
// boundary. Contract coverage marker: "climate_control".

namespace espcontrol::cards {

inline bool climate_control_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::CLIMATE;
}

inline bool climate_control_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const DisplayProfile &display) {
  if (!climate_control_driver_matches(context)) return false;
  setup_climate_control_button(
    slot.btn, slot.icon_lbl, slot.sensor_container, slot.sensor_lbl,
    slot.unit_lbl, slot.text_lbl, config, display_icon_font(display));
  return true;
}

inline bool climate_control_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return climate_control_driver_matches(context);
}

inline bool climate_control_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return climate_control_driver_matches(context);
}

inline bool climate_control_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!climate_control_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

inline ClimateControlCtx *climate_control_driver_track(
    const Context &context, lv_obj_t *owner, ClimateControlCtx *climate) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_climate_control_with_owner(owner, climate)
    : grid_track_climate_control_runtime(owner, climate);
}

struct ClimateControlDriverEnvironment {
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *range_number_font = nullptr;
  const lv_font_t *unit_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *option_title_font = nullptr;
  const lv_font_t *option_value_font = nullptr;
  const lv_font_t *option_menu_font = nullptr;
  const lv_font_t *card_icon_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
};

inline ClimateControlDriverEnvironment climate_control_driver_environment(
    const CardPalette &palette, const DisplayProfile &display,
    const BtnSlot &slot) {
  ClimateControlDriverEnvironment environment;
  environment.accent_color = palette.has_on
    ? palette.on_val : DEFAULT_SLIDER_COLOR;
  environment.secondary_color = palette.has_off
    ? palette.off_val : SECONDARY_GREY;
  environment.tertiary_color = palette.has_sensor_color
    ? palette.sensor_val : TERTIARY_GREY;
  environment.number_font = display_volume_number_font(display);
  environment.range_number_font = display_sensor_font(display);
  environment.unit_font = display_volume_label_font(display)
    ? display_volume_label_font(display)
    : lv_obj_get_style_text_font(slot.unit_lbl, LV_PART_MAIN);
  environment.label_font = display_volume_label_font(display)
    ? display_volume_label_font(display)
    : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  environment.option_title_font = display_climate_option_title_font(display)
    ? display_climate_option_title_font(display)
    : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  environment.option_value_font = display_climate_option_value_font(display)
    ? display_climate_option_value_font(display)
    : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  environment.option_menu_font = display_climate_option_title_font(display)
    ? display_climate_option_title_font(display)
    : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  environment.card_icon_font = display_climate_card_icon_font(display);
  environment.icon_font = display_icon_font(display);
  environment.width_compensation_percent =
    display_volume_width_percent(display);
  return environment;
}

inline ClimateControlCtx *climate_control_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const ClimateControlDriverEnvironment &environment) {
  if (!climate_control_driver_matches(context) || config.entity.empty()) {
    return nullptr;
  }
  ClimateControlCtx *climate = climate_control_driver_track(
    context, slot.btn,
    create_climate_control_context(
      slot.btn, slot.icon_lbl, slot.text_lbl, config,
      environment.accent_color, environment.secondary_color,
      environment.tertiary_color, environment.number_font,
      environment.range_number_font,
      environment.unit_font, environment.label_font,
      environment.option_title_font, environment.option_value_font,
      environment.option_menu_font, environment.card_icon_font,
      environment.icon_font, environment.width_compensation_percent,
      slot.sensor_container, slot.sensor_lbl, slot.unit_lbl));
  subscribe_climate_control_state(climate);
  return climate;
}

inline bool climate_control_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const ClimateControlDriverEnvironment &environment) {
  if (!climate_control_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  climate_control_driver_bind_data(slot, config, context, environment);
  return true;
}

inline bool climate_control_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const ClimateControlDriverEnvironment &environment) {
  if (!climate_control_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  ClimateControlCtx *climate = climate_control_driver_bind_data(
    slot, config, context, environment);
  if (!climate) return true;
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    ClimateControlCtx *context = static_cast<ClimateControlCtx *>(
      lv_event_get_user_data(event));
    if (context) climate_control_open_modal(context);
  }, LV_EVENT_CLICKED, climate);
  return true;
}

inline bool climate_control_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!climate_control_driver_matches(context)) return false;
  ClimateControlCtx *climate = button
    ? static_cast<ClimateControlCtx *>(lv_obj_get_user_data(button)) : nullptr;
  if (climate) climate_control_open_modal(climate);
  return true;
}

}  // namespace espcontrol::cards
