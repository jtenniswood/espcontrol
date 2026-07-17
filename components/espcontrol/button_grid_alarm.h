#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Alarm card controls ───────────────────────────────────────────────

constexpr uint32_t ALARM_TRIGGERED_COLOR = 0xC62828;
constexpr uint32_t ALARM_CARD_CTX_MAGIC = 0x414C4D43;    // ALMC
constexpr uint32_t ALARM_ACTION_CTX_MAGIC = 0x414C4D41;  // ALMA
constexpr int ALARM_MAX_ACTIONS = 5;
constexpr int ALARM_MAX_VISIBLE_ACTIONS = 3;

struct AlarmCardCtx {
  uint32_t magic = ALARM_CARD_CTX_MAGIC;
  std::string entity_id;
  std::string label;
  std::string options;
  std::string state;
  std::string arm_mode;
  std::string pending_action_mode;
  lv_timer_t *arm_delay_timer = nullptr;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *grid_page = nullptr;
  lv_obj_t *page = nullptr;
  TransientStatusLabel *status_label = nullptr;
  lv_timer_t *pending_action_timer = nullptr;
  std::function<void(espcontrol::DisplayTakeoverKind)> begin_display_takeover;
  std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;
  uint32_t arm_delay_started_ms = 0;
  int arm_delay_seconds = -1;
  int arm_delay_total_seconds = -1;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *pin_label_font = nullptr;
  const lv_font_t *key_label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  const lv_font_t *arming_title_font = nullptr;
  uint32_t on_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  int width_compensation_percent = 100;
  int grid_cols = 3;
  bool available = false;
  bool show_status_icon = false;
  bool show_status_label = false;
  bool pending_action_had_code = false;
  bool critical_takeover_active = false;
  bool arming_modal_auto_opened = false;
};

struct AlarmActionCtx {
  uint32_t magic = ALARM_ACTION_CTX_MAGIC;
  AlarmCardCtx *card = nullptr;
  std::string mode;
  bool requires_pin = true;
};

struct AlarmPinModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *pin_lbl = nullptr;
  AlarmActionCtx *active = nullptr;
  AlarmActionCtx active_action;
  std::string pin;
};

struct AlarmControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *rail = nullptr;
  lv_obj_t *mode_btn[ALARM_MAX_ACTIONS] = {};
  lv_obj_t *mode_icon[ALARM_MAX_ACTIONS] = {};
  lv_obj_t *mode_label[ALARM_MAX_ACTIONS] = {};
  lv_obj_t *arming_view = nullptr;
  lv_obj_t *arming_title = nullptr;
  lv_obj_t *arming_countdown = nullptr;
  lv_obj_t *arming_progress = nullptr;
  lv_obj_t *arming_progress_fill = nullptr;
  lv_obj_t *arming_disarm_btn = nullptr;
  lv_obj_t *arming_disarm_label = nullptr;
  AlarmActionCtx actions[ALARM_MAX_ACTIONS];
  AlarmActionCtx arming_disarm_action;
  AlarmCardCtx *active = nullptr;
  int action_count = 0;
};

struct AlarmToastUi {
  lv_obj_t *box = nullptr;
  lv_timer_t *timer = nullptr;
};

struct AlarmDeferredAction {
  AlarmActionCtx action;
  std::string code;
  lv_timer_t *timer = nullptr;
  bool submit_pin = false;
};

inline bool alarm_card_show_status_icon(const ParsedCfg &p) {
  return normalize_alarm_icon_display(cfg_option_value(p.options, "icon_display")) == "status";
}

inline bool alarm_card_show_status_label(const ParsedCfg &p) {
  return normalize_alarm_label_display(cfg_option_value(p.options, "label_display")) == "status";
}

inline AlarmControlModalUi &alarm_control_modal_ui() {
  static AlarmControlModalUi ui;
  return ui;
}

inline AlarmPinModalUi &alarm_pin_modal_ui() {
  static AlarmPinModalUi ui;
  return ui;
}

inline AlarmToastUi &alarm_toast_ui() {
  static AlarmToastUi ui;
  return ui;
}

inline AlarmDeferredAction &alarm_deferred_action() {
  static AlarmDeferredAction action;
  return action;
}

inline bool alarm_card_context_valid(AlarmCardCtx *ctx) {
  return ctx != nullptr && ctx->magic == ALARM_CARD_CTX_MAGIC;
}

inline bool alarm_action_context_valid(AlarmActionCtx *action) {
  return action != nullptr && action->magic == ALARM_ACTION_CTX_MAGIC &&
         alarm_card_context_valid(action->card);
}

inline const char *alarm_card_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon("Security");
}

inline void setup_alarm_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, alarm_card_icon(p));
  lv_label_set_text(s.text_lbl, p.label.empty() ? espcontrol_i18n("Alarm") : p.label.c_str());
}

inline bool alarm_pin_arm_required(const std::string &options) {
  return cfg_option_value(options, "pin_arm") != "0";
}

inline bool alarm_pin_disarm_required(const std::string &options) {
  return cfg_option_value(options, "pin_disarm") != "0";
}

inline bool alarm_action_valid(const std::string &mode) {
  return card_runtime_alarm_action_mode_valid(mode);
}

inline size_t alarm_action_mode_count() {
  size_t count = card_runtime_alarm_action_mode_count();
  return count < ALARM_MAX_ACTIONS ? count : ALARM_MAX_ACTIONS;
}

inline const char *alarm_action_mode_at(size_t index) {
  return card_runtime_alarm_action_mode_at(index);
}

inline bool alarm_action_default_visible(const std::string &mode) {
  return mode == "away" || mode == "home" || mode == "disarm";
}

inline bool alarm_action_visible(const std::string &options, const std::string &mode) {
  std::string actions = cfg_option_value(options, "actions");
  if (actions.empty()) return alarm_action_default_visible(mode);
  bool saw_valid = false;
  size_t start = 0;
  while (start <= actions.length()) {
    size_t end = actions.find('|', start);
    if (end == std::string::npos) end = actions.length();
    std::string action = actions.substr(start, end - start);
    if (alarm_action_valid(action)) {
      saw_valid = true;
      if (action == mode) return true;
    }
    start = end + 1;
  }
  return !saw_valid && alarm_action_default_visible(mode);
}

inline int alarm_visible_action_modes(const std::string &options,
                                      std::string *modes,
                                      int max_modes) {
  if (!modes || max_modes <= 0) return 0;
  int count = 0;
  size_t action_count = alarm_action_mode_count();
  for (size_t i = 0; i < action_count && count < max_modes; i++) {
    std::string mode = alarm_action_mode_at(i);
    if (!mode.empty() && alarm_action_visible(options, mode)) modes[count++] = mode;
  }
  return count;
}

inline bool alarm_action_requires_pin(const std::string &options, const std::string &mode) {
  return mode == "disarm" ? alarm_pin_disarm_required(options) : alarm_pin_arm_required(options);
}

inline const char *alarm_action_label(const std::string &mode) {
  if (mode == "away") return espcontrol_i18n("Arm Away");
  if (mode == "home") return espcontrol_i18n("Arm Home");
  if (mode == "night") return espcontrol_i18n("Arm Night");
  if (mode == "vacation") return espcontrol_i18n("Arm Vacation");
  if (mode == "disarm") return espcontrol_i18n("Disarm");
  return espcontrol_i18n("Alarm");
}

inline const char *alarm_action_icon(const std::string &mode) {
  return find_icon(alarm_action_icon_name(mode));
}

inline void setup_alarm_action_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, alarm_action_icon(p.sensor));
  if (!p.icon.empty() && p.icon != "Auto") {
    lv_label_set_text(s.icon_lbl, find_icon(p.icon.c_str()));
  }
  lv_label_set_text(s.text_lbl,
    p.label.empty() ? alarm_action_label(p.sensor) : p.label.c_str());
}

inline const char *alarm_action_service(const std::string &mode) {
  return card_runtime_alarm_action_service(mode);
}

inline const char *alarm_control_button_label(const std::string &mode) {
  if (mode == "home") return espcontrol_i18n("Home");
  if (mode == "away") return espcontrol_i18n("Away");
  if (mode == "night") return espcontrol_i18n("Night");
  if (mode == "vacation") return espcontrol_i18n("Vacation");
  if (mode == "disarm") return espcontrol_i18n("Disarmed");
  return alarm_action_label(mode);
}

inline std::string alarm_state_label(const std::string &state) {
  if (state.empty()) return espcontrol_i18n(std::string("Unavailable"));
  if (state == "disarmed") return espcontrol_i18n(std::string("Disarmed"));
  if (state == "armed_away") return espcontrol_i18n(std::string("Armed Away"));
  if (state == "armed_home") return espcontrol_i18n(std::string("Armed Home"));
  if (state == "armed_night") return espcontrol_i18n(std::string("Armed Night"));
  if (state == "armed_vacation") return espcontrol_i18n(std::string("Armed Vacation"));
  if (state == "armed_custom_bypass") return espcontrol_i18n(std::string("Armed Custom"));
  if (state == "arming") return espcontrol_i18n(std::string("Arming"));
  if (state == "pending") return espcontrol_i18n(std::string("Pending"));
  if (state == "triggered") return espcontrol_i18n(std::string("Triggered"));
  if (state == "unavailable") return espcontrol_i18n(std::string("Unavailable"));
  if (state == "unknown") return espcontrol_i18n(std::string("Unknown"));
  return sentence_cap_text(state);
}

inline bool alarm_state_is_armed(const std::string &state) {
  return state.compare(0, 5, "armed") == 0;
}

inline bool alarm_state_is_active(const std::string &state) {
  return alarm_state_is_armed(state) || state == "arming" || state == "pending";
}

inline bool alarm_state_is_delay(const std::string &state) {
  return state == "arming" || state == "pending";
}

inline std::string alarm_action_achieved_state(const std::string &mode) {
  if (mode == "away") return "armed_away";
  if (mode == "home") return "armed_home";
  if (mode == "night") return "armed_night";
  if (mode == "vacation") return "armed_vacation";
  if (mode == "disarm") return "disarmed";
  return "";
}

inline std::string alarm_normalized_arm_mode(const std::string &arm_mode) {
  if (arm_mode == "away") return "armed_away";
  if (arm_mode == "home") return "armed_home";
  if (arm_mode == "night") return "armed_night";
  if (arm_mode == "vacation") return "armed_vacation";
  if (arm_mode == "armed_away" || arm_mode == "armed_home" ||
      arm_mode == "armed_night" || arm_mode == "armed_vacation" ||
      arm_mode == "armed_custom_bypass") {
    return arm_mode;
  }
  return "";
}

inline std::string alarm_effective_state(const std::string &state,
                                         const std::string &arm_mode) {
  if (state != "arming") return state;
  std::string normalized = alarm_normalized_arm_mode(arm_mode);
  return normalized.empty() ? state : normalized;
}

inline const char *alarm_state_icon(const std::string &state,
                                    const std::string &arm_mode = "") {
  std::string effective = alarm_effective_state(state, arm_mode);
  if (effective == "armed_home") return find_icon("Shield Home");
  if (effective == "armed_away" || effective == "armed_custom_bypass") return find_icon("Shield Lock");
  if (effective == "armed_night") return find_icon("Weather Night");
  if (effective == "armed_vacation") return find_icon("Airplane");
  if (effective == "disarmed") return find_icon("Shield Off");
  if (effective == "triggered") return find_icon("Alarm Light");
  return find_icon("Alarm");
}

inline bool alarm_action_state_matches(const std::string &mode, const std::string &state,
                                       const std::string &arm_mode = "") {
  std::string achieved_state = alarm_action_achieved_state(mode);
  return !achieved_state.empty() && alarm_effective_state(state, arm_mode) == achieved_state;
}

inline bool alarm_action_state_progressed(const std::string &mode, const std::string &state,
                                          const std::string &arm_mode = "") {
  if (alarm_action_state_matches(mode, state, arm_mode)) return true;
  return mode != "disarm" && (state == "arming" || state == "pending");
}

inline bool alarm_state_releases_label(const std::string &state) {
  return state == "disarmed" || alarm_state_is_armed(state);
}

inline void alarm_apply_card_status_icon(AlarmCardCtx *ctx) {
  if (!ctx || !ctx->icon_lbl || !ctx->show_status_icon) return;
  lv_label_set_text(ctx->icon_lbl, alarm_state_icon(ctx->state, ctx->arm_mode));
}

inline std::string alarm_state_control_mode(const std::string &state) {
  if (state == "armed_home") return "home";
  if (state == "armed_away") return "away";
  if (state == "armed_night") return "night";
  if (state == "armed_vacation") return "vacation";
  if (state == "disarmed") return "disarm";
  return "";
}

inline std::string alarm_control_button_label_for_state(const std::string &mode,
                                                        const std::string &state,
                                                        const std::string &arm_mode) {
  std::string effective_state = alarm_effective_state(state, arm_mode);
  if (alarm_state_control_mode(effective_state) == mode) {
    return sentence_cap_text(effective_state);
  }
  return alarm_control_button_label(mode);
}

inline void alarm_control_update_modal(AlarmCardCtx *ctx);
inline void alarm_control_open_modal(AlarmCardCtx *ctx);
inline void alarm_control_hide_modal();
inline void alarm_refresh_arming_takeover(AlarmCardCtx *ctx);

inline AlarmCardCtx *&alarm_arming_takeover_ctx() {
  static AlarmCardCtx *ctx = nullptr;
  return ctx;
}

inline int alarm_parse_delay_seconds(const std::string &value) {
  if (value.empty()) return -1;
  std::string lower = value;
  for (char &ch : lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (lower == "unknown" || lower == "unavailable" || lower == "none" ||
      lower == "null") return -1;

  if (value.find(':') != std::string::npos) {
    int total = 0;
    int part = 0;
    bool saw_digit = false;
    bool saw_part = false;
    for (char ch : value) {
      if (ch >= '0' && ch <= '9') {
        saw_digit = true;
        saw_part = true;
        part = part * 10 + (ch - '0');
      } else if (ch == ':') {
        if (!saw_part) return -1;
        total = total * 60 + part;
        part = 0;
        saw_part = false;
      } else if (saw_part) {
        break;
      }
    }
    if (!saw_digit || !saw_part) return -1;
    return total * 60 + part;
  }

  int seconds = 0;
  bool saw_digit = false;
  for (char ch : value) {
    if (ch >= '0' && ch <= '9') {
      saw_digit = true;
      if (seconds < 86400) seconds = seconds * 10 + (ch - '0');
    } else if (saw_digit) {
      break;
    }
  }
  return saw_digit ? seconds : -1;
}

inline int alarm_remaining_delay_seconds(AlarmCardCtx *ctx) {
  if (!ctx || ctx->arm_delay_seconds < 0) return -1;
  if (ctx->arm_delay_seconds == 0) return 0;
  uint32_t elapsed_ms = lv_tick_get() - ctx->arm_delay_started_ms;
  int elapsed_seconds = static_cast<int>(elapsed_ms / 1000);
  int remaining = ctx->arm_delay_seconds - elapsed_seconds;
  return remaining > 0 ? remaining : 0;
}

inline std::string alarm_delay_label(int seconds) {
  if (seconds < 0) return "";
  char buf[16];
  if (seconds >= 60) {
    snprintf(buf, sizeof(buf), "%d:%02d", seconds / 60, seconds % 60);
  } else {
    snprintf(buf, sizeof(buf), "%ds", seconds);
  }
  return std::string(buf);
}

inline int alarm_delay_progress_percent(AlarmCardCtx *ctx, int remaining) {
  if (!ctx || remaining < 0) return -1;
  int total = ctx->arm_delay_total_seconds > 0 ? ctx->arm_delay_total_seconds : ctx->arm_delay_seconds;
  if (total <= 0) return -1;
  if (remaining > total) remaining = total;
  int pct = (remaining * 100 + total / 2) / total;
  if (pct < 0) return 0;
  if (pct > 100) return 100;
  return pct;
}

inline void alarm_arm_delay_timer_cb(lv_timer_t *timer) {
  AlarmCardCtx *ctx = static_cast<AlarmCardCtx *>(lv_timer_get_user_data(timer));
  if (!ctx) return;
  alarm_control_update_modal(ctx);
  if (!alarm_state_is_delay(ctx->state) || alarm_remaining_delay_seconds(ctx) <= 0) {
    alarm_refresh_arming_takeover(ctx);
    lv_timer_pause(timer);
  }
}

inline void alarm_arm_delay_refresh_timer(AlarmCardCtx *ctx) {
  if (!ctx) return;
  bool should_run = alarm_state_is_delay(ctx->state) && alarm_remaining_delay_seconds(ctx) > 0;
  if (!should_run) {
    if (ctx->arm_delay_timer) lv_timer_pause(ctx->arm_delay_timer);
    return;
  }
  if (!ctx->arm_delay_timer) {
    ctx->arm_delay_timer = lv_timer_create(alarm_arm_delay_timer_cb, 1000, ctx);
  } else {
    lv_timer_resume(ctx->arm_delay_timer);
    lv_timer_reset(ctx->arm_delay_timer);
  }
}

inline bool alarm_control_modal_shows_arming(AlarmCardCtx *ctx) {
  return ctx && alarm_state_is_delay(ctx->state);
}

inline bool alarm_delay_takeover_active(AlarmCardCtx *ctx) {
  return ctx && alarm_card_context_valid(ctx) && alarm_state_is_delay(ctx->state) &&
         alarm_remaining_delay_seconds(ctx) > 0;
}

inline bool alarm_display_takeover_active(AlarmCardCtx *ctx) {
  return alarm_delay_takeover_active(ctx) ||
         (ctx && alarm_card_context_valid(ctx) && ctx->state == "triggered");
}

inline bool alarm_arming_takeover_active() {
  return alarm_delay_takeover_active(alarm_arming_takeover_ctx());
}

inline bool alarm_display_takeover_active() {
  return alarm_display_takeover_active(alarm_arming_takeover_ctx());
}

inline void alarm_release_arming_takeover(AlarmCardCtx *ctx) {
  if (!ctx) return;
  if (alarm_arming_takeover_ctx() == ctx) alarm_arming_takeover_ctx() = nullptr;
  if (ctx->critical_takeover_active) {
    ctx->critical_takeover_active = false;
    if (ctx->end_display_takeover) {
      ctx->end_display_takeover(espcontrol::DisplayTakeoverKind::CRITICAL);
    }
  }
  AlarmControlModalUi &ui = alarm_control_modal_ui();
  if (ctx->arming_modal_auto_opened && ui.active == ctx) {
    alarm_control_hide_modal();
  }
  ctx->arming_modal_auto_opened = false;
}

inline void alarm_refresh_arming_takeover(AlarmCardCtx *ctx) {
  bool should_take_over = ctx && alarm_card_context_valid(ctx) && ctx->available &&
                          alarm_display_takeover_active(ctx);
  if (!should_take_over) {
    alarm_release_arming_takeover(ctx);
    return;
  }

  AlarmCardCtx *active = alarm_arming_takeover_ctx();
  if (active && active != ctx) alarm_release_arming_takeover(active);
  alarm_arming_takeover_ctx() = ctx;

  AlarmControlModalUi &ui = alarm_control_modal_ui();
  if (ui.active != ctx || !ui.overlay) {
    alarm_control_open_modal(ctx);
    ctx->arming_modal_auto_opened = true;
  } else {
    lv_obj_move_foreground(ui.overlay);
  }

  if (!ctx->critical_takeover_active) {
    ctx->critical_takeover_active = true;
    if (ctx->begin_display_takeover) {
      ctx->begin_display_takeover(espcontrol::DisplayTakeoverKind::CRITICAL);
    }
  }
}

inline void alarm_control_set_hidden(lv_obj_t *obj, bool hidden) {
  if (!obj) return;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

inline void alarm_clear_pending_action(AlarmCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->pending_action_timer) {
    lv_timer_del(ctx->pending_action_timer);
    ctx->pending_action_timer = nullptr;
  }
  ctx->pending_action_mode.clear();
  ctx->pending_action_had_code = false;
}

inline void alarm_clear_pending_action_if_progressed(AlarmCardCtx *ctx) {
  if (!ctx || ctx->pending_action_mode.empty()) return;
  if (!alarm_action_state_progressed(ctx->pending_action_mode, ctx->state, ctx->arm_mode)) return;
  alarm_clear_pending_action(ctx);
}

inline void alarm_set_card_state_colors(AlarmCardCtx *ctx, uint32_t checked_color) {
  if (!ctx || !ctx->btn) return;
  lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(ctx->off_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(checked_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
  lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(checked_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
}

inline void alarm_apply_home_state(AlarmCardCtx *ctx, const std::string &state) {
  if (!ctx || !ctx->btn) return;
  bool delay_state_changed = alarm_state_is_delay(state) && ctx->state != state;
  ctx->state = state;
  if (delay_state_changed) {
    ctx->arm_delay_started_ms = lv_tick_get();
    ctx->arm_delay_total_seconds = ctx->arm_delay_seconds > 0 ? ctx->arm_delay_seconds : -1;
  }
  if (!alarm_state_is_delay(ctx->state)) ctx->arm_delay_total_seconds = -1;
  bool unavailable = state.empty() || state == "unavailable" || state == "unknown";
  ctx->available = !unavailable;

  bool triggered = state == "triggered";
  bool active = alarm_state_is_active(state) || triggered;
  alarm_set_card_state_colors(ctx, triggered ? ALARM_TRIGGERED_COLOR : ctx->on_color);
  set_card_checked_state(ctx->btn, active);

  alarm_apply_card_status_icon(ctx);
  transient_status_label_show_if_changed(
    ctx->status_label,
    alarm_state_label(state),
    ctx->show_status_label
      ? false
      : (alarm_state_releases_label(state) && !unavailable && !triggered));
  alarm_clear_pending_action_if_progressed(ctx);
  alarm_control_update_modal(ctx);
  alarm_arm_delay_refresh_timer(ctx);
  alarm_refresh_arming_takeover(ctx);
}

inline void alarm_apply_home_arm_mode(AlarmCardCtx *ctx, const std::string &arm_mode) {
  if (!ctx) return;
  ctx->arm_mode = arm_mode;
  alarm_apply_card_status_icon(ctx);
  alarm_clear_pending_action_if_progressed(ctx);
  alarm_control_update_modal(ctx);
}

inline void alarm_apply_home_arm_delay(AlarmCardCtx *ctx, const std::string &delay) {
  if (!ctx) return;
  int seconds = alarm_parse_delay_seconds(delay);
  ctx->arm_delay_seconds = seconds;
  if (seconds >= 0 && seconds > ctx->arm_delay_total_seconds) {
    ctx->arm_delay_total_seconds = seconds;
  } else if (seconds < 0) {
    ctx->arm_delay_total_seconds = -1;
  }
  ctx->arm_delay_started_ms = lv_tick_get();
  alarm_control_update_modal(ctx);
  alarm_arm_delay_refresh_timer(ctx);
  alarm_refresh_arming_takeover(ctx);
}

inline void subscribe_alarm_state(AlarmCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      alarm_apply_home_state(ctx, string_ref_limited(state, HA_SHORT_STATE_MAX_LEN));
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("arm_mode"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef arm_mode) {
      alarm_apply_home_arm_mode(ctx, string_ref_limited(arm_mode, HA_SHORT_STATE_MAX_LEN));
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("delay"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef delay) {
      alarm_apply_home_arm_delay(ctx, string_ref_limited(delay, HA_SHORT_STATE_MAX_LEN));
    })
  );
}

inline void alarm_apply_action_availability(AlarmCardCtx *ctx, const std::string &state) {
  if (!ctx || !ctx->btn) return;
  bool unavailable = state.empty() || state == "unavailable" || state == "unknown";
  ctx->available = !unavailable;
}

inline void alarm_apply_action_state(AlarmCardCtx *ctx, const std::string &mode,
                                     const std::string &state) {
  alarm_apply_action_availability(ctx, state);
  if (!ctx || !ctx->btn) return;
  ctx->state = state;
  bool unavailable = state.empty() || state == "unavailable" || state == "unknown";
  bool active = !unavailable && alarm_action_state_matches(mode, state, ctx->arm_mode);
  set_card_checked_state(ctx->btn, active);
  alarm_clear_pending_action_if_progressed(ctx);
  alarm_control_update_modal(ctx);
  alarm_arm_delay_refresh_timer(ctx);
  alarm_refresh_arming_takeover(ctx);
}

inline void alarm_apply_action_arm_mode(AlarmCardCtx *ctx, const std::string &mode,
                                        const std::string &arm_mode) {
  if (!ctx || !ctx->btn) return;
  ctx->arm_mode = arm_mode;
  bool unavailable = ctx->state.empty() || ctx->state == "unavailable" || ctx->state == "unknown";
  bool active = !unavailable && alarm_action_state_matches(mode, ctx->state, ctx->arm_mode);
  set_card_checked_state(ctx->btn, active);
  alarm_clear_pending_action_if_progressed(ctx);
  alarm_control_update_modal(ctx);
  alarm_refresh_arming_takeover(ctx);
}

inline void subscribe_alarm_action_availability(AlarmCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ctx->available = true;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      alarm_apply_action_availability(ctx, string_ref_limited(state, HA_SHORT_STATE_MAX_LEN));
    })
  );
}

inline void subscribe_alarm_action_state(AlarmCardCtx *ctx, const std::string &mode) {
  if (!ctx || ctx->entity_id.empty()) return;
  ctx->available = true;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx, mode](esphome::StringRef state) {
      alarm_apply_action_state(ctx, mode, string_ref_limited(state, HA_SHORT_STATE_MAX_LEN));
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("arm_mode"),
    std::function<void(esphome::StringRef)>([ctx, mode](esphome::StringRef arm_mode) {
      alarm_apply_action_arm_mode(ctx, mode, string_ref_limited(arm_mode, HA_SHORT_STATE_MAX_LEN));
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("delay"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef delay) {
      alarm_apply_home_arm_delay(ctx, string_ref_limited(delay, HA_SHORT_STATE_MAX_LEN));
    })
  );
}

inline void alarm_toast_timer_cb(lv_timer_t *timer) {
  AlarmToastUi &ui = alarm_toast_ui();
  if (ui.box) lv_obj_del(ui.box);
  ui.box = nullptr;
  ui.timer = nullptr;
  lv_timer_del(timer);
}

inline void alarm_hide_toast() {
  AlarmToastUi &ui = alarm_toast_ui();
  if (ui.box) lv_obj_del(ui.box);
  if (ui.timer) lv_timer_del(ui.timer);
  ui = AlarmToastUi();
}

inline lv_coord_t alarm_failure_banner_width(AlarmCardCtx *ctx) {
  lv_coord_t screen_w = control_modal_screen_width();
  int cols = ctx && ctx->grid_cols > 0 ? ctx->grid_cols : 3;
  if (cols < 1) cols = 1;

  lv_obj_t *grid = ctx && ctx->grid_page ? ctx->grid_page : lv_scr_act();
  lv_coord_t pad_left = 0;
  lv_coord_t pad_right = 0;
  lv_coord_t gap_col = 0;
  if (grid) {
    lv_obj_update_layout(grid);
    pad_left = lv_obj_get_style_pad_left(grid, LV_PART_MAIN);
    pad_right = lv_obj_get_style_pad_right(grid, LV_PART_MAIN);
    gap_col = lv_obj_get_style_pad_column(grid, LV_PART_MAIN);
  }

  int span_cols = cols < 3 ? cols : 3;
  lv_coord_t usable_w = screen_w - pad_left - pad_right - gap_col * (cols - 1);
  lv_coord_t cell_w = usable_w > 0 ? usable_w / cols : screen_w / cols;
  lv_coord_t width = cell_w * span_cols + gap_col * (span_cols - 1);
  lv_coord_t max_w = screen_w - 36;
  if (width > max_w) width = max_w;
  if (width < 120) width = max_w;
  return width;
}

inline void alarm_show_failure(AlarmCardCtx *ctx, const std::string &message) {
  alarm_hide_toast();
  AlarmToastUi &ui = alarm_toast_ui();
  lv_coord_t width = alarm_failure_banner_width(ctx);
  ControlModalToastShell shell = control_modal_open_toast(width, 10, ALARM_TRIGGERED_COLOR);
  ui.box = shell.box;

  lv_obj_t *label = lv_label_create(ui.box);
  lv_label_set_text(label, message.empty() ? espcontrol_i18n("Alarm action failed") : espcontrol_i18n(message).c_str());
  if (ctx && ctx->label_font) lv_obj_set_style_text_font(label, ctx->label_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, width - 24);
  lv_obj_center(label);

  lv_obj_update_layout(ui.box);
  lv_obj_align(ui.box, LV_ALIGN_BOTTOM_MID, 0, -18);
  ui.timer = lv_timer_create(alarm_toast_timer_cb, 3000, nullptr);
  lv_obj_move_foreground(ui.box);
}

inline void alarm_pending_action_timer_cb(lv_timer_t *timer) {
  AlarmCardCtx *ctx = static_cast<AlarmCardCtx *>(lv_timer_get_user_data(timer));
  if (ctx && ctx->pending_action_timer == timer) {
    bool had_code = ctx->pending_action_had_code;
    std::string mode = ctx->pending_action_mode;
    ctx->pending_action_timer = nullptr;
    ctx->pending_action_mode.clear();
    ctx->pending_action_had_code = false;
    if (!alarm_action_state_progressed(mode, ctx->state, ctx->arm_mode)) {
      alarm_show_failure(ctx, had_code ? "PIN was not accepted" : "Alarm did not change");
    }
  }
  lv_timer_del(timer);
}

inline void alarm_start_pending_action(AlarmCardCtx *ctx,
                                       const std::string &mode,
                                       bool had_code) {
  if (!ctx) return;
  alarm_clear_pending_action(ctx);
  ctx->pending_action_mode = mode;
  ctx->pending_action_had_code = had_code;
  ctx->pending_action_timer = lv_timer_create(alarm_pending_action_timer_cb, 3500, ctx);
}

inline void send_alarm_action(AlarmActionCtx *action, const std::string &code) {
  if (!alarm_action_context_valid(action) || action->card->entity_id.empty() ||
      !ha_api_available()) return;
  const char *service = alarm_action_service(action->mode);
  if (service == nullptr) return;

  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, code.empty() ? 1 : 2)) return;
  ha_action_add_entity(req, action->card->entity_id);
  if (!code.empty()) {
    ha_action_add_data(req, "code", code.c_str());
  }

  AlarmCardCtx *card = action->card;
  if (ha_action_send(req)) {
    alarm_start_pending_action(card, action->mode, !code.empty());
  }
}

inline uint32_t alarm_control_active_color(AlarmCardCtx *ctx, const std::string &mode) {
  return ctx ? ctx->on_color : DEFAULT_SLIDER_COLOR;
}

inline uint32_t alarm_control_inactive_color(AlarmCardCtx *ctx) {
  return ctx ? ctx->off_color : SECONDARY_GREY;
}

inline lv_coord_t alarm_control_mode_button_radius(const ControlModalLayout &layout,
                                                   lv_coord_t width,
                                                   lv_coord_t height) {
  lv_coord_t radius = (width < height ? width : height) / 2;
  if (control_modal_uses_compact_portrait_tuning(layout)) {
    lv_coord_t compact_radius = control_modal_scaled_px(10, layout.short_side);
    if (compact_radius < 8) compact_radius = 8;
    if (radius > compact_radius) radius = (radius + compact_radius) / 2;
  }
  return radius;
}

inline lv_coord_t alarm_control_rail_radius(const ControlModalLayout &layout,
                                            lv_coord_t control_radius,
                                            lv_coord_t button_inset) {
  if (control_modal_uses_compact_portrait_tuning(layout)) {
    lv_coord_t radius = control_modal_scaled_px(14, layout.short_side);
    if (radius < control_radius) radius = control_radius;
    return (control_radius + button_inset + radius) / 2;
  }
  return control_radius + button_inset;
}

inline void alarm_control_update_delay_progress(AlarmControlModalUi &ui,
                                                AlarmCardCtx *ctx,
                                                int remaining) {
  if (!ui.arming_progress || !ui.arming_progress_fill) return;
  int pct = alarm_delay_progress_percent(ctx, remaining);
  alarm_control_set_hidden(ui.arming_progress, pct < 0);
  alarm_control_set_hidden(ui.arming_progress_fill, pct < 0);
  if (pct < 0) return;

  lv_obj_update_layout(ui.arming_progress);
  lv_coord_t track_w = lv_obj_get_width(ui.arming_progress);
  lv_coord_t track_h = lv_obj_get_height(ui.arming_progress);
  if (track_w <= 0 || track_h <= 0) return;
  lv_coord_t fill_w = static_cast<lv_coord_t>((int32_t) track_w * pct / 100);
  if (fill_w <= 0) {
    lv_obj_add_flag(ui.arming_progress_fill, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_clear_flag(ui.arming_progress_fill, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(ui.arming_progress_fill, fill_w, track_h);
  lv_obj_set_style_radius(ui.arming_progress_fill, track_h / 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui.arming_progress_fill,
    lv_color_hex(ctx ? ctx->on_color : DEFAULT_SLIDER_COLOR), LV_PART_MAIN);
  lv_obj_align(ui.arming_progress_fill, LV_ALIGN_LEFT_MID, 0, 0);
}

inline void alarm_control_update_modal(AlarmCardCtx *ctx) {
  AlarmControlModalUi &ui = alarm_control_modal_ui();
  if (!ctx || ui.active != ctx) return;

  bool show_arming = alarm_control_modal_shows_arming(ctx);
  alarm_control_set_hidden(ui.rail, show_arming);
  alarm_control_set_hidden(ui.arming_view, !show_arming);
  if (ui.arming_view) {
    if (ui.arming_title) lv_label_set_text(ui.arming_title, alarm_state_label(ctx->state).c_str());
    int remaining = show_arming ? alarm_remaining_delay_seconds(ctx) : -1;
    if (ui.arming_countdown) {
      alarm_control_set_hidden(ui.arming_countdown, remaining < 0);
      if (remaining >= 0) {
        std::string countdown = alarm_delay_label(remaining);
        lv_label_set_text(ui.arming_countdown, countdown.c_str());
      }
    }
    alarm_control_update_delay_progress(ui, ctx, remaining);
    ui.arming_disarm_action.card = ctx;
    ui.arming_disarm_action.mode = "disarm";
    ui.arming_disarm_action.requires_pin = alarm_action_requires_pin(ctx->options, "disarm");
  }

  std::string active_mode = alarm_state_control_mode(
    alarm_effective_state(ctx->state, ctx->arm_mode));
  for (int i = 0; i < ui.action_count && i < ALARM_MAX_ACTIONS; i++) {
    lv_obj_t *btn = ui.mode_btn[i];
    if (!btn) continue;
    std::string mode = ui.actions[i].mode;
    bool selected = active_mode == mode;
    uint32_t bg = selected ? alarm_control_active_color(ctx, mode)
                           : alarm_control_inactive_color(ctx);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    if (ui.mode_label[i]) {
      std::string label = selected
        ? alarm_control_button_label_for_state(mode, ctx->state, ctx->arm_mode)
        : alarm_control_button_label(mode);
      lv_label_set_text(ui.mode_label[i], label.c_str());
    }
  }
}

inline void alarm_control_hide_modal() {
  AlarmControlModalUi &ui = alarm_control_modal_ui();
  AlarmCardCtx *active = ui.active;
  bool release_takeover = active && active->arming_modal_auto_opened &&
                          alarm_arming_takeover_ctx() == active;
  control_modal_delete_overlay(ControlModalKind::ALARM_CONTROL, ui.overlay);
  ui = AlarmControlModalUi();
  if (release_takeover) alarm_release_arming_takeover(active);
}

inline void alarm_pin_hide_modal() {
  PinKeypadUi &pin_ui = pin_keypad_ui();
  if (pin_ui.kind == PinKeypadKind::ALARM) {
    pin_keypad_hide_modal();
    return;
  }
  AlarmPinModalUi &ui = alarm_pin_modal_ui();
  ui.pin.clear();
  control_modal_delete_overlay(ControlModalKind::ALARM_PIN, ui.overlay);
  ui = AlarmPinModalUi();
}

inline void alarm_action_activate(AlarmActionCtx *action);
inline void alarm_run_deferred_action_cb(lv_timer_t *timer) {
  AlarmDeferredAction &deferred = alarm_deferred_action();
  if (deferred.timer == timer) deferred.timer = nullptr;
  AlarmActionCtx action = deferred.action;
  std::string code = deferred.code;
  bool submit_pin = deferred.submit_pin;
  deferred.action = AlarmActionCtx();
  deferred.code.clear();
  deferred.submit_pin = false;
  lv_timer_del(timer);

  if (!alarm_action_context_valid(&action)) return;
  if (submit_pin) {
    alarm_pin_hide_modal();
    send_alarm_action(&action, code);
    return;
  }
  alarm_action_activate(&action);
}

inline void alarm_defer_action(AlarmActionCtx *action,
                               const std::string &code = "",
                               bool submit_pin = false) {
  if (!alarm_action_context_valid(action)) return;
  AlarmDeferredAction &deferred = alarm_deferred_action();
  if (deferred.timer) {
    lv_timer_del(deferred.timer);
    deferred.timer = nullptr;
  }
  deferred.action = *action;
  deferred.code = code;
  deferred.submit_pin = submit_pin;
  deferred.timer = lv_timer_create(alarm_run_deferred_action_cb, 1, nullptr);
}

inline void alarm_control_mode_cb(lv_event_t *e) {
  AlarmActionCtx *action = static_cast<AlarmActionCtx *>(lv_event_get_user_data(e));
  alarm_defer_action(action);
}

inline bool alarm_pin_keypad_submit(const std::string &code, void *user_data) {
  AlarmActionCtx *action = static_cast<AlarmActionCtx *>(user_data);
  if (!alarm_action_context_valid(action) || code.empty()) return false;
  alarm_defer_action(action, code, true);
  return true;
}

inline void alarm_pin_keypad_closed() {
  alarm_pin_modal_ui() = AlarmPinModalUi();
}

inline void alarm_pin_open_modal(AlarmActionCtx *action) {
  if (!alarm_action_context_valid(action) || !action->card->available) return;
  const lv_font_t *label_font = action->card->btn
    ? lv_obj_get_style_text_font(action->card->btn, LV_PART_MAIN)
    : nullptr;
  const lv_font_t *pin_label_font = action->card->pin_label_font
    ? action->card->pin_label_font
    : label_font;
  const lv_font_t *key_label_font = action->card->key_label_font
    ? action->card->key_label_font
    : label_font;
  const lv_font_t *icon_font = action->card->icon_font
    ? action->card->icon_font
    : action->card->icon_lbl
    ? lv_obj_get_style_text_font(action->card->icon_lbl, LV_PART_MAIN)
    : label_font;

  control_modal_force_close_active();
  AlarmPinModalUi &ui = alarm_pin_modal_ui();
  ui.active_action = *action;
  ui.active = &ui.active_action;
  ui.pin.clear();
  pin_keypad_open_modal(
    PinKeypadKind::ALARM,
    action->card->btn,
    action->card->width_compensation_percent,
    pin_label_font,
    key_label_font,
    icon_font,
    "Enter Pin",
    alarm_pin_keypad_submit,
    ui.active,
    alarm_pin_keypad_closed);
}

inline void alarm_action_activate(AlarmActionCtx *action) {
  if (!alarm_action_context_valid(action) || !action->card->available) return;
  if (action->requires_pin) {
    alarm_pin_open_modal(action);
    return;
  }
  send_alarm_action(action, "");
}

inline lv_obj_t *alarm_control_create_mode_button(
    lv_obj_t *parent,
    AlarmCardCtx *ctx,
    const char *mode,
    lv_coord_t width,
    lv_coord_t height,
    lv_coord_t radius,
    const lv_font_t *icon_font,
    const lv_font_t *label_font,
    lv_obj_t **icon_out,
    lv_obj_t **label_out) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, width, height);
  apply_width_compensation(btn, ctx ? ctx->width_compensation_percent : 100);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(alarm_control_inactive_color(ctx)), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  control_modal_apply_pressed_fill(btn);

  lv_obj_t *content = lv_obj_create(btn);
  lv_obj_set_size(content, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(content, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *icon = lv_label_create(content);
  lv_label_set_text(icon, alarm_action_icon(mode ? std::string(mode) : ""));
  lv_obj_set_style_text_color(icon, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (icon_font) lv_obj_set_style_text_font(icon, icon_font, LV_PART_MAIN);
  apply_width_compensation(icon, ctx ? ctx->width_compensation_percent : 100);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_align(icon, LV_ALIGN_CENTER, 0, -height / 7);

  lv_obj_t *label = lv_label_create(content);
  lv_label_set_text(label, alarm_control_button_label(mode ? std::string(mode) : ""));
  lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(label, label_font, LV_PART_MAIN);
  apply_width_compensation(label, ctx ? ctx->width_compensation_percent : 100);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, height / 5);

  if (icon_out) *icon_out = icon;
  if (label_out) *label_out = label;
  return btn;
}

inline void alarm_control_create_arming_view(AlarmControlModalUi &ui,
                                             AlarmCardCtx *ctx,
                                             const ControlModalLayout &layout,
                                             const lv_font_t *title_font,
                                             const lv_font_t *countdown_font,
                                             const lv_font_t *label_font) {
  ui.arming_view = lv_obj_create(ui.panel);
  lv_obj_set_size(ui.arming_view, layout.panel_w, layout.panel_h);
  lv_obj_set_style_bg_opa(ui.arming_view, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arming_view, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.arming_view, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.arming_view, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.arming_view, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.arming_view, LV_OBJ_FLAG_HIDDEN);

  uint32_t primary_color = alarm_control_active_color(ctx, "");
  uint32_t primary_text_color = readable_text_color_for_bg(primary_color);
  bool compact_portrait_layout = control_modal_uses_compact_portrait_tuning(layout);
  lv_coord_t status_center_y = -control_modal_scaled_px(64, layout.short_side);
  lv_coord_t countdown_gap = control_modal_scaled_px(28, layout.short_side);
  lv_coord_t disarm_extra_padding = 0;
  if (compact_portrait_layout) {
    status_center_y = -control_modal_scaled_px(56, layout.short_side);
    countdown_gap = control_modal_scaled_px(34, layout.short_side);
    disarm_extra_padding = control_modal_scaled_px(24, layout.short_side);
  }

  ui.arming_title = lv_label_create(ui.arming_view);
  lv_label_set_text(ui.arming_title, espcontrol_i18n("Arming"));
  lv_obj_set_style_text_color(ui.arming_title, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.arming_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (title_font) lv_obj_set_style_text_font(ui.arming_title, title_font, LV_PART_MAIN);
  apply_width_compensation(ui.arming_title, ctx ? ctx->width_compensation_percent : 100);
  lv_obj_set_width(ui.arming_title, layout.panel_w - layout.inset * 2);
  lv_obj_align(ui.arming_title, LV_ALIGN_CENTER, 0, status_center_y);
  lv_obj_update_layout(ui.arming_title);
  lv_coord_t countdown_y = status_center_y + lv_obj_get_height(ui.arming_title) / 2 + countdown_gap;

  ui.arming_countdown = lv_label_create(ui.arming_view);
  lv_label_set_text(ui.arming_countdown, "");
  lv_obj_set_style_text_color(ui.arming_countdown, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.arming_countdown, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (countdown_font) lv_obj_set_style_text_font(ui.arming_countdown, countdown_font, LV_PART_MAIN);
  apply_width_compensation(ui.arming_countdown, ctx ? ctx->width_compensation_percent : 100);
  lv_obj_set_width(ui.arming_countdown, layout.panel_w - layout.inset * 2);
  lv_obj_align(ui.arming_countdown, LV_ALIGN_CENTER, 0, countdown_y);
  lv_obj_add_flag(ui.arming_countdown, LV_OBJ_FLAG_HIDDEN);
  lv_obj_update_layout(ui.arming_countdown);
  lv_coord_t countdown_h = lv_obj_get_height(ui.arming_countdown);
  if (countdown_h <= 0 && countdown_font) countdown_h = countdown_font->line_height;
  if (countdown_h <= 0) countdown_h = control_modal_scaled_px(28, layout.short_side);

  lv_coord_t progress_h = control_modal_scaled_px(12, layout.short_side);
  if (progress_h < 8) progress_h = 8;
  if (progress_h > 16) progress_h = 16;
  lv_coord_t progress_w = layout.panel_w / 3;
  if (progress_w < progress_h * 4) progress_w = progress_h * 4;
  lv_coord_t progress_gap = control_modal_scaled_px(20, layout.short_side);
  if (progress_gap < 14) progress_gap = 14;
  if (compact_portrait_layout) progress_gap = control_modal_scaled_px(24, layout.short_side);
  if (compact_portrait_layout && progress_gap < 16) progress_gap = 16;
  lv_coord_t progress_center_y = countdown_y + countdown_h / 2 + progress_gap + progress_h / 2;

  ui.arming_progress = lv_obj_create(ui.arming_view);
  lv_obj_set_size(ui.arming_progress, progress_w, progress_h);
  lv_obj_set_style_bg_color(ui.arming_progress, lv_color_hex(primary_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.arming_progress, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arming_progress, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.arming_progress, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.arming_progress, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(ui.arming_progress, progress_h / 2, LV_PART_MAIN);
  lv_obj_clear_flag(ui.arming_progress, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.arming_progress, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.arming_progress, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align(ui.arming_progress, LV_ALIGN_CENTER, 0, progress_center_y);

  ui.arming_progress_fill = lv_obj_create(ui.arming_progress);
  lv_obj_set_size(ui.arming_progress_fill, 0, progress_h);
  lv_obj_set_style_bg_color(ui.arming_progress_fill, lv_color_hex(primary_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.arming_progress_fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arming_progress_fill, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.arming_progress_fill, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.arming_progress_fill, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(ui.arming_progress_fill, progress_h / 2, LV_PART_MAIN);
  lv_obj_clear_flag(ui.arming_progress_fill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.arming_progress_fill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.arming_progress_fill, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align(ui.arming_progress_fill, LV_ALIGN_LEFT_MID, 0, 0);

  lv_coord_t disarm_h = control_modal_scaled_px(52, layout.short_side);
  if (disarm_h < 44) disarm_h = 44;
  if (disarm_h > layout.panel_h / 8) disarm_h = layout.panel_h / 8;
  ui.arming_disarm_btn = control_modal_create_round_button(
    ui.arming_view, disarm_h, espcontrol_i18n("Disarm"), label_font,
    primary_color,
    primary_color,
    ctx ? ctx->width_compensation_percent : 100);
  lv_obj_set_style_radius(ui.arming_disarm_btn, disarm_h / 2, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arming_disarm_btn, 0, LV_PART_MAIN);
  ui.arming_disarm_label = lv_obj_get_child(ui.arming_disarm_btn, 0);
  if (ui.arming_disarm_label) {
    lv_obj_set_style_text_color(ui.arming_disarm_label, lv_color_hex(primary_text_color), LV_PART_MAIN);
    lv_obj_update_layout(ui.arming_disarm_label);
    lv_coord_t disarm_w = lv_obj_get_width(ui.arming_disarm_label) + disarm_h + disarm_extra_padding;
    lv_coord_t max_disarm_w = layout.panel_w - layout.inset * 2;
    if (disarm_w > max_disarm_w) disarm_w = max_disarm_w;
    lv_obj_set_width(ui.arming_disarm_btn, disarm_w);
    lv_obj_center(ui.arming_disarm_label);
  }
  lv_obj_align(ui.arming_disarm_btn, LV_ALIGN_BOTTOM_MID, 0, -layout.panel_h / 14);
  ui.arming_disarm_action.card = ctx;
  ui.arming_disarm_action.mode = "disarm";
  ui.arming_disarm_action.requires_pin = ctx ? alarm_action_requires_pin(ctx->options, "disarm") : true;
  lv_obj_add_event_cb(ui.arming_disarm_btn, alarm_control_mode_cb, LV_EVENT_CLICKED,
    &ui.arming_disarm_action);
}

inline void alarm_control_open_modal(AlarmCardCtx *ctx) {
  if (!alarm_card_context_valid(ctx) || !ctx->available) return;
  const lv_font_t *label_font = ctx->label_font
    ? ctx->label_font
    : ctx->btn ? lv_obj_get_style_text_font(ctx->btn, LV_PART_MAIN) : nullptr;
  const lv_font_t *icon_font = ctx->icon_font
    ? ctx->icon_font
    : ctx->icon_lbl ? lv_obj_get_style_text_font(ctx->icon_lbl, LV_PART_MAIN) : label_font;
  const lv_font_t *title_font = ctx->arming_title_font
    ? ctx->arming_title_font
    : ctx->key_label_font ? ctx->key_label_font : label_font;
  const lv_font_t *countdown_font = ctx->key_label_font ? ctx->key_label_font : label_font;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::ALARM_CONTROL, ctx->btn, ctx->width_compensation_percent,
    icon_font, alarm_control_hide_modal);

  AlarmControlModalUi &ui = alarm_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;

  ControlModalLayout &layout = shell.layout;
  std::string modes[ALARM_MAX_ACTIONS];
  ui.action_count = alarm_visible_action_modes(ctx->options, modes, ALARM_MAX_VISIBLE_ACTIONS);
  if (ui.action_count < 1) {
    ui.action_count = 1;
    modes[0] = "disarm";
  }

  lv_coord_t rail_w = layout.panel_w * 58 / 100;
  if (rail_w < control_modal_scaled_px(156, layout.short_side))
    rail_w = control_modal_scaled_px(156, layout.short_side);
  lv_coord_t max_rail_w = layout.short_side * 62 / 100;
  if (rail_w > max_rail_w) rail_w = max_rail_w;
  if (rail_w > layout.panel_w - layout.inset * 2) rail_w = layout.panel_w - layout.inset * 2;
  lv_coord_t rail_h = layout.panel_h - layout.inset * 2;
  if (rail_h < control_modal_scaled_px(300, layout.short_side))
    rail_h = control_modal_scaled_px(300, layout.short_side);
  if (rail_h > layout.panel_h - layout.inset * 2)
    rail_h = layout.panel_h - layout.inset * 2;
  if (rail_h < control_modal_scaled_px(240, layout.short_side))
    rail_h = control_modal_scaled_px(240, layout.short_side);
  lv_coord_t button_inset = control_modal_scaled_px(8, layout.short_side);
  if (button_inset < 4) button_inset = 4;
  lv_coord_t button_gap = control_modal_scaled_px(8, layout.short_side);
  if (button_gap < 4) button_gap = 4;
  lv_coord_t btn_w = rail_w - button_inset * 2;
  lv_coord_t btn_h = (rail_h - button_inset * 2 - button_gap * (ui.action_count - 1)) / ui.action_count;
  if (btn_w < 1) btn_w = rail_w;
  if (btn_h < 1) btn_h = rail_h / ui.action_count;
  lv_coord_t control_radius = alarm_control_mode_button_radius(layout, btn_w, btn_h);
  lv_coord_t background_radius = alarm_control_rail_radius(layout, control_radius, button_inset);

  ui.rail = lv_obj_create(ui.panel);
  lv_obj_set_size(ui.rail, rail_w, rail_h);
  apply_width_compensation(ui.rail, ctx->width_compensation_percent);
  lv_obj_set_style_radius(ui.rail, background_radius, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui.rail, lv_color_hex(alarm_control_inactive_color(ctx)), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.rail, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.rail, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.rail, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.rail, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.rail, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(ui.rail, LV_ALIGN_CENTER, 0, 0);

  for (int i = 0; i < ui.action_count && i < ALARM_MAX_ACTIONS; i++) {
    ui.mode_btn[i] = alarm_control_create_mode_button(
      ui.rail, ctx, modes[i].c_str(), btn_w, btn_h, control_radius,
      icon_font, label_font, &ui.mode_icon[i], &ui.mode_label[i]);
    lv_obj_set_pos(ui.mode_btn[i], button_inset, button_inset + i * (btn_h + button_gap));
    ui.actions[i].card = ctx;
    ui.actions[i].mode = modes[i];
    ui.actions[i].requires_pin = alarm_action_requires_pin(ctx->options, ui.actions[i].mode);
    lv_obj_add_event_cb(ui.mode_btn[i], alarm_control_mode_cb, LV_EVENT_CLICKED, &ui.actions[i]);
  }

  alarm_control_create_arming_view(ui, ctx, layout, title_font, countdown_font, label_font);
  alarm_control_update_modal(ctx);
  lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
}

inline void alarm_configure_page_grid(lv_obj_t *page, int num_slots, int cols) {
  if (!page) return;
  int slot_count = bounded_grid_slots(num_slots);
  int col_count = cols > 0 ? cols : 1;
  if (col_count > MAX_GRID_SLOTS) col_count = MAX_GRID_SLOTS;
  int row_count = (slot_count + col_count - 1) / col_count;
  if (row_count < 1) row_count = 1;
  if (row_count > MAX_GRID_SLOTS) row_count = MAX_GRID_SLOTS;

  static lv_coord_t col_dsc[MAX_GRID_SLOTS + 1];
  static lv_coord_t row_dsc[MAX_GRID_SLOTS + 1];
  for (int i = 0; i < col_count; i++) col_dsc[i] = LV_GRID_FR(1);
  col_dsc[col_count] = LV_GRID_TEMPLATE_LAST;
  for (int i = 0; i < row_count; i++) row_dsc[i] = LV_GRID_FR(1);
  row_dsc[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_grid_dsc_array(page, col_dsc, row_dsc);
}

inline AlarmCardCtx *create_alarm_card_context(
    BtnSlot &slot,
    const ParsedCfg &p,
    lv_obj_t *main_page_obj,
    int num_slots,
    int cols,
    uint32_t on_color,
    uint32_t off_color,
    uint32_t tertiary_color,
    const lv_font_t *icon_font,
    const lv_font_t *arming_title_font,
    const lv_font_t *value_font,
    const lv_font_t *key_font,
    const lv_font_t *label_font,
    lv_color_t text_color,
    int width_compensation_percent,
    bool build_default_page = false,
    std::function<void(espcontrol::DisplayTakeoverKind)> begin_display_takeover = nullptr,
    std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover = nullptr) {
  AlarmCardCtx *ctx = new AlarmCardCtx();
  ctx->entity_id = p.entity;
  ctx->label = p.label.empty() ? espcontrol_i18n(std::string("Alarm")) : p.label;
  ctx->options = p.options;
  ctx->btn = slot.btn;
  ctx->icon_lbl = slot.icon_lbl;
  ctx->show_status_icon = alarm_card_show_status_icon(p);
  ctx->show_status_label = alarm_card_show_status_label(p);
  ctx->label_font = label_font;
  ctx->pin_label_font = key_font ? key_font : (label_font ? label_font : value_font);
  ctx->key_label_font = key_font ? key_font : (label_font ? label_font : value_font);
  ctx->icon_font = icon_font;
  ctx->arming_title_font = arming_title_font;
  ctx->on_color = on_color;
  ctx->off_color = off_color;
  ctx->tertiary_color = tertiary_color;
  ctx->width_compensation_percent = width_compensation_percent;
  ctx->grid_cols = cols > 0 ? cols : 1;
  ctx->begin_display_takeover = begin_display_takeover;
  ctx->end_display_takeover = end_display_takeover;
  ctx->status_label = create_transient_status_label(
    slot.text_lbl, ctx->show_status_label ? "--" : ctx->label);
  alarm_set_card_state_colors(ctx, ctx->on_color);
  if (!build_default_page) return ctx;

  int NS = bounded_grid_slots(num_slots);
  int COLS = cols > 0 ? cols : 1;
  ctx->page = lv_obj_create(NULL);
  ctx->grid_page = ctx->page;
  lv_obj_set_style_bg_color(ctx->page, lv_color_hex(DARK_OVERLAY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ctx->page, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_layout(ctx->page, LV_LAYOUT_GRID);
  alarm_configure_page_grid(ctx->page, NS, COLS);
  lv_obj_clear_flag(ctx->page, LV_OBJ_FLAG_SCROLLABLE);

  if (main_page_obj) {
    lv_obj_set_style_pad_top(ctx->page, lv_obj_get_style_pad_top(main_page_obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(ctx->page, lv_obj_get_style_pad_bottom(main_page_obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_left(ctx->page, lv_obj_get_style_pad_left(main_page_obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_right(ctx->page, lv_obj_get_style_pad_right(main_page_obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_row(ctx->page, lv_obj_get_style_pad_row(main_page_obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_column(ctx->page, lv_obj_get_style_pad_column(main_page_obj, LV_PART_MAIN), LV_PART_MAIN);
  }

  lv_coord_t radius = lv_obj_get_style_radius(slot.btn, LV_PART_MAIN);
  lv_coord_t pad = lv_obj_get_style_pad_top(slot.btn, LV_PART_MAIN);

  lv_obj_t *back_btn = create_grid_card_button(ctx->page, radius, pad, label_font, text_color);
  apply_button_colors(back_btn, false, DEFAULT_SLIDER_COLOR, true, off_color);
  lv_obj_set_grid_cell(back_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  BtnSlot back_slot = create_dynamic_card_slot(back_btn, icon_font, value_font, label_font, text_color);
  apply_width_compensation(back_slot.icon_lbl, width_compensation_percent);
  apply_slot_text_width_compensation(back_slot, width_compensation_percent);
  lv_label_set_text(back_slot.icon_lbl, "\U000F0141");
  lv_label_set_text(back_slot.text_lbl, espcontrol_i18n("Back"));
  lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (target) lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  }, LV_EVENT_CLICKED, main_page_obj);

  std::string modes[ALARM_MAX_ACTIONS];
  int mode_count = alarm_visible_action_modes(p.options, modes, ALARM_MAX_VISIBLE_ACTIONS);
  int page_pos = 1;
  for (int i = 0; i < mode_count && page_pos < NS; i++) {
    std::string mode = modes[i];

    lv_obj_t *action_btn = create_grid_card_button(ctx->page, radius, pad, label_font, text_color);
    apply_button_colors(action_btn, false, DEFAULT_SLIDER_COLOR, true, off_color);
    int col = page_pos % COLS;
    int row = page_pos / COLS;
    lv_obj_set_grid_cell(action_btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    BtnSlot action_slot = create_dynamic_card_slot(action_btn, icon_font, value_font, label_font, text_color);
    apply_width_compensation(action_slot.icon_lbl, width_compensation_percent);
    apply_slot_text_width_compensation(action_slot, width_compensation_percent);
    lv_label_set_text(action_slot.icon_lbl, alarm_action_icon(mode));
    lv_label_set_text(action_slot.text_lbl, alarm_action_label(mode));
    apply_push_button_transition(action_btn);

    AlarmActionCtx *action_ctx = new AlarmActionCtx();
    action_ctx->card = ctx;
    action_ctx->mode = mode;
    action_ctx->requires_pin = alarm_action_requires_pin(p.options, mode);
    lv_obj_add_event_cb(action_btn, [](lv_event_t *e) {
      AlarmActionCtx *action = static_cast<AlarmActionCtx *>(lv_event_get_user_data(e));
      alarm_action_activate(action);
    }, LV_EVENT_CLICKED, action_ctx);
    page_pos++;
  }
  return ctx;
}

inline void alarm_card_open_page(AlarmCardCtx *ctx) {
  if (!alarm_card_context_valid(ctx)) return;
  alarm_control_open_modal(ctx);
}
