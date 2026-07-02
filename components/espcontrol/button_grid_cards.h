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
  if (p.precision == "icon") {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    const char *icon_cp = (p.icon.empty() || p.icon == "Auto")
      ? find_icon("Auto") : find_icon(p.icon.c_str());
    lv_label_set_text(s.icon_lbl, icon_cp);
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    return;
  }
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
  if (month < 1 || month > 12) return espcontrol_i18n("Date");
  return espcontrol_i18n(months[month - 1]);
}

inline void apply_calendar_card_text(const CalendarCardRef &ref,
                                     const CalendarDateState &state) {
  char value_buf[8];
  char label_buf[32];
  const char *value_text = "--";
  const char *unit_text = "";
  const char *label_text = espcontrol_i18n("Date");

  if (ref.show_time) {
    if (state.time_valid &&
        state.day >= 1 && state.day <= 31 &&
        state.month >= 1 && state.month <= 12 &&
        state.hour >= 0 && state.hour <= 23 &&
        state.minute >= 0 && state.minute <= 59) {
      format_clock_time_without_suffix(value_buf, sizeof(value_buf),
                                       state.hour, state.minute, state.use_12h);
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

inline void refresh_calendar_cards() {
  CalendarDateState &state = calendar_date_state();
  CalendarCardRef *refs = calendar_card_refs();
  int count = calendar_card_count();
  for (int i = 0; i < count; i++) {
    apply_calendar_card_text(refs[i], state);
  }
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

  refresh_calendar_cards();
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

  refresh_calendar_cards();
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
  ha_subscribe_state(
    source,
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
  bool show_label;
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
  std::string tz_id = timezone_id_from_option(effective_timezone_option(tz_option));
  if (tz_id.empty()) return espcontrol_i18n("World Clock");
  if (tz_id == "UTC") return "UTC";
  size_t slash = tz_id.rfind('/');
  std::string city = slash == std::string::npos ? tz_id : tz_id.substr(slash + 1);
  for (char &ch : city) {
    if (ch == '_') ch = ' ';
  }
  return city.empty() ? espcontrol_i18n(std::string("World Clock")) : city;
}

inline bool timezone_localtime(const std::string &tz_option, time_t epoch, struct tm &out) {
  int offset_minutes = 0;
  if (!timezone_offset_minutes_at_utc(effective_timezone_option(tz_option), epoch, offset_minutes)) return false;
  time_t local_epoch = epoch + static_cast<time_t>(offset_minutes) * 60;
  return gmtime_r(&local_epoch, &out) != nullptr;
}

inline void apply_timezone_card_text(const TimezoneCardRef &ref,
                                     bool valid,
                                     time_t epoch,
                                     const std::string &active_timezone,
                                     bool use_12h) {
  std::string tz_option = ref.timezone.empty() ? active_timezone : ref.timezone;
  std::string label = ref.show_label
    ? (ref.label.empty() ? timezone_city_label(tz_option) : ref.label)
    : std::string("");
  const char *value_text = "--:--";
  const char *unit_text = "";
  char value_buf[8];

  if (valid) {
    struct tm local_tm;
    if (timezone_localtime(tz_option, epoch, local_tm)) {
      int hour = local_tm.tm_hour;
      int minute = local_tm.tm_min;
      format_clock_time_without_suffix(value_buf, sizeof(value_buf),
                                       hour, minute, use_12h);
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
                                   const std::string &label,
                                   bool show_label = true) {
  int &count = timezone_card_count();
  if (count >= MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("timezone", "Too many timezone cards; skipping time updates");
    return;
  }
  timezone_card_refs()[count++] = {value_lbl, unit_lbl, label_lbl, timezone, label, show_label};
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
  lv_label_set_text(s.text_lbl, espcontrol_i18n("Date"));
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

inline void setup_clock_card(BtnSlot &s, const ParsedCfg &p,
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
  lv_label_set_text(s.text_lbl, "");
  register_timezone_card(s.sensor_lbl, s.unit_lbl, s.text_lbl, p.entity, "", false);
}

inline void setup_weather_card(BtnSlot &s, bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.icon_lbl, find_icon("Weather Cloudy"));
  lv_label_set_text(s.sensor_lbl, "");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, espcontrol_i18n("Cloudy"));
}

inline bool weather_card_is_daily_strip(const ParsedCfg &p) {
  return p.type == "weather" && p.precision == "daily_strip";
}

inline bool weather_card_is_hourly_strip(const ParsedCfg &p) {
  return p.type == "weather" && p.precision == "hourly_strip";
}

inline bool weather_card_is_hero(const ParsedCfg &p) {
  return p.type == "weather" && p.precision == "hero";
}

inline bool weather_card_shows_forecast(const ParsedCfg &p) {
  return p.type == "weather_forecast" ||
    (p.type == "weather" &&
     (card_runtime_weather_forecast_precision(p.precision) || weather_card_is_daily_strip(p)));
}

inline std::string weather_card_forecast_day(const ParsedCfg &p) {
  return p.precision == "today" ? "today" : "tomorrow";
}

// Temperature values in the daily/hourly strips render smaller than the
// full-size sensor font. 256 == 100%; scale around the label center so the
// shrunken text stays aligned within its flex column. Kept well under 100% so
// wider negative strings (e.g. "-5/-12") still fit inside the column.
constexpr int32_t WEATHER_STRIP_TEMP_SCALE = 190;

inline void apply_weather_strip_temp_scale(lv_obj_t *label) {
  if (!label) return;
  lv_obj_set_style_transform_pivot_x(label, lv_pct(50), LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(label, lv_pct(50), LV_PART_MAIN);
  lv_obj_set_style_transform_scale_x(label, WEATHER_STRIP_TEMP_SCALE, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(label, WEATHER_STRIP_TEMP_SCALE, LV_PART_MAIN);
}

inline lv_obj_t *create_weather_strip_day_column(lv_obj_t *parent,
                                                 const lv_font_t *weekday_font,
                                                 const lv_font_t *icon_font,
                                                 const lv_font_t *value_font,
                                                 lv_color_t text_color,
                                                 WeatherDailyStripDayRef &out) {
  lv_obj_t *column = lv_obj_create(parent);
  lv_obj_set_size(column, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(column, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(column, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(column, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(column, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(column, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(column, 2, LV_PART_MAIN);
  lv_obj_set_layout(column, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(column, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(column, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(column, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  out.column = column;
  out.weekday_lbl = lv_label_create(column);
  if (weekday_font) lv_obj_set_style_text_font(out.weekday_lbl, weekday_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(out.weekday_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(out.weekday_lbl, "--");

  out.icon_lbl = lv_label_create(column);
  if (icon_font) lv_obj_set_style_text_font(out.icon_lbl, icon_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(out.icon_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(out.icon_lbl, find_icon("Weather Cloudy"));

  out.value_lbl = lv_label_create(column);
  if (value_font) lv_obj_set_style_text_font(out.value_lbl, value_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(out.value_lbl, text_color, LV_PART_MAIN);
  apply_weather_strip_temp_scale(out.value_lbl);
  lv_label_set_text(out.value_lbl, "--/--");
  return column;
}

inline void setup_weather_daily_strip_card(BtnSlot &s, const ParsedCfg &p,
                                           bool has_sensor_color, uint32_t sensor_val,
                                           int width_compensation_percent = 100) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);

  const lv_font_t *icon_font = s.icon_lbl
    ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN) : nullptr;
  const lv_font_t *weekday_font = s.text_lbl
    ? lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN) : nullptr;
  const lv_font_t *value_font = s.sensor_lbl
    ? lv_obj_get_style_text_font(s.sensor_lbl, LV_PART_MAIN) : nullptr;
  lv_color_t text_color = s.text_lbl
    ? lv_obj_get_style_text_color(s.text_lbl, LV_PART_MAIN) : lv_color_white();

  lv_obj_t *strip_container = lv_obj_create(s.btn);
  lv_obj_set_align(strip_container, LV_ALIGN_CENTER);
  lv_obj_set_size(strip_container, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_clear_flag(strip_container, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(strip_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(strip_container, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(strip_container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(strip_container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(strip_container, 6, LV_PART_MAIN);
  lv_obj_set_layout(strip_container, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(strip_container, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(strip_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(strip_container, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  WeatherDailyStripDayRef days[WEATHER_DAILY_STRIP_DAY_COUNT];
  for (int i = 0; i < WEATHER_DAILY_STRIP_DAY_COUNT; i++) {
    create_weather_strip_day_column(
      strip_container, weekday_font, icon_font, value_font, text_color, days[i]);
  }
  apply_width_compensation(strip_container, width_compensation_percent);
  register_weather_daily_strip_card(
    s.btn, strip_container, days, WEATHER_DAILY_STRIP_DAY_COUNT, p.entity);
}

inline lv_obj_t *create_weather_hourly_strip_hour_column(lv_obj_t *parent,
                                                         const lv_font_t *hour_font,
                                                         const lv_font_t *icon_font,
                                                         const lv_font_t *temp_font,
                                                         lv_color_t text_color,
                                                         WeatherHourlyStripHourRef &out) {
  lv_obj_t *column = lv_obj_create(parent);
  lv_obj_set_size(column, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(column, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(column, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(column, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(column, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(column, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(column, 2, LV_PART_MAIN);
  lv_obj_set_layout(column, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(column, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(column, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(column, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  out.column = column;
  out.hour_lbl = lv_label_create(column);
  if (hour_font) lv_obj_set_style_text_font(out.hour_lbl, hour_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(out.hour_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(out.hour_lbl, "--");

  out.icon_lbl = lv_label_create(column);
  if (icon_font) lv_obj_set_style_text_font(out.icon_lbl, icon_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(out.icon_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(out.icon_lbl, find_icon("Weather Cloudy"));

  out.temp_lbl = lv_label_create(column);
  if (temp_font) lv_obj_set_style_text_font(out.temp_lbl, temp_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(out.temp_lbl, text_color, LV_PART_MAIN);
  apply_weather_strip_temp_scale(out.temp_lbl);
  lv_label_set_text(out.temp_lbl, "--");
  return column;
}

inline void setup_weather_hourly_strip_card(BtnSlot &s, const ParsedCfg &p,
                                            bool has_sensor_color, uint32_t sensor_val,
                                            int width_compensation_percent = 100) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);

  const lv_font_t *icon_font = s.icon_lbl
    ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN) : nullptr;
  const lv_font_t *hour_font = s.text_lbl
    ? lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN) : nullptr;
  const lv_font_t *temp_font = s.sensor_lbl
    ? lv_obj_get_style_text_font(s.sensor_lbl, LV_PART_MAIN) : nullptr;
  lv_color_t text_color = s.text_lbl
    ? lv_obj_get_style_text_color(s.text_lbl, LV_PART_MAIN) : lv_color_white();

  lv_obj_t *strip_container = lv_obj_create(s.btn);
  lv_obj_set_align(strip_container, LV_ALIGN_CENTER);
  lv_obj_set_size(strip_container, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_clear_flag(strip_container, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(strip_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(strip_container, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(strip_container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(strip_container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(strip_container, 4, LV_PART_MAIN);
  lv_obj_set_layout(strip_container, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(strip_container, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(strip_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(strip_container, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  WeatherHourlyStripHourRef hours[WEATHER_HOURLY_STRIP_HOUR_COUNT];
  for (int i = 0; i < WEATHER_HOURLY_STRIP_HOUR_COUNT; i++) {
    create_weather_hourly_strip_hour_column(
      strip_container, hour_font, icon_font, temp_font, text_color, hours[i]);
  }
  apply_width_compensation(strip_container, width_compensation_percent);
  register_weather_hourly_strip_card(
    s.btn, strip_container, hours, WEATHER_HOURLY_STRIP_HOUR_COUNT, p.entity);
}

constexpr lv_coord_t HERO_LAYOUT_REF_HEIGHT = 100;
constexpr lv_coord_t HERO_LAYOUT_MARGIN_BASE = 3;
constexpr lv_coord_t HERO_LAYOUT_GAP_BASE = 2;
constexpr lv_coord_t HERO_LAYOUT_BOTTOM_SAFE_BASE = 2;

inline lv_coord_t hero_layout_scaled_px(lv_coord_t btn_height, lv_coord_t base_px) {
  if (btn_height <= 0) return base_px;
  lv_coord_t inner = btn_height - HERO_LAYOUT_MARGIN_BASE * 2;
  if (inner <= 0) return base_px;
  lv_coord_t ref_inner = HERO_LAYOUT_REF_HEIGHT - HERO_LAYOUT_MARGIN_BASE * 2;
  return std::max(static_cast<lv_coord_t>(1),
    (base_px * inner + ref_inner / 2) / ref_inner);
}

struct HeroLayoutVertical {
  lv_coord_t y;
  lv_coord_t gap;
};

inline HeroLayoutVertical hero_layout_vertical(lv_obj_t *btn, lv_coord_t margin_v,
                                               lv_coord_t row_gap, lv_coord_t icon_h,
                                               lv_coord_t cond_h, lv_coord_t temp_h,
                                               lv_coord_t range_h) {
  HeroLayoutVertical result{margin_v, row_gap};
  if (!btn) return result;
  lv_coord_t btn_h = lv_obj_get_height(btn);
  lv_coord_t bottom_safe = hero_layout_scaled_px(btn_h, HERO_LAYOUT_BOTTOM_SAFE_BASE);
  lv_coord_t usable_h = btn_h - margin_v - bottom_safe;
  if (usable_h < 0) usable_h = 0;

  lv_coord_t gap = row_gap;
  lv_coord_t block_h = icon_h + cond_h + temp_h + range_h + gap * 3;
  while (gap > 0 && block_h > usable_h - margin_v) {
    gap--;
    block_h = icon_h + cond_h + temp_h + range_h + gap * 3;
  }

  lv_coord_t y = margin_v;
  if (block_h < usable_h - margin_v) {
    lv_coord_t extra = (usable_h - margin_v) - block_h;
    y = margin_v + extra / 2;
  }
  if (y + block_h > btn_h - bottom_safe) {
    y = btn_h - bottom_safe - block_h;
  }
  if (y < margin_v) y = margin_v;
  result.y = y;
  result.gap = gap;
  return result;
}

inline lv_obj_t *create_hero_metric_row(lv_obj_t *parent,
                                        const lv_font_t *value_font,
                                        const lv_font_t *unit_font,
                                        lv_color_t text_color,
                                        lv_obj_t **value_lbl,
                                        lv_obj_t **unit_lbl) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(row, 2, LV_PART_MAIN);
  lv_obj_set_layout(row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  lv_obj_t *value = lv_label_create(row);
  if (value_font) lv_obj_set_style_text_font(value, value_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(value, text_color, LV_PART_MAIN);
  lv_label_set_text(value, "--");

  lv_obj_t *unit = lv_label_create(row);
  if (unit_font) lv_obj_set_style_text_font(unit, unit_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(unit, text_color, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(unit, 0, LV_PART_MAIN);
  lv_label_set_text(unit, "");

  if (value_lbl) *value_lbl = value;
  if (unit_lbl) *unit_lbl = unit;
  return row;
}

inline void apply_weather_hero_card_layout(BtnSlot &s, lv_obj_t *range_row,
                                           lv_obj_t *today_lbl, lv_obj_t *today_unit_lbl,
                                           bool large_slot, bool large_numbers,
                                           const lv_font_t *icon_font = nullptr,
                                           const lv_font_t *label_font = nullptr,
                                           const lv_font_t *sensor_font = nullptr,
                                           const lv_font_t *large_sensor_font = nullptr,
                                           int large_unit_offset_percent = -10) {
  if (!s.btn || !s.icon_lbl || !s.text_lbl || !s.sensor_container || !range_row) return;
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(range_row, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_transform_scale_x(s.icon_lbl, 256, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(s.icon_lbl, 256, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_x(s.text_lbl, 256, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(s.text_lbl, 256, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_x(s.sensor_container, 256, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(s.sensor_container, 256, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_x(range_row, 256, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(range_row, 256, LV_PART_MAIN);
  lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(s.text_lbl, LV_SIZE_CONTENT);

  lv_coord_t btn_h = lv_obj_get_height(s.btn);
  lv_coord_t margin_v = hero_layout_scaled_px(btn_h, HERO_LAYOUT_MARGIN_BASE);
  lv_coord_t row_gap = hero_layout_scaled_px(btn_h, HERO_LAYOUT_GAP_BASE);

  const lv_font_t *value_font = label_font;
  if (large_slot) {
    if (large_numbers && large_sensor_font) value_font = large_sensor_font;
    else if (sensor_font) value_font = sensor_font;
  }

  if (icon_font) lv_obj_set_style_text_font(s.icon_lbl, icon_font, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(s.text_lbl, label_font, LV_PART_MAIN);
  if (value_font && s.sensor_lbl) lv_obj_set_style_text_font(s.sensor_lbl, value_font, LV_PART_MAIN);
  if (label_font && s.unit_lbl) {
    lv_obj_set_style_text_font(s.unit_lbl, label_font, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(s.unit_lbl, 0, LV_PART_MAIN);
    lv_obj_set_style_translate_y(s.unit_lbl, 0, LV_PART_MAIN);
  }
  lv_obj_set_style_pad_column(s.sensor_container, 2, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(s.sensor_container, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(s.sensor_container, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  if (today_lbl && value_font) lv_obj_set_style_text_font(today_lbl, value_font, LV_PART_MAIN);
  if (today_unit_lbl && label_font) {
    lv_obj_set_style_text_font(today_unit_lbl, label_font, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(today_unit_lbl, 0, LV_PART_MAIN);
    lv_obj_set_style_translate_y(today_unit_lbl, 0, LV_PART_MAIN);
  }

  lv_obj_update_layout(s.btn);

  lv_coord_t icon_h = lv_obj_get_height(s.icon_lbl);
  lv_coord_t cond_h = lv_obj_get_height(s.text_lbl);
  lv_coord_t temp_h = lv_obj_get_height(s.sensor_container);
  lv_coord_t range_h = lv_obj_get_height(range_row);
  HeroLayoutVertical layout = hero_layout_vertical(
    s.btn, margin_v, row_gap, icon_h, cond_h, temp_h, range_h);

  lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_MID, 0, layout.y);
  layout.y += icon_h + layout.gap;
  lv_obj_align(s.text_lbl, LV_ALIGN_TOP_MID, 0, layout.y);
  layout.y += cond_h + layout.gap;
  lv_obj_align(s.sensor_container, LV_ALIGN_TOP_MID, 0, layout.y);
  layout.y += temp_h + layout.gap;
  lv_obj_align(range_row, LV_ALIGN_TOP_MID, 0, layout.y);
}

inline void setup_weather_hero_card(BtnSlot &s, const ParsedCfg &p,
                                    bool has_sensor_color, uint32_t sensor_val,
                                    bool large_slot, bool large_numbers,
                                    const lv_font_t *icon_font = nullptr,
                                    const lv_font_t *label_font = nullptr,
                                    const lv_font_t *sensor_font = nullptr,
                                    const lv_font_t *large_sensor_font = nullptr,
                                    int large_unit_offset_percent = -10) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.icon_lbl, find_icon("Weather Cloudy"));
  lv_label_set_text(s.text_lbl, espcontrol_i18n(std::string("Cloudy")).c_str());
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, display_temperature_unit_symbol());

  lv_color_t text_color = s.text_lbl
    ? lv_obj_get_style_text_color(s.text_lbl, LV_PART_MAIN) : lv_color_white();
  lv_obj_t *today_lbl = nullptr;
  lv_obj_t *today_unit_lbl = nullptr;
  lv_obj_t *range_row = create_hero_metric_row(
    s.btn, label_font, label_font, text_color, &today_lbl, &today_unit_lbl);
  lv_label_set_text(today_lbl, "--/--");
  lv_label_set_text(today_unit_lbl, display_temperature_unit_symbol());

  register_weather_hero_card(
    s.btn, s.icon_lbl, s.text_lbl, s.sensor_lbl, s.unit_lbl,
    range_row, today_lbl, today_unit_lbl, p.entity);
  apply_weather_hero_card_layout(
    s, range_row, today_lbl, today_unit_lbl, large_slot, large_numbers, icon_font,
    label_font, sensor_font, large_sensor_font, large_unit_offset_percent);
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
  lv_label_set_text(s.unit_lbl, display_temperature_unit_symbol());
  std::string day = weather_card_forecast_day(p);
  std::string label = p.label.empty()
    ? (day == "today" ? espcontrol_i18n(std::string("Today")) : espcontrol_i18n(std::string("Tomorrow")))
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  apply_width_compensation(s.sensor_container, width_compensation_percent);
  apply_width_compensation(s.text_lbl, width_compensation_percent);
  register_weather_forecast_card(s.btn, s.icon_lbl, s.sensor_lbl, s.unit_lbl, s.text_lbl,
    p.entity, day, p.label);
}

inline void apply_push_button_transition(lv_obj_t *btn);
inline void clear_push_button_transition(lv_obj_t *btn);

inline void setup_garage_card(BtnSlot &s, const ParsedCfg &p) {
  if (garage_command_mode(p.sensor)) {
    lv_label_set_text(s.icon_lbl, garage_command_icon(p));
    lv_label_set_text(s.text_lbl, garage_card_show_status(p) ? "--" : garage_card_label(p));
    apply_push_button_transition(s.btn);
    return;
  }
  lv_label_set_text(s.icon_lbl, garage_closed_icon(p.icon));
  lv_label_set_text(s.text_lbl, garage_card_show_status(p) ? "--" : garage_card_label(p));
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

inline const char *screen_lock_locked_icon(const ParsedCfg &p) {
  (void) p;
  return find_icon("Lock");
}

inline const char *screen_lock_unlocked_icon(const ParsedCfg &p) {
  (void) p;
  return find_icon("Lock Open");
}

inline std::string screen_lock_card_label() {
  return screen_lock_enabled()
    ? espcontrol_i18n(std::string("Screen Locked"))
    : espcontrol_i18n(std::string("Screen Unlocked"));
}

inline void screen_lock_register_card(const BtnSlot &s, const ParsedCfg &p) {
  ScreenLockCardRef ref;
  ref.btn = s.btn;
  ref.icon_lbl = s.icon_lbl;
  ref.text_lbl = s.text_lbl;
  ref.locked_icon = screen_lock_locked_icon(p);
  ref.unlocked_icon = screen_lock_unlocked_icon(p);
  screen_lock_card_refs().push_back(ref);
  screen_lock_register_controlled_button(s.btn);
}

inline void setup_screen_lock_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl,
    screen_lock_enabled() ? screen_lock_locked_icon(p) : screen_lock_unlocked_icon(p));
  std::string label = screen_lock_card_label();
  lv_label_set_text(s.text_lbl, label.c_str());
  screen_lock_register_card(s, p);
  apply_push_button_transition(s.btn);
}

inline void apply_push_button_transition(lv_obj_t *btn) {
  if (!btn) return;
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

inline void clear_push_button_transition(lv_obj_t *btn) {
  if (!btn) return;
  lv_obj_remove_local_style_prop(btn, LV_STYLE_TRANSITION,
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
      lv_label_set_text(s.text_lbl, espcontrol_i18n("Push"));
    }
  }
}

inline void setup_local_action_card(BtnSlot &s, const ParsedCfg &p);

inline void setup_action_card(BtnSlot &s, const ParsedCfg &p) {
  if (action_card_local_action(p)) {
    setup_local_action_card(s, p);
    return;
  }
  std::string action_label = p.label.empty()
    ? (p.entity.empty() ? espcontrol_i18n(std::string("Action")) : p.entity)
    : p.label;
  lv_label_set_text(s.text_lbl, action_label.c_str());
  const char *icon_cp = (p.icon.empty() || p.icon == "Auto") ? find_icon("Flash") : find_icon(p.icon.c_str());
  lv_label_set_text(s.icon_lbl, icon_cp);
  if (action_card_state_icon_mode(p) || action_card_state_text_mode(p)) {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  } else if (action_card_state_numeric_mode(p)) {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s.sensor_lbl, "--");
    std::string unit = trim_display_unit(action_card_state_unit(p));
    lv_label_set_text(s.unit_lbl, unit.c_str());
  }
  apply_push_button_transition(s.btn);
}

inline void setup_local_action_card(BtnSlot &s, const ParsedCfg &p) {
  std::string label = p.label.empty() ? (p.entity.empty() ? "Local Action" : sentence_cap_text(p.entity)) : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  const char *icon_cp = (p.icon.empty() || p.icon == "Auto") ? find_icon("Gesture Tap") : find_icon(p.icon.c_str());
  lv_label_set_text(s.icon_lbl, icon_cp);
  apply_push_button_transition(s.btn);
}

inline void send_local_sensor_update(const std::string &key, float value) {
  for (auto &s : local_sensor_registry()) {
    if (s.key != key || s.is_text) continue;
    char buf[32];
    if (s.precision == 1) snprintf(buf, sizeof(buf), "%.1f", value);
    else if (s.precision == 2) snprintf(buf, sizeof(buf), "%.2f", value);
    else snprintf(buf, sizeof(buf), "%.0f", value);
    if (s.sensor_lbl) lv_label_set_text(s.sensor_lbl, buf);
    return;
  }
  ESP_LOGW("espcontrol", "Local sensor '%s' not registered", key.c_str());
}

inline void send_local_sensor_update(const std::string &key, const char *value) {
  for (auto &s : local_sensor_registry()) {
    if (s.key != key || !s.is_text) continue;
    if (s.text_lbl) set_wrapped_button_label_text(s.text_lbl, value ? value : "--");
    return;
  }
  ESP_LOGW("espcontrol", "Local sensor '%s' not registered", key.c_str());
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

inline void setup_local_sensor_card(BtnSlot &s, const ParsedCfg &p,
                                    bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);

  bool is_text = (p.precision == "text");
  LocalSensorControl ctrl;
  ctrl.key = p.entity;
  ctrl.is_text = is_text;
  ctrl.precision = 0;
  ctrl.sensor_lbl = nullptr;
  ctrl.text_lbl = nullptr;

  if (is_text) {
    setup_toggle_visual(s, p);
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    set_wrapped_button_label_text(s.text_lbl, "--");
    ctrl.text_lbl = s.text_lbl;
  } else {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s.sensor_lbl, "--");
    if (!p.unit.empty()) lv_label_set_text(s.unit_lbl, trim_display_unit(p.unit).c_str());
    if (!p.label.empty()) lv_label_set_text(s.text_lbl, p.label.c_str());
    ctrl.sensor_lbl = s.sensor_lbl;
    if (!p.precision.empty()) ctrl.precision = atoi(p.precision.c_str());
  }

  auto &reg = local_sensor_registry();
  bool found = false;
  for (auto &existing : reg) {
    if (existing.key == ctrl.key) { existing = ctrl; found = true; break; }
  }
  if (!found) reg.push_back(ctrl);

#ifdef USE_SENSOR
  if (!is_text) {
    for (auto *esp_s : esphome::App.get_sensors()) {
      char oid_buf[128];
      if (std::string(esp_s->get_object_id_to(oid_buf).c_str()) != ctrl.key) continue;
      auto *lbl = ctrl.sensor_lbl;
      int prec = ctrl.precision;
      esp_s->add_on_state_callback([lbl, prec](float val) {
        if (!lbl || std::isnan(val)) return;
        char buf[32];
        if (prec == 1) snprintf(buf, sizeof(buf), "%.1f", val);
        else if (prec == 2) snprintf(buf, sizeof(buf), "%.2f", val);
        else snprintf(buf, sizeof(buf), "%.0f", val);
        lv_label_set_text(lbl, buf);
      });
      if (!std::isnan(esp_s->state) && ctrl.sensor_lbl) {
        char buf[32];
        if (ctrl.precision == 1) snprintf(buf, sizeof(buf), "%.1f", esp_s->state);
        else if (ctrl.precision == 2) snprintf(buf, sizeof(buf), "%.2f", esp_s->state);
        else snprintf(buf, sizeof(buf), "%.0f", esp_s->state);
        lv_label_set_text(ctrl.sensor_lbl, buf);
      }
      break;
    }
  }
#endif
#ifdef USE_TEXT_SENSOR
  if (is_text) {
    for (auto *esp_ts : esphome::App.get_text_sensors()) {
      char oid_buf[128];
      if (std::string(esp_ts->get_object_id_to(oid_buf).c_str()) != ctrl.key) continue;
      auto *lbl = ctrl.text_lbl;
      esp_ts->add_on_state_callback([lbl](std::string val) {
        if (!lbl) return;
        set_wrapped_button_label_text(lbl, val);
      });
      if (!esp_ts->state.empty() && ctrl.text_lbl)
        set_wrapped_button_label_text(ctrl.text_lbl, esp_ts->state);
      break;
    }
  }
#endif
}

inline const char *door_window_closed_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(door_window_closed_icon_name(p.precision));
}

inline const char *door_window_open_icon(const ParsedCfg &p) {
  if (!p.icon_on.empty() && p.icon_on != "Auto") return find_icon(p.icon_on.c_str());
  return find_icon(door_window_open_icon_name(p.precision));
}

inline void setup_door_window_card(BtnSlot &s, const ParsedCfg &p,
                                   bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.icon_lbl, door_window_closed_icon(p));
  std::string label = p.label.empty()
    ? (normalize_door_window_subtype(p.precision) == "window"
        ? espcontrol_i18n(std::string("Window"))
        : espcontrol_i18n(std::string("Door")))
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
}

inline const char *presence_clear_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon("Motion Sensor Off");
}

inline const char *presence_detected_icon(const ParsedCfg &p) {
  if (!p.icon_on.empty() && p.icon_on != "Auto") return find_icon(p.icon_on.c_str());
  return find_icon("Motion Sensor");
}

inline void setup_presence_card(BtnSlot &s, const ParsedCfg &p,
                                bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.icon_lbl, presence_clear_icon(p));
  std::string label = p.label.empty() ? espcontrol_i18n(std::string("Presence")) : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
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
                                            const lv_font_t *value_font,
                                            bool subpage_chevron_enabled = true,
                                            int subpage_chevron_x = 0,
                                            int subpage_chevron_y = 2,
                                            int subpage_chevron_text_width_percent = 94) {
  setup_toggle_visual(s, p);
  if (p.precision == "text") {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    set_wrapped_button_label_text(s.text_lbl, "--");
    set_subpage_chevron_visible(
      s, subpage_chevron_enabled, subpage_chevron_x, subpage_chevron_y,
      subpage_chevron_text_width_percent);
    return;
  }

  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (value_font) lv_obj_set_style_text_font(s.sensor_lbl, value_font, LV_PART_MAIN);
  lv_label_set_text(s.sensor_lbl, "--");
  std::string unit = trim_display_unit(p.unit);
  lv_label_set_text(s.unit_lbl, unit.c_str());
  std::string subpage_label = p.label.empty() ? espcontrol_i18n(std::string("Subpage")) : p.label;
  lv_label_set_text(s.text_lbl, subpage_label.c_str());
  set_subpage_chevron_visible(
    s, subpage_chevron_enabled, subpage_chevron_x, subpage_chevron_y,
    subpage_chevron_text_width_percent);
}
