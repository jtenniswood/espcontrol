#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Home Assistant timer card controls ────────────────────────────────

constexpr int TIMER_DEFAULT_SECONDS = 10 * 60;
constexpr int TIMER_MIN_SECONDS = 60;
constexpr int TIMER_MAX_SECONDS = (99 * 60 + 59) * 60;
constexpr uint32_t TIMER_CARD_CTX_MAGIC = 0x54494D52;  // TIMR

struct TimerCardCtx {
  uint32_t magic = TIMER_CARD_CTX_MAGIC;
  std::string entity_id;
  std::string label;
  std::string state = "idle";
  lv_obj_t *btn = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  lv_timer_t *tick_timer = nullptr;
  lv_obj_t *grid_page = nullptr;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  uint32_t on_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = DEFAULT_OFF_COLOR;
  uint32_t tertiary_color = DEFAULT_TERTIARY_COLOR;
  int width_compensation_percent = 100;
  int default_seconds = TIMER_DEFAULT_SECONDS;
  int duration_seconds = TIMER_DEFAULT_SECONDS;
  int remaining_seconds = TIMER_DEFAULT_SECONDS;
  time_t finishes_at_epoch = 0;
  uint32_t state_updated_ms = 0;
  bool available = true;
};

struct TimerControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *hours_lbl = nullptr;
  lv_obj_t *minutes_lbl = nullptr;
  lv_obj_t *set_btn = nullptr;
  lv_obj_t *pause_btn = nullptr;
  lv_obj_t *resume_btn = nullptr;
  lv_obj_t *cancel_btn = nullptr;
  lv_obj_t *finish_btn = nullptr;
  TimerCardCtx *active = nullptr;
  int selected_seconds = TIMER_DEFAULT_SECONDS;
};

inline TimerControlModalUi &timer_control_modal_ui() {
  static TimerControlModalUi ui;
  return ui;
}

inline bool timer_card_context_valid(TimerCardCtx *ctx) {
  return ctx != nullptr && ctx->magic == TIMER_CARD_CTX_MAGIC;
}

inline int timer_clamp_seconds(int seconds) {
  if (seconds < TIMER_MIN_SECONDS) return TIMER_MIN_SECONDS;
  if (seconds > TIMER_MAX_SECONDS) return TIMER_MAX_SECONDS;
  return seconds;
}

inline int timer_default_seconds_from_options(const std::string &options) {
  std::string minutes_value = cfg_option_value(options, "default_minutes");
  char *end = nullptr;
  long minutes = std::strtol(minutes_value.c_str(), &end, 10);
  if (end == minutes_value.c_str() || minutes < 1) minutes = 10;
  if (minutes > 99 * 60 + 59) minutes = 99 * 60 + 59;
  return static_cast<int>(minutes) * 60;
}

inline bool timer_parse_fixed_int(const char *text, size_t len, size_t pos,
                                  size_t digits, int &out) {
  if (!text || pos + digits > len) return false;
  int value = 0;
  for (size_t i = 0; i < digits; i++) {
    char c = text[pos + i];
    if (c < '0' || c > '9') return false;
    value = value * 10 + (c - '0');
  }
  out = value;
  return true;
}

inline bool timer_parse_duration_text(const std::string &value, int &seconds) {
  if (value.empty() || value == "unknown" || value == "unavailable") return false;
  int parts[3] = {0, 0, 0};
  int part_count = 0;
  size_t start = 0;
  while (start <= value.length() && part_count < 3) {
    size_t end = value.find(':', start);
    if (end == std::string::npos) end = value.length();
    if (end == start) return false;
    char *parse_end = nullptr;
    std::string token = value.substr(start, end - start);
    long parsed = std::strtol(token.c_str(), &parse_end, 10);
    if (parse_end == token.c_str() || parsed < 0) return false;
    parts[part_count++] = static_cast<int>(parsed);
    start = end + 1;
    if (end == value.length()) break;
  }
  if (part_count == 2) {
    seconds = parts[0] * 60 + parts[1];
  } else if (part_count == 3) {
    seconds = parts[0] * 3600 + parts[1] * 60 + parts[2];
  } else {
    seconds = parts[0];
  }
  if (seconds < 0) seconds = 0;
  return true;
}

inline int64_t timer_days_from_civil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

inline bool timer_parse_ha_timestamp(esphome::StringRef value, time_t &epoch) {
  std::string text = string_ref_limited(value, 40);
  const char *s = text.c_str();
  size_t len = text.size();
  if (len < 19 || s[4] != '-' || s[7] != '-' || (s[10] != 'T' && s[10] != ' ')) return false;
  int year, month, day, hour, minute, second;
  if (!timer_parse_fixed_int(s, len, 0, 4, year) ||
      !timer_parse_fixed_int(s, len, 5, 2, month) ||
      !timer_parse_fixed_int(s, len, 8, 2, day) ||
      !timer_parse_fixed_int(s, len, 11, 2, hour) ||
      !timer_parse_fixed_int(s, len, 14, 2, minute) ||
      !timer_parse_fixed_int(s, len, 17, 2, second)) {
    return false;
  }
  if (month < 1 || month > 12 || day < 1 || day > 31 ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
      second < 0 || second > 60) {
    return false;
  }

  size_t tz_pos = 19;
  while (tz_pos < len && s[tz_pos] != 'Z' && s[tz_pos] != '+' && s[tz_pos] != '-') tz_pos++;
  int offset_seconds = 0;
  if (tz_pos < len && (s[tz_pos] == '+' || s[tz_pos] == '-')) {
    int offset_hour, offset_minute;
    if (!timer_parse_fixed_int(s, len, tz_pos + 1, 2, offset_hour) ||
        tz_pos + 3 >= len || s[tz_pos + 3] != ':' ||
        !timer_parse_fixed_int(s, len, tz_pos + 4, 2, offset_minute) ||
        offset_hour > 23 || offset_minute > 59) {
      return false;
    }
    offset_seconds = (offset_hour * 60 + offset_minute) * 60;
    if (s[tz_pos] == '-') offset_seconds = -offset_seconds;
  }

  int64_t days = timer_days_from_civil(year, static_cast<unsigned>(month),
                                       static_cast<unsigned>(day));
  int64_t seconds_since_epoch = days * 86400 + hour * 3600 + minute * 60 + second;
  seconds_since_epoch -= offset_seconds;
  if (seconds_since_epoch < 0) return false;
  epoch = static_cast<time_t>(seconds_since_epoch);
  return true;
}

inline void timer_format_duration(int seconds, char *buf, size_t len) {
  if (!buf || len == 0) return;
  if (seconds < 0) seconds = 0;
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  if (hours > 0) snprintf(buf, len, "%d:%02d:%02d", hours, minutes, secs);
  else snprintf(buf, len, "%d:%02d", minutes, secs);
}

inline int timer_display_remaining_seconds(TimerCardCtx *ctx) {
  if (!ctx) return TIMER_DEFAULT_SECONDS;
  if (ctx->state == "active") {
    time_t now_epoch = std::time(nullptr);
    if (ctx->finishes_at_epoch > 0 && now_epoch > 0) {
      int remaining = static_cast<int>(ctx->finishes_at_epoch - now_epoch);
      return remaining < 0 ? 0 : remaining;
    }
    uint32_t elapsed = ctx->state_updated_ms ? (esphome::millis() - ctx->state_updated_ms) / 1000 : 0;
    return ctx->remaining_seconds > static_cast<int>(elapsed)
      ? ctx->remaining_seconds - static_cast<int>(elapsed)
      : 0;
  }
  if (ctx->state == "paused") return ctx->remaining_seconds;
  return ctx->default_seconds;
}

inline std::string timer_state_label(const std::string &state) {
  if (state == "active") return espcontrol_i18n(std::string("Active"));
  if (state == "paused") return espcontrol_i18n(std::string("Paused"));
  if (state == "idle") return espcontrol_i18n(std::string("Idle"));
  if (state == "unavailable") return espcontrol_i18n(std::string("Unavailable"));
  if (state == "unknown" || state.empty()) return espcontrol_i18n(std::string("Unknown"));
  return sentence_cap_text(state);
}

inline void timer_update_modal(TimerCardCtx *ctx);

inline void timer_update_card(TimerCardCtx *ctx) {
  if (!timer_card_context_valid(ctx)) return;
  if (ctx->state == "unknown" || ctx->state == "unavailable") {
    if (ctx->value_lbl) lv_label_set_text(ctx->value_lbl, timer_state_label(ctx->state).c_str());
    if (ctx->btn) lv_obj_add_state(ctx->btn, LV_STATE_DISABLED);
  } else {
    char buf[20];
    timer_format_duration(timer_display_remaining_seconds(ctx), buf, sizeof(buf));
    if (ctx->value_lbl) lv_label_set_text(ctx->value_lbl, buf);
    if (ctx->btn) lv_obj_clear_state(ctx->btn, LV_STATE_DISABLED);
  }
  if (ctx->label_lbl) {
    if (ctx->state == "active" || ctx->state == "paused") {
      lv_label_set_text(ctx->label_lbl, timer_state_label(ctx->state).c_str());
    } else {
      lv_label_set_text(ctx->label_lbl, ctx->label.c_str());
    }
  }
  if (ctx->btn) set_card_checked_state(ctx->btn, ctx->state == "active");
  timer_update_modal(ctx);
}

inline void timer_tick_cb(lv_timer_t *timer) {
  TimerCardCtx *ctx = static_cast<TimerCardCtx *>(lv_timer_get_user_data(timer));
  if (!timer_card_context_valid(ctx)) return;
  if (ctx->state == "active") timer_update_card(ctx);
}

inline void setup_timer_card(BtnSlot &s, const ParsedCfg &p) {
  if (s.icon_lbl) {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  if (s.sensor_container) {
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(s.sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_move_foreground(s.sensor_container);
  }
  if (s.sensor_lbl) {
    char buf[20];
    timer_format_duration(timer_default_seconds_from_options(p.options), buf, sizeof(buf));
    lv_label_set_text(s.sensor_lbl, buf);
  }
  if (s.unit_lbl) lv_label_set_text(s.unit_lbl, "");
  if (s.text_lbl) {
    lv_label_set_text(s.text_lbl, p.label.empty() ? espcontrol_i18n("Timer") : p.label.c_str());
    lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    configure_button_label_wrap(s.text_lbl);
    lv_obj_move_foreground(s.text_lbl);
  }
  apply_push_button_transition(s.btn);
}

inline TimerCardCtx *create_timer_card_context(BtnSlot &s, const ParsedCfg &p,
                                               uint32_t on_color,
                                               uint32_t off_color,
                                               uint32_t tertiary_color,
                                               const lv_font_t *number_font,
                                               const lv_font_t *label_font,
                                               const lv_font_t *icon_font,
                                               int width_compensation_percent) {
  TimerCardCtx *ctx = new TimerCardCtx();
  ctx->entity_id = p.entity;
  ctx->label = p.label.empty() ? espcontrol_i18n(std::string("Timer")) : p.label;
  ctx->btn = s.btn;
  ctx->value_lbl = s.sensor_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->number_font = number_font;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->on_color = on_color;
  ctx->off_color = off_color;
  ctx->tertiary_color = tertiary_color;
  ctx->width_compensation_percent = width_compensation_percent;
  ctx->default_seconds = timer_default_seconds_from_options(p.options);
  ctx->duration_seconds = ctx->default_seconds;
  ctx->remaining_seconds = ctx->default_seconds;
  ctx->state_updated_ms = esphome::millis();
  ctx->tick_timer = lv_timer_create(timer_tick_cb, 1000, ctx);
  if (ctx->tick_timer) lv_timer_pause(ctx->tick_timer);
  lv_obj_set_user_data(s.btn, ctx);
  return ctx;
}

inline void timer_send_start_action(TimerCardCtx *ctx, int seconds) {
  if (!timer_card_context_valid(ctx)) return;
  seconds = timer_clamp_seconds(seconds);
  char duration[16];
  snprintf(duration, sizeof(duration), "%02d:%02d:%02d",
           seconds / 3600, (seconds % 3600) / 60, seconds % 60);
  ha_send_entity_action(ctx->entity_id, "timer.start", "duration", duration);
}

inline void timer_send_simple_action(TimerCardCtx *ctx, const char *service) {
  if (!timer_card_context_valid(ctx)) return;
  ha_send_entity_action(ctx->entity_id, service);
}

inline void timer_send_resume_action(TimerCardCtx *ctx) {
  if (!timer_card_context_valid(ctx)) return;
  ha_send_entity_action(ctx->entity_id, "timer.start");
}

inline void timer_modal_refresh_duration_labels() {
  TimerControlModalUi &ui = timer_control_modal_ui();
  int selected = timer_clamp_seconds(ui.selected_seconds);
  int total_minutes = selected / 60;
  int hours = total_minutes / 60;
  int minutes = total_minutes % 60;
  char buf[8];
  if (ui.hours_lbl) {
    snprintf(buf, sizeof(buf), "%02d", hours);
    lv_label_set_text(ui.hours_lbl, buf);
  }
  if (ui.minutes_lbl) {
    snprintf(buf, sizeof(buf), "%02d", minutes);
    lv_label_set_text(ui.minutes_lbl, buf);
  }
}

inline void timer_adjust_selected_duration(int delta_seconds) {
  TimerControlModalUi &ui = timer_control_modal_ui();
  ui.selected_seconds = timer_clamp_seconds(ui.selected_seconds + delta_seconds);
  timer_modal_refresh_duration_labels();
}

inline void timer_set_obj_visible(lv_obj_t *obj, bool visible) {
  if (!obj) return;
  if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

inline void timer_update_modal(TimerCardCtx *ctx) {
  TimerControlModalUi &ui = timer_control_modal_ui();
  if (!timer_card_context_valid(ctx) || ui.active != ctx) return;
  bool active = ctx->state == "active";
  bool paused = ctx->state == "paused";
  timer_set_obj_visible(ui.pause_btn, active);
  timer_set_obj_visible(ui.resume_btn, paused);
  timer_set_obj_visible(ui.cancel_btn, active || paused);
  timer_set_obj_visible(ui.finish_btn, active || paused);
}

inline lv_obj_t *timer_create_picker_button(lv_obj_t *parent,
                                            const char *icon,
                                            const lv_font_t *icon_font,
                                            uint32_t color,
                                            lv_coord_t size,
                                            int width_compensation_percent) {
  return control_modal_create_round_button(
    parent, size, icon, icon_font, DARK_BORDER, color, width_compensation_percent);
}

inline void timer_control_hide_modal() {
  TimerControlModalUi &ui = timer_control_modal_ui();
  control_modal_delete_overlay(ControlModalKind::TIMER_CONTROL, ui.overlay);
  ui = TimerControlModalUi();
}

inline void timer_card_open_modal(TimerCardCtx *ctx) {
  if (!timer_card_context_valid(ctx)) return;
  TimerControlModalUi &ui = timer_control_modal_ui();
  ui = TimerControlModalUi();
  ui.active = ctx;
  ui.selected_seconds = timer_clamp_seconds(
    (ctx->state == "active" || ctx->state == "paused")
      ? timer_display_remaining_seconds(ctx)
      : ctx->default_seconds);

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::TIMER_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0156", true, timer_control_hide_modal);
  if (!shell.overlay || !shell.panel) return;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;

  lv_coord_t content_w = shell.content_w;
  lv_coord_t gap = shell.layout.controls_gap;
  if (gap < 10) gap = 10;
  lv_coord_t column_w = (content_w - gap) / 2;
  lv_coord_t button_size = shell.layout.btn_size;
  lv_coord_t value_h = ctx->number_font && ctx->number_font->line_height > 0
    ? ctx->number_font->line_height + shell.layout.inset / 2
    : button_size;

  ui.title_lbl = control_modal_create_title(
    ui.panel, ctx->label, content_w, ctx->label_font, ctx->width_compensation_percent);
  lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, shell.layout.inset + shell.layout.back_size / 2);

  lv_obj_t *hours_col = lv_obj_create(ui.panel);
  lv_obj_t *minutes_col = lv_obj_create(ui.panel);
  lv_obj_t *columns[2] = {hours_col, minutes_col};
  for (lv_obj_t *col : columns) {
    lv_obj_set_size(col, column_w, button_size * 2 + value_h + gap * 2);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(col, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(col, 0, LV_PART_MAIN);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
  }
  lv_obj_align(hours_col, LV_ALIGN_CENTER, -(column_w + gap) / 2, -shell.layout.inset / 2);
  lv_obj_align(minutes_col, LV_ALIGN_CENTER, (column_w + gap) / 2, -shell.layout.inset / 2);

  lv_obj_t *hours_up = timer_create_picker_button(
    hours_col, find_icon("Chevron Up"), ctx->icon_font, ctx->tertiary_color,
    button_size, ctx->width_compensation_percent);
  lv_obj_t *hours_down = timer_create_picker_button(
    hours_col, find_icon("Chevron Down"), ctx->icon_font, ctx->tertiary_color,
    button_size, ctx->width_compensation_percent);
  lv_obj_t *minutes_up = timer_create_picker_button(
    minutes_col, find_icon("Chevron Up"), ctx->icon_font, ctx->tertiary_color,
    button_size, ctx->width_compensation_percent);
  lv_obj_t *minutes_down = timer_create_picker_button(
    minutes_col, find_icon("Chevron Down"), ctx->icon_font, ctx->tertiary_color,
    button_size, ctx->width_compensation_percent);

  ui.hours_lbl = lv_label_create(hours_col);
  ui.minutes_lbl = lv_label_create(minutes_col);
  lv_obj_t *labels[2] = {ui.hours_lbl, ui.minutes_lbl};
  for (lv_obj_t *label : labels) {
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->number_font) lv_obj_set_style_text_font(label, ctx->number_font, LV_PART_MAIN);
    lv_obj_set_width(label, column_w);
  }

  lv_obj_align(hours_up, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_align(ui.hours_lbl, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align(hours_down, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_align(minutes_up, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_align(ui.minutes_lbl, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align(minutes_down, LV_ALIGN_BOTTOM_MID, 0, 0);
  timer_modal_refresh_duration_labels();

  lv_obj_add_event_cb(hours_up, [](lv_event_t *) { timer_adjust_selected_duration(3600); },
                      LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(hours_down, [](lv_event_t *) { timer_adjust_selected_duration(-3600); },
                      LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(minutes_up, [](lv_event_t *) { timer_adjust_selected_duration(60); },
                      LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(minutes_down, [](lv_event_t *) { timer_adjust_selected_duration(-60); },
                      LV_EVENT_CLICKED, nullptr);

  lv_coord_t action_h = shell.layout.btn_size * 3 / 5;
  if (action_h < 42) action_h = 42;
  lv_coord_t action_min_w = content_w / 3;
  if (action_min_w < 96) action_min_w = 96;
  lv_coord_t action_max_w = content_w;
  lv_coord_t action_y = -shell.layout.inset;

  ui.set_btn = control_modal_create_text_button(
    ui.panel, espcontrol_i18n(std::string("Set Timer")), action_max_w, action_min_w,
    action_h, action_h / 2, ctx->on_color, ctx->label_font);
  ui.pause_btn = control_modal_create_text_button(
    ui.panel, espcontrol_i18n(std::string("Pause")), action_max_w, action_min_w,
    action_h, action_h / 2, ctx->tertiary_color, ctx->label_font);
  ui.resume_btn = control_modal_create_text_button(
    ui.panel, espcontrol_i18n(std::string("Resume")), action_max_w, action_min_w,
    action_h, action_h / 2, ctx->on_color, ctx->label_font);
  ui.cancel_btn = control_modal_create_text_button(
    ui.panel, espcontrol_i18n(std::string("Cancel")), action_max_w, action_min_w,
    action_h, action_h / 2, ctx->off_color, ctx->label_font);
  ui.finish_btn = control_modal_create_text_button(
    ui.panel, espcontrol_i18n(std::string("Finish")), action_max_w, action_min_w,
    action_h, action_h / 2, ctx->tertiary_color, ctx->label_font);

  lv_obj_align(ui.set_btn, LV_ALIGN_BOTTOM_MID, 0, action_y - action_h - gap);
  lv_obj_align(ui.pause_btn, LV_ALIGN_BOTTOM_LEFT, shell.layout.inset, action_y);
  lv_obj_align(ui.resume_btn, LV_ALIGN_BOTTOM_LEFT, shell.layout.inset, action_y);
  lv_obj_align(ui.cancel_btn, LV_ALIGN_BOTTOM_MID, 0, action_y);
  lv_obj_align(ui.finish_btn, LV_ALIGN_BOTTOM_RIGHT, -shell.layout.inset, action_y);

  lv_obj_add_event_cb(ui.set_btn, [](lv_event_t *) {
    TimerControlModalUi &ui = timer_control_modal_ui();
    timer_send_start_action(ui.active, ui.selected_seconds);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.pause_btn, [](lv_event_t *) {
    timer_send_simple_action(timer_control_modal_ui().active, "timer.pause");
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.resume_btn, [](lv_event_t *) {
    timer_send_resume_action(timer_control_modal_ui().active);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.cancel_btn, [](lv_event_t *) {
    timer_send_simple_action(timer_control_modal_ui().active, "timer.cancel");
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.finish_btn, [](lv_event_t *) {
    timer_send_simple_action(timer_control_modal_ui().active, "timer.finish");
  }, LV_EVENT_CLICKED, nullptr);

  timer_update_modal(ctx);
  if (ui.close_btn) lv_obj_move_foreground(ui.close_btn);
}

inline void subscribe_timer_card_state(TimerCardCtx *ctx) {
  if (!timer_card_context_valid(ctx) || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    [ctx](std::string state) {
      if (!timer_card_context_valid(ctx)) return;
      ctx->state = state.empty() ? "unknown" : state;
      ctx->available = ctx->state != "unknown" && ctx->state != "unavailable";
      ctx->state_updated_ms = esphome::millis();
      if (ctx->state == "idle") {
        ctx->remaining_seconds = ctx->default_seconds;
        ctx->finishes_at_epoch = 0;
      }
      if (ctx->tick_timer) {
        if (ctx->state == "active") lv_timer_resume(ctx->tick_timer);
        else lv_timer_pause(ctx->tick_timer);
      }
      timer_update_card(ctx);
    });
  ha_subscribe_attribute(
    ctx->entity_id, std::string("duration"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        if (!timer_card_context_valid(ctx)) return;
        int seconds = 0;
        if (timer_parse_duration_text(string_ref_limited(value, 32), seconds) && seconds > 0) {
          ctx->duration_seconds = timer_clamp_seconds(seconds);
          if (ctx->state == "idle") ctx->remaining_seconds = ctx->duration_seconds;
        }
        timer_update_card(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("remaining"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        if (!timer_card_context_valid(ctx)) return;
        int seconds = 0;
        if (timer_parse_duration_text(string_ref_limited(value, 32), seconds)) {
          ctx->remaining_seconds = timer_clamp_seconds(seconds);
          ctx->state_updated_ms = esphome::millis();
        }
        timer_update_card(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("finishes_at"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        if (!timer_card_context_valid(ctx)) return;
        time_t finish_epoch = 0;
        if (timer_parse_ha_timestamp(value, finish_epoch)) ctx->finishes_at_epoch = finish_epoch;
        else ctx->finishes_at_epoch = 0;
        timer_update_card(ctx);
      })
  );
}
