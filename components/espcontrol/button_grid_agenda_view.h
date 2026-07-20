#ifndef ESPCONTROL_BUTTON_GRID_AGENDA_VIEW_H
#define ESPCONTROL_BUTTON_GRID_AGENDA_VIEW_H

#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.
//
// Dedicated full-screen agenda view, opened by tapping an agenda grid card or
// the photo screensaver's agenda overlay. Renders every day of the next two
// weeks inside the shared control-modal shell: events grouped with the same
// day-row anatomy as the card, muted "No upcoming events" rows for empty
// days, week separators, and the today marker.

namespace espcontrol {

inline constexpr int kAgendaViewDays = 14;  // default span
inline constexpr std::size_t kAgendaViewMaxEvents = 40;

struct AgendaViewUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *list = nullptr;
  const lv_font_t *title_font = nullptr;
  const lv_font_t *small_font = nullptr;
  const lv_font_t *date_small_font = nullptr;
  const lv_font_t *row_icon_font = nullptr;
  const lv_font_t *big_font = nullptr;
  std::string entities;
  bool use_12h = false;
  int32_t today = 0;
  int days = kAgendaViewDays;
  bool hide_empty_days = false;
};

// Device-level settings for the shared view, pushed from YAML each service
// tick. The view deliberately does not inherit the card's or overlay's span:
// it is the "show me more" surface, so it carries its own.
struct AgendaViewSettings {
  int days = kAgendaViewDays;
  bool hide_empty_days = false;
};

inline AgendaViewSettings &agenda_view_settings() {
  static AgendaViewSettings settings;
  return settings;
}

inline AgendaViewUi &agenda_view_ui() {
  static AgendaViewUi ui;
  return ui;
}

// The screensaver's agenda overlay box, registered from YAML so the
// touchscreen-level wake can yield to taps that should open this view.
inline lv_obj_t *&agenda_overlay_box() {
  static lv_obj_t *box = nullptr;
  return box;
}

// True when a raw screen touch (already mapped into LVGL coordinates) must
// NOT wake the screensaver: either the agenda view is open above it, or the
// touch is on the visible agenda overlay whose click opens the view.
inline bool agenda_touch_blocks_wake(int32_t x, int32_t y) {
  if (control_modal_active().overlay != nullptr) return true;
  lv_obj_t *box = agenda_overlay_box();
  if (box == nullptr || !lv_obj_is_valid(box)) return false;
  if (lv_obj_has_flag(box, LV_OBJ_FLAG_HIDDEN)) return false;
  lv_obj_t *saver = lv_obj_get_parent(box);
  if (saver != nullptr && lv_obj_has_flag(saver, LV_OBJ_FLAG_HIDDEN)) return false;
  lv_area_t area;
  lv_obj_get_coords(box, &area);
  return x >= area.x1 && x <= area.x2 && y >= area.y1 && y <= area.y2;
}

inline AgendaFetcher &agenda_view_fetcher() {
  static AgendaFetcher fetcher;
  return fetcher;
}

inline uint32_t &agenda_view_generation() {
  static uint32_t generation = 1;
  return generation;
}

inline void agenda_view_hide() {
  AgendaViewUi &ui = agenda_view_ui();
  agenda_view_generation()++;
  ui.panel = nullptr;
  ui.list = nullptr;
  control_modal_delete_overlay(ControlModalKind::AGENDA_VIEW, ui.overlay);
}

// Muted filler row for a day with nothing scheduled, like the reference.
inline void agenda_view_empty_row_(lv_obj_t *events_col, int min_height,
                                   const lv_font_t *small_font) {
  lv_obj_t *box = lv_obj_create(events_col);
  lv_obj_set_size(box, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(box, lv_color_hex(0x42A5F5), 0);
  lv_obj_set_style_bg_opa(box, LV_OPA_10, 0);
  lv_obj_set_style_radius(box, 5, 0);
  lv_obj_set_style_border_width(box, 0, 0);
  lv_obj_set_style_pad_all(box, 6, 0);
  lv_obj_set_style_pad_left(box, 11, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_min_height(box, min_height, 0);
  lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  lv_obj_t *bar = lv_obj_create(box);
  lv_obj_set_size(bar, 3, lv_pct(100));
  lv_obj_add_flag(bar, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_align(bar, LV_ALIGN_LEFT_MID, -8, 0);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x42A5F5), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 2, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);

  agenda_row_label_(box, espcontrol_i18n("No upcoming events"), small_font,
                    0xFFFFFF, LV_OPA_50);
}

inline void agenda_view_render_(const AgendaList &list) {
  AgendaViewUi &ui = agenda_view_ui();
  if (ui.list == nullptr || !lv_obj_is_valid(ui.list)) return;
  lv_obj_clean(ui.list);

  const int big_h = ui.big_font ? lv_font_get_line_height(ui.big_font) : 24;
  const int two_line_h =
      agenda_two_line_event_height_(ui.title_font, ui.small_font,
                                    ui.row_icon_font);
  int date_col_w = big_h + big_h / 4;
  if (date_col_w < 40) date_col_w = 40;

  const std::vector<AgendaEntry> &entries = list.entries();
  std::size_t idx = 0;
  for (int32_t day = ui.today; day < ui.today + ui.days; day++) {
    if (day != ui.today &&
        agenda_week_start_(day) != agenda_week_start_(day - 1)) {
      agenda_week_separator_(ui.list);
    }
    // With empty days hidden, skip the whole row rather than drawing a date
    // column with nothing beside it.
    if (ui.hide_empty_days) {
      bool has_event = false;
      for (std::size_t probe = idx; probe < entries.size(); probe++) {
        if (entries[probe].when.day_number > day) break;
        if (entries[probe].when.day_number == day ||
            (day == ui.today && entries[probe].when.day_number < day)) {
          has_event = true;
          break;
        }
      }
      if (!has_event) continue;
    }
    lv_obj_t *events_col =
        agenda_day_row_(ui.list, day, date_col_w, ui.date_small_font,
                        ui.big_font, two_line_h, day == ui.today);
    bool any = false;
    // Events that started before today (ongoing multi-day) group under today.
    while (idx < entries.size() && entries[idx].when.day_number <= day) {
      const AgendaEntry &entry = entries[idx];
      if (entry.when.day_number == day ||
          (day == ui.today && entry.when.day_number < day)) {
        agenda_event_box_(events_col, entry, ui.today, ui.use_12h,
                          agenda_source_color_from(ui.entities, entry.source),
                          ui.title_font, ui.small_font, ui.row_icon_font);
        any = true;
      }
      idx++;
    }
    if (!any && !ui.hide_empty_days)
      agenda_view_empty_row_(events_col, two_line_h, ui.small_font);
  }
}

// Open the dedicated view and fetch a fresh two-week window. The panel clock
// is supplied by the caller (YAML) or the card service's last tick.
inline void agenda_view_open(const std::string &entities_csv,
                             const lv_font_t *title_font,
                             const lv_font_t *small_font,
                             const lv_font_t *date_small_font,
                             const lv_font_t *row_icon_font,
                             const lv_font_t *big_font,
                             const lv_font_t *icon_font, bool use_12h,
                             int year, int month, int day, int hour, int minute,
                             int second, lv_obj_t *source_btn,
                             int width_compensation_percent) {
  if (entities_csv.empty()) return;
  ControlModalShell shell = control_modal_open_shell(
      ControlModalKind::AGENDA_VIEW, source_btn, width_compensation_percent,
      icon_font, agenda_view_hide);
  if (shell.overlay == nullptr || shell.panel == nullptr) return;

  AgendaViewUi &ui = agenda_view_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.title_font = title_font;
  ui.small_font = small_font;
  ui.date_small_font = date_small_font;
  ui.row_icon_font = row_icon_font;
  ui.big_font = big_font;
  ui.entities = entities_csv;
  ui.use_12h = use_12h;
  ui.today = agenda_days_from_civil(year, month, day);
  const AgendaViewSettings &settings = agenda_view_settings();
  ui.days = settings.days >= 1 && settings.days <= 60 ? settings.days
                                                      : kAgendaViewDays;
  ui.hide_empty_days = settings.hide_empty_days;
  control_modal_set_active(ControlModalKind::AGENDA_VIEW, shell.overlay,
                           agenda_view_hide,
                           ControlModalDismissPolicy::DISMISS);

  ControlModalLayout &layout = shell.layout;
  lv_coord_t gap = control_modal_scaled_px(18, layout.short_side);
  if (gap < 10) gap = 10;
  lv_coord_t title_y = layout.inset + layout.back_size / 2;
  lv_coord_t list_y = layout.inset + layout.back_size + gap;
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < 60) list_h = 60;
  lv_coord_t list_pad = control_modal_scaled_px(14, layout.short_side);
  if (list_pad < 6) list_pad = 6;
  lv_coord_t list_w = shell.content_w - list_pad * 2;
  if (list_w < 80) list_w = shell.content_w;

  lv_obj_t *title = control_modal_create_title(
      ui.panel, espcontrol_i18n("Agenda"),
      shell.content_w - layout.back_size - gap, title_font,
      width_compensation_percent);
  lv_obj_set_style_text_color(title, lv_color_hex(DARK_TEXT_MUTED),
                              LV_PART_MAIN);
  lv_obj_update_layout(title);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0,
               title_y - lv_obj_get_height(title) / 2);

  ui.list = control_modal_create_scroll_list(ui.panel, list_w, list_h, 10);
  lv_obj_align(ui.list, LV_ALIGN_TOP_MID, 0, list_y);

  // Fetch a fresh window; render whatever the fetcher already has so the view
  // isn't blank while the round-trip completes.
  AgendaFetcher &fetcher = agenda_view_fetcher();
  if (!fetcher.last_result().empty()) agenda_view_render_(fetcher.last_result());
  fetcher.set_entities(agenda_entity_ids(entities_csv));
  fetcher.set_max_events(kAgendaViewMaxEvents);
  const uint32_t generation = agenda_view_generation();
  fetcher.set_on_ready([generation](const AgendaList &result) {
    if (generation != agenda_view_generation()) return;
    agenda_view_render_(result);
  });
  char start[24];
  std::snprintf(start, sizeof(start), "%04d-%02d-%02d %02d:%02d:%02d", year,
                month, day, hour, minute, second);
  const int32_t today = agenda_days_from_civil(year, month, day);
  int ey = 0, em = 0, ed = 0;
  agenda_civil_from_days(today + ui.days, &ey, &em, &ed);
  char end[24];
  std::snprintf(end, sizeof(end), "%04d-%02d-%02d 00:00:00", ey, em, ed);
  const int64_t now_epoch = static_cast<int64_t>(today) * 86400 +
                            hour * 3600 + minute * 60 + second;
  fetcher.refresh(std::string(start), std::string(end), now_epoch);
}

}  // namespace espcontrol

#endif  // ESPCONTROL_BUTTON_GRID_AGENDA_VIEW_H
