#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Timer card.
// Counts down a HA timer.* entity. Tap idle/paused = start, tap active =
// cancel, with an optional second tap confirmation before cancel.

struct TimerConfirmationCtx {
  lv_obj_t *prompt_lbl = nullptr;
  std::string original_text;
  std::function<void()> on_confirm;
  lv_timer_t *timeout_timer = nullptr;
  bool armed = false;
};

inline void timer_confirmation_disarm(TimerConfirmationCtx *ctx) {
  if (!ctx || !ctx->armed) return;
  ctx->armed = false;
  if (ctx->timeout_timer) {
    lv_timer_del(ctx->timeout_timer);
    ctx->timeout_timer = nullptr;
  }
  if (ctx->prompt_lbl)
    lv_label_set_text(ctx->prompt_lbl, ctx->original_text.c_str());
  ctx->on_confirm = nullptr;
}

inline void timer_confirmation_timeout_cb(lv_timer_t *timer) {
  TimerConfirmationCtx *ctx =
    static_cast<TimerConfirmationCtx *>(lv_timer_get_user_data(timer));
  if (!ctx) return;
  ctx->timeout_timer = nullptr;
  ctx->armed = false;
  if (ctx->prompt_lbl)
    lv_label_set_text(ctx->prompt_lbl, ctx->original_text.c_str());
  ctx->on_confirm = nullptr;
}

inline bool timer_confirmation_try(TimerConfirmationCtx *ctx, lv_obj_t *prompt_lbl,
                                   const char *prompt_text, uint16_t timeout_secs,
                                   std::function<void()> on_confirm) {
  if (!ctx) {
    if (on_confirm) on_confirm();
    return true;
  }
  if (ctx->armed) {
    auto cb = ctx->on_confirm;
    timer_confirmation_disarm(ctx);
    if (cb) cb();
    return true;
  }
  ctx->prompt_lbl = prompt_lbl;
  if (prompt_lbl) {
    const char *current = lv_label_get_text(prompt_lbl);
    ctx->original_text = current ? current : "";
    lv_label_set_text(prompt_lbl, prompt_text ? prompt_text : "Confirm?");
  } else {
    ctx->original_text.clear();
  }
  ctx->on_confirm = on_confirm;
  ctx->armed = true;
  uint32_t period = static_cast<uint32_t>(timeout_secs ? timeout_secs : 3) * 1000;
  ctx->timeout_timer = lv_timer_create(timer_confirmation_timeout_cb, period, ctx);
  if (ctx->timeout_timer) lv_timer_set_repeat_count(ctx->timeout_timer, 1);
  return false;
}

struct TimerCardCtx {
  std::string entity_id;
  lv_obj_t *btn = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  std::string state;
  int duration_secs = 0;
  int remaining_secs = 0;
  uint32_t remaining_anchor_ms = 0;
  uint32_t finished_at_ms = 0;
  uint32_t generation = 0;
  time_t finishes_at_epoch = 0;
  lv_timer_t *tick_timer = nullptr;
  bool confirm_enabled = false;
  uint16_t confirm_timeout_secs = 3;
  TimerConfirmationCtx confirm;
};

inline int parse_timer_hms(esphome::StringRef value) {
  int hours = 0;
  int minutes = 0;
  int seconds = 0;
  if (sscanf(value.c_str(), "%d:%d:%d", &hours, &minutes, &seconds) == 3)
    return hours * 3600 + minutes * 60 + seconds;
  return 0;
}

inline time_t parse_timer_iso8601_to_epoch(esphome::StringRef value) {
  const char *source = value.c_str();
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int consumed = 0;
  if (sscanf(source, "%4d-%2d-%2dT%2d:%2d:%2d%n",
             &year, &month, &day, &hour, &minute, &second, &consumed) != 6)
    return 0;

  const char *cursor = source + consumed;
  if (*cursor == '.') {
    cursor++;
    while (*cursor >= '0' && *cursor <= '9') cursor++;
  }

  int timezone_offset = 0;
  if (*cursor == 'Z') {
    timezone_offset = 0;
  } else if (*cursor == '+' || *cursor == '-') {
    int sign = (*cursor == '-') ? -1 : 1;
    int offset_hours = 0;
    int offset_minutes = 0;
    if (sscanf(cursor + 1, "%2d:%2d", &offset_hours, &offset_minutes) != 2)
      return 0;
    timezone_offset = sign * (offset_hours * 3600 + offset_minutes * 60);
  }

  int civil_year = (month <= 2) ? year - 1 : year;
  int era = (civil_year >= 0 ? civil_year : civil_year - 399) / 400;
  unsigned year_of_era = static_cast<unsigned>(civil_year - era * 400);
  unsigned day_of_year =
    (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  unsigned day_of_era =
    year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
  long days = static_cast<long>(era) * 146097 + static_cast<long>(day_of_era) - 719468;
  return static_cast<time_t>(days) * 86400 + hour * 3600 + minute * 60 + second -
         timezone_offset;
}

inline void format_timer_secs(int seconds, char *buffer, size_t buffer_size) {
  if (seconds < 0) seconds = 0;
  if (seconds >= 3600) {
    int hours = seconds / 3600;
    int minutes = (seconds / 60) % 60;
    snprintf(buffer, buffer_size, "%d:%02d", hours, minutes);
  } else {
    int minutes = seconds / 60;
    int remaining_seconds = seconds % 60;
    snprintf(buffer, buffer_size, "%d:%02d", minutes, remaining_seconds);
  }
}

inline void timer_card_refresh(TimerCardCtx *ctx) {
  if (!ctx || !ctx->value_lbl) return;
  int seconds = 0;
  if (ctx->state == "active") {
    time_t now_epoch = ::time(nullptr);
    if (ctx->finishes_at_epoch != 0 && now_epoch > 100000) {
      seconds = static_cast<int>(ctx->finishes_at_epoch - now_epoch);
    } else {
      int elapsed = static_cast<int>((esphome::millis() - ctx->remaining_anchor_ms) / 1000);
      seconds = ctx->remaining_secs - elapsed;
    }
    if (seconds < 0) seconds = 0;
  } else if (ctx->state == "paused") {
    seconds = ctx->remaining_secs;
  } else {
    bool just_finished = ctx->finished_at_ms != 0 &&
      (esphome::millis() - ctx->finished_at_ms) < 5000;
    seconds = just_finished ? 0 : ctx->duration_secs;
  }

  char buffer[16];
  format_timer_secs(seconds, buffer, sizeof(buffer));
  lv_label_set_text(ctx->value_lbl, buffer);
}

inline void timer_card_tick_cb(lv_timer_t *timer) {
  TimerCardCtx *ctx = static_cast<TimerCardCtx *>(lv_timer_get_user_data(timer));
  if (!ctx || ctx->generation != ha_subscription_generation()) return;
  timer_card_refresh(ctx);
}

inline void subscribe_timer_card(TimerCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;

  ha_subscribe_state(
    ctx->entity_id,
    [ctx](esphome::StringRef state) {
      std::string previous = ctx->state;
      ctx->state = std::string(state.c_str(), state.size());
      if (ctx->state != "active") timer_confirmation_disarm(&ctx->confirm);
      if (ctx->btn) set_card_checked_state(ctx->btn, ctx->state == "active");

      if (previous == "active" && ctx->state == "idle") {
        int elapsed = static_cast<int>((esphome::millis() - ctx->remaining_anchor_ms) / 1000);
        int estimated_remaining = ctx->remaining_secs - elapsed;
        if (estimated_remaining < 2) {
          ctx->finished_at_ms = esphome::millis();
          if (ctx->finished_at_ms == 0) ctx->finished_at_ms = 1;
        } else {
          ctx->finished_at_ms = 0;
        }
      } else if (ctx->state == "active") {
        ctx->finished_at_ms = 0;
      }
      if (ctx->state != "active") ctx->finishes_at_epoch = 0;
      timer_card_refresh(ctx);
    });

  ha_subscribe_attribute(
    ctx->entity_id,
    "duration",
    [ctx](esphome::StringRef state) {
      ctx->duration_secs = parse_timer_hms(state);
      timer_card_refresh(ctx);
    });

  ha_subscribe_attribute(
    ctx->entity_id,
    "remaining",
    [ctx](esphome::StringRef state) {
      ctx->remaining_secs = parse_timer_hms(state);
      ctx->remaining_anchor_ms = esphome::millis();
      timer_card_refresh(ctx);
    });

  ha_subscribe_attribute(
    ctx->entity_id,
    "finishes_at",
    [ctx](esphome::StringRef state) {
      ctx->finishes_at_epoch =
        state.size() == 0 || state.c_str()[0] == '\0'
          ? 0
          : parse_timer_iso8601_to_epoch(state);
      timer_card_refresh(ctx);
    });
}

inline void handle_timer_card_click(TimerCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  if (ctx->state == "active") {
    std::string entity_id = ctx->entity_id;
    auto cancel = [entity_id]() { ha_send_entity_action(entity_id, "timer.cancel"); };
    if (ctx->confirm_enabled) {
      timer_confirmation_try(&ctx->confirm, ctx->text_lbl, espcontrol_i18n("Confirm?"),
                             ctx->confirm_timeout_secs, cancel);
    } else {
      cancel();
    }
  } else {
    timer_confirmation_disarm(&ctx->confirm);
    ha_send_entity_action(ctx->entity_id, "timer.start");
  }
}

inline void setup_timer_card(BtnSlot &slot, const ParsedCfg &cfg,
                             const lv_font_t *value_font) {
  lv_obj_add_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (value_font)
    lv_obj_set_style_text_font(slot.sensor_lbl, value_font, LV_PART_MAIN);
  lv_label_set_text(slot.sensor_lbl, "0:00");
  lv_label_set_text(slot.unit_lbl, "");
  std::string label = cfg.label.empty() ? espcontrol_i18n(std::string("Timer")) : cfg.label;
  lv_label_set_text(slot.text_lbl, label.c_str());
}

inline uint16_t timer_parse_confirm_timeout(const std::string &unit) {
  int value = atoi(unit.c_str());
  if (value < 1) value = 3;
  if (value > 30) value = 30;
  return static_cast<uint16_t>(value);
}

inline TimerCardCtx *create_timer_card_context(BtnSlot &slot, const ParsedCfg &cfg) {
  TimerCardCtx *ctx = new TimerCardCtx();
  ctx->entity_id = cfg.entity;
  ctx->btn = slot.btn;
  ctx->value_lbl = slot.sensor_lbl;
  ctx->text_lbl = slot.text_lbl;
  ctx->generation = ha_subscription_generation();
  ctx->confirm_enabled = cfg.sensor == "confirm";
  ctx->confirm_timeout_secs = timer_parse_confirm_timeout(cfg.unit);
  lv_obj_set_user_data(slot.btn, ctx);
  subscribe_timer_card(ctx);
  ctx->tick_timer = lv_timer_create(timer_card_tick_cb, 250, ctx);
  return ctx;
}
