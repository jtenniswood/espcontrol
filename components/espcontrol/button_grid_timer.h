#pragma once

// Timer card ported from benJDtom/espcontrol PR #136.
// ── Reusable confirmation prompt ──────────────────────────────────────
// Two-tap confirmation for destructive actions. Captures a label's text,
// swaps in a prompt ("Confirm"), and arms a one-shot timeout. A second
// tap within the window invokes the supplied callback; otherwise the
// timeout restores the original text and disarms.
struct ConfirmationCtx {
  lv_obj_t *prompt_lbl = nullptr;
  std::string original_text;
  std::function<void()> on_confirm;
  lv_timer_t *timeout_timer = nullptr;
  bool armed = false;
};

inline void confirmation_disarm(ConfirmationCtx *c) {
  if (!c || !c->armed) return;
  c->armed = false;
  if (c->timeout_timer) {
    lv_timer_del(c->timeout_timer);
    c->timeout_timer = nullptr;
  }
  if (c->prompt_lbl)
    lv_label_set_display_text(c->prompt_lbl, c->original_text.c_str());
  c->on_confirm = nullptr;
}

inline void confirmation_timeout_cb(lv_timer_t *t) {
  ConfirmationCtx *c = (ConfirmationCtx *)lv_timer_get_user_data(t);
  if (!c) return;
  // The timer is one-shot via lv_timer_set_repeat_count(1); LVGL deletes it
  // automatically after this callback returns. Drop our pointer before
  // calling disarm so it does not double-delete.
  c->timeout_timer = nullptr;
  c->armed = false;
  if (c->prompt_lbl)
    lv_label_set_display_text(c->prompt_lbl, c->original_text.c_str());
  c->on_confirm = nullptr;
}

// Returns true if the action was performed (second tap), false if armed.
inline bool confirmation_try(ConfirmationCtx *c, lv_obj_t *prompt_lbl,
                             const char *prompt_text, uint16_t timeout_secs,
                             std::function<void()> on_confirm) {
  if (!c) {
    if (on_confirm) on_confirm();
    return true;
  }
  if (c->armed) {
    auto cb = c->on_confirm;
    confirmation_disarm(c);
    if (cb) cb();
    return true;
  }
  c->prompt_lbl = prompt_lbl;
  if (prompt_lbl) {
    const char *cur = lv_label_get_text(prompt_lbl);
    c->original_text = cur ? cur : "";
    lv_label_set_display_text(prompt_lbl, prompt_text ? prompt_text : espcontrol_i18n("Confirm"));
  } else {
    c->original_text.clear();
  }
  c->on_confirm = on_confirm;
  c->armed = true;
  uint32_t period = (uint32_t)(timeout_secs ? timeout_secs : 3) * 1000;
  c->timeout_timer = lv_timer_create(confirmation_timeout_cb, period, c);
  if (c->timeout_timer) lv_timer_set_repeat_count(c->timeout_timer, 1);
  else confirmation_disarm(c);
  return false;
}

// ── Timer card ────────────────────────────────────────────────────────
// Counts down a HA timer.* entity. Tap idle/paused = start (paused = resume),
// tap active = cancel (optionally with confirmation prompt).
struct TimerCardCtx {
  std::string entity_id;
  lv_obj_t *btn = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  std::string state;       // "idle" / "active" / "paused" / ""
  int duration_secs = 0;
  int remaining_secs = 0;
  uint32_t remaining_anchor_ms = 0;
  uint32_t finished_at_ms = 0;  // millis() when state went active → idle; 0 otherwise
  time_t finishes_at_epoch = 0; // wall-clock epoch when an active timer fires; 0 if unknown
  lv_timer_t *tick_timer = nullptr;
  bool confirm_enabled = false;
  uint16_t confirm_timeout_secs = 3;
  ConfirmationCtx confirm;
  bool cancel_requested = false;
  void detach() {
    ha_release_callbacks_for_owner(this);
    if (tick_timer) lv_timer_del(tick_timer);
    tick_timer = nullptr;
    if (confirm.timeout_timer) lv_timer_del(confirm.timeout_timer);
    confirm.timeout_timer = nullptr;
    confirm.armed = false;
    confirm.on_confirm = nullptr;
    value_lbl = nullptr;
    text_lbl = nullptr;
    btn = nullptr;
  }
  ~TimerCardCtx() { detach(); }
};

inline int parse_timer_hms(esphome::StringRef value) {
  // HA timer attrs are formatted "H:MM:SS" (e.g. "0:05:00").
  const std::string text(value.c_str(), value.size());
  int h = 0, m = 0, s = 0, n = 0;
  if (sscanf(text.c_str(), "%d:%d:%d%n", &h, &m, &s, &n) == 3 &&
      n == (int)text.size() && h >= 0 && h <= 596522 &&
      m >= 0 && m < 60 && s >= 0 && s < 60)
    return h * 3600 + m * 60 + s;
  return 0;
}

// Parse HA's ISO 8601 finishes_at (e.g. "2026-05-08T12:34:56.789012-06:00" or
// "...Z") into a unix epoch. Returns 0 on failure.
inline time_t parse_iso8601_to_epoch(esphome::StringRef value) {
  const std::string text(value.c_str(), value.size());
  const char *s = text.c_str();
  int Y, Mo, D, h, mi, sec;
  int n = 0;
  if (sscanf(s, "%4d-%2d-%2dT%2d:%2d:%2d%n", &Y, &Mo, &D, &h, &mi, &sec, &n) != 6)
    return 0;
  if (Y < 1970 || Y > 9999 || Mo < 1 || Mo > 12 || D < 1 || D > 31 ||
      h < 0 || h > 23 || mi < 0 || mi > 59 || sec < 0 || sec > 59) return 0;
  const char *p = s + n;
  // Skip optional fractional seconds.
  if (*p == '.') {
    ++p;
    while (*p >= '0' && *p <= '9') ++p;
  }
  int tz_off = 0;
  if (*p == 'Z') {
    if (p[1] != '\0') return 0;
    tz_off = 0;
  } else if (*p == '+' || *p == '-') {
    int sign = (*p == '-') ? -1 : 1;
    int th = 0, tm = 0;
    int consumed = 0;
    if (sscanf(p + 1, "%2d:%2d%n", &th, &tm, &consumed) != 2 ||
        p[1 + consumed] != '\0' || th > 23 || tm > 59) return 0;
    tz_off = sign * (th * 3600 + tm * 60);
  } else return 0;
  // timegm() isn't always available on ESP toolchain; compute UTC manually.
  // Days since 1970-01-01 using civil_from_days style algorithm.
  int y = (Mo <= 2) ? Y - 1 : Y;
  int era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (Mo + (Mo > 2 ? -3 : 9)) + 2) / 5 + D - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = (long)era * 146097 + (long)doe - 719468;
  time_t t = (time_t)days * 86400 + h * 3600 + mi * 60 + sec - tz_off;
  return t;
}

inline void format_timer_secs(int secs, char *buf, size_t bufsz) {
  if (secs < 0) secs = 0;
  if (secs >= 3600) {
    // Drop seconds at or above one hour so the string stays short enough
    // to fit the card. Reverts to M:SS automatically once the timer drops
    // back below an hour.
    int h = secs / 3600;
    int m = (secs / 60) % 60;
    snprintf(buf, bufsz, "%d:%02d", h, m);
  } else {
    int m = secs / 60;
    int s = secs % 60;
    snprintf(buf, bufsz, "%d:%02d", m, s);
  }
}

inline void timer_card_refresh(TimerCardCtx *ctx) {
  if (!ctx || !ctx->value_lbl) return;
  int secs;
  if (ctx->state == "active") {
    // Prefer wall clock vs. HA's absolute finishes_at when system time is
    // synced. This stays accurate across device restarts mid-countdown,
    // where HA's `remaining` is stale until the next state change.
    time_t now_epoch = ::time(nullptr);
    if (ctx->finishes_at_epoch != 0 && now_epoch > 100000) {
      secs = (int)(ctx->finishes_at_epoch - now_epoch);
    } else {
      int elapsed = (int)((esphome::millis() - ctx->remaining_anchor_ms) / 1000);
      secs = ctx->remaining_secs - elapsed;
    }
    if (secs < 0) secs = 0;
  } else if (ctx->state == "paused") {
    secs = ctx->remaining_secs;
  } else if (ctx->state != "idle") {
    lv_label_set_display_text(ctx->value_lbl, "--:--");
    return;
  } else {
    // Idle: hold at 0:00 for a short grace window after a timer completes
    // (HA flips to idle just before the visible countdown reaches zero),
    // then revert to the configured duration so the user can see what the
    // next start will count down from.
    bool just_finished = ctx->finished_at_ms != 0 &&
      (esphome::millis() - ctx->finished_at_ms) < 5000;
    secs = just_finished ? 0 : ctx->duration_secs;
  }
  char buf[16];
  format_timer_secs(secs, buf, sizeof(buf));
  lv_label_set_display_text(ctx->value_lbl, buf);
}

inline void timer_card_tick_cb(lv_timer_t *t) {
  TimerCardCtx *ctx = (TimerCardCtx *)lv_timer_get_user_data(t);
  if (ctx) timer_card_refresh(ctx);
}

inline void subscribe_timer_card(TimerCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  HaCallbackOwnerScope callback_owner(ctx);
  // Main state
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        std::string prev = ctx->state;
        ctx->state = std::string(state.c_str(), state.size());
        // Cancel any pending confirmation if state changed away from active.
        if (ctx->state != "active") confirmation_disarm(&ctx->confirm);
        if (ctx->btn) {
          if (ctx->state == "active") lv_obj_add_state(ctx->btn, LV_STATE_CHECKED);
          else lv_obj_clear_state(ctx->btn, LV_STATE_CHECKED);
        }
        if (prev == "active" && ctx->state == "idle") {
          // Distinguish a natural finish (countdown reached zero) from a
          // cancel (user tapped while running). At a natural finish the
          // estimated remaining is ~0; on cancel it's still well above 0.
          int elapsed = (int)((esphome::millis() - ctx->remaining_anchor_ms) / 1000);
          int est_remaining = ctx->remaining_secs - elapsed;
          if (ctx->finishes_at_epoch && ::time(nullptr) > 100000)
            est_remaining = (int)(ctx->finishes_at_epoch - ::time(nullptr));
          if (!ctx->cancel_requested && est_remaining < 2) {
            ctx->finished_at_ms = esphome::millis();
            if (ctx->finished_at_ms == 0) ctx->finished_at_ms = 1;  // 0 means "no finish recorded"
          } else {
            ctx->finished_at_ms = 0;
          }
        } else if (ctx->state == "active") {
          ctx->finished_at_ms = 0;
        }
        if (ctx->state != "active") ctx->cancel_requested = false;
        timer_card_refresh(ctx);
      })
  );
  // Duration attribute (configured run length)
  ha_subscribe_attribute(
    ctx->entity_id, std::string("duration"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        ctx->duration_secs = parse_timer_hms(state);
        timer_card_refresh(ctx);
      })
  );
  // Remaining attribute (only fires while active/paused)
  ha_subscribe_attribute(
    ctx->entity_id, std::string("remaining"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        ctx->remaining_secs = parse_timer_hms(state);
        ctx->remaining_anchor_ms = esphome::millis();
        timer_card_refresh(ctx);
      })
  );
  // Absolute finish time (ISO 8601, present while active). Survives a device
  // restart mid-countdown, unlike `remaining` which only updates on state
  // changes and goes stale across reconnects.
  ha_subscribe_attribute(
    ctx->entity_id, std::string("finishes_at"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        if (state.size() == 0 || state.c_str()[0] == '\0') {
          ctx->finishes_at_epoch = 0;
        } else {
          ctx->finishes_at_epoch = parse_iso8601_to_epoch(state);
        }
        timer_card_refresh(ctx);
      })
  );
}

inline void handle_timer_card_click(TimerCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !ha_api_state_connected()) return;
  if (ctx->state == "active") {
    auto do_cancel = [ctx]() { ctx->cancel_requested = ha_send_entity_action(ctx->entity_id, "timer.cancel"); };
    if (ctx->confirm_enabled) {
      confirmation_try(&ctx->confirm, ctx->text_lbl, espcontrol_i18n("Confirm"),
                       ctx->confirm_timeout_secs, do_cancel);
    } else {
      do_cancel();
    }
  } else if (ctx->state == "idle" || ctx->state == "paused") {
    confirmation_disarm(&ctx->confirm);
    ha_send_entity_action(ctx->entity_id, "timer.start");
  }
}

inline void setup_timer_card(BtnSlot &s, const ParsedCfg &p,
                             const lv_font_t *value_font) {
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (value_font) lv_obj_set_style_text_font(s.sensor_lbl, value_font, LV_PART_MAIN);
  lv_label_set_display_text(s.sensor_lbl, "--:--");
  lv_label_set_display_text(s.unit_lbl, "");
  lv_label_set_display_text(s.text_lbl, p.label.empty() ? espcontrol_i18n("Timer") : p.label.c_str());
}

inline uint16_t timer_parse_confirm_timeout(const std::string &unit) {
  int n = atoi(unit.c_str());
  if (n < 1) n = 3;
  if (n > 30) n = 30;
  return (uint16_t)n;
}
