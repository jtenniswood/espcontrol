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
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = DEFAULT_OFF_COLOR;
  int width_compensation_percent = 100;
  bool available = false;
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
  return ctx->entities.size() == 1 ? ctx->entities[0].entity_id : "Calendar";
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
  };

  if (!any_available || now <= 0) {
    if (ctx->icon_lbl) lv_label_set_text(ctx->icon_lbl, "");
    lv_label_set_text(ctx->value_lbl, "--");
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    set_bg(ctx->off_color);
    return;
  }

  const HaCalendarEntityState *best = nullptr;
  if (!ha_calendar_find_best_entity(ctx, &best, now) || best == nullptr) {
    // No events with valid data
    if (ctx->icon_lbl) lv_label_set_text(ctx->icon_lbl, "");
    lv_label_set_text(ctx->value_lbl, "\xe2\x80\x94");  // em dash
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    set_bg(ctx->off_color);
    return;
  }

  bool is_current_mode = (ctx->display_mode == "current");
  time_t event_start = best->start_epoch;
  time_t event_end = best->end_epoch;
  bool event_active = best->active || (event_end > 0 && event_start <= now && event_end > now);

  if (is_current_mode) {
    if (!event_active) {
      if (ctx->icon_lbl) lv_label_set_text(ctx->icon_lbl, "");
      lv_label_set_text(ctx->value_lbl, "Free");
      if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
      set_bg(ctx->off_color);
      return;
    }
    int64_t secs_remaining = (event_end > now) ? (int64_t)(event_end - now) : 0;
    char num_buf[16] = {}, unit_buf[8] = {};
    ha_calendar_format_countdown(secs_remaining, num_buf, sizeof(num_buf),
                                  unit_buf, sizeof(unit_buf));
    if (ctx->icon_lbl) lv_label_set_text(ctx->icon_lbl, "Ends in");
    lv_label_set_text(ctx->value_lbl, num_buf);
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, unit_buf);
    lv_label_set_text(ctx->label_lbl, best->title.c_str());
    set_bg(ctx->off_color);
    return;
  }

  // Next mode (default)
  if (event_active) {
    int64_t secs_into = (now > event_start) ? (int64_t)(now - event_start) : 0;
    bool first_5_min = secs_into < HA_CALENDAR_URGENT_SECS;
    if (ctx->icon_lbl) lv_label_set_text(ctx->icon_lbl, "");
    lv_label_set_text(ctx->value_lbl, "Now");
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    lv_label_set_text(ctx->label_lbl, best->title.c_str());
    set_bg(first_5_min ? ctx->accent_color : ctx->off_color);
    return;
  }

  // Upcoming event
  int64_t secs_until = (event_start > now) ? (int64_t)(event_start - now) : 0;
  char num_buf[16] = {}, unit_buf[8] = {};
  ha_calendar_format_countdown(secs_until, num_buf, sizeof(num_buf), unit_buf, sizeof(unit_buf));
  if (ctx->icon_lbl) lv_label_set_text(ctx->icon_lbl, "In");
  lv_label_set_text(ctx->value_lbl, num_buf);
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, unit_buf);
  lv_label_set_text(ctx->label_lbl, best->title.c_str());
  set_bg(secs_until <= HA_CALENDAR_URGENT_SECS ? ctx->accent_color : ctx->off_color);
}

// ── Setup and context creation ──────────────────────────────────────────────

inline void setup_ha_calendar_card(BtnSlot &s, const ParsedCfg &p, uint32_t secondary_color) {
  lv_obj_set_style_bg_color(s.btn, lv_color_hex(secondary_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.icon_lbl, "");
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
  ctx->display_mode = cfg_option_value(p.options, "display_mode");
  ctx->modal_layout = cfg_option_value(p.options, "modal_layout");
  ctx->accent_color = accent_color;
  ctx->off_color = off_color;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->value_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->value_font = value_font;
  ctx->label_font = label_font;
  ctx->list_font = list_font ? list_font : label_font;
  ctx->icon_font = icon_font;
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

  return ctx;
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
        media_parse_ha_timestamp(attr, ctx->entities[i].start_epoch);
        ha_calendar_apply_card_face(ctx);
      }));

    ha_subscribe_attribute(eid, std::string("end_time"),
      std::function<void(esphome::StringRef)>([ctx, i](esphome::StringRef attr) {
        media_parse_ha_timestamp(attr, ctx->entities[i].end_epoch);
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
  ui.active = nullptr;
  control_modal_delete_overlay(ControlModalKind::HA_CALENDAR, ui.overlay);
  ui = HaCalendarModalUi();
}

// ── Modal: event parsing ─────────────────────────────────────────────────────

// Parse "H:MM|H:MM|title\n" lines, convert to epochs relative to today's midnight.
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
    std::snprintf(row.start_display, sizeof(row.start_display), "%d:%02d", sh, sm);
    std::snprintf(row.end_display, sizeof(row.end_display), "%d:%02d", eh, em);
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
    if (is_active) {
      lv_obj_set_style_bg_opa(row, LV_OPA_20, LV_PART_MAIN);
      lv_obj_set_style_bg_color(row, lv_color_hex(ctx->accent_color), LV_PART_MAIN);
    } else {
      lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    }

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

    // Top line: title (flex-grow) + countdown
    lv_obj_t *top = lv_obj_create(row);
    lv_obj_set_size(top, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(top, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top, 0, LV_PART_MAIN);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title_lbl = lv_label_create(top);
    lv_label_set_text(title_lbl, ev.title[0] ? ev.title : "(untitled)");
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(title_lbl, 1);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(title_col), LV_PART_MAIN);
    if (ctx->list_font) lv_obj_set_style_text_font(title_lbl, ctx->list_font, LV_PART_MAIN);
    apply_width_compensation(title_lbl, ctx->width_compensation_percent);

    lv_obj_t *cd_lbl = lv_label_create(top);
    lv_label_set_text(cd_lbl, cd);
    lv_obj_set_style_text_color(cd_lbl, lv_color_hex(title_col), LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(cd_lbl, ctx->label_font, LV_PART_MAIN);

    // Time range line
    char time_range[20] = {};
    std::snprintf(time_range, sizeof(time_range), "%s \xe2\x80\x93 %s",
      ev.start_display, ev.end_display);
    lv_obj_t *time_lbl = lv_label_create(row);
    lv_label_set_text(time_lbl, time_range);
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(time_col), LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(time_lbl, ctx->label_font, LV_PART_MAIN);

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
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_START);
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

    lv_obj_t *sl = lv_label_create(tcol);
    lv_label_set_text(sl, ev.start_display);
    lv_obj_set_style_text_color(sl, lv_color_hex(time_col), LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(sl, ctx->label_font, LV_PART_MAIN);

    lv_obj_t *el = lv_label_create(tcol);
    lv_label_set_text(el, ev.end_display);
    lv_obj_set_style_text_color(el, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(el, ctx->label_font, LV_PART_MAIN);

    // Accent bar
    lv_obj_t *bar = lv_obj_create(row);
    lv_obj_set_size(bar, 2, LV_SIZE_CONTENT);
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

    lv_obj_t *title_lbl = lv_label_create(info);
    lv_label_set_text(title_lbl, ev.title[0] ? ev.title : "(untitled)");
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title_lbl, lv_pct(100));
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(title_col), LV_PART_MAIN);
    if (ctx->list_font) lv_obj_set_style_text_font(title_lbl, ctx->list_font, LV_PART_MAIN);
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
    if (ctx->label_font) lv_obj_set_style_text_font(cd_lbl, ctx->label_font, LV_PART_MAIN);

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

// ── Modal: open ──────────────────────────────────────────────────────────────

inline void ha_calendar_open_modal(HaCalendarCardCtx *ctx) {
  if (!ha_calendar_ctx_valid(ctx) || ctx->entities.empty()) return;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::HA_CALENDAR, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0152", false, ha_calendar_modal_hide);

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
  lv_coord_t list_pad = control_modal_scaled_px(14, layout.short_side);
  if (list_pad < 6) list_pad = 6;
  lv_coord_t list_w = content_w - list_pad * 2;
  if (list_w < 80) { list_w = content_w; list_pad = 0; }

  std::string title_text = (ctx->entities.size() == 1)
    ? ha_calendar_card_label(ctx) + " \xc2\xb7 Today"
    : std::string("Today");
  ui.title_lbl = control_modal_create_title(
    ui.panel, title_text, content_w - layout.back_size - gap,
    ctx->list_font, ctx->width_compensation_percent);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_update_layout(ui.title_lbl);
  lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0,
    title_y - lv_obj_get_height(ui.title_lbl) / 2);

  lv_coord_t row_gap = control_modal_scaled_px(6, layout.short_side);
  if (row_gap < 4) row_gap = 4;
  ui.list = control_modal_create_scroll_list(ui.panel, list_w, list_h, row_gap);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset + list_pad, list_y);

  lv_obj_move_foreground(ui.overlay);

  // Kick off one request per entity
  ui.pending_entity_count = static_cast<int>(ctx->entities.size());
  ui.started_ms = esphome::millis();
  ha_calendar_modal_set_status("Loading");
  for (const auto &estate : ctx->entities) {
    ha_calendar_request_events_for_entity(ctx, estate.entity_id);
  }
}
