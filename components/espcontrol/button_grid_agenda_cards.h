#ifndef ESPCONTROL_BUTTON_GRID_AGENDA_CARDS_H
#define ESPCONTROL_BUTTON_GRID_AGENDA_CARDS_H

#pragma once

// Runtime registry and renderer for Agenda cards. Each card holds a
// comma-separated list of calendar entities; a shared service tick (driven
// from YAML with the panel time) fetches upcoming events through
// calendar.get_events and renders them as a Calendar-Card-Pro-style list:
// dimmed day headings, then one row per event with a colored accent bar, the
// event title, and its time underneath. Rows beyond the tile clip.

#include "calendar_agenda.h"
#include "calendar_agenda_runtime.h"

namespace espcontrol {

inline constexpr int kMaxAgendaCards = 6;
inline constexpr std::size_t kAgendaCardMaxEvents = 20;
inline constexpr uint32_t kAgendaCardRefreshMs = 10 * 60 * 1000;
inline constexpr uint32_t kAgendaAccentFallback = 0x4FC3F7;

// Row glyphs, drawn from the agenda icon font.
inline constexpr const char *kAgendaClockGlyph = "\U000F0150";  // mdi-clock-outline
inline constexpr const char *kAgendaPinGlyph = "\U000F07D9";    // mdi-map-marker-outline

// Per-calendar accent colors, indexed by the entity's position in the card's
// calendar list (Calendar-Card-Pro-style attribution).
inline constexpr uint32_t kAgendaCalendarColors[] = {
    0x66BB6A,  // green
    0xEF5350,  // red
    0x42A5F5,  // blue
    0xAB47BC,  // purple
    0xFFB300,  // amber
    0x26C6DA,  // cyan
};

inline uint32_t agenda_source_color(uint8_t source, uint32_t /*unused*/) {
  return kAgendaCalendarColors[source % (sizeof(kAgendaCalendarColors) /
                                         sizeof(kAgendaCalendarColors[0]))];
}

// Per-calendar color: an explicit "calendar.x:#RRGGBB" suffix in the card's
// calendar list wins; otherwise the palette assigns by position.
inline uint32_t agenda_source_color_from(const std::string &entities_csv,
                                         uint8_t source) {
  const std::vector<std::string> items = agenda_split_entities(entities_csv);
  if (source < items.size()) {
    const uint32_t configured = agenda_entity_color(items[source]);
    if (configured != 0) return configured;
  }
  return agenda_source_color(source, 0);
}

struct AgendaCardRef {
  lv_obj_t *list{nullptr};
  const lv_font_t *title_font{nullptr};
  const lv_font_t *small_font{nullptr};
  const lv_font_t *date_small_font{nullptr};
  const lv_font_t *row_icon_font{nullptr};
  int days{14};
  bool hide_empty_days{false};
  const lv_font_t *big_font{nullptr};
  const lv_font_t *icon_font{nullptr};
  int width_compensation_percent{100};
  uint32_t accent{0};
  std::string entities;
  std::string label;
  uint32_t last_fetch_ms{0};
  bool fetched_once{false};
};

inline AgendaCardRef *agenda_card_refs() {
  static AgendaCardRef refs[kMaxAgendaCards];
  return refs;
}

inline int &agenda_card_count() {
  static int count = 0;
  return count;
}

inline AgendaFetcher *agenda_card_fetchers() {
  static AgendaFetcher fetchers[kMaxAgendaCards];
  return fetchers;
}

// Bumped on every grid rebuild and subpage teardown so a late fetch callback
// can never touch widgets from a previous grid generation.
inline uint32_t &agenda_cards_generation() {
  static uint32_t generation = 1;
  return generation;
}

inline void reset_agenda_cards() {
  agenda_card_count() = 0;
  agenda_cards_generation()++;
}

// Retire only the registrations whose widgets have been destroyed, leaving the
// rest registered. The subpage teardown in grid phase 2 runs *after* phase 1
// has registered the main grid's cards, so clearing the whole registry there
// silently unregisters them: the card keeps its canvas but is never serviced
// again. Entries are blanked in place rather than compacted, because in-flight
// fetch callbacks capture their index.
inline void prune_dead_agenda_cards() {
  for (int i = 0; i < agenda_card_count(); i++) {
    AgendaCardRef &ref = agenda_card_refs()[i];
    if (ref.list != nullptr && !lv_obj_is_valid(ref.list)) ref = AgendaCardRef();
  }
}

// The panel clock as of the last service tick, so a card registered during a
// grid rebuild can render its fetcher's cached events immediately instead of
// sitting blank until the next tick's fetch answers.
inline int32_t &agenda_service_today() {
  static int32_t today = 0;
  return today;
}
inline bool &agenda_service_use_12h() {
  static bool use_12h = false;
  return use_12h;
}
inline bool &agenda_service_clock_seen() {
  static bool seen = false;
  return seen;
}

// Full civil time of the last service tick (5 s granularity), for features
// that must build calendar.get_events windows outside the tick itself.
struct AgendaServiceClock {
  int year = 0, month = 1, day = 1, hour = 0, minute = 0, second = 0;
};
inline AgendaServiceClock &agenda_service_clock() {
  static AgendaServiceClock clock;
  return clock;
}

inline void agenda_card_render(AgendaCardRef &ref, const AgendaList &list,
                               int32_t today_number, bool use_12h);

// Build the list container that fills the card and register it for updates.
// Fonts are borrowed from the slot's own labels so every device profile keeps
// its typography without new font roles.
inline void register_agenda_card(lv_obj_t *btn, const lv_font_t *title_font,
                                 const lv_font_t *small_font,
                                 const lv_font_t *date_small_font,
                                 const lv_font_t *row_icon_font,
                                 const lv_font_t *big_font,
                                 const lv_font_t *icon_font,
                                 int width_compensation_percent,
                                 int days, bool hide_empty_days, uint32_t accent,
                                 const std::string &entities,
                                 const std::string &label) {
  int &count = agenda_card_count();
  // Reuse a slot retired by prune_dead_agenda_cards() before growing, so a
  // run of subpage rebuilds cannot walk the registry up to its cap.
  int index = -1;
  for (int i = 0; i < count; i++) {
    if (agenda_card_refs()[i].list == nullptr) { index = i; break; }
  }
  if (index < 0) {
    if (count >= kMaxAgendaCards) {
      ESP_LOGW("agenda", "Too many agenda cards; skipping updates for extras");
      return;
    }
    index = count;
  }

  lv_obj_t *list = lv_obj_create(btn);
  lv_obj_set_size(list, lv_pct(100), lv_pct(100));
  lv_obj_align(list, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 6, 0);
  lv_obj_set_style_pad_row(list, 10, 0);
  // Drag to scroll through the full fetched list; a plain tap bubbles to the
  // card button, which opens the dedicated agenda view.
  lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(list, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  AgendaCardRef &ref = agenda_card_refs()[index];
  ref = AgendaCardRef();
  ref.list = list;
  ref.title_font = title_font;
  ref.small_font = small_font;
  ref.date_small_font = date_small_font;
  ref.row_icon_font = row_icon_font;
  ref.big_font = big_font;
  ref.icon_font = icon_font;
  ref.width_compensation_percent = width_compensation_percent;
  ref.days = days > 0 ? days : 14;
  ref.hide_empty_days = hide_empty_days;
  ref.accent = accent;
  ref.entities = entities;
  ref.label = label;
  const AgendaList &cached = agenda_card_fetchers()[index].last_result();
  if (agenda_service_clock_seen() && !cached.empty()) {
    agenda_card_render(ref, cached, agenda_service_today(),
                       agenda_service_use_12h());
  }
  if (index == count) count++;
}

inline lv_obj_t *agenda_row_label_(lv_obj_t *parent, const char *text,
                                   const lv_font_t *font, uint32_t color,
                                   lv_opa_t opa) {
  lv_obj_t *lbl = lv_label_create(parent);
  lv_label_set_text(lbl, text);
  if (font != nullptr) lv_obj_set_style_text_font(lbl, font, 0);
  lv_obj_set_style_text_color(lbl, lv_color_hex(color), 0);
  lv_obj_set_style_text_opa(lbl, opa, 0);
  lv_obj_set_width(lbl, lv_pct(100));
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
  return lbl;
}

// ── Shared row building blocks ───────────────────────────────────────────
// The card and the screensaver overlay render the same Calendar-Card-Pro
// anatomy: a day row holding a date column (weekday / big day number / MONTH)
// beside a column of accent-tinted event boxes.

// Natural height of a two-line (title + time) event box; the date column is
// sized to this so day rows read at one uniform height with no stretching.
inline int agenda_two_line_event_height_(const lv_font_t *title_font,
                                         const lv_font_t *small_font,
                                         const lv_font_t *row_icon_font = nullptr) {
  const int title_lh = title_font ? lv_font_get_line_height(title_font) : 16;
  const int small_lh = small_font ? lv_font_get_line_height(small_font) : 12;
  // The time line carries an icon, which may be the taller of the two.
  const int icon_lh = row_icon_font ? lv_font_get_line_height(row_icon_font) : 0;
  const int line_lh = icon_lh > small_lh ? icon_lh : small_lh;
  return title_lh + line_lh + 20;  // pad_top 9 + pad_bottom 9 + pad_row 2
}

// One line of secondary text preceded by an MDI glyph, as in the reference
// layout: a clock before the time range, a pin before the location. The text
// grows so anything right-aligned after it lands on the row's right edge.
inline lv_obj_t *agenda_icon_line_(lv_obj_t *parent, const char *glyph,
                                   const char *text,
                                   const lv_font_t *row_icon_font,
                                   const lv_font_t *small_font,
                                   lv_opa_t text_opa) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_set_style_pad_column(row, 5, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *icon = lv_label_create(row);
  lv_label_set_text(icon, glyph);
  if (row_icon_font != nullptr)
    lv_obj_set_style_text_font(icon, row_icon_font, 0);
  lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_opa(icon, LV_OPA_40, 0);

  lv_obj_t *label = lv_label_create(row);
  lv_label_set_text(label, text);
  if (small_font != nullptr) lv_obj_set_style_text_font(label, small_font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_opa(label, text_opa, 0);
  lv_obj_set_flex_grow(label, 1);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  return row;
}

// Monday-start week anchor for the day, used to draw week separators.
inline int32_t agenda_week_start_(int32_t day_number) {
  return day_number - static_cast<int32_t>((agenda_weekday(day_number) + 6) % 7);
}

inline void agenda_week_separator_(lv_obj_t *parent) {
  lv_obj_t *line = lv_obj_create(parent);
  lv_obj_set_size(line, lv_pct(100), 1);
  lv_obj_set_style_bg_color(line, lv_color_hex(0x4FC3F7), 0);
  lv_obj_set_style_bg_opa(line, LV_OPA_40, 0);
  lv_obj_set_style_border_width(line, 0, 0);
  lv_obj_set_style_radius(line, 0, 0);
  lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
}

inline const char *agenda_weekday_name_(int32_t day_number) {
  static const char *const kWeekdays[] = {"SUN", "MON", "TUE", "WED",
                                          "THU", "FRI", "SAT"};
  return kWeekdays[agenda_weekday(day_number)];
}

// Creates a day row with its date column and returns the events column that
// event boxes for this day should be added to.
inline lv_obj_t *agenda_day_row_(lv_obj_t *parent, int32_t day_number,
                                 int date_col_w, const lv_font_t *small_font,
                                 const lv_font_t *big_font,
                                 int date_col_min_h = 0,
                                 bool is_today = false) {
  static const char *const kMonths[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  lv_obj_t *day_row = lv_obj_create(parent);
  lv_obj_set_size(day_row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(day_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(day_row, 0, 0);
  lv_obj_set_style_pad_all(day_row, 0, 0);
  lv_obj_set_style_pad_column(day_row, 6, 0);
  lv_obj_clear_flag(day_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(day_row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_flex_flow(day_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(day_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  int year = 0, month = 1, day = 1;
  agenda_civil_from_days(day_number, &year, &month, &day);
  lv_obj_t *date_col = lv_obj_create(day_row);
  lv_obj_set_size(date_col, date_col_w, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(date_col, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(date_col, 0, 0);
  lv_obj_set_style_pad_all(date_col, 0, 0);
  lv_obj_set_style_pad_row(date_col, 0, 0);
  lv_obj_clear_flag(date_col, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(date_col, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_flex_flow(date_col, LV_FLEX_FLOW_COLUMN);
  // Center the stack within the two-line event height so the date and a
  // normal event box read as one row of equal height.
  lv_obj_set_flex_align(date_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  if (date_col_min_h > 0)
    lv_obj_set_style_min_height(date_col, date_col_min_h, 0);
  if (is_today) {
    // Blue dot beside the day number, like the reference's today marker.
    lv_obj_t *dot = lv_obj_create(date_col);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x4FC3F7), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
  }

  char text[8];
  std::snprintf(text, sizeof(text), "%s", agenda_weekday_name_(day_number));
  lv_obj_t *weekday = agenda_row_label_(date_col, text, small_font, 0xFFFFFF,
                                        LV_OPA_60);
  lv_obj_set_style_text_align(weekday, LV_TEXT_ALIGN_CENTER, 0);
  std::snprintf(text, sizeof(text), "%d", day);
  lv_obj_t *num = agenda_row_label_(date_col, text, big_font, 0xFFFFFF,
                                    LV_OPA_COVER);
  lv_obj_set_style_text_align(num, LV_TEXT_ALIGN_CENTER, 0);
  std::snprintf(text, sizeof(text), "%s", kMonths[(month - 1) % 12]);
  lv_obj_t *month_lbl = agenda_row_label_(date_col, text, small_font, 0xFFFFFF,
                                          LV_OPA_60);
  lv_obj_set_style_text_align(month_lbl, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *events_col = lv_obj_create(day_row);
  lv_obj_set_size(events_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(events_col, 1);
  lv_obj_set_style_bg_opa(events_col, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(events_col, 0, 0);
  lv_obj_set_style_pad_all(events_col, 0, 0);
  lv_obj_set_style_pad_row(events_col, 4, 0);
  lv_obj_clear_flag(events_col, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(events_col, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_flex_flow(events_col, LV_FLEX_FLOW_COLUMN);
  return events_col;
}

// Event box: accent-tinted background with a solid accent bar on its left
// edge, single-line ellipsized title with the "in N days" marker right-aligned
// beside it, the time range, and the location when present.
inline void agenda_event_box_(lv_obj_t *events_col, const AgendaEntry &entry,
                              int32_t today_number, bool use_12h,
                              uint32_t color, const lv_font_t *title_font,
                              const lv_font_t *small_font,
                              const lv_font_t *row_icon_font) {
  const bool has_location = !entry.location.empty();
  const int title_h = title_font ? lv_font_get_line_height(title_font) : 16;
  lv_obj_t *box = lv_obj_create(events_col);
  lv_obj_set_size(box, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(box, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(box, LV_OPA_20, 0);
  lv_obj_set_style_radius(box, 5, 0);
  lv_obj_set_style_border_width(box, 0, 0);
  lv_obj_set_style_pad_all(box, 6, 0);
  lv_obj_set_style_pad_top(box, 9, 0);
  lv_obj_set_style_pad_bottom(box, 9, 0);
  lv_obj_set_style_pad_left(box, 11, 0);
  lv_obj_set_style_pad_row(box, 2, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *bar = lv_obj_create(box);
  lv_obj_set_size(bar, 3, lv_pct(100));
  lv_obj_add_flag(bar, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_align(bar, LV_ALIGN_LEFT_MID, -8, 0);
  lv_obj_set_style_bg_color(bar, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 2, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *title = lv_label_create(box);
  lv_label_set_text(title, entry.summary.c_str());
  if (title_font != nullptr)
    lv_obj_set_style_text_font(title, title_font, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_width(title, lv_pct(100));
  // A single ellipsized line, like the reference: LONG_DOT only truncates
  // once the height is pinned to one text line.
  lv_obj_set_height(title, title_h);
  lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);

  char range[48];
  agenda_format_range(range, sizeof(range), entry, use_12h, today_number);
  lv_obj_t *time_row = agenda_icon_line_(box, kAgendaClockGlyph, range,
                                         row_icon_font, small_font, LV_OPA_60);

  // The relative marker shares the time line, right-aligned.
  char rel[16];
  agenda_format_relative(rel, sizeof(rel), entry.when.day_number, today_number);
  if (rel[0] != '\0') {
    lv_obj_t *marker = lv_label_create(time_row);
    lv_label_set_text(marker, rel);
    if (small_font != nullptr)
      lv_obj_set_style_text_font(marker, small_font, 0);
    lv_obj_set_style_text_color(marker, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(marker, LV_OPA_50, 0);
  }

  if (has_location) {
    agenda_icon_line_(box, kAgendaPinGlyph, entry.location.c_str(),
                      row_icon_font, small_font, LV_OPA_40);
  }
}

// Render the agenda into the card's list container: one row per day with a
// date column beside accent-tinted event boxes — the Calendar Card Pro layout.
inline void agenda_card_render(AgendaCardRef &ref, const AgendaList &list,
                               int32_t today_number, bool use_12h) {
  if (ref.list == nullptr) return;
  lv_obj_clean(ref.list);

  if (list.empty()) {
    agenda_row_label_(ref.list, espcontrol_i18n("No upcoming events"),
                      ref.small_font, 0xFFFFFF, LV_OPA_60);
    return;
  }

  const int big_h = ref.big_font ? lv_font_get_line_height(ref.big_font) : 24;
  const int two_line_h = agenda_two_line_event_height_(ref.title_font, ref.small_font,
                                                      ref.row_icon_font);
  int date_col_w = big_h + big_h / 4;   // fits two digits of the day number
  if (date_col_w < 40) date_col_w = 40;

  const std::vector<AgendaEntry> &entries = list.entries();
  lv_obj_t *events_col = nullptr;
  for (std::size_t i = 0; i < entries.size(); i++) {
    const AgendaEntry &entry = entries[i];
    const bool new_day = list.starts_new_day(i);
    const bool new_week =
        new_day && events_col != nullptr &&
        agenda_week_start_(entry.when.day_number) !=
            agenda_week_start_(entries[i - 1].when.day_number);
    if (new_week) agenda_week_separator_(ref.list);

    if (new_day) {
      events_col = agenda_day_row_(ref.list, entry.when.day_number, date_col_w,
                                   ref.date_small_font, ref.big_font,
                                   two_line_h,
                                   entry.when.day_number == today_number);
    }
    if (events_col == nullptr) continue;
    agenda_event_box_(events_col, entry, today_number, use_12h,
                      agenda_source_color_from(ref.entities, entry.source),
                      ref.title_font, ref.small_font, ref.row_icon_font);
  }
}

// ── Screensaver overlay rendering ────────────────────────────────────────
// A compact agenda for the photo screensaver: the next event, the current
// day's events, or a short upcoming list — rendered with the same day-row and
// tinted-event-box anatomy as the grid card, into a caller-owned box whose
// background (and its opacity) the YAML controls. Returns false when there is
// nothing to show so the caller can hide the box entirely.

enum class AgendaOverlayStyle : uint8_t { NEXT_EVENT, TODAY, UPCOMING };

inline bool agenda_overlay_render(lv_obj_t *box, const AgendaList &list,
                                  AgendaOverlayStyle style, int32_t today_number,
                                  bool use_12h, const lv_font_t *title_font,
                                  const lv_font_t *small_font,
                                  const lv_font_t *date_small_font,
                                  const lv_font_t *row_icon_font,
                                  const lv_font_t *big_font,
                                  int max_events = 5,
                                  const char *entities_csv = "") {
  if (box == nullptr) return false;
  lv_obj_clean(box);
  if (list.empty()) return false;
  const std::vector<AgendaEntry> &entries = list.entries();

  const int big_h = big_font ? lv_font_get_line_height(big_font) : 24;
  const int two_line_h = agenda_two_line_event_height_(title_font, small_font,
                                                      row_icon_font);
  int date_col_w = big_h + big_h / 4;
  if (date_col_w < 40) date_col_w = 40;

  if (max_events < 1) max_events = 1;
  const std::string entities_text(entities_csv != nullptr ? entities_csv : "");
  std::vector<const AgendaEntry *> picked;
  if (style == AgendaOverlayStyle::NEXT_EVENT) {
    picked.push_back(&entries.front());
  } else if (style == AgendaOverlayStyle::TODAY) {
    for (const AgendaEntry &entry : entries) {
      if (entry.when.day_number != today_number) continue;
      picked.push_back(&entry);
      if (static_cast<int>(picked.size()) >= max_events) break;
    }
    // Nothing left today: fall back to the next event so the overlay stays
    // useful instead of disappearing.
    if (picked.empty()) picked.push_back(&entries.front());
  } else {
    for (const AgendaEntry &entry : entries) {
      picked.push_back(&entry);
      if (static_cast<int>(picked.size()) >= max_events) break;
    }
  }

  // The overlay does not scroll, so the day the limit cut through is the one
  // worth flagging: it is showing an incomplete picture of that day. Days
  // beyond it were never being shown at all, so counting them would say
  // little about what is missing here.
  const int32_t last_day = picked.back()->when.day_number;
  std::size_t shown_on_last_day = 0;
  for (const AgendaEntry *entry : picked)
    if (entry->when.day_number == last_day) shown_on_last_day++;
  const std::size_t last_day_total = agenda_events_on_day(entries, last_day);
  const std::size_t hidden = last_day_total > shown_on_last_day
                                 ? last_day_total - shown_on_last_day
                                 : 0;

  int32_t prev_day = INT32_MIN;
  lv_obj_t *events_col = nullptr;
  for (std::size_t i = 0; i < picked.size(); i++) {
    const AgendaEntry *entry = picked[i];
    if (entry->when.day_number != prev_day || events_col == nullptr) {
      if (events_col != nullptr &&
          agenda_week_start_(entry->when.day_number) !=
              agenda_week_start_(prev_day)) {
        agenda_week_separator_(box);
      }
      prev_day = entry->when.day_number;
      events_col = agenda_day_row_(box, entry->when.day_number, date_col_w,
                                   date_small_font, big_font, two_line_h,
                                   entry->when.day_number == today_number);
    }
    agenda_event_box_(events_col, *entry, today_number, use_12h,
                      agenda_source_color_from(entities_text, entry->source),
                      title_font, small_font, row_icon_font);
  }

  if (hidden > 0 && events_col != nullptr) {
    char more[24];
    std::snprintf(more, sizeof(more), "+%u more", (unsigned) hidden);
    // Inside that day's column, so it reads as belonging to the day it
    // describes rather than to the overlay as a whole.
    lv_obj_t *label = agenda_row_label_(events_col, more, small_font, 0xFFFFFF,
                                        LV_OPA_50);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
  }
  return true;
}

// Periodic service driven from YAML with the panel clock. Fetches each card's
// calendars on first sight and every kAgendaCardRefreshMs afterwards.
inline void agenda_cards_service(int year, int month, int day, int hour,
                                 int minute, int second, bool use_12h) {
  const uint32_t now_ms = esphome::millis();
  const int32_t today = agenda_days_from_civil(year, month, day);
  agenda_service_today() = today;
  agenda_service_use_12h() = use_12h;
  agenda_service_clock_seen() = true;
  agenda_service_clock() = AgendaServiceClock{year, month, day, hour, minute, second};
  for (int i = 0; i < agenda_card_count(); i++) {
    AgendaCardRef &ref = agenda_card_refs()[i];
    if (ref.entities.empty()) continue;
    if (ref.list == nullptr || !lv_obj_is_valid(ref.list)) continue;
    if (ref.fetched_once && now_ms - ref.last_fetch_ms < kAgendaCardRefreshMs) {
      continue;
    }
    ref.fetched_once = true;
    ref.last_fetch_ms = now_ms;

    AgendaFetcher &fetcher = agenda_card_fetchers()[i];
    fetcher.set_entities(agenda_entity_ids(ref.entities));
    fetcher.set_max_events(kAgendaCardMaxEvents);
    const uint32_t generation = agenda_cards_generation();
    const int index = i;
    lv_obj_t *const expected = ref.list;
    fetcher.set_on_ready([index, generation, expected, today, use_12h](const AgendaList &list) {
      if (generation != agenda_cards_generation()) return;  // grid rebuilt
      if (index >= agenda_card_count()) return;
      AgendaCardRef &current = agenda_card_refs()[index];
      // The slot may have been retired and handed to another card since the
      // fetch went out, so match the widget as well as the index.
      if (current.list != expected) return;
      // Belt and braces: never render into a widget LVGL no longer tracks.
      if (current.list == nullptr || !lv_obj_is_valid(current.list)) return;
      agenda_card_render(current, list, today, use_12h);
    });

    char start[24];
    std::snprintf(start, sizeof(start), "%04d-%02d-%02d %02d:%02d:%02d", year,
                  month, day, hour, minute, second);
    int ey = 0, em = 0, ed = 0;
    agenda_civil_from_days(today + ref.days, &ey, &em, &ed);
    char end[24];
    std::snprintf(end, sizeof(end), "%04d-%02d-%02d 00:00:00", ey, em, ed);
    const int64_t now_epoch = static_cast<int64_t>(today) * 86400 +
                              hour * 3600 + minute * 60 + second;
    fetcher.refresh(std::string(start), std::string(end), now_epoch);
  }
}

}  // namespace espcontrol

#endif  // ESPCONTROL_BUTTON_GRID_AGENDA_CARDS_H
