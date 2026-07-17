#pragma once

// Shared lifecycle driver for the Cover All Controls card. The specialised
// position, tilt, presets, supported-feature, Home Assistant, and modal
// helpers remain in button_grid_sliders.h; this driver owns the grid/subpage
// boundary. Contract coverage marker: "cover".

namespace espcontrol::cards {

inline bool cover_modal_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::COVER_MODAL;
}

inline void cover_modal_driver_track_slider_cleanup(BtnSlot &slot) {
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

inline bool cover_modal_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!cover_modal_driver_matches(context)) return false;
  setup_cover_modal_card(slot, config);
  cover_modal_driver_track_slider_cleanup(slot);
  return true;
}

inline bool cover_modal_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return cover_modal_driver_matches(context);
}

inline bool cover_modal_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return cover_modal_driver_matches(context);
}

inline bool cover_modal_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!cover_modal_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

inline CoverControlCtx *cover_modal_driver_track(
    const Context &context, lv_obj_t *owner, CoverControlCtx *cover) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_cover_control_with_owner(owner, cover)
    : grid_track_cover_control_runtime(owner, cover);
}

struct CoverModalDriverEnvironment {
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = SECONDARY_GREY;
  const lv_font_t *option_title_font = nullptr;
  const lv_font_t *option_value_font = nullptr;
  const lv_font_t *option_menu_font = nullptr;
  const lv_font_t *card_icon_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  std::function<void(const std::string &)> add_parent_indicator;
};

inline CoverModalDriverEnvironment cover_modal_driver_environment(
    const CardPalette &palette, const DisplayProfile &display,
    const BtnSlot &slot) {
  CoverModalDriverEnvironment environment;
  environment.accent_color = palette.has_on
    ? palette.on_val : DEFAULT_SLIDER_COLOR;
  environment.secondary_color = palette.off_val;
  environment.option_title_font = display_climate_option_title_font(display)
    ? display_climate_option_title_font(display)
    : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  environment.option_value_font = display_climate_option_value_font(display)
    ? display_climate_option_value_font(display)
    : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  environment.option_menu_font = environment.option_title_font;
  environment.card_icon_font = display_climate_card_icon_font(display);
  environment.icon_font = display_icon_font(display);
  environment.width_compensation_percent =
    display_volume_width_percent(display);
  return environment;
}

inline CoverControlCtx *cover_modal_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CoverModalDriverEnvironment &environment) {
  if (!cover_modal_driver_matches(context) || config.entity.empty()) {
    return nullptr;
  }
  CoverControlCtx *cover = cover_modal_driver_track(
    context, slot.btn,
    create_cover_control_context(
      slot, config, environment.accent_color, environment.secondary_color,
      environment.option_title_font, environment.option_value_font,
      environment.option_menu_font, environment.card_icon_font,
      environment.icon_font, environment.width_compensation_percent));
  subscribe_cover_control_state(cover);
  return cover;
}

inline bool cover_modal_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CoverModalDriverEnvironment &environment) {
  if (!cover_modal_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  cover_modal_driver_bind_data(slot, config, context, environment);
  return true;
}

inline bool cover_modal_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CoverModalDriverEnvironment &environment) {
  if (!cover_modal_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  CoverControlCtx *cover = cover_modal_driver_bind_data(
    slot, config, context, environment);
  if (!cover) return true;
  if (environment.add_parent_indicator) {
    environment.add_parent_indicator(cover->entity_id);
  }
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    CoverControlCtx *context = static_cast<CoverControlCtx *>(
      lv_event_get_user_data(event));
    if (context) cover_control_open_modal(context);
  }, LV_EVENT_CLICKED, cover);
  return true;
}

inline bool cover_modal_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!cover_modal_driver_matches(context)) return false;
  CoverControlCtx *cover = button
    ? static_cast<CoverControlCtx *>(lv_obj_get_user_data(button)) : nullptr;
  if (cover) cover_control_open_modal(cover);
  return true;
}

}  // namespace espcontrol::cards
