#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Configure a button as a read-only sensor card (non-clickable, shows value + unit)
inline void setup_sensor_card(BtnSlot &s, const ParsedCfg &p,
                              bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (!p.unit.empty()) {
    std::string unit = trim_display_unit(p.unit);
    lv_label_set_text(s.unit_lbl, unit.c_str());
  }
  if (!p.label.empty()) {
    lv_label_set_text(s.text_lbl, p.label.c_str());
  }
}

struct CalendarCardRef {
  lv_obj_t *value_lbl;
  lv_obj_t *unit_lbl;
  lv_obj_t *label_lbl;
  bool show_time;
};

inline CalendarCardRef *calendar_card_refs() {
  static CalendarCardRef refs[MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS];
  return refs;
}

inline int &calendar_card_count() {
  static int count = 0;
  return count;
}

struct CalendarDateState {
  bool date_valid;
  int day;
  int month;
  bool time_valid;
  int hour;
  int minute;
  bool use_12h;
};

inline CalendarDateState &calendar_date_state() {
  static CalendarDateState state = {false, 0, 0, false, 0, 0, false};
  return state;
}

inline bool calendar_date_valid() {
  return calendar_date_state().date_valid;
}

inline bool calendar_card_shows_time(const ParsedCfg &p) {
  return p.precision == "datetime";
}

inline void apply_calendar_card_text(const CalendarCardRef &ref,
                                     const CalendarDateState &state);

inline void reset_calendar_cards() {
  calendar_card_count() = 0;
}

inline void register_calendar_card(lv_obj_t *value_lbl, lv_obj_t *unit_lbl,
                                   lv_obj_t *label_lbl, bool show_time) {
  int &count = calendar_card_count();
  if (count >= MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("calendar", "Too many calendar cards; skipping date updates");
    return;
  }
  calendar_card_refs()[count++] = {value_lbl, unit_lbl, label_lbl, show_time};
  CalendarDateState &state = calendar_date_state();
  apply_calendar_card_text(calendar_card_refs()[count - 1], state);
}

inline const char *calendar_month_name(int month) {
  static const char *months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
  if (month < 1 || month > 12) return "Date";
  return months[month - 1];
}

inline void apply_calendar_card_text(const CalendarCardRef &ref,
                                     const CalendarDateState &state) {
  char value_buf[8];
  char label_buf[32];
  const char *value_text = "--";
  const char *unit_text = "";
  const char *label_text = "Date";

  if (ref.show_time) {
    if (state.time_valid &&
        state.day >= 1 && state.day <= 31 &&
        state.month >= 1 && state.month <= 12 &&
        state.hour >= 0 && state.hour <= 23 &&
        state.minute >= 0 && state.minute <= 59) {
      if (state.use_12h) {
        int hour12 = state.hour % 12;
        if (hour12 == 0) hour12 = 12;
        snprintf(value_buf, sizeof(value_buf), "%d:%02d", hour12, state.minute);
        unit_text = state.hour < 12 ? "am" : "pm";
      } else {
        snprintf(value_buf, sizeof(value_buf), "%02d:%02d", state.hour, state.minute);
      }
      value_text = value_buf;
      snprintf(label_buf, sizeof(label_buf), "%d %s", state.day, calendar_month_name(state.month));
      label_text = label_buf;
    }
  } else if (state.date_valid &&
             state.day >= 1 && state.day <= 31 &&
             state.month >= 1 && state.month <= 12) {
    snprintf(value_buf, sizeof(value_buf), "%d", state.day);
    value_text = value_buf;
    label_text = calendar_month_name(state.month);
  }
  if (ref.value_lbl) lv_label_set_text(ref.value_lbl, value_text);
  if (ref.unit_lbl) lv_label_set_text(ref.unit_lbl, unit_text);
  if (ref.label_lbl) lv_label_set_text(ref.label_lbl, label_text);
}

inline void update_calendar_cards(bool valid, int day, int month) {
  CalendarDateState &state = calendar_date_state();
  state.date_valid = valid && day >= 1 && day <= 31 && month >= 1 && month <= 12;
  state.day = state.date_valid ? day : 0;
  state.month = state.date_valid ? month : 0;
  if (!state.date_valid && !state.time_valid) {
    state.hour = 0;
    state.minute = 0;
    state.use_12h = false;
  }

  CalendarCardRef *refs = calendar_card_refs();
  int count = calendar_card_count();
  for (int i = 0; i < count; i++) {
    apply_calendar_card_text(refs[i], state);
  }
}

inline void update_calendar_cards_time(bool valid, int day, int month,
                                       int hour, int minute, bool use_12h) {
  CalendarDateState &state = calendar_date_state();
  state.time_valid = valid &&
                     day >= 1 && day <= 31 &&
                     month >= 1 && month <= 12 &&
                     hour >= 0 && hour <= 23 &&
                     minute >= 0 && minute <= 59;
  state.hour = state.time_valid ? hour : 0;
  state.minute = state.time_valid ? minute : 0;
  state.use_12h = state.time_valid ? use_12h : false;
  if (state.time_valid) {
    state.date_valid = true;
    state.day = day;
    state.month = month;
  }

  CalendarCardRef *refs = calendar_card_refs();
  int count = calendar_card_count();
  for (int i = 0; i < count; i++) {
    apply_calendar_card_text(refs[i], state);
  }
}

inline bool parse_calendar_date_text(const std::string &value, int &day, int &month) {
  if (value.length() < 10) return false;
  if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
      !std::isdigit(static_cast<unsigned char>(value[1])) ||
      !std::isdigit(static_cast<unsigned char>(value[2])) ||
      !std::isdigit(static_cast<unsigned char>(value[3])) ||
      value[4] != '-' ||
      !std::isdigit(static_cast<unsigned char>(value[5])) ||
      !std::isdigit(static_cast<unsigned char>(value[6])) ||
      value[7] != '-' ||
      !std::isdigit(static_cast<unsigned char>(value[8])) ||
      !std::isdigit(static_cast<unsigned char>(value[9]))) {
    return false;
  }
  int parsed_month = (value[5] - '0') * 10 + (value[6] - '0');
  int parsed_day = (value[8] - '0') * 10 + (value[9] - '0');
  if (parsed_day < 1 || parsed_day > 31 || parsed_month < 1 || parsed_month > 12) return false;
  day = parsed_day;
  month = parsed_month;
  return true;
}

inline bool parse_calendar_date_text_dmy(const std::string &value, int &day, int &month) {
  if (value.length() < 10) return false;
  if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
      !std::isdigit(static_cast<unsigned char>(value[1])) ||
      (value[2] != '/' && value[2] != '-') ||
      !std::isdigit(static_cast<unsigned char>(value[3])) ||
      !std::isdigit(static_cast<unsigned char>(value[4])) ||
      (value[5] != '/' && value[5] != '-') ||
      !std::isdigit(static_cast<unsigned char>(value[6])) ||
      !std::isdigit(static_cast<unsigned char>(value[7])) ||
      !std::isdigit(static_cast<unsigned char>(value[8])) ||
      !std::isdigit(static_cast<unsigned char>(value[9]))) {
    return false;
  }
  int parsed_day = (value[0] - '0') * 10 + (value[1] - '0');
  int parsed_month = (value[3] - '0') * 10 + (value[4] - '0');
  if (parsed_day < 1 || parsed_day > 31 || parsed_month < 1 || parsed_month > 12) return false;
  day = parsed_day;
  month = parsed_month;
  return true;
}

inline bool update_calendar_cards_from_date_text(const std::string &value) {
  int day = 0;
  int month = 0;
  bool valid = parse_calendar_date_text(value, day, month) ||
               parse_calendar_date_text_dmy(value, day, month);
  if (valid) update_calendar_cards(true, day, month);
  return valid;
}

inline std::string calendar_date_entity_or_default(const std::string &entity_id) {
  return entity_id.empty() ? std::string("sensor.date") : entity_id;
}

inline void subscribe_calendar_date_source(const std::string &entity_id) {
  std::string source = calendar_date_entity_or_default(entity_id);
  static std::vector<std::string> subscribed;
  for (const auto &existing : subscribed) {
    if (existing == source) return;
  }
  subscribed.push_back(source);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    source, {},
    std::function<void(esphome::StringRef)>([](esphome::StringRef state) {
      update_calendar_cards_from_date_text(string_ref_limited(state, 16));
    })
  );
}

struct TimezoneCardRef {
  lv_obj_t *value_lbl;
  lv_obj_t *unit_lbl;
  lv_obj_t *label_lbl;
  std::string timezone;
  std::string label;
};

inline TimezoneCardRef *timezone_card_refs() {
  static TimezoneCardRef refs[MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS];
  return refs;
}

inline int &timezone_card_count() {
  static int count = 0;
  return count;
}

inline void reset_timezone_cards() {
  timezone_card_count() = 0;
}

inline std::string timezone_city_label(const std::string &tz_option) {
  std::string tz_id = timezone_id_from_option(tz_option);
  if (tz_id.empty()) return "World Clock";
  if (tz_id == "UTC") return "UTC";
  size_t slash = tz_id.rfind('/');
  std::string city = slash == std::string::npos ? tz_id : tz_id.substr(slash + 1);
  for (char &ch : city) {
    if (ch == '_') ch = ' ';
  }
  return city.empty() ? std::string("World Clock") : city;
}

inline void set_posix_timezone_for_epoch(const std::string &tz_option, time_t epoch) {
  std::string tz_id = timezone_id_from_option(tz_option);
  struct tm utc_tm;
  gmtime_r(&epoch, &utc_tm);
  const char *posix = resolve_posix_tz_at_utc(tz_id, utc_point_from_tm(utc_tm));
  setenv("TZ", posix, 1);
  tzset();
}

inline bool timezone_localtime(const std::string &tz_option, time_t epoch, struct tm &out) {
  int offset_minutes = 0;
  if (!timezone_offset_minutes_at_utc(tz_option, epoch, offset_minutes)) return false;
  time_t local_epoch = epoch + static_cast<time_t>(offset_minutes) * 60;
  return gmtime_r(&local_epoch, &out) != nullptr;
}

inline void apply_timezone_card_text(const TimezoneCardRef &ref,
                                     bool valid,
                                     time_t epoch,
                                     const std::string &active_timezone,
                                     bool use_12h) {
  std::string tz_option = ref.timezone.empty() ? active_timezone : ref.timezone;
  std::string label = ref.label.empty() ? timezone_city_label(tz_option) : ref.label;
  const char *value_text = "--:--";
  const char *unit_text = "";
  char value_buf[8];

  if (valid) {
    struct tm local_tm;
    if (timezone_localtime(tz_option, epoch, local_tm)) {
      int hour = local_tm.tm_hour;
      int minute = local_tm.tm_min;
      if (use_12h) {
        int hour12 = hour % 12;
        if (hour12 == 0) hour12 = 12;
        snprintf(value_buf, sizeof(value_buf), "%d:%02d", hour12, minute);
        unit_text = hour < 12 ? "am" : "pm";
      } else {
        snprintf(value_buf, sizeof(value_buf), "%02d:%02d", hour, minute);
      }
      value_text = value_buf;
    }
  }

  if (ref.value_lbl) lv_label_set_text(ref.value_lbl, value_text);
  if (ref.unit_lbl) lv_label_set_text(ref.unit_lbl, unit_text);
  if (ref.label_lbl) lv_label_set_text(ref.label_lbl, label.c_str());
}

inline void register_timezone_card(lv_obj_t *value_lbl, lv_obj_t *unit_lbl,
                                   lv_obj_t *label_lbl,
                                   const std::string &timezone,
                                   const std::string &label) {
  int &count = timezone_card_count();
  if (count >= MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("timezone", "Too many timezone cards; skipping time updates");
    return;
  }
  timezone_card_refs()[count++] = {value_lbl, unit_lbl, label_lbl, timezone, label};
  apply_timezone_card_text(timezone_card_refs()[count - 1], false, 0, timezone, false);
}

inline void update_timezone_cards(bool valid,
                                  time_t epoch,
                                  const std::string &active_timezone,
                                  bool use_12h) {
  TimezoneCardRef *refs = timezone_card_refs();
  int count = timezone_card_count();
  for (int i = 0; i < count; i++) {
    apply_timezone_card_text(refs[i], valid, epoch, active_timezone, use_12h);
  }
  if (count > 0) {
    if (valid) set_posix_timezone_for_epoch(active_timezone, epoch);
    else apply_timezone(active_timezone);
  }
}

inline void setup_calendar_card(BtnSlot &s, const ParsedCfg &p,
                                bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, "Date");
  register_calendar_card(s.sensor_lbl, s.unit_lbl, s.text_lbl, calendar_card_shows_time(p));
}

inline void setup_timezone_card(BtnSlot &s, const ParsedCfg &p,
                                bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.sensor_lbl, "--:--");
  lv_label_set_text(s.unit_lbl, "");
  std::string label = p.label.empty() ? timezone_city_label(p.entity) : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  register_timezone_card(s.sensor_lbl, s.unit_lbl, s.text_lbl, p.entity, p.label);
}

inline void setup_weather_card(BtnSlot &s, bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(s.icon_lbl, find_icon("Weather Cloudy"));
  lv_label_set_text(s.text_lbl, "Weather");
}

inline bool weather_card_shows_forecast(const ParsedCfg &p) {
  return p.type == "weather_forecast" ||
    (p.type == "weather" && (p.precision == "today" || p.precision == "tomorrow"));
}

inline std::string weather_card_forecast_day(const ParsedCfg &p) {
  return p.precision == "today" ? "today" : "tomorrow";
}

inline void setup_weather_forecast_card(BtnSlot &s, const ParsedCfg &p,
                                        bool has_sensor_color, uint32_t sensor_val,
                                        int width_compensation_percent = 100) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.sensor_lbl, "--/--");
  lv_label_set_text(s.unit_lbl, "");
  std::string day = weather_card_forecast_day(p);
  std::string label = p.label.empty()
    ? (day == "today" ? "Today" : "Tomorrow")
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  apply_width_compensation(s.sensor_container, width_compensation_percent);
  apply_width_compensation(s.text_lbl, width_compensation_percent);
  register_weather_forecast_card(s.sensor_lbl, s.unit_lbl, s.text_lbl, p.entity, day, p.label);
}

inline void apply_push_button_transition(lv_obj_t *btn);

inline void setup_garage_card(BtnSlot &s, const ParsedCfg &p) {
  if (garage_command_mode(p.sensor)) {
    lv_label_set_text(s.icon_lbl, garage_command_icon(p));
    lv_label_set_text(s.text_lbl, garage_card_label(p));
    apply_push_button_transition(s.btn);
    return;
  }
  lv_label_set_text(s.icon_lbl, garage_closed_icon(p.icon));
  lv_label_set_text(s.text_lbl, garage_card_label(p));
}

inline void setup_lock_card(BtnSlot &s, const ParsedCfg &p) {
  if (lock_command_mode(p.sensor)) {
    lv_label_set_text(s.icon_lbl, lock_command_icon(p));
    lv_label_set_text(s.text_lbl, lock_card_label(p));
    apply_push_button_transition(s.btn);
    return;
  }
  lv_label_set_text(s.icon_lbl, lock_locked_icon(p.icon));
  lv_label_set_text(s.text_lbl, lock_card_label(p));
}

inline void apply_push_button_transition(lv_obj_t *btn) {
  static const lv_style_prop_t push_props[] = {LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV};
  static lv_style_transition_dsc_t push_trans;
  static bool push_trans_inited = false;
  if (!push_trans_inited) {
    lv_style_transition_dsc_init(&push_trans, push_props, lv_anim_path_ease_out, 400, 0, NULL);
    push_trans_inited = true;
  }
  lv_obj_set_style_transition(btn, &push_trans,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
}

inline void setup_internal_relay_card(BtnSlot &s, const ParsedCfg &p) {
  bool push_mode = internal_relay_push_mode(p);
  std::string label = internal_relay_label(p);
  lv_label_set_text(s.text_lbl, label.c_str());
  const char *icon_off = internal_relay_icon(p, push_mode);
  lv_label_set_text(s.icon_lbl, icon_off);
  if (push_mode) {
    apply_push_button_transition(s.btn);
    return;
  }
  bool has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
  const char *icon_on = has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
  apply_internal_relay_state(s.btn, s.icon_lbl, internal_relay_state(p.entity),
    has_icon_on, icon_off, icon_on);
}

// Set icon and label on a toggle/push button based on its config
inline void setup_toggle_visual(BtnSlot &s, const ParsedCfg &p) {
  if (!p.entity.empty()) {
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    const char* icon_cp = "\U000F0493";
    if (p.icon.empty() || p.icon == "Auto") {
      std::string domain = p.entity.substr(0, p.entity.find('.'));
      icon_cp = domain_default_icon(domain);
    } else {
      icon_cp = find_icon(p.icon.c_str());
    }
    lv_label_set_text(s.icon_lbl, icon_cp);

    if (!p.sensor.empty()) {
      if (!p.unit.empty()) {
        std::string unit = trim_display_unit(p.unit);
        lv_label_set_text(s.unit_lbl, unit.c_str());
      }
    }
  } else {
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    if (!p.icon.empty() && p.icon != "Auto") {
      lv_label_set_text(s.icon_lbl, find_icon(p.icon.c_str()));
    } else if (p.type == "push") {
      lv_label_set_text(s.icon_lbl, "\U000F0741");
      apply_push_button_transition(s.btn);
    }
    if (p.type == "push" && p.label.empty()) {
      lv_label_set_text(s.text_lbl, "Push");
    }
  }
}

inline void setup_action_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.text_lbl, p.label.empty() ? (p.entity.empty() ? "Action" : p.entity.c_str()) : p.label.c_str());
  const char *icon_cp = (p.icon.empty() || p.icon == "Auto") ? find_icon("Flash") : find_icon(p.icon.c_str());
  lv_label_set_text(s.icon_lbl, icon_cp);
  apply_push_button_transition(s.btn);
}

inline void setup_local_action_card(BtnSlot &s, const ParsedCfg &p) {
  std::string label = p.label.empty() ? (p.entity.empty() ? "Local Action" : sentence_cap_text(p.entity)) : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  const char *icon_cp = (p.icon.empty() || p.icon == "Auto") ? find_icon("Gesture Tap") : find_icon(p.icon.c_str());
  lv_label_set_text(s.icon_lbl, icon_cp);
  apply_push_button_transition(s.btn);
}

inline void setup_text_sensor_card(BtnSlot &s, const ParsedCfg &p,
                                   bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  setup_toggle_visual(s, p);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  set_wrapped_button_label_text(s.text_lbl, "--");
}

inline bool subpage_parent_sensor_state_enabled(const ParsedCfg &p) {
  return p.type == "subpage" &&
         !p.sensor.empty() &&
         p.sensor != "indicator";
}

inline bool subpage_parent_text_state_enabled(const ParsedCfg &p) {
  return subpage_parent_sensor_state_enabled(p) &&
         p.precision == "text";
}

inline bool subpage_parent_icon_entity_state_enabled(const ParsedCfg &p) {
  return p.type == "subpage" &&
         p.sensor == "indicator" &&
         !p.entity.empty();
}

inline void setup_subpage_parent_state_card(BtnSlot &s, const ParsedCfg &p,
                                            const lv_font_t *value_font) {
  setup_toggle_visual(s, p);
  if (p.precision == "text") {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    set_wrapped_button_label_text(s.text_lbl, "--");
    return;
  }

  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (value_font) lv_obj_set_style_text_font(s.sensor_lbl, value_font, LV_PART_MAIN);
  lv_label_set_text(s.sensor_lbl, "--");
  std::string unit = trim_display_unit(p.unit);
  lv_label_set_text(s.unit_lbl, unit.c_str());
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Subpage" : p.label.c_str());
}
