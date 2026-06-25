#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Home Assistant calendar card: countdown to upcoming/active Google Calendar events.

constexpr uint32_t HA_CALENDAR_CTX_MAGIC = 0x43414C43;  // CALC
constexpr int      HA_CALENDAR_MAX_EVENTS = 24;
constexpr size_t   HA_CALENDAR_TITLE_MAX_LEN = 64;
constexpr size_t   HA_CALENDAR_TIME_DISPLAY_MAX_LEN = 8;  // "H:MM\0"
constexpr size_t   HA_CALENDAR_RESPONSE_MAX_LEN = 2048;
constexpr uint32_t HA_CALENDAR_REQUEST_TIMEOUT_MS = 15000;
constexpr int      HA_CALENDAR_URGENT_SECS = 300;  // 5 minutes
constexpr size_t   HA_CALENDAR_MODAL_INTERNAL_FREE_MIN_BYTES = HA_ACTION_INTERNAL_FREE_MIN_BYTES;
constexpr size_t   HA_CALENDAR_MODAL_INTERNAL_LARGEST_MIN_BYTES = HA_ACTION_INTERNAL_LARGEST_MIN_BYTES;
// Spacing for the dual "1h 51m" countdown: negative pulls each tiny unit tight
// against its number; the group gap is the breathing room between "1h" and "51m".
constexpr int      HA_CALENDAR_HM_UNIT_GAP = -6;
// "1" in the bold number font carries a wide right side bearing, so a unit that
// follows a digit ending in 1 needs an extra pull to look as tight as the rest.
constexpr int      HA_CALENDAR_HM_UNIT_GAP_ONE = -13;
constexpr int      HA_CALENDAR_HM_GROUP_GAP = 2;

// ── Structs ──────────────────────────────────────────────────────────────────

struct HaCalendarEventRow {
  char title[HA_CALENDAR_TITLE_MAX_LEN + 1] = {};
  char start_display[HA_CALENDAR_TIME_DISPLAY_MAX_LEN] = {};
  char end_display[HA_CALENDAR_TIME_DISPLAY_MAX_LEN] = {};
  time_t start_epoch = 0;
  time_t end_epoch = 0;
};

struct HaCalendarEntityState {
  std::string entity_id;
  bool active = false;      // HA state == "on"
  bool available = false;
  std::string title;        // "message" attribute (current/next event title)
  time_t start_epoch = 0;  // "start_time" attribute parsed to UTC epoch
  time_t end_epoch = 0;    // "end_time" attribute
};

struct HaCalendarCardCtx {
  uint32_t magic = HA_CALENDAR_CTX_MAGIC;
  std::vector<HaCalendarEntityState> entities;
  std::string configured_label;
  std::string display_mode;   // "current" or "next_event"

  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;    // repurposed: "In" / "Now" / "" text label
  lv_obj_t *value_lbl = nullptr;   // sensor_lbl: countdown number or "Now"
  lv_obj_t *unit_lbl = nullptr;    // "min" / "hr" / "days" (or "h" in dual h+m)
  lv_obj_t *value2_lbl = nullptr;  // second number for the "1h 51m" dual format
  lv_obj_t *unit2_lbl = nullptr;   // "m" suffix for the "1h 51m" dual format
  lv_obj_t *label_lbl = nullptr;   // text_lbl: event title or card label
  lv_obj_t *status_lbl = nullptr;  // event-card bottom-left status ("Now" / "In 12m")
  lv_obj_t *progress_slider = nullptr;
  const lv_font_t *value_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *list_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  const lv_font_t *unit_font = nullptr;   // original unit-label font (for restore)
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = DEFAULT_OFF_COLOR;
  int width_compensation_percent = 100;
  bool urgent_color_enabled = false;
  bool current_progress_enabled = false;
  int urgent_secs = HA_CALENDAR_URGENT_SECS;
  int next_now_secs = HA_CALENDAR_URGENT_SECS;
  bool available = false;
  lv_timer_t *refresh_timer = nullptr;  // periodic re-render so countdown ticks

  // Background event cache: Next mode needs the next upcoming event even while
  // another event is active, which the HA entity state alone can't provide. A
  // periodic calendar.get_events poll fills these.
  uint32_t refresh_ticks = 0;        // counts refresh ticks to schedule polls
  bool next_valid = false;           // a polled next-upcoming event is known
  time_t next_start_epoch = 0;       // committed next event start (after now)
  std::string next_title;            // committed next event title
  bool poll_active_valid = false;    // poll found a currently-active event
  time_t poll_active_start = 0;     // correct start epoch for active event
  time_t poll_active_end = 0;       // correct end epoch for active event
  std::string poll_active_title;
  int fetch_pending = 0;             // entity responses still outstanding
  bool fetch_done = false;           // a poll has completed at least once
  time_t fetch_best_start = 0;       // candidate during an in-flight poll
  std::string fetch_best_title;
  time_t fetch_active_start = 0;    // candidate active event start (in-flight)
  time_t fetch_active_end = 0;      // candidate active event end (in-flight)
  std::string fetch_active_title;
};

struct HaCalendarModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *list = nullptr;
  lv_obj_t *status_lbl = nullptr;
  HaCalendarCardCtx *active = nullptr;
  HaCalendarEventRow events[HA_CALENDAR_MAX_EVENTS];
  int event_count = 0;
  int pending_entity_count = 0;  // decrements as responses arrive; render when 0
  uint32_t started_ms = 0;
  bool waiting_for_ha = false;
  lv_timer_t *refresh_timer = nullptr;  // re-renders countdowns while the modal is open
};

inline HaCalendarModalUi &ha_calendar_modal_ui() {
  static HaCalendarModalUi ui;
  return ui;
}

inline bool ha_calendar_ctx_valid(HaCalendarCardCtx *ctx) {
  return ctx != nullptr && ctx->magic == HA_CALENDAR_CTX_MAGIC;
}

inline bool ha_calendar_ctx_current(HaCalendarCardCtx *ctx, uint32_t generation) {
  return ha_calendar_ctx_valid(ctx) &&
         generation == ha_subscription_generation() &&
         ctx->btn != nullptr &&
         lv_obj_get_user_data(ctx->btn) == ctx;
}

inline bool ha_calendar_modal_heap_available(const char *stage) {
  return ha_internal_heap_available(
    stage ? stage : "calendar modal",
    HA_CALENDAR_MODAL_INTERNAL_FREE_MIN_BYTES,
    HA_CALENDAR_MODAL_INTERNAL_LARGEST_MIN_BYTES);
}

// ── Card face helpers ────────────────────────────────────────────────────────

inline std::string ha_calendar_card_label(HaCalendarCardCtx *ctx) {
  if (!ctx) return "Calendar";
  if (!ctx->configured_label.empty()) return ctx->configured_label;
  return "Calendar";
}

inline bool ha_calendar_find_best_entity(HaCalendarCardCtx *ctx,
                                          const HaCalendarEntityState **best_out,
                                          time_t now) {
  if (!ctx || now <= 0) return false;
  const HaCalendarEntityState *best = nullptr;
  for (const auto &e : ctx->entities) {
    if (!e.available || e.start_epoch == 0) continue;
    if (best == nullptr) { best = &e; continue; }
    if (e.active && !best->active) { best = &e; continue; }
    if (!e.active && best->active) continue;
    if (e.start_epoch < best->start_epoch) best = &e;
  }
  *best_out = best;
  return best != nullptr;
}

inline void ha_calendar_format_countdown(int64_t secs,
                                          char *num_buf, size_t num_len,
                                          char *unit_buf, size_t unit_len) {
  if (secs < 60) {
    std::snprintf(num_buf, num_len, "1");
    std::strncpy(unit_buf, "min", unit_len - 1);
  } else if (secs < 3600) {
    std::snprintf(num_buf, num_len, "%d", (int)(secs / 60));
    std::strncpy(unit_buf, "min", unit_len - 1);
  } else if (secs < 48LL * 3600) {
    std::snprintf(num_buf, num_len, "%d", (int)(secs / 3600));
    std::strncpy(unit_buf, "hr", unit_len - 1);
  } else {
    std::snprintf(num_buf, num_len, "%d", (int)(secs / 86400));
    std::strncpy(unit_buf, "days", unit_len - 1);
  }
  num_buf[num_len - 1] = '\0';
  unit_buf[unit_len - 1] = '\0';
}

inline void ha_calendar_compact_countdown_text(int64_t secs, char *buf, size_t len) {
  if (!buf || len == 0) return;
  if (secs < 60) {
    std::snprintf(buf, len, "1m");
  } else if (secs < 3600) {
    std::snprintf(buf, len, "%dm", (int)(secs / 60));
  } else if (secs < 48LL * 3600) {
    int hrs = (int)(secs / 3600);
    int mins = (int)((secs % 3600) / 60);
    if (mins > 0) std::snprintf(buf, len, "%dh %dm", hrs, mins);
    else std::snprintf(buf, len, "%dh", hrs);
  } else {
    std::snprintf(buf, len, "%dd", (int)(secs / 86400));
  }
  buf[len - 1] = '\0';
}

inline void ha_calendar_remaining_text(time_t end_epoch, time_t now,
                                       char *buf, size_t len) {
  if (!buf || len == 0) return;
  if (end_epoch <= now) {
    std::snprintf(buf, len, "%s", espcontrol_i18n("Now"));
    buf[len - 1] = '\0';
    return;
  }
  char compact[20] = {};
  ha_calendar_compact_countdown_text((int64_t)(end_epoch - now), compact, sizeof(compact));
  std::snprintf(buf, len, "%s %s", compact, espcontrol_i18n_key("left"));
  buf[len - 1] = '\0';
}

// Left margin to pull a tiny "h"/"m" tight against the number it follows. A
// number ending in "1" leaves extra whitespace, so it gets a stronger pull.
inline int ha_calendar_unit_gap(const char *num) {
  size_t n = num ? std::strlen(num) : 0;
  bool ends_in_one = (n > 0 && num[n - 1] == '1');
  return ends_in_one ? HA_CALENDAR_HM_UNIT_GAP_ONE : HA_CALENDAR_HM_UNIT_GAP;
}

// Populate the value flex-row for a countdown. For the 1h–48h range it renders
// the dual "1h 51m" format across the four row labels ([big h][tiny "h"]
// [big m][tiny "m"]); every other range uses a single big number + unit and the
// two extra labels stay hidden. Colors are left to the caller.
inline void ha_calendar_set_countdown_labels(HaCalendarCardCtx *ctx, int64_t secs) {
  if (ctx->value2_lbl) lv_obj_add_flag(ctx->value2_lbl, LV_OBJ_FLAG_HIDDEN);
  if (ctx->unit2_lbl) lv_obj_add_flag(ctx->unit2_lbl, LV_OBJ_FLAG_HIDDEN);

  if (secs >= 3600 && secs < 48LL * 3600 && ctx->value2_lbl && ctx->unit2_lbl) {
    int hrs = (int)(secs / 3600);
    int mins = (int)((secs % 3600) / 60);
    char hbuf[8] = {};
    std::snprintf(hbuf, sizeof(hbuf), "%d", hrs);
    lv_obj_clear_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ctx->value_lbl, hbuf);
    if (ctx->unit_lbl) {
      lv_label_set_text(ctx->unit_lbl, "h");
      if (ctx->unit_font) lv_obj_set_style_text_font(ctx->unit_lbl, ctx->unit_font, LV_PART_MAIN);
      // Pull the "h" tight against the hours digit (cancel its glyph side bearing).
      lv_obj_set_style_margin_left(ctx->unit_lbl, ha_calendar_unit_gap(hbuf), LV_PART_MAIN);
    }
    // Omit the minutes group on a clean hour boundary ("2h" rather than "2h 0m").
    if (mins > 0) {
      char mbuf[8] = {};
      std::snprintf(mbuf, sizeof(mbuf), "%d", mins);
      lv_obj_clear_flag(ctx->value2_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ctx->value2_lbl, mbuf);
      lv_obj_set_style_margin_left(ctx->value2_lbl, HA_CALENDAR_HM_GROUP_GAP, LV_PART_MAIN);
      lv_obj_clear_flag(ctx->unit2_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ctx->unit2_lbl, "m");
      lv_obj_set_style_margin_left(ctx->unit2_lbl, ha_calendar_unit_gap(mbuf), LV_PART_MAIN);
    }
    return;
  }

  char num_buf[16] = {}, unit_buf[8] = {};
  ha_calendar_format_countdown(secs, num_buf, sizeof(num_buf), unit_buf, sizeof(unit_buf));
  lv_obj_clear_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(ctx->value_lbl, num_buf);
  if (ctx->unit_lbl) {
    lv_label_set_text(ctx->unit_lbl, unit_buf);
    lv_obj_set_style_margin_left(ctx->unit_lbl, 0, LV_PART_MAIN);  // single mode: normal spacing
  }
}

// Set the bottom event title, keeping the same label font as other card text.
inline void ha_calendar_set_title(HaCalendarCardCtx *ctx, const char *text) {
  if (!ctx->label_lbl) return;
  const lv_font_t *font = ctx->label_font;
  if (font) lv_obj_set_style_text_font(ctx->label_lbl, font, LV_PART_MAIN);
  lv_coord_t lh = (font && font->line_height > 0) ? font->line_height : 16;
  lv_label_set_long_mode(ctx->label_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(ctx->label_lbl, lv_pct(100));
  lv_obj_set_height(ctx->label_lbl, LV_SIZE_CONTENT);
  lv_label_set_text(ctx->label_lbl, text ? text : "");
  lv_obj_update_layout(ctx->label_lbl);
  lv_coord_t title_h = lv_obj_get_height(ctx->label_lbl);
  if (title_h <= 0) title_h = lh;
  if (title_h > lh * 2) title_h = lh * 2;
  lv_obj_set_height(ctx->label_lbl, title_h);
  lv_obj_align(ctx->label_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

inline lv_coord_t ha_calendar_event_card_pad(HaCalendarCardCtx *ctx) {
  if (!ctx || !ctx->btn) return 0;
  return 0;
}

inline lv_coord_t ha_calendar_event_status_pad(HaCalendarCardCtx *ctx) {
  return ha_calendar_event_card_pad(ctx);
}

inline lv_coord_t ha_calendar_event_text_width(HaCalendarCardCtx *ctx,
                                               lv_coord_t pad) {
  if (!ctx || !ctx->btn) return lv_pct(100);
  lv_obj_update_layout(ctx->btn);
  lv_coord_t width = lv_obj_get_width(ctx->btn) - pad * 2;
  return width > 1 ? width : lv_pct(100);
}

inline void ha_calendar_set_event_title(HaCalendarCardCtx *ctx, const char *text) {
  if (!ctx || !ctx->label_lbl) return;
  lv_coord_t pad = ha_calendar_event_card_pad(ctx);
  const lv_font_t *font = ctx->list_font ? ctx->list_font : ctx->label_font;
  if (font) lv_obj_set_style_text_font(ctx->label_lbl, font, LV_PART_MAIN);
  lv_obj_set_style_text_line_space(ctx->label_lbl, -1, LV_PART_MAIN);
  lv_obj_set_style_text_align(ctx->label_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_label_set_long_mode(ctx->label_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_size(ctx->label_lbl, ha_calendar_event_text_width(ctx, pad),
                  font && font->line_height > 0 ? font->line_height * 2 - 1 : LV_SIZE_CONTENT);
  lv_label_set_text(ctx->label_lbl, text ? text : "");
  lv_obj_align(ctx->label_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_move_foreground(ctx->label_lbl);
}

inline void ha_calendar_set_event_status(HaCalendarCardCtx *ctx, const char *text) {
  if (!ctx || !ctx->status_lbl) return;
  lv_coord_t pad = ha_calendar_event_status_pad(ctx);
  const lv_font_t *font = ctx->label_font;
  if (font) lv_obj_set_style_text_font(ctx->status_lbl, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(ctx->status_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_label_set_long_mode(ctx->status_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(ctx->status_lbl, ha_calendar_event_text_width(ctx, pad));
  lv_label_set_text(ctx->status_lbl, text ? text : "");
  if (text && text[0]) lv_obj_clear_flag(ctx->status_lbl, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(ctx->status_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align(ctx->status_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  lv_obj_move_foreground(ctx->status_lbl);
}

inline void ha_calendar_update_current_progress(HaCalendarCardCtx *ctx,
                                                time_t start_epoch,
                                                time_t end_epoch,
                                                time_t now) {
  if (!ctx || !ctx->progress_slider) return;
  int pct = 0;
  if (start_epoch > 0 && end_epoch > start_epoch && now > start_epoch) {
    int64_t total = (int64_t)(end_epoch - start_epoch);
    int64_t elapsed = (int64_t)(now - start_epoch);
    if (elapsed < 0) elapsed = 0;
    if (elapsed > total) elapsed = total;
    pct = (int)((elapsed * 100 + total / 2) / total);
  }
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  lv_slider_set_value(ctx->progress_slider, pct, LV_ANIM_OFF);
  SliderCtx *slider_ctx = (SliderCtx *)lv_obj_get_user_data(ctx->progress_slider);
  slider_update_ctx_fill(slider_ctx, ctx->btn, pct);
}

inline void ha_calendar_show_status_icon(HaCalendarCardCtx *ctx, const char *icon_name) {
  if (!ctx || !ctx->icon_lbl) return;
  lv_coord_t pad = ha_calendar_event_card_pad(ctx);
  lv_obj_clear_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(ctx->icon_lbl, find_icon(icon_name));
  if (ctx->icon_font) lv_obj_set_style_text_font(ctx->icon_lbl, ctx->icon_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(ctx->icon_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_align(ctx->icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_move_foreground(ctx->icon_lbl);
}

inline void ha_calendar_show_no_events(HaCalendarCardCtx *ctx) {
  if (!ctx) return;
  lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
  ha_calendar_show_status_icon(ctx, "Calendar Month");
  ha_calendar_set_event_title(ctx, "");
  ha_calendar_set_event_status(ctx, espcontrol_i18n("No events"));
}

inline void ha_calendar_apply_card_face(HaCalendarCardCtx *ctx) {
  if (!ha_calendar_ctx_valid(ctx)) return;
  if (!ctx->value_lbl || !ctx->label_lbl) return;

  time_t now = std::time(nullptr);

  bool any_available = false;
  for (const auto &e : ctx->entities) {
    if (e.available) { any_available = true; break; }
  }
  ctx->available = any_available;
  apply_control_availability(ctx->btn, ctx->btn, ctx->available, false);

  lv_label_set_text(ctx->label_lbl, ha_calendar_card_label(ctx).c_str());

  auto set_bg = [&](uint32_t color) {
    lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(color),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ctx->btn, LV_GRAD_DIR_NONE,
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
  };

  // Reset label styling to defaults (white text, original unit font); specific
  // Current-mode phases override these below.
  if (ctx->unit_lbl) {
    if (ctx->unit_font) lv_obj_set_style_text_font(ctx->unit_lbl, ctx->unit_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
  lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_clear_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);  // shown by default; phase words hide it
  // Dual "1h 51m" extras stay hidden unless the countdown helper turns them on.
  if (ctx->value2_lbl) lv_obj_add_flag(ctx->value2_lbl, LV_OBJ_FLAG_HIDDEN);
  if (ctx->unit2_lbl) lv_obj_add_flag(ctx->unit2_lbl, LV_OBJ_FLAG_HIDDEN);
  if (ctx->icon_lbl) lv_obj_add_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);  // only shown for "Done"/"Free"
  if (ctx->status_lbl) lv_obj_add_flag(ctx->status_lbl, LV_OBJ_FLAG_HIDDEN);
  ha_calendar_update_current_progress(ctx, 0, 0, now);
  if (ctx->label_lbl) {
    lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    // Default to left-aligned titles; the centered "Free" state overrides this.
    lv_obj_set_style_text_align(ctx->label_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  }

  if (!any_available || now <= 0) {
    lv_label_set_text(ctx->value_lbl, "--");
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    set_bg(ctx->off_color);
    return;
  }

  const HaCalendarEntityState *best = nullptr;
  if (!ha_calendar_find_best_entity(ctx, &best, now) || best == nullptr) {
    lv_label_set_text(ctx->value_lbl, "");
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "--");
    set_bg(ctx->off_color);
    return;
  }

  bool is_current_mode = (ctx->display_mode == "current");
  bool is_next_event_mode = (ctx->display_mode == "next_event");
  time_t event_start = best->start_epoch;
  time_t event_end = best->end_epoch;
  bool event_active = best->active || (event_end > 0 && event_start <= now && event_end > now);

  if (is_current_mode) {
    if (!event_active) {
      set_bg(ctx->off_color);
      ha_calendar_update_current_progress(ctx, 0, 0, now);
      ha_calendar_show_no_events(ctx);
      return;
    }
    // Active meeting: keep the card background dim, show the status icon, and
    // pin the event title to the bottom of the card.
    //
    // Entity-attribute times arrive as local strings without TZ offset, so on a
    // UTC device they parse hours early. Prefer poll-supplied epochs (correct
    // UTC values from get_events) when available; fall back to an active face
    // with no fill until the first poll completes if entity epochs are stale.
    if (ctx->poll_active_valid) {
      event_start = ctx->poll_active_start;
      event_end   = ctx->poll_active_end;
    } else if (event_end <= now) {
      ha_calendar_set_event_title(ctx, best->title.c_str());
      char remaining[32] = {};
      ha_calendar_remaining_text(event_end, now, remaining, sizeof(remaining));
      ha_calendar_set_event_status(ctx, remaining);
      lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
      if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
      if (ctx->label_lbl) lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
      set_bg(ctx->off_color);
      return;
    }
    set_bg(ctx->off_color);
    ha_calendar_update_current_progress(ctx, event_start, event_end, now);
    ha_calendar_set_event_title(ctx, best->title.c_str());
    char remaining[32] = {};
    ha_calendar_remaining_text(event_end, now, remaining, sizeof(remaining));
    ha_calendar_set_event_status(ctx, remaining);
    lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->unit_lbl) lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->label_lbl) lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->label_lbl) lv_obj_move_foreground(ctx->label_lbl);
    return;
  }

  if (is_next_event_mode) {
    const char *title = nullptr;
    time_t next_start = 0;
    bool show_now = false;
    if (ctx->poll_active_valid && ctx->next_now_secs > 0 &&
        now >= ctx->poll_active_start &&
        now < ctx->poll_active_start + ctx->next_now_secs &&
        !ctx->poll_active_title.empty()) {
      title = ctx->poll_active_title.c_str();
      show_now = true;
    } else if (ctx->next_valid && ctx->next_start_epoch > now && !ctx->next_title.empty()) {
      title = ctx->next_title.c_str();
      next_start = ctx->next_start_epoch;
    } else if (!event_active && event_start > now && !best->title.empty()) {
      title = best->title.c_str();
      next_start = event_start;
    }

    set_bg(ctx->off_color);
    if (!title && next_start <= now && !show_now) {
      ha_calendar_show_no_events(ctx);
      return;
    }
    ha_calendar_set_event_title(ctx, title ? title : ha_calendar_card_label(ctx).c_str());
    char status[32] = {};
    if (show_now) {
      std::snprintf(status, sizeof(status), "%s", espcontrol_i18n("Now"));
    } else if (next_start > now) {
      char compact[20] = {};
      ha_calendar_compact_countdown_text((int64_t)(next_start - now), compact, sizeof(compact));
      std::snprintf(status, sizeof(status), "%s %s", espcontrol_i18n("In"), compact);
    }
    ha_calendar_set_event_status(ctx, status);
    lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->unit_lbl) lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->label_lbl) lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->label_lbl) lv_obj_move_foreground(ctx->label_lbl);
    return;
  }

  // Next mode (default): prefer the polled next-upcoming event so the tile can
  // count down to the next event even while another one is currently active
  // (the HA entity state alone only exposes the active event in that case).
  // Show a countdown (number + unit) to `title`, optionally escalating to the
  // accent card when the event is inside the user-configured warning window.
  auto show_countdown = [&](int64_t secs_until, const char *title) {
    ha_calendar_set_countdown_labels(ctx, secs_until);
    ha_calendar_set_title(ctx, title);
    if (ctx->urgent_color_enabled && secs_until <= ctx->urgent_secs) {
      lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
      if (ctx->unit_lbl) lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
      if (ctx->label_lbl) lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
      set_bg(ctx->accent_color);
    } else {
      set_bg(ctx->off_color);
    }
  };

  // 1 & 2: a polled upcoming event today → count down to it (even while another
  // meeting is currently active).
  if (ctx->next_valid && ctx->next_start_epoch > now) {
    show_countdown((int64_t)(ctx->next_start_epoch - now), ctx->next_title.c_str());
    return;
  }

  // 3: nothing upcoming today, but a meeting is active now → "Now" using the
  // same value/title layout and colours as the other default card states.
  if (event_active) {
    lv_obj_clear_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ctx->value_lbl, espcontrol_i18n("Now"));
    if (ctx->unit_lbl) {
      lv_label_set_text(ctx->unit_lbl, "");
      lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    }
    ha_calendar_set_title(ctx, best->title.c_str());
    lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->label_lbl)
      lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    set_bg(ctx->off_color);
    return;
  }

  // 4: poll completed and there are no more events today → "Done for the day".
  if (ctx->fetch_done && !ctx->next_valid) {
    lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    if (ctx->icon_lbl) {
      lv_obj_clear_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ctx->icon_lbl, find_icon("Sofa"));
      lv_obj_set_style_text_color(ctx->icon_lbl, lv_color_hex(ctx->accent_color), LV_PART_MAIN);
      lv_obj_align(ctx->icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    }
    ha_calendar_set_title(ctx, espcontrol_i18n("Done for the day"));
    set_bg(ctx->off_color);
    return;
  }

  // Fallback before the first poll completes: count down to the entity's next
  // event (which may be tomorrow).
  show_countdown((event_start > now) ? (int64_t)(event_start - now) : 0, best->title.c_str());
}

// Defined below (after the get_events request machinery).
inline void ha_calendar_card_fetch(HaCalendarCardCtx *ctx);

// Periodic tick so the countdown advances even when the HA entity state is
// unchanged (calendar entities only push updates when an event starts/ends).
// Also drives the background get_events poll that feeds Next mode.
inline void ha_calendar_refresh_timer_cb(lv_timer_t *timer) {
  HaCalendarCardCtx *ctx = static_cast<HaCalendarCardCtx *>(lv_timer_get_user_data(timer));
  if (!ha_calendar_ctx_valid(ctx)) { lv_timer_del(timer); return; }
  // If the button was reconfigured (its user_data now points to a different
  // context), this timer is stale — stop it so it can't write to a reused widget.
  if (ctx->btn && lv_obj_get_user_data(ctx->btn) != ctx) { lv_timer_del(timer); return; }
  ctx->refresh_ticks++;
  // Poll calendar.get_events shortly after startup and then every ~5 minutes.
  // Both modes need this: Next mode for the upcoming event, Current mode for
  // correct UTC epochs (entity-attribute times have no TZ offset and parse
  // incorrectly on non-local-timezone devices).
  if (ctx->refresh_ticks == 1 || ctx->refresh_ticks % 10 == 0) {
    ha_calendar_card_fetch(ctx);
  }
  ha_calendar_apply_card_face(ctx);
}

// ── Setup and context creation ──────────────────────────────────────────────

inline void setup_ha_calendar_card(BtnSlot &s, const ParsedCfg &p, uint32_t secondary_color) {
  lv_obj_set_style_bg_color(s.btn, lv_color_hex(secondary_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Calendar" : p.label.c_str());
  apply_push_button_transition(s.btn);
}

inline HaCalendarCardCtx *create_ha_calendar_card_context(
    BtnSlot &s,
    const ParsedCfg &p,
    uint32_t accent_color,
    uint32_t off_color,
    const lv_font_t *value_font,
    const lv_font_t *label_font,
    const lv_font_t *list_font,
    const lv_font_t *icon_font,
    int width_compensation_percent) {
  HaCalendarCardCtx *ctx = new HaCalendarCardCtx();
  ctx->configured_label = p.label;
  ctx->display_mode = (p.precision == "current" || p.precision == "next_event")
    ? p.precision
    : cfg_option_value(p.options, "display_mode");
  if (ctx->display_mode != "current" && ctx->display_mode != "next_event") ctx->display_mode = "current";
  ESP_LOGD("ha_calendar", "setup entities='%s' options='%s' -> display_mode='%s'",
           p.entity.c_str(), p.options.c_str(), ctx->display_mode.c_str());
  ctx->accent_color = accent_color;
  ctx->off_color = off_color;
  ctx->urgent_color_enabled = cfg_option_token_present(p.options, "urgent_color");
  ctx->current_progress_enabled =
    ctx->display_mode == "current" && cfg_option_token_present(p.options, "current_progress");
  ctx->urgent_secs = normalize_ha_calendar_urgent_minutes(
    cfg_option_value(p.options, "urgent_minutes")) * 60;
  ctx->next_now_secs = normalize_ha_calendar_next_now_minutes(
    cfg_option_value(p.options, "next_now_minutes")) * 60;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;  // used for the "Done for the day" glyph
  ctx->value_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->unit_font = s.unit_lbl ? lv_obj_get_style_text_font(s.unit_lbl, LV_PART_MAIN) : nullptr;

  ctx->status_lbl = lv_label_create(s.btn);
  if (ctx->status_lbl) {
    if (label_font) lv_obj_set_style_text_font(ctx->status_lbl, label_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->status_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_long_mode(ctx->status_lbl, LV_LABEL_LONG_DOT);
    lv_label_set_text(ctx->status_lbl, "");
    lv_obj_add_flag(ctx->status_lbl, LV_OBJ_FLAG_HIDDEN);
  }

  // Two extra labels appended to the value flex-row so the "1h 51m" dual format
  // renders as [big "1"][tiny "h"][big "51"][tiny "m"]. Hidden unless that format
  // is active. The minutes group gets a small left margin so "1h" and "51m" hug
  // their own units but keep a gap between the two groups.
  if (s.sensor_container) {
    ctx->value2_lbl = lv_label_create(s.sensor_container);
    if (value_font) lv_obj_set_style_text_font(ctx->value2_lbl, value_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->value2_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_margin_left(ctx->value2_lbl, HA_CALENDAR_HM_GROUP_GAP, LV_PART_MAIN);
    lv_label_set_text(ctx->value2_lbl, "");
    lv_obj_add_flag(ctx->value2_lbl, LV_OBJ_FLAG_HIDDEN);

    ctx->unit2_lbl = lv_label_create(s.sensor_container);
    if (ctx->unit_font) lv_obj_set_style_text_font(ctx->unit2_lbl, ctx->unit_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->unit2_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(ctx->unit2_lbl, 6, LV_PART_MAIN);
    lv_label_set_text(ctx->unit2_lbl, "");
    lv_obj_add_flag(ctx->unit2_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  ctx->value_font = value_font;
  ctx->label_font = label_font;
  ctx->list_font = list_font ? list_font : label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;

  if (ctx->current_progress_enabled) {
    ctx->progress_slider = setup_slider_widget(s.btn, accent_color, true);
    if (ctx->progress_slider) {
      SliderCtx *slider_ctx = new SliderCtx();
      slider_ctx->fill = lv_obj_get_child(s.btn, 0);
      slider_ctx->horizontal = true;
      slider_ctx->inverted = false;
      slider_ctx->radius = lv_obj_get_style_radius(s.btn, LV_PART_MAIN);
      slider_ctx->interactive = false;
      lv_obj_set_user_data(ctx->progress_slider, (void *)slider_ctx);
      lv_obj_clear_flag(ctx->progress_slider, LV_OBJ_FLAG_CLICKABLE);
      slider_bind_geometry_refresh(s.btn, ctx->progress_slider);
      ha_calendar_update_current_progress(ctx, 0, 0, std::time(nullptr));
      lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
      apply_push_button_transition(s.btn);
    }
  }

  lv_obj_set_user_data(s.btn, ctx);

  // Parse comma-delimited entity list from p.entity
  const std::string &raw = p.entity;
  size_t start = 0;
  while (start <= raw.size()) {
    size_t comma = raw.find(',', start);
    if (comma == std::string::npos) comma = raw.size();
    std::string eid = raw.substr(start, comma - start);
    size_t s_start = eid.find_first_not_of(' ');
    if (s_start != std::string::npos) {
      size_t s_end = eid.find_last_not_of(' ');
      eid = eid.substr(s_start, s_end - s_start + 1);
    } else {
      eid.clear();
    }
    if (!eid.empty()) {
      HaCalendarEntityState estate;
      estate.entity_id = eid;
      ctx->entities.push_back(std::move(estate));
    }
    if (comma >= raw.size()) break;
    start = comma + 1;
  }

  // Clamp long event titles to two lines with an ellipsis so they can't grow
  // upward over the countdown value (matches the spec's 2-line title rule).
  if (ctx->label_lbl) {
    lv_label_set_long_mode(ctx->label_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(ctx->label_lbl, lv_pct(100));
    const lv_font_t *lf = ctx->label_font
      ? ctx->label_font
      : lv_obj_get_style_text_font(ctx->label_lbl, LV_PART_MAIN);
    lv_coord_t lh = (lf && lf->line_height > 0) ? lf->line_height : 16;
    lv_obj_set_height(ctx->label_lbl, lh * 2);
    lv_obj_align(ctx->label_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  }

  // Re-render every 30s so the countdown advances without an HA state change.
  ctx->refresh_timer = lv_timer_create(ha_calendar_refresh_timer_cb, 30000, ctx);

  return ctx;
}

// HA calendar entity attributes (start_time/end_time) are naive *local* strings
// like "2026-06-04 10:30:00" with no timezone offset. media_parse_ha_timestamp
// treats an offset-less timestamp as UTC, which skews the countdown by the local
// UTC offset. Interpret naive timestamps in the device's local timezone here;
// defer to the offset-aware parser when an explicit offset/Z is present.
inline bool ha_calendar_parse_local_timestamp(esphome::StringRef value, time_t &epoch) {
  std::string text = string_ref_limited(value, 40);
  const char *s = text.c_str();
  size_t len = text.size();
  if (len < 19 || s[4] != '-' || s[7] != '-' || (s[10] != 'T' && s[10] != ' ')) return false;
  size_t tz_pos = 19;
  while (tz_pos < len && s[tz_pos] != 'Z' && s[tz_pos] != '+' && s[tz_pos] != '-') tz_pos++;
  if (tz_pos < len) return media_parse_ha_timestamp(value, epoch);  // has offset → UTC math

  int year, month, day, hour, minute, second;
  if (!media_parse_fixed_int(s, len, 0, 4, year) ||
      !media_parse_fixed_int(s, len, 5, 2, month) ||
      !media_parse_fixed_int(s, len, 8, 2, day) ||
      !media_parse_fixed_int(s, len, 11, 2, hour) ||
      !media_parse_fixed_int(s, len, 14, 2, minute) ||
      !media_parse_fixed_int(s, len, 17, 2, second)) {
    return false;
  }
  struct tm tmv = {};
  tmv.tm_year = year - 1900;
  tmv.tm_mon = month - 1;
  tmv.tm_mday = day;
  tmv.tm_hour = hour;
  tmv.tm_min = minute;
  tmv.tm_sec = second;
  tmv.tm_isdst = -1;  // let mktime resolve DST for the local zone
  time_t e = std::mktime(&tmv);
  if (e == static_cast<time_t>(-1)) return false;
  epoch = e;
  return true;
}

// ── HA subscriptions ────────────────────────────────────────────────────────

inline void subscribe_ha_calendar_state(HaCalendarCardCtx *ctx) {
  if (!ha_calendar_ctx_valid(ctx)) return;
  register_ha_control_availability(ctx->btn, ctx->btn, false);
  for (size_t i = 0; i < ctx->entities.size(); i++) {
    if (ctx->entities[i].entity_id.empty()) continue;
    ha_subscribe_state(ctx->entities[i].entity_id,
      std::function<void(esphome::StringRef)>([ctx, i](esphome::StringRef state) {
        bool unavail = ha_state_unavailable_ref(state);
        ctx->entities[i].available = !unavail;
        ctx->entities[i].active = !unavail && (state == "on");
        bool any = false;
        for (const auto &e : ctx->entities) if (e.available) { any = true; break; }
        apply_control_availability(ctx->btn, ctx->btn, any, false);
        // Next mode: re-poll get_events whenever the state changes — both on the
        // first state after boot and at every event boundary (an event starting
        // or ending flips this state). Otherwise the tile lingers on "Now" with
        // a just-started/just-ended event until the next periodic poll.
        if (ctx->fetch_pending <= 0 && ha_api_state_connected()) {
          ha_calendar_card_fetch(ctx);
        }
        ha_calendar_apply_card_face(ctx);
      }));
  }
}

inline void subscribe_ha_calendar_attributes(HaCalendarCardCtx *ctx) {
  if (!ha_calendar_ctx_valid(ctx)) return;
  for (size_t i = 0; i < ctx->entities.size(); i++) {
    if (ctx->entities[i].entity_id.empty()) continue;
    const std::string &eid = ctx->entities[i].entity_id;

    ha_subscribe_attribute(eid, std::string("message"),
      std::function<void(esphome::StringRef)>([ctx, i](esphome::StringRef attr) {
        ctx->entities[i].title = string_ref_limited(attr, HA_CALENDAR_TITLE_MAX_LEN);
        ha_calendar_apply_card_face(ctx);
      }));

    ha_subscribe_attribute(eid, std::string("start_time"),
      std::function<void(esphome::StringRef)>([ctx, i](esphome::StringRef attr) {
        ha_calendar_parse_local_timestamp(attr, ctx->entities[i].start_epoch);
        ha_calendar_apply_card_face(ctx);
      }));

    ha_subscribe_attribute(eid, std::string("end_time"),
      std::function<void(esphome::StringRef)>([ctx, i](esphome::StringRef attr) {
        ha_calendar_parse_local_timestamp(attr, ctx->entities[i].end_epoch);
        ha_calendar_apply_card_face(ctx);
      }));
  }
}

// ── Modal: status helpers ────────────────────────────────────────────────────

inline void ha_calendar_modal_set_status(const char *text) {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  bool wants = text && text[0];
  if (!wants && !ui.status_lbl) return;
  if (!ui.status_lbl && ui.list) {
    ui.status_lbl = lv_label_create(ui.list);
    if (!ui.status_lbl) return;
    lv_label_set_long_mode(ui.status_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui.status_lbl, lv_pct(100));
    lv_obj_set_style_text_color(ui.status_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.status_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ui.active && ui.active->label_font)
      lv_obj_set_style_text_font(ui.status_lbl, ui.active->label_font, LV_PART_MAIN);
  }
  if (!ui.status_lbl) return;
  lv_label_set_text(ui.status_lbl, text ? text : "");
  if (wants) lv_obj_clear_flag(ui.status_lbl, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(ui.status_lbl, LV_OBJ_FLAG_HIDDEN);
}

inline void ha_calendar_modal_hide() {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  if (ui.refresh_timer) { lv_timer_del(ui.refresh_timer); ui.refresh_timer = nullptr; }
  ui.active = nullptr;
  control_modal_delete_overlay(ControlModalKind::HA_CALENDAR, ui.overlay);
  ui = HaCalendarModalUi();
}

// ── Modal: event parsing ─────────────────────────────────────────────────────

// Parse "start_epoch|end_epoch|title\n" lines returned by calendar.get_events.
// Device 12h/24h clock format. The time tick (time.yaml) publishes the device's
// "Screen: Clock Format" here so the modal can format event times to match.
inline bool &ha_calendar_use_12h_flag() {
  static bool v = false;
  return v;
}
inline void ha_calendar_set_use_12h(bool v) { ha_calendar_use_12h_flag() = v; }

// Format "H:MM" honoring the device clock format (24h: 14:00 / 12h: 2:00).
inline void ha_calendar_format_hm(char *buf, size_t len, int hour, int minute) {
  if (ha_calendar_use_12h_flag()) {
    int h12 = hour % 12;
    if (h12 == 0) h12 = 12;
    std::snprintf(buf, len, "%d:%02d", h12, minute);
  } else {
    std::snprintf(buf, len, "%d:%02d", hour, minute);
  }
}

inline void ha_calendar_parse_and_merge(const char *payload) {
  if (!payload || !payload[0]) return;
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  const char *p = payload;
  while (*p) {
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    const char *line_end = p;
    while (*line_end && *line_end != '\n' && *line_end != '\r') line_end++;
    size_t line_len = static_cast<size_t>(line_end - p);
    if (line_len < 3) { p = line_end; continue; }

    const char *sep1 = static_cast<const char *>(std::memchr(p, '|', line_len));
    if (!sep1) { p = line_end; continue; }
    const char *sep2 = static_cast<const char *>(
      std::memchr(sep1 + 1, '|', line_len - static_cast<size_t>(sep1 + 1 - p)));
    if (!sep2) { p = line_end; continue; }

    long long start_sec = 0, end_sec = 0;
    if (std::sscanf(p, "%lld", &start_sec) != 1 ||
        std::sscanf(sep1 + 1, "%lld", &end_sec) != 1) {
      p = line_end; continue;
    }
    if (ui.event_count >= HA_CALENDAR_MAX_EVENTS) break;

    HaCalendarEventRow &row = ui.events[ui.event_count++];
    row.start_epoch = static_cast<time_t>(start_sec);
    row.end_epoch   = static_cast<time_t>(end_sec);

    // Format local display times from epoch
    struct tm *lt = std::localtime(&row.start_epoch);
    if (lt) ha_calendar_format_hm(row.start_display, sizeof(row.start_display), lt->tm_hour, lt->tm_min);
    lt = std::localtime(&row.end_epoch);
    if (lt) ha_calendar_format_hm(row.end_display, sizeof(row.end_display), lt->tm_hour, lt->tm_min);

    size_t title_len = static_cast<size_t>(line_end - (sep2 + 1));
    if (title_len > HA_CALENDAR_TITLE_MAX_LEN) title_len = HA_CALENDAR_TITLE_MAX_LEN;
    std::strncpy(row.title, sep2 + 1, title_len);
    row.title[title_len] = '\0';

    p = line_end;
  }
}

// Insertion sort by start_epoch (small N).
inline void ha_calendar_sort_events() {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  for (int i = 1; i < ui.event_count; i++) {
    HaCalendarEventRow key = ui.events[i];
    int j = i - 1;
    while (j >= 0 && ui.events[j].start_epoch > key.start_epoch) {
      ui.events[j + 1] = ui.events[j];
      j--;
    }
    ui.events[j + 1] = key;
  }
}

// ── Modal: rendering ─────────────────────────────────────────────────────────

inline void ha_calendar_render_compact(HaCalendarCardCtx *ctx, lv_coord_t content_w) {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  if (!ui.list) return;
  lv_obj_clean(ui.list);
  ui.status_lbl = nullptr;
  time_t now = std::time(nullptr);

  for (int i = 0; i < ui.event_count; i++) {
    HaCalendarEventRow &ev = ui.events[i];
    bool is_active = ev.start_epoch <= now && ev.end_epoch > now;

    uint32_t row_text_col = is_active ? DARK_TEXT_PRIMARY : DARK_TEXT_MUTED;

    lv_obj_t *row = lv_obj_create(ui.list);
    if (!row) {
      ESP_LOGW("ha_calendar", "Unable to create calendar modal row");
      ha_calendar_modal_set_status("Not enough memory");
      return;
    }
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row, 6, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);

    const lv_font_t *list_f = ctx->list_font ? ctx->list_font : ctx->label_font;
    const lv_font_t *meta_f = ctx->label_font ? ctx->label_font : list_f;

    // Line 1: meeting name on its own, full width.
    lv_obj_t *title_lbl = lv_label_create(row);
    if (!title_lbl) {
      ESP_LOGW("ha_calendar", "Unable to create calendar modal title");
      ha_calendar_modal_set_status("Not enough memory");
      return;
    }
    lv_label_set_text(title_lbl, ev.title[0] ? ev.title : espcontrol_i18n("(untitled)"));
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title_lbl, lv_pct(100));
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(row_text_col), LV_PART_MAIN);
    if (list_f) lv_obj_set_style_text_font(title_lbl, list_f, LV_PART_MAIN);
    apply_width_compensation(title_lbl, ctx->width_compensation_percent);

    // Line 2: time range.
    lv_obj_t *bottom = lv_obj_create(row);
    if (!bottom) {
      ESP_LOGW("ha_calendar", "Unable to create calendar modal metadata row");
      ha_calendar_modal_set_status("Not enough memory");
      return;
    }
    lv_obj_set_size(bottom, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bottom, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(bottom, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(bottom, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bottom, 0, LV_PART_MAIN);
    lv_obj_clear_flag(bottom, LV_OBJ_FLAG_SCROLLABLE);
    char time_range[20] = {};
    std::snprintf(time_range, sizeof(time_range), "%s \xe2\x80\x93 %s",
      ev.start_display, ev.end_display);
    lv_obj_t *time_lbl = lv_label_create(bottom);
    if (!time_lbl) {
      ESP_LOGW("ha_calendar", "Unable to create calendar modal time label");
      ha_calendar_modal_set_status("Not enough memory");
      return;
    }
    lv_label_set_text(time_lbl, time_range);
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(row_text_col), LV_PART_MAIN);
    if (meta_f) lv_obj_set_style_text_font(time_lbl, meta_f, LV_PART_MAIN);

  }

  if (ui.event_count == 0) ha_calendar_modal_set_status("No more events today");
}

inline void ha_calendar_render_modal(HaCalendarCardCtx *ctx) {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  if (!ha_calendar_ctx_valid(ctx) || ui.active != ctx || !ui.list) return;

  ha_calendar_sort_events();

  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  lv_coord_t content_w = lv_obj_get_width(ui.list);
  if (content_w <= 0) content_w = layout.panel_w - layout.inset * 2;

  ha_calendar_render_compact(ctx, content_w);
}

// ── Modal: service call ──────────────────────────────────────────────────────

inline uint32_t next_ha_calendar_call_id() {
  static uint32_t call_id = 430000;
  return call_id++;
}

inline std::string ha_calendar_local_datetime_str(time_t epoch) {
  struct tm *local = std::localtime(&epoch);
  char buf[25] = {};
  if (local) {
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
      local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
      local->tm_hour, local->tm_min, local->tm_sec);
  }
  return std::string(buf);
}

inline time_t ha_calendar_today_midnight_epoch() {
  time_t now = std::time(nullptr);
  struct tm *local = std::localtime(&now);
  if (!local) return 0;
  struct tm midnight = *local;
  midnight.tm_hour = 0;
  midnight.tm_min = 0;
  midnight.tm_sec = 0;
  midnight.tm_isdst = -1;
  return std::mktime(&midnight);
}

inline time_t ha_calendar_tomorrow_midnight_epoch() {
  time_t today = ha_calendar_today_midnight_epoch();
  if (today == 0) return 0;
  struct tm *local = std::localtime(&today);
  if (!local) return 0;
  struct tm tomorrow = *local;
  tomorrow.tm_mday += 1;
  tomorrow.tm_isdst = -1;
  return std::mktime(&tomorrow);
}

// Response template for one entity: emits "start_epoch|end_epoch|title\n" for
// events that have not yet ended. Epoch seconds are timezone-independent.
inline std::string ha_calendar_response_template(const std::string &entity_id) {
  std::string max_t = std::to_string(HA_CALENDAR_TITLE_MAX_LEN);
  std::string max_r = std::to_string(HA_CALENDAR_RESPONSE_MAX_LEN - 10);
  return
    "{% set e='" + entity_id + "' %}"
    "{% set evts=response.get(e,{}).get('events',[]) %}"
    "{% set now_ts=utcnow()|as_timestamp|int %}"
    "{% set ns=namespace(out='') %}"
    "{% for ev in evts %}"
    "{% set end_ts=ev.end|as_datetime|as_timestamp|int %}"
    "{% if end_ts>now_ts %}"
    "{% set start_ts=ev.start|as_datetime|as_timestamp|int %}"
    "{% set title=(ev.summary|default('')|string)[:" + max_t + "] %}"
    "{% set line=start_ts|string+'|'+end_ts|string+'|'+title %}"
    "{% if ns.out|length+line|length+1<=" + max_r + " %}"
    "{% set ns.out=ns.out+('\\n' if ns.out else '')+line %}"
    "{% endif %}"
    "{% endif %}"
    "{% endfor %}"
    "{{ ns.out }}";
}

inline void ha_calendar_request_events_for_entity(HaCalendarCardCtx *ctx,
                                                    const std::string &entity_id) {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  if (!ha_api_state_connected()) {
    if (ui.active == ctx) {
      ui.waiting_for_ha = true;
      ha_calendar_modal_set_status("Waiting for Home Assistant");
    }
    return;
  }

  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_ha_calendar_call_id();
  const uint32_t generation = ha_subscription_generation();
  std::string tmpl = ha_calendar_response_template(entity_id);
  std::string start_dt = ha_calendar_local_datetime_str(ha_calendar_today_midnight_epoch());
  std::string end_dt = ha_calendar_local_datetime_str(ha_calendar_tomorrow_midnight_epoch());

  if (!ha_action_begin(req, "calendar.get_events", false, 3, call_id)) {
    if (ui.active == ctx && ha_calendar_ctx_current(ctx, generation)) {
      ui.pending_entity_count--;
      if (ui.pending_entity_count <= 0) ha_calendar_modal_set_status("Could not load");
    }
    return;
  }
  req.wants_response = true;
  req.response_template = decltype(req.response_template)(tmpl);
  ha_action_add_entity(req, entity_id);
  ha_action_add_data(req, "start_date_time", start_dt.c_str());
  ha_action_add_data(req, "end_date_time", end_dt.c_str());

  if (!ha_register_action_response_callback(req.call_id,
    [ctx, call_id, generation](const esphome::api::ActionResponse &response) {
      (void) call_id;
      HaCalendarModalUi &ui = ha_calendar_modal_ui();
      if (ui.active != ctx || !ha_calendar_ctx_current(ctx, generation)) return;
      ui.pending_entity_count--;
      if (!response.is_success()) {
        if (ui.pending_entity_count <= 0)
          ha_calendar_modal_set_status("Could not load");
        return;
      }
      JsonVariantConst rv = response.get_json()["response"];
      const char *payload = rv.as<const char *>();
      if (payload) ha_calendar_parse_and_merge(payload);
      if (ui.pending_entity_count <= 0) {
        ui.waiting_for_ha = false;
        ha_calendar_render_modal(ctx);
      }
    })) {
    if (ui.active == ctx && ha_calendar_ctx_current(ctx, generation)) {
      ui.pending_entity_count--;
      if (ui.pending_entity_count <= 0) ha_calendar_modal_set_status("Could not load");
    }
    return;
  }

  if (!ha_action_send(req)) {
    ha_cancel_action_response_callback(call_id, "send failed");
    if (ui.active == ctx && ha_calendar_ctx_current(ctx, generation)) {
      ui.pending_entity_count--;
      if (ui.pending_entity_count <= 0) {
        if (!ha_api_state_connected()) {
          ui.waiting_for_ha = true;
          ha_calendar_modal_set_status("Waiting for Home Assistant");
        } else {
          ha_calendar_modal_set_status("Could not load");
        }
      }
    }
  }
}

// ── Card face: background next-event poll ─────────────────────────────────────

// Scan a "H:MM|H:MM|title" payload for the earliest event that STARTS after now
// and fold it into the card's in-flight candidate (ctx->fetch_best_*).
inline void ha_calendar_card_scan_payload(HaCalendarCardCtx *ctx,
                                          const char *payload) {
  if (!payload || !payload[0]) return;
  time_t now = std::time(nullptr);
  const char *p = payload;
  while (*p) {
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    const char *line_end = p;
    while (*line_end && *line_end != '\n' && *line_end != '\r') line_end++;
    size_t line_len = static_cast<size_t>(line_end - p);
    const char *sep1 = static_cast<const char *>(std::memchr(p, '|', line_len));
    if (sep1) {
      long long start_sec = 0, end_sec = 0;
      if (std::sscanf(p, "%lld", &start_sec) == 1 &&
          std::sscanf(sep1 + 1, "%lld", &end_sec) == 1) {
        time_t start = static_cast<time_t>(start_sec);
        time_t end   = static_cast<time_t>(end_sec);
        if (start > now) {
          // Future event: candidate for next-upcoming
          if (ctx->fetch_best_start == 0 || start < ctx->fetch_best_start) {
            ctx->fetch_best_start = start;
            const char *sep2 = static_cast<const char *>(
              std::memchr(sep1 + 1, '|', line_len - static_cast<size_t>(sep1 + 1 - p)));
            if (sep2) {
              size_t tlen = static_cast<size_t>(line_end - (sep2 + 1));
              if (tlen > HA_CALENDAR_TITLE_MAX_LEN) tlen = HA_CALENDAR_TITLE_MAX_LEN;
              ctx->fetch_best_title.assign(sep2 + 1, tlen);
            } else {
              ctx->fetch_best_title.clear();
            }
          }
        } else if (end > now) {
          // Currently active event: capture correct epochs/title for Current
          // mode and the Next Event card's short "Now" handoff window.
          if (ctx->fetch_active_end == 0 || end > ctx->fetch_active_end) {
            ctx->fetch_active_start = start;
            ctx->fetch_active_end   = end;
            const char *sep2 = static_cast<const char *>(
              std::memchr(sep1 + 1, '|', line_len - static_cast<size_t>(sep1 + 1 - p)));
            if (sep2) {
              size_t tlen = static_cast<size_t>(line_end - (sep2 + 1));
              if (tlen > HA_CALENDAR_TITLE_MAX_LEN) tlen = HA_CALENDAR_TITLE_MAX_LEN;
              ctx->fetch_active_title.assign(sep2 + 1, tlen);
            } else {
              ctx->fetch_active_title.clear();
            }
          }
        }
      }
    }
    p = line_end;
  }
}

// Poll calendar.get_events for every configured entity and commit the earliest
// upcoming event into ctx->next_* so Next mode can show it even while an event
// is active. Modeled on the modal's per-entity request, but card-scoped.
inline void ha_calendar_card_fetch(HaCalendarCardCtx *ctx) {
  if (!ha_calendar_ctx_valid(ctx) || ctx->entities.empty()) return;
  if (!ha_api_state_connected()) return;

  ctx->fetch_best_start = 0;
  ctx->fetch_best_title.clear();
  ctx->fetch_active_start = 0;
  ctx->fetch_active_end = 0;
  ctx->fetch_active_title.clear();
  ctx->fetch_pending = static_cast<int>(ctx->entities.size());
  const uint32_t generation = ha_subscription_generation();
  std::string start_dt = ha_calendar_local_datetime_str(ha_calendar_today_midnight_epoch());
  std::string end_dt = ha_calendar_local_datetime_str(ha_calendar_tomorrow_midnight_epoch());

  auto finish_one = [generation](HaCalendarCardCtx *c) {
    if (!ha_calendar_ctx_current(c, generation)) return;
    if (c->fetch_pending > 0) c->fetch_pending--;
    if (c->fetch_pending <= 0) {
      c->next_start_epoch = c->fetch_best_start;
      c->next_title = c->fetch_best_title;
      c->next_valid = (c->fetch_best_start != 0);
      // Commit correct active-event epochs for Current mode progress bar
      if (c->fetch_active_end != 0) {
        c->poll_active_start = c->fetch_active_start;
        c->poll_active_end   = c->fetch_active_end;
        c->poll_active_title = c->fetch_active_title;
        c->poll_active_valid = true;
      } else {
        c->poll_active_valid = false;
        c->poll_active_title.clear();
      }
      c->fetch_done = true;
      ha_calendar_apply_card_face(c);
    }
  };

  for (const auto &e : ctx->entities) {
    std::string entity_id = e.entity_id;
    esphome::api::HomeassistantActionRequest req;
    uint32_t call_id = next_ha_calendar_call_id();
    std::string tmpl = ha_calendar_response_template(entity_id);
    if (!ha_action_begin(req, "calendar.get_events", false, 3, call_id)) {
      finish_one(ctx);
      continue;
    }
    req.wants_response = true;
    req.response_template = decltype(req.response_template)(tmpl);
    ha_action_add_entity(req, entity_id);
    ha_action_add_data(req, "start_date_time", start_dt.c_str());
    ha_action_add_data(req, "end_date_time", end_dt.c_str());

    if (!ha_register_action_response_callback(req.call_id,
      [ctx, generation, finish_one](const esphome::api::ActionResponse &response) {
        if (!ha_calendar_ctx_current(ctx, generation)) return;
        if (response.is_success()) {
          JsonVariantConst rv = response.get_json()["response"];
          const char *payload = rv.as<const char *>();
          if (payload) ha_calendar_card_scan_payload(ctx, payload);
        }
        finish_one(ctx);
      })) {
      finish_one(ctx);
      continue;
    }
    if (!ha_action_send(req)) {
      ha_cancel_action_response_callback(call_id, "send failed");
      finish_one(ctx);
    }
  }
}

// ── Modal: open ──────────────────────────────────────────────────────────────

// Re-render the open modal as cached events move between future and active.
inline void ha_calendar_modal_refresh_cb(lv_timer_t *) {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  if (ui.active && ui.list && ha_calendar_ctx_valid(ui.active) && ui.event_count > 0) {
    ha_calendar_render_modal(ui.active);
  }
}

inline void ha_calendar_open_modal(HaCalendarCardCtx *ctx) {
  if (!ha_calendar_ctx_valid(ctx) || ctx->entities.empty()) return;
  if (!ha_calendar_modal_heap_available("calendar modal open")) return;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::HA_CALENDAR, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, ha_calendar_modal_hide);
  if (!shell.overlay || !shell.panel) return;

  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;
  ui.event_count = 0;
  ui.waiting_for_ha = false;
  std::memset(ui.events, 0, sizeof(ui.events));

  ControlModalLayout &layout = shell.layout;
  lv_coord_t content_w = shell.content_w;
  lv_coord_t gap = control_modal_scaled_px(18, layout.short_side);
  if (gap < 10) gap = 10;
  lv_coord_t list_y = layout.inset + layout.back_size + gap;
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < 60) list_h = 60;
  lv_coord_t list_pad = control_modal_scaled_px(4, layout.short_side);
  if (list_pad < 2) list_pad = 2;
  lv_coord_t list_w = content_w - list_pad * 2;
  if (list_w < 80) { list_w = content_w; list_pad = 0; }

  lv_coord_t row_gap = control_modal_scaled_px(12, layout.short_side);
  if (row_gap < 8) row_gap = 8;
  ui.list = control_modal_create_scroll_list(ui.panel, list_w, list_h, row_gap);
  if (!ui.list) {
    ESP_LOGW("ha_calendar", "Unable to create calendar modal list");
    ha_calendar_modal_hide();
    return;
  }
  // Right gutter so the scrollbar sits beside the rows instead of over content.
  lv_coord_t sb_gutter = control_modal_scaled_px(10, layout.short_side);
  if (sb_gutter < 6) sb_gutter = 6;
  lv_obj_set_style_pad_right(ui.list, sb_gutter, LV_PART_MAIN);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset + list_pad, list_y);
  if (ui.close_btn) lv_obj_move_foreground(ui.close_btn);

  lv_obj_move_foreground(ui.overlay);

  // Keep active/future row styling current while the modal stays open.
  ui.refresh_timer = lv_timer_create(ha_calendar_modal_refresh_cb, 30000, nullptr);

  // Kick off one request per entity
  ui.pending_entity_count = static_cast<int>(ctx->entities.size());
  ui.started_ms = esphome::millis();
  ha_calendar_modal_set_status("Loading");
  for (const auto &estate : ctx->entities) {
    ha_calendar_request_events_for_entity(ctx, estate.entity_id);
  }
}
