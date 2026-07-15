#pragma once

// Shared lifecycle driver for the local "clock" and "timezone" runtime types.
// Calendar intentionally remains on the legacy DATE_TIME path until the
// informational-entity migration, even though it shares the generated driver
// identifier and some formatting helpers.

namespace espcontrol::cards {

inline bool date_time_driver_matches(const Context &context) {
  using Type = card_runtime::CardTypeId;
  if (context.runtime.driver != card_runtime::CardDriverId::DATE_TIME) return false;
  return context.runtime.type == Type::CLOCK ||
         context.runtime.type == Type::TIMEZONE;
}

inline bool date_time_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette) {
  if (!date_time_driver_matches(context)) return false;

  if (palette.has_sensor_color) {
    lv_obj_set_style_bg_color(
      slot.btn, lv_color_hex(palette.sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
        static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_add_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(slot.sensor_lbl, "--:--");
  lv_label_set_text(slot.unit_lbl, "");

  if (context.runtime.type == card_runtime::CardTypeId::TIMEZONE) {
    const std::string label = config.label.empty()
      ? timezone_city_label(config.entity)
      : config.label;
    lv_label_set_text(slot.text_lbl, label.c_str());
    register_timezone_card(
      slot.sensor_lbl, slot.unit_lbl, slot.text_lbl,
      config.entity, config.label);
  } else {
    lv_label_set_text(slot.text_lbl, "");
    register_timezone_card(
      slot.sensor_lbl, slot.unit_lbl, slot.text_lbl,
      config.entity, "", false);
  }
  return true;
}

inline bool date_time_driver_attach_interaction(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!date_time_driver_matches(context)) return false;
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  return true;
}

inline bool date_time_driver_bind_data(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  // Both local-time cards are registered during visual setup and receive
  // updates from update_timezone_cards(). They do not own an HA subscription.
  return date_time_driver_matches(context);
}

inline bool date_time_driver_large_layout(
    const Context &context, int row_span, int col_span) {
  if (context.runtime.type == card_runtime::CardTypeId::CLOCK) {
    return large_number_square_card_layout(row_span, col_span) ||
           card_span_is_wide(row_span, col_span);
  }
  return large_number_square_card_layout(row_span, col_span);
}

inline bool date_time_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const DisplayProfile &display, int row_span, int col_span) {
  if (!date_time_driver_matches(context)) return false;
  const bool large_layout =
    date_time_driver_large_layout(context, row_span, col_span);
  const bool large_numbers = card_large_numbers_supported(config) &&
    !card_large_numbers_disabled(config) &&
    (large_layout || card_large_numbers_enabled(config));
  if (!large_layout || !large_numbers || !display_large_sensor_font(display)) {
    return true;
  }

  apply_large_sensor_number_style(
    slot, display_large_sensor_font(display),
    display_large_sensor_unit_offset_percent(display));
  if (card_span_is_wide(row_span, col_span)) {
    const lv_align_t align =
      context.runtime.type == card_runtime::CardTypeId::CLOCK
        ? LV_ALIGN_LEFT_MID
        : LV_ALIGN_CENTER;
    apply_wide_large_date_time_card_layout(slot, align);
  }
  return true;
}

inline bool date_time_driver_cleanup(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  // The central timezone registry is reset before each grid rebuild. The
  // explicit lifecycle stage documents that these cards own no allocations.
  return date_time_driver_matches(context);
}

}  // namespace espcontrol::cards
