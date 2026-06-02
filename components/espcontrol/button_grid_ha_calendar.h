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
