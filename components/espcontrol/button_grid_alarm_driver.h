#pragma once

// Shared lifecycle driver for Alarm cards. The specialised PIN entry,
// arming countdown, critical display takeover, Home Assistant, and modal
// helpers remain in button_grid_alarm.h; this driver owns the grid/subpage
// boundary. Contract coverage marker: "alarm".

namespace espcontrol::cards {

inline bool alarm_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::ALARM;
}

inline bool alarm_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!alarm_driver_matches(context)) return false;
  setup_alarm_card(slot, config);
  return true;
}

inline bool alarm_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return alarm_driver_matches(context);
}

inline bool alarm_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return alarm_driver_matches(context);
}

inline bool alarm_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!alarm_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

inline AlarmCardCtx *alarm_driver_track(
    const Context &context, lv_obj_t *owner, AlarmCardCtx *alarm) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_alarm_card_with_owner(owner, alarm)
    : grid_track_alarm_card_runtime(owner, alarm);
}

struct AlarmDriverEnvironment {
  const ParsedCfg *parent_config = nullptr;
  lv_obj_t *grid_page = nullptr;
  int slot_count = 0;
  int grid_cols = 1;
  uint32_t on_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  const lv_font_t *icon_font = nullptr;
  const lv_font_t *arming_title_font = nullptr;
  const lv_font_t *value_font = nullptr;
  const lv_font_t *key_font = nullptr;
  const lv_font_t *label_font = nullptr;
  lv_color_t text_color = lv_color_hex(0xFFFFFF);
  int width_compensation_percent = 100;
  std::function<void(espcontrol::DisplayTakeoverKind)> begin_display_takeover;
  std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;
  AlarmDelayAudioHooks alarm_delay_audio;
  std::function<void(const std::string &)> add_parent_indicator;
};

inline AlarmDriverEnvironment alarm_driver_environment(
    const CardPalette &palette, const DisplayProfile &display,
    const BtnSlot &slot, const GridConfig &grid_config,
    lv_obj_t *grid_page, int slot_count, int grid_cols) {
  AlarmDriverEnvironment environment;
  environment.grid_page = grid_page;
  environment.slot_count = slot_count;
  environment.grid_cols = grid_cols;
  environment.on_color = palette.has_on
    ? palette.on_val : DEFAULT_SLIDER_COLOR;
  environment.off_color = palette.off_val;
  environment.tertiary_color = palette.sensor_val;
  environment.icon_font = display_icon_font(display);
  environment.label_font = slot.text_lbl
    ? lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN) : nullptr;
  environment.arming_title_font = display_media_title_font_or(
    display, environment.label_font);
  environment.value_font = display_sensor_font(display);
  environment.key_font = display_optional_media_title_font(display);
  environment.text_color = slot.text_lbl
    ? lv_obj_get_style_text_color(slot.text_lbl, LV_PART_MAIN)
    : lv_color_hex(0xFFFFFF);
  environment.width_compensation_percent =
    display_main_width_percent(display);
  environment.begin_display_takeover = grid_config.begin_display_takeover;
  environment.end_display_takeover = grid_config.end_display_takeover;
  environment.alarm_delay_audio = grid_config.alarm_delay_audio;
  return environment;
}

inline ParsedCfg alarm_driver_effective_config(
    const ParsedCfg &config, const Context &context,
    const AlarmDriverEnvironment &environment) {
  ParsedCfg effective = config;
  if (context.surface != Surface::SUBPAGE || !environment.parent_config) {
    return effective;
  }
  if (effective.entity.empty()) {
    effective.entity = environment.parent_config->entity;
  }
  if (effective.options.empty()) {
    effective.options = environment.parent_config->options;
  }
  return effective;
}

inline AlarmCardCtx *alarm_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const AlarmDriverEnvironment &environment) {
  if (!alarm_driver_matches(context)) return nullptr;
  ParsedCfg effective = alarm_driver_effective_config(
    config, context, environment);
  if (effective.entity.empty()) return nullptr;

  AlarmCardCtx *alarm = alarm_driver_track(
    context, slot.btn,
    create_alarm_card_context(
      slot, effective, environment.grid_page, environment.slot_count,
      environment.grid_cols, environment.on_color, environment.off_color,
      environment.tertiary_color, environment.icon_font,
      environment.arming_title_font, environment.value_font,
      environment.key_font, environment.label_font,
      environment.text_color, environment.width_compensation_percent,
      false, environment.begin_display_takeover,
      environment.end_display_takeover));
  alarm_delay_audio_register_context(alarm);
  alarm->audio_hooks = environment.alarm_delay_audio;
  if (context.surface == Surface::SUBPAGE) {
    alarm->grid_page = environment.grid_page;
  }
  lv_obj_set_user_data(slot.btn, alarm);
  subscribe_alarm_state(alarm);
  if (effective.label.empty() && !alarm->show_status_label) {
    subscribe_friendly_name(alarm->status_label, effective.entity);
  }
  return alarm;
}

inline bool alarm_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const AlarmDriverEnvironment &environment) {
  if (!alarm_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  alarm_driver_bind_data(slot, config, context, environment);
  return true;
}

inline bool alarm_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const AlarmDriverEnvironment &environment) {
  if (!alarm_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  AlarmCardCtx *alarm = alarm_driver_bind_data(
    slot, config, context, environment);
  if (!alarm) return true;
  if (environment.add_parent_indicator) {
    environment.add_parent_indicator(alarm->entity_id);
  }
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    AlarmCardCtx *context = static_cast<AlarmCardCtx *>(
      lv_event_get_user_data(event));
    if (alarm_card_context_valid(context)) alarm_card_open_page(context);
  }, LV_EVENT_CLICKED, alarm);
  return true;
}

inline bool alarm_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!alarm_driver_matches(context)) return false;
  AlarmCardCtx *alarm = button
    ? static_cast<AlarmCardCtx *>(lv_obj_get_user_data(button)) : nullptr;
  if (alarm_card_context_valid(alarm)) alarm_card_open_page(alarm);
  return true;
}

}  // namespace espcontrol::cards
