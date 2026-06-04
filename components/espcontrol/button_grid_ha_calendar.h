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
constexpr size_t   HA_CALENDAR_TITLE_SMALL_LEN = 18;  // titles longer than this use the smaller font

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
  std::string display_mode;   // "current" or "" (next)
  std::string modal_layout;   // "column" or "" (compact)

  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;    // repurposed: "In" / "Now" / "" text label
  lv_obj_t *value_lbl = nullptr;   // sensor_lbl: countdown number or "Now"
  lv_obj_t *unit_lbl = nullptr;    // "min" / "hr" / "days"
  lv_obj_t *label_lbl = nullptr;   // text_lbl: event title or card label
  const lv_font_t *value_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *list_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  const lv_font_t *chrome_icon_font = nullptr;  // smaller icon font for the modal back button
  const lv_font_t *unit_font = nullptr;   // original unit-label font (for restore)
  const lv_font_t *small_font = nullptr;  // smaller font for long event titles
  const lv_font_t *tiny_font = nullptr;   // smallest font for modal secondary text
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = DEFAULT_OFF_COLOR;
  int width_compensation_percent = 100;
  bool available = false;
  lv_timer_t *refresh_timer = nullptr;  // periodic re-render so countdown ticks

  // Background event cache: Next mode needs the next upcoming event even while
  // another event is active, which the HA entity state alone can't provide. A
  // periodic calendar.get_events poll fills these.
  uint32_t refresh_ticks = 0;        // counts refresh ticks to schedule polls
  bool next_valid = false;           // a polled next-upcoming event is known
  time_t next_start_epoch = 0;       // committed next event start (after now)
  std::string next_title;            // committed next event title
  int fetch_pending = 0;             // entity responses still outstanding
  bool fetch_done = false;           // a poll has completed at least once
  time_t fetch_best_start = 0;       // candidate during an in-flight poll
  std::string fetch_best_title;
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

// Set the bottom event title, choosing a smaller font for long names so they
// fit better before ellipsizing. Re-applies the 2-line clamp for the chosen font.
inline void ha_calendar_set_title(HaCalendarCardCtx *ctx, const char *text) {
  if (!ctx->label_lbl) return;
  const lv_font_t *font = ctx->label_font;
  size_t len = text ? std::strlen(text) : 0;
  if (len > HA_CALENDAR_TITLE_SMALL_LEN && ctx->small_font &&
      ctx->small_font->line_height > 0 &&
      (!font || ctx->small_font->line_height < font->line_height)) {
    font = ctx->small_font;  // long title → smaller font
  }
  if (font) lv_obj_set_style_text_font(ctx->label_lbl, font, LV_PART_MAIN);
  lv_coord_t lh = (font && font->line_height > 0) ? font->line_height : 16;
  lv_label_set_long_mode(ctx->label_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(ctx->label_lbl, lv_pct(100));
  lv_obj_set_height(ctx->label_lbl, lh * 2);
  lv_obj_align(ctx->label_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_label_set_text(ctx->label_lbl, text ? text : "");
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
  if (ctx->icon_lbl) lv_obj_add_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);  // only shown for "Done for the day"
  if (ctx->label_lbl)
    lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);

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
  time_t event_start = best->start_epoch;
  time_t event_end = best->end_epoch;
  bool event_active = best->active || (event_end > 0 && event_start <= now && event_end > now);

  if (is_current_mode) {
    if (!event_active) {
      // Free: no event running. Hide the title and the number slot, and show a
      // bold, accent-colored "Free" on the top line.
      lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ctx->label_lbl, "");
      if (ctx->unit_lbl) {
        lv_label_set_text(ctx->unit_lbl, "Free");
        if (ctx->label_font)
          lv_obj_set_style_text_font(ctx->unit_lbl, ctx->label_font, LV_PART_MAIN);
        lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(ctx->accent_color), LV_PART_MAIN);
      }
      set_bg(ctx->off_color);
      return;
    }
    // Active meeting: the tile is a vertical progress split — bright primary
    // darkened primary "elapsed" growing in from the left, bright primary
    // "remaining" on the right — so the bright portion shrinks toward the right
    // edge as the meeting progresses. Drawn as a hard-stop horizontal background
    // gradient so it covers the full tile.
    int64_t secs_remaining = (event_end > now) ? (int64_t)(event_end - now) : 0;
    int64_t secs_into = (now > event_start) ? (int64_t)(now - event_start) : 0;
    int64_t total = (event_end > event_start) ? (int64_t)(event_end - event_start) : 0;
    int pct_elapsed = (total > 0) ? (int)((secs_into * 100) / total) : 0;
    if (pct_elapsed < 0) pct_elapsed = 0;
    if (pct_elapsed > 100) pct_elapsed = 100;

    lv_style_selector_t sel = static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT;
    uint8_t stop = static_cast<uint8_t>(pct_elapsed * 255 / 100);
    lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(darken_color(ctx->accent_color, 62)), sel);
    lv_obj_set_style_bg_grad_color(ctx->btn, lv_color_hex(ctx->accent_color), sel);
    lv_obj_set_style_bg_grad_dir(ctx->btn, LV_GRAD_DIR_HOR, sel);
    lv_obj_set_style_bg_main_stop(ctx->btn, stop, sel);
    lv_obj_set_style_bg_grad_stop(ctx->btn, stop, sel);

    ha_calendar_set_title(ctx, best->title.c_str());
    if (secs_remaining <= HA_CALENDAR_URGENT_SECS) {
      // About to end: show the minutes-left number.
      char num_buf[16] = {}, unit_buf[8] = {};
      ha_calendar_format_countdown(secs_remaining, num_buf, sizeof(num_buf),
                                    unit_buf, sizeof(unit_buf));
      lv_obj_clear_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ctx->value_lbl, num_buf);
      if (ctx->unit_lbl) {
        lv_label_set_text(ctx->unit_lbl, unit_buf);
        if (ctx->unit_font) lv_obj_set_style_text_font(ctx->unit_lbl, ctx->unit_font, LV_PART_MAIN);
      }
    } else {
      // Word phase: "Just started" early, "In progress" otherwise.
      lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
      if (ctx->unit_lbl) {
        lv_label_set_text(ctx->unit_lbl,
          secs_into < HA_CALENDAR_URGENT_SECS ? "Just started" : "In progress");
        if (ctx->label_font) lv_obj_set_style_text_font(ctx->unit_lbl, ctx->label_font, LV_PART_MAIN);
      }
    }
    // White text so it reads across both the bright and darkened halves.
    lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->unit_lbl) lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (ctx->label_lbl) lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    return;
  }

  // Next mode (default): prefer the polled next-upcoming event so the tile can
  // count down to the next event even while another one is currently active
  // (the HA entity state alone only exposes the active event in that case).
  // Show a countdown (number + unit) to `title`, escalating to a loud accent
  // card with dark text when the event is imminent (≤ 5 min away).
  auto show_countdown = [&](int64_t secs_until, const char *title) {
    char num_buf[16] = {}, unit_buf[8] = {};
    ha_calendar_format_countdown(secs_until, num_buf, sizeof(num_buf), unit_buf, sizeof(unit_buf));
    lv_label_set_text(ctx->value_lbl, num_buf);
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, unit_buf);
    ha_calendar_set_title(ctx, title);
    if (secs_until <= HA_CALENDAR_URGENT_SECS) {
      uint32_t on_accent = 0x1A1A1A;  // dark text for contrast on the accent fill
      lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(on_accent), LV_PART_MAIN);
      if (ctx->unit_lbl) lv_obj_set_style_text_color(ctx->unit_lbl, lv_color_hex(on_accent), LV_PART_MAIN);
      if (ctx->label_lbl) lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(on_accent), LV_PART_MAIN);
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

  // 3: nothing upcoming today, but a meeting is active now → "Now" (accent while
  // it has just started, dimmed once well into it).
  if (event_active) {
    int64_t secs_into = (now > event_start) ? (int64_t)(now - event_start) : 0;
    bool just_started = secs_into < HA_CALENDAR_URGENT_SECS;
    lv_obj_add_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
    if (ctx->unit_lbl) {
      lv_label_set_text(ctx->unit_lbl, "Now");
      if (ctx->label_font) lv_obj_set_style_text_font(ctx->unit_lbl, ctx->label_font, LV_PART_MAIN);
      lv_obj_set_style_text_color(ctx->unit_lbl,
        lv_color_hex(just_started ? ctx->accent_color : DARK_TEXT_MUTED), LV_PART_MAIN);
    }
    ha_calendar_set_title(ctx, best->title.c_str());
    if (!just_started && ctx->label_lbl)
      lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    set_bg(just_started ? darken_color(ctx->accent_color, 62) : ctx->off_color);
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
    ha_calendar_set_title(ctx, "Done for the day");
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
  // Poll calendar.get_events shortly after startup and then every ~5 minutes so
  // Next mode always knows the next upcoming event, even during an active one.
  // Current mode counts down the active event's end and needs no poll.
  if (ctx->display_mode != "current" &&
      (ctx->refresh_ticks == 1 || ctx->refresh_ticks % 10 == 0)) {
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
    const lv_font_t *small_font,
    const lv_font_t *tiny_font,
    const lv_font_t *chrome_icon_font,
    int width_compensation_percent) {
  HaCalendarCardCtx *ctx = new HaCalendarCardCtx();
  ctx->configured_label = p.label;
  ctx->display_mode = cfg_option_value(p.options, "display_mode");
  ctx->modal_layout = cfg_option_value(p.options, "modal_layout");
  ESP_LOGD("ha_calendar", "setup entities='%s' options='%s' -> display_mode='%s' modal_layout='%s'",
           p.entity.c_str(), p.options.c_str(), ctx->display_mode.c_str(), ctx->modal_layout.c_str());
  ctx->accent_color = accent_color;
  ctx->off_color = off_color;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;  // used for the "Done for the day" glyph
  ctx->value_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->unit_font = s.unit_lbl ? lv_obj_get_style_text_font(s.unit_lbl, LV_PART_MAIN) : nullptr;
  ctx->value_font = value_font;
  ctx->label_font = label_font;
  ctx->list_font = list_font ? list_font : label_font;
  ctx->icon_font = icon_font;
  ctx->chrome_icon_font = chrome_icon_font;
  ctx->small_font = small_font;
  ctx->tiny_font = tiny_font;
  ctx->width_compensation_percent = width_compensation_percent;
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
        // Next mode: kick the first get_events poll as soon as state arrives
        // (API is connected), instead of waiting for the 30s tick — otherwise
        // the tile briefly shows the active event ("Now") before it learns the
        // next upcoming one.
        if (ctx->display_mode != "current" && !ctx->fetch_done &&
            ctx->fetch_pending <= 0 && ha_api_state_connected()) {
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

// Parse "H:MM|H:MM|title\n" lines, convert to epochs relative to today's midnight.
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

inline void ha_calendar_parse_and_merge(const char *payload, time_t midnight) {
  if (!payload || !payload[0]) return;
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  const char *p = payload;
  while (*p) {
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    const char *line_end = p;
    while (*line_end && *line_end != '\n' && *line_end != '\r') line_end++;
    size_t line_len = static_cast<size_t>(line_end - p);
    if (line_len < 5) { p = line_end; continue; }

    const char *sep1 = static_cast<const char *>(std::memchr(p, '|', line_len));
    if (!sep1) { p = line_end; continue; }
    const char *sep2 = static_cast<const char *>(
      std::memchr(sep1 + 1, '|', line_len - static_cast<size_t>(sep1 + 1 - p)));
    if (!sep2) { p = line_end; continue; }

    int sh = 0, sm = 0, eh = 0, em = 0;
    if (std::sscanf(p, "%d:%d", &sh, &sm) != 2 ||
        std::sscanf(sep1 + 1, "%d:%d", &eh, &em) != 2) {
      p = line_end; continue;
    }
    if (ui.event_count >= HA_CALENDAR_MAX_EVENTS) break;

    HaCalendarEventRow &row = ui.events[ui.event_count++];
    ha_calendar_format_hm(row.start_display, sizeof(row.start_display), sh, sm);
    ha_calendar_format_hm(row.end_display, sizeof(row.end_display), eh, em);
    row.start_epoch = midnight + static_cast<time_t>(sh * 3600 + sm * 60);
    row.end_epoch   = midnight + static_cast<time_t>(eh * 3600 + em * 60);
    if (row.end_epoch <= row.start_epoch) row.end_epoch += 86400;

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
    bool is_urgent = !is_active && ev.start_epoch > now &&
                     (ev.start_epoch - now) <= HA_CALENDAR_URGENT_SECS;

    uint32_t title_col = is_active || is_urgent ? ctx->accent_color : DARK_TEXT_PRIMARY;
    uint32_t time_col  = is_active ? ctx->accent_color : DARK_TEXT_MUTED;

    lv_obj_t *row = lv_obj_create(ui.list);
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
    if (is_active) {
      lv_obj_set_style_bg_opa(row, LV_OPA_20, LV_PART_MAIN);
      lv_obj_set_style_bg_color(row, lv_color_hex(ctx->accent_color), LV_PART_MAIN);
    } else {
      lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    }

    const lv_font_t *small_f = ctx->small_font ? ctx->small_font : ctx->label_font;
    const lv_font_t *tiny_f = ctx->tiny_font ? ctx->tiny_font : small_f;

    // Countdown text
    char cd[24] = {};
    if (is_active) {
      std::strncpy(cd, "Now", sizeof(cd) - 1);
    } else if (ev.start_epoch > now) {
      int64_t secs = static_cast<int64_t>(ev.start_epoch - now);
      char num[12] = {}, unit[8] = {};
      ha_calendar_format_countdown(secs, num, sizeof(num), unit, sizeof(unit));
      std::snprintf(cd, sizeof(cd), "In %s %s", num, unit);
    }

    // Line 1: meeting name on its own, full width (so it isn't squeezed by the
    // countdown and the scrollbar can't sit on top of the countdown).
    lv_obj_t *title_lbl = lv_label_create(row);
    lv_label_set_text(title_lbl, ev.title[0] ? ev.title : "(untitled)");
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title_lbl, lv_pct(100));
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(title_col), LV_PART_MAIN);
    if (small_f) lv_obj_set_style_text_font(title_lbl, small_f, LV_PART_MAIN);
    apply_width_compensation(title_lbl, ctx->width_compensation_percent);

    // Line 2: time range (left) + countdown (right), on the same baseline.
    lv_obj_t *bottom = lv_obj_create(row);
    lv_obj_set_size(bottom, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bottom, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(bottom, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(bottom, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bottom, 0, LV_PART_MAIN);
    lv_obj_clear_flag(bottom, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bottom, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    char time_range[20] = {};
    std::snprintf(time_range, sizeof(time_range), "%s \xe2\x80\x93 %s",
      ev.start_display, ev.end_display);
    lv_obj_t *time_lbl = lv_label_create(bottom);
    lv_label_set_text(time_lbl, time_range);
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(time_col), LV_PART_MAIN);
    if (tiny_f) lv_obj_set_style_text_font(time_lbl, tiny_f, LV_PART_MAIN);

    lv_obj_t *cd_lbl = lv_label_create(bottom);
    lv_label_set_text(cd_lbl, cd);
    lv_obj_set_style_text_color(cd_lbl, lv_color_hex(title_col), LV_PART_MAIN);
    if (tiny_f) lv_obj_set_style_text_font(cd_lbl, tiny_f, LV_PART_MAIN);
    lv_obj_set_style_margin_right(cd_lbl, 5, LV_PART_MAIN);  // nudge countdown left

    // Divider
    if (i + 1 < ui.event_count) {
      lv_obj_t *div = lv_obj_create(ui.list);
      lv_obj_set_size(div, lv_pct(90), 1);
      lv_obj_set_style_bg_color(div, lv_color_hex(DARK_BORDER), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(div, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_border_width(div, 0, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(div, 0, LV_PART_MAIN);
    }
  }

  if (ui.event_count == 0) ha_calendar_modal_set_status("No more events today");
}

inline void ha_calendar_render_column(HaCalendarCardCtx *ctx, lv_coord_t content_w) {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  if (!ui.list) return;
  lv_obj_clean(ui.list);
  ui.status_lbl = nullptr;
  time_t now = std::time(nullptr);

  for (int i = 0; i < ui.event_count; i++) {
    HaCalendarEventRow &ev = ui.events[i];
    bool is_active = ev.start_epoch <= now && ev.end_epoch > now;
    bool is_urgent = !is_active && ev.start_epoch > now &&
                     (ev.start_epoch - now) <= HA_CALENDAR_URGENT_SECS;

    uint32_t accent = (is_active || is_urgent) ? ctx->accent_color : DARK_BORDER;
    uint32_t title_col = (is_active || is_urgent) ? ctx->accent_color : DARK_TEXT_PRIMARY;
    uint32_t time_col  = is_active ? ctx->accent_color : DARK_TEXT_MUTED;

    lv_obj_t *row = lv_obj_create(ui.list);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row, 6, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    if (is_active) {
      lv_obj_set_style_bg_opa(row, LV_OPA_20, LV_PART_MAIN);
      lv_obj_set_style_bg_color(row, lv_color_hex(ctx->accent_color), LV_PART_MAIN);
    } else {
      lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    }

    // Time column (start + end stacked)
    lv_obj_t *tcol = lv_obj_create(row);
    lv_obj_set_size(tcol, 48, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(tcol, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(tcol, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(tcol, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tcol, 0, LV_PART_MAIN);
    lv_obj_clear_flag(tcol, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(tcol, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tcol, LV_FLEX_FLOW_COLUMN);

    const lv_font_t *small_f = ctx->small_font ? ctx->small_font : ctx->label_font;
    const lv_font_t *tiny_f = ctx->tiny_font ? ctx->tiny_font : small_f;
    // Start time: prominent (white, or accent when active). End time: smaller, muted.
    lv_obj_t *sl = lv_label_create(tcol);
    lv_label_set_text(sl, ev.start_display);
    lv_obj_set_style_text_color(sl, lv_color_hex(is_active ? ctx->accent_color : DARK_TEXT_PRIMARY), LV_PART_MAIN);
    if (small_f) lv_obj_set_style_text_font(sl, small_f, LV_PART_MAIN);

    lv_obj_t *el = lv_label_create(tcol);
    lv_label_set_text(el, ev.end_display);
    lv_obj_set_style_text_color(el, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    if (tiny_f) lv_obj_set_style_text_font(el, tiny_f, LV_PART_MAIN);

    // Accent bar — spans the full height of the two text lines.
    lv_coord_t bar_h = 0;
    if (small_f && small_f->line_height > 0) bar_h += small_f->line_height;
    if (tiny_f && tiny_f->line_height > 0) bar_h += tiny_f->line_height;
    if (bar_h <= 0) bar_h = 34;
    lv_obj_t *bar = lv_obj_create(row);
    lv_obj_set_size(bar, 3, bar_h);
    lv_obj_set_style_bg_color(bar, lv_color_hex(accent), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_hor(bar, 6, LV_PART_MAIN);

    // Info column: title + countdown
    lv_obj_t *info = lv_obj_create(row);
    lv_obj_set_flex_grow(info, 1);
    lv_obj_set_height(info, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(info, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(info, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info, 0, LV_PART_MAIN);
    lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(info, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title_lbl = lv_label_create(info);
    lv_label_set_text(title_lbl, ev.title[0] ? ev.title : "(untitled)");
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title_lbl, lv_pct(100));
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(title_col), LV_PART_MAIN);
    if (small_f) lv_obj_set_style_text_font(title_lbl, small_f, LV_PART_MAIN);
    apply_width_compensation(title_lbl, ctx->width_compensation_percent);

    char cd[24] = {};
    if (is_active) {
      std::strncpy(cd, "Now", sizeof(cd) - 1);
    } else if (ev.start_epoch > now) {
      int64_t secs = static_cast<int64_t>(ev.start_epoch - now);
      char num[12] = {}, unit[8] = {};
      ha_calendar_format_countdown(secs, num, sizeof(num), unit, sizeof(unit));
      std::snprintf(cd, sizeof(cd), "In %s %s", num, unit);
    }
    lv_obj_t *cd_lbl = lv_label_create(info);
    lv_label_set_text(cd_lbl, cd);
    lv_obj_set_style_text_color(cd_lbl, lv_color_hex(title_col), LV_PART_MAIN);
    if (tiny_f) lv_obj_set_style_text_font(cd_lbl, tiny_f, LV_PART_MAIN);

    if (i + 1 < ui.event_count) {
      lv_obj_t *div = lv_obj_create(ui.list);
      lv_obj_set_size(div, lv_pct(90), 1);
      lv_obj_set_style_bg_color(div, lv_color_hex(DARK_BORDER), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(div, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_border_width(div, 0, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(div, 0, LV_PART_MAIN);
    }
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

  if (ctx->modal_layout == "column") {
    ha_calendar_render_column(ctx, content_w);
  } else {
    ha_calendar_render_compact(ctx, content_w);
  }
}

// ── Modal: service call ──────────────────────────────────────────────────────

inline uint32_t next_ha_calendar_call_id() {
  static uint32_t call_id = 430000;
  return call_id++;
}

inline std::string ha_calendar_today_end_str() {
  time_t now = std::time(nullptr);
  struct tm *local = std::localtime(&now);
  char buf[25] = {};
  if (local) {
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT23:59:59",
      local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
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

// Response template for one entity: emits "H:MM|H:MM|title\n" for remaining events today.
inline std::string ha_calendar_response_template(const std::string &entity_id) {
  std::string max_t = std::to_string(HA_CALENDAR_TITLE_MAX_LEN);
  std::string max_r = std::to_string(HA_CALENDAR_RESPONSE_MAX_LEN - 10);
  return
    "{% set e='" + entity_id + "' %}"
    "{% set evts=response.get(e,{}).get('events',[]) %}"
    "{% set now_dt=now() %}"
    "{% set ns=namespace(out='') %}"
    "{% for ev in evts %}"
    "{% set end_dt=ev.end|as_datetime|as_local %}"
    "{% if end_dt>now_dt %}"
    "{% set start_dt=ev.start|as_datetime|as_local %}"
    "{% set title=(ev.summary|default('')|string)[:" + max_t + "] %}"
    "{% set line=start_dt.strftime('%H:%M')+'|'+end_dt.strftime('%H:%M')+'|'+title %}"
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
  std::string tmpl = ha_calendar_response_template(entity_id);
  std::string end_dt = ha_calendar_today_end_str();
  time_t midnight = ha_calendar_today_midnight_epoch();

  if (!ha_action_begin(req, "calendar.get_events", false, 2, call_id)) {
    if (ui.active == ctx) {
      ui.pending_entity_count--;
      if (ui.pending_entity_count <= 0) ha_calendar_modal_set_status("Could not load");
    }
    return;
  }
  req.wants_response = true;
  req.response_template = decltype(req.response_template)(tmpl);
  ha_action_add_entity(req, entity_id);
  ha_action_add_data(req, "end_date_time", end_dt.c_str());

  if (!ha_register_action_response_callback(req.call_id,
    [ctx, call_id, midnight](const esphome::api::ActionResponse &response) {
      (void) call_id;
      HaCalendarModalUi &ui = ha_calendar_modal_ui();
      if (ui.active != ctx) return;
      ui.pending_entity_count--;
      if (!response.is_success()) {
        if (ui.pending_entity_count <= 0)
          ha_calendar_modal_set_status("Could not load");
        return;
      }
      JsonVariantConst rv = response.get_json()["response"];
      const char *payload = rv.as<const char *>();
      if (payload) ha_calendar_parse_and_merge(payload, midnight);
      if (ui.pending_entity_count <= 0) {
        ui.waiting_for_ha = false;
        ha_calendar_render_modal(ctx);
      }
    })) {
    ui.pending_entity_count--;
    if (ui.pending_entity_count <= 0) ha_calendar_modal_set_status("Could not load");
    return;
  }

  if (!ha_action_send(req)) {
    ha_cancel_action_response_callback(call_id, "send failed");
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

// ── Card face: background next-event poll ─────────────────────────────────────

// Scan a "H:MM|H:MM|title" payload for the earliest event that STARTS after now
// and fold it into the card's in-flight candidate (ctx->fetch_best_*).
inline void ha_calendar_card_scan_payload(HaCalendarCardCtx *ctx,
                                          const char *payload, time_t midnight) {
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
      int sh = 0, sm = 0;
      if (std::sscanf(p, "%d:%d", &sh, &sm) == 2) {
        time_t start = midnight + static_cast<time_t>(sh * 3600 + sm * 60);
        if (start > now &&
            (ctx->fetch_best_start == 0 || start < ctx->fetch_best_start)) {
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
  ctx->fetch_pending = static_cast<int>(ctx->entities.size());
  std::string end_dt = ha_calendar_today_end_str();
  time_t midnight = ha_calendar_today_midnight_epoch();

  auto finish_one = [](HaCalendarCardCtx *c) {
    if (c->fetch_pending > 0) c->fetch_pending--;
    if (c->fetch_pending <= 0) {
      c->next_start_epoch = c->fetch_best_start;
      c->next_title = c->fetch_best_title;
      c->next_valid = (c->fetch_best_start != 0);
      c->fetch_done = true;  // a poll has now completed → "Done for the day" is meaningful
      ha_calendar_apply_card_face(c);
    }
  };

  for (const auto &e : ctx->entities) {
    std::string entity_id = e.entity_id;
    esphome::api::HomeassistantActionRequest req;
    uint32_t call_id = next_ha_calendar_call_id();
    std::string tmpl = ha_calendar_response_template(entity_id);
    if (!ha_action_begin(req, "calendar.get_events", false, 2, call_id)) {
      finish_one(ctx);
      continue;
    }
    req.wants_response = true;
    req.response_template = decltype(req.response_template)(tmpl);
    ha_action_add_entity(req, entity_id);
    ha_action_add_data(req, "end_date_time", end_dt.c_str());

    if (!ha_register_action_response_callback(req.call_id,
      [ctx, midnight, finish_one](const esphome::api::ActionResponse &response) {
        if (!ha_calendar_ctx_valid(ctx)) return;
        if (response.is_success()) {
          JsonVariantConst rv = response.get_json()["response"];
          const char *payload = rv.as<const char *>();
          if (payload) ha_calendar_card_scan_payload(ctx, payload, midnight);
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

// Re-render the open modal so countdowns ("In 3 min", "Now") advance over time
// (events are cached; this only recomputes against the current clock).
inline void ha_calendar_modal_refresh_cb(lv_timer_t *) {
  HaCalendarModalUi &ui = ha_calendar_modal_ui();
  if (ui.active && ui.list && ha_calendar_ctx_valid(ui.active) && ui.event_count > 0) {
    ha_calendar_render_modal(ui.active);
  }
}

inline void ha_calendar_open_modal(HaCalendarCardCtx *ctx) {
  if (!ha_calendar_ctx_valid(ctx) || ctx->entities.empty()) return;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::HA_CALENDAR, ctx->btn, ctx->width_compensation_percent,
    ctx->chrome_icon_font ? ctx->chrome_icon_font : ctx->icon_font,
    "\U000F0141", false, ha_calendar_modal_hide);

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
  lv_coord_t title_y = layout.inset + layout.back_size / 2;
  lv_coord_t list_y = layout.inset + layout.back_size + gap;
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < 60) list_h = 60;
  lv_coord_t list_pad = control_modal_scaled_px(4, layout.short_side);
  if (list_pad < 2) list_pad = 2;
  lv_coord_t list_w = content_w - list_pad * 2;
  if (list_w < 80) { list_w = content_w; list_pad = 0; }

  std::string title_text = (ctx->entities.size() == 1)
    ? ha_calendar_card_label(ctx) + " \xc2\xb7 Today"
    : std::string("Today");
  ui.title_lbl = control_modal_create_title(
    ui.panel, title_text, content_w - layout.back_size - gap,
    ctx->label_font, ctx->width_compensation_percent);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_update_layout(ui.title_lbl);
  lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_RIGHT, 0,
    title_y - lv_obj_get_height(ui.title_lbl) / 2);

  lv_coord_t row_gap = control_modal_scaled_px(6, layout.short_side);
  if (row_gap < 4) row_gap = 4;
  ui.list = control_modal_create_scroll_list(ui.panel, list_w, list_h, row_gap);
  // Right gutter so the scrollbar sits beside the rows instead of over the
  // right-most content (countdowns).
  lv_coord_t sb_gutter = control_modal_scaled_px(10, layout.short_side);
  if (sb_gutter < 6) sb_gutter = 6;
  lv_obj_set_style_pad_right(ui.list, sb_gutter, LV_PART_MAIN);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset + list_pad, list_y);

  lv_obj_move_foreground(ui.overlay);

  // Keep the countdowns ticking while the modal stays open.
  ui.refresh_timer = lv_timer_create(ha_calendar_modal_refresh_cb, 30000, nullptr);

  // Kick off one request per entity
  ui.pending_entity_count = static_cast<int>(ctx->entities.size());
  ui.started_ms = esphome::millis();
  ha_calendar_modal_set_status("Loading");
  for (const auto &estate : ctx->entities) {
    ha_calendar_request_events_for_entity(ctx, estate.entity_id);
  }
}
