#ifndef ESPCONTROL_BUTTON_GRID_AGENDA_DRIVER_H
#define ESPCONTROL_BUTTON_GRID_AGENDA_DRIVER_H

#pragma once

// Lifecycle driver for the Agenda card: a read-only tile showing the next
// upcoming event from one or more Home Assistant calendars. Registration and
// rendering live in button_grid_agenda_cards.h; fetching goes through the
// shared calendar.get_events response-template path.
//
// Contract coverage marker: "agenda".

namespace espcontrol::cards {

inline bool agenda_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::AGENDA;
}

inline bool agenda_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display) {
  if (!agenda_driver_matches(context)) return false;

  // Near-black canvas like the Calendar Card Pro dark theme, so the tinted
  // event boxes carry the color; a configured sensor color still overrides.
  const lv_color_t agenda_canvas =
      lv_color_hex(palette.has_sensor_color ? palette.sensor_val : 0x101216);
  lv_obj_set_style_bg_color(
    slot.btn, agenda_canvas,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
      static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  // Scrolling the list would otherwise flash the button's pressed color
  // across the whole card; opening the agenda view is feedback enough.
  lv_obj_set_style_bg_color(
    slot.btn, agenda_canvas,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
      static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  // The agenda draws its own event list over the whole tile; the standard
  // icon/sensor/name widgets stay hidden. Fonts are borrowed from the slot's
  // labels so device typography profiles apply unchanged: the name label's
  // font for event titles and the unit label's smaller font for times and
  // day headings.
  lv_obj_add_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.text_lbl, LV_OBJ_FLAG_HIDDEN);
  // Deliberate hierarchy: medium-weight titles, clearly smaller secondary
  // text, and a mid-size day number — with fallbacks to the slot's fonts for
  // profiles that predate the agenda roles.
  const lv_font_t *title_font = display.fonts.agenda_title != nullptr
      ? display.fonts.agenda_title
      : lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  const lv_font_t *small_font = display.fonts.agenda_secondary != nullptr
      ? display.fonts.agenda_secondary
      : (display.fonts.small_text != nullptr
             ? display.fonts.small_text
             : lv_obj_get_style_text_font(slot.unit_lbl, LV_PART_MAIN));
  const lv_font_t *big_font = display.fonts.agenda_day != nullptr
      ? display.fonts.agenda_day
      : (display.fonts.media_title != nullptr
             ? display.fonts.media_title
             : lv_obj_get_style_text_font(slot.sensor_lbl, LV_PART_MAIN));
  const lv_font_t *date_small_font = display.fonts.agenda_date_small != nullptr
      ? display.fonts.agenda_date_small
      : small_font;
  const lv_font_t *row_icon_font = display.fonts.agenda_icon;
  const uint32_t accent = palette.has_on ? palette.on_val : 0;
  register_agenda_card(slot.btn, title_font, small_font, date_small_font,
                       row_icon_font, big_font, display.fonts.icon,
                       display_main_width_percent(display),
                       agenda_card_days(config),
                       agenda_card_hides_empty_days(config), accent,
                       config.entity, config.label);
  return true;
}

inline bool agenda_driver_attach_interaction(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!agenda_driver_matches(context)) return false;
  // Tapping the card opens the dedicated two-week agenda view.
  lv_obj_add_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *e) {
    lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_current_target(e));
    if (!agenda_service_clock_seen()) return;
    for (int i = 0; i < agenda_card_count(); i++) {
      AgendaCardRef &ref = agenda_card_refs()[i];
      if (ref.list == nullptr || lv_obj_get_parent(ref.list) != btn) continue;
      const AgendaServiceClock &clock = agenda_service_clock();
      agenda_view_open(ref.entities, ref.title_font, ref.small_font,
                       ref.date_small_font, ref.row_icon_font, ref.big_font,
                       ref.icon_font,
                       agenda_service_use_12h(), clock.year, clock.month,
                       clock.day, clock.hour, clock.minute, clock.second, btn,
                       ref.width_compensation_percent);
      return;
    }
  }, LV_EVENT_CLICKED, nullptr);
  return true;
}

inline bool agenda_driver_bind_data(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  // Fetching needs the panel clock, which YAML supplies to
  // agenda_cards_service(); registration during visual setup is enough here.
  return agenda_driver_matches(context);
}

inline bool agenda_driver_refresh_layout(
    BtnSlot &, const ParsedCfg &, const Context &context,
    const DisplayProfile &, int, int) {
  return agenda_driver_matches(context);
}

inline bool agenda_driver_cleanup(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  // The agenda registry is reset before each grid rebuild; cards own no
  // dynamic allocations of their own.
  return agenda_driver_matches(context);
}

}  // namespace espcontrol::cards

#endif  // ESPCONTROL_BUTTON_GRID_AGENDA_DRIVER_H
