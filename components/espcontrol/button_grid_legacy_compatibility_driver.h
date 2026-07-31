#pragma once

namespace espcontrol::cards {

inline bool legacy_compatibility_driver_matches(const Context &context) {
  return context.legacy_dispatch && context.family == Family::TODO;
}

inline bool legacy_compatibility_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display,
    int row_span, int col_span) {
  if (!legacy_compatibility_driver_matches(context)) return false;
  setup_todo_card(slot, config, palette.off_val);
  if (large_number_square_card_layout(row_span, col_span) &&
      card_large_numbers_active_for_layout(config, row_span, col_span) &&
      display_large_sensor_font(display)) {
    apply_large_sensor_number_style(
      slot, display_large_sensor_font(display),
      display_large_sensor_unit_offset_percent(display));
  }
  return true;
}

inline bool legacy_compatibility_driver_bind(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display,
    int row_span, int col_span) {
  if (!legacy_compatibility_driver_matches(context)) return false;
  if (config.entity.empty()) return true;

  TodoCardCtx *todo = create_todo_card_context(
    slot, config, palette.on_val, palette.off_val,
    large_number_square_card_layout(row_span, col_span) &&
        card_large_numbers_active_for_layout(config, row_span, col_span) &&
        display_large_sensor_font(display)
      ? display_large_sensor_font(display) : display_sensor_font(display),
    lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN),
    display_media_title_font_or(
      display, lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN)),
    display_icon_font(display), display_main_width_percent(display),
    card_span_is_single(row_span, col_span));

  if (context.surface == Surface::SUBPAGE) {
    grid_delete_with_owner(slot.btn, todo);
    lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
      TodoCardCtx *ctx = static_cast<TodoCardCtx *>(lv_event_get_user_data(event));
      if (todo_card_context_valid(ctx)) todo_card_open_modal(ctx);
    }, LV_EVENT_CLICKED, todo);
  } else {
    grid_track_runtime_allocation(slot.btn, todo);
  }
  subscribe_todo_state(todo);
  subscribe_todo_friendly_name(todo);
  return true;
}

inline bool legacy_compatibility_driver_handle_main_click(
    const Context &context, lv_obj_t *button) {
  if (!legacy_compatibility_driver_matches(context)) return false;
  TodoCardCtx *todo = static_cast<TodoCardCtx *>(lv_obj_get_user_data(button));
  if (todo_card_context_valid(todo)) todo_card_open_modal(todo);
  return true;
}

}  // namespace espcontrol::cards
