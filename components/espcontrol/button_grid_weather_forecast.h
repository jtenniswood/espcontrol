#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

inline std::string normalize_weather_state(std::string state) {
  state = trim_display_unit(state);
  std::string normalized;
  normalized.reserve(state.size());
  bool last_dash = false;
  for (char ch : state) {
    unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch)) {
      normalized.push_back(static_cast<char>(std::tolower(uch)));
      last_dash = false;
    } else if (!last_dash) {
      normalized.push_back('-');
      last_dash = true;
    }
  }
  while (!normalized.empty() && normalized.back() == '-') normalized.pop_back();
  if (normalized.compare(0, 4, "mdi-") == 0) normalized = normalized.substr(4);
  if (normalized.compare(0, 8, "weather-") == 0) normalized = normalized.substr(8);
  if (normalized == "clear" || normalized == "mostly-clear" || normalized == "mostly-sunny") return "sunny";
  if (normalized == "clear-day") return "sunny";
  if (normalized == "overcast" || normalized == "broken-clouds" ||
      normalized == "mostly-cloudy" || normalized == "scattered-clouds") return "cloudy";
  if (normalized == "foggy") return "fog";
  if (normalized == "night") return "clear-night";
  if (normalized == "mostly-clear-night" || normalized == "night-clear") return "clear-night";
  if (normalized == "partly-cloudy") return "partlycloudy";
  if (normalized == "partly-sunny" || normalized == "few-clouds") return "partlycloudy";
  if (normalized == "partly-cloudy-day") return "partlycloudy";
  if (normalized == "partly-cloudy-night" || normalized == "mostly-cloudy-night" ||
      normalized == "cloudy-night" || normalized == "few-clouds-night" ||
      normalized == "night-cloudy") return "night-partly-cloudy";
  if (normalized == "drizzle" || normalized == "light-rain" ||
      normalized == "rain" || normalized == "showers") return "rainy";
  if (normalized == "heavy-rain" || normalized == "heavy-showers") return "pouring";
  if (normalized == "possibly-rainy-day" || normalized == "possibly-rainy-night") return "rainy";
  if (normalized == "possibly-sleet-day" || normalized == "possibly-sleet-night") return "snowy-rainy";
  if (normalized == "possibly-snow-day" || normalized == "possibly-snow-night") return "snowy";
  if (normalized == "possibly-thunderstorm-day" || normalized == "possibly-thunderstorm-night") return "lightning-rainy";
  if (normalized == "freezing-rain") return "snowy-rainy";
  if (normalized == "blizzard" || normalized == "heavy-snow") return "snowy-heavy";
  if (normalized == "sleet") return "snowy-rainy";
  if (normalized == "snow") return "snowy";
  if (normalized == "storm" || normalized == "stormy" ||
      normalized == "thunderstorm" || normalized == "thunderstorms") return "lightning";
  if (normalized == "sunny-off" || normalized == "unknown") return "unavailable";
  return normalized;
}

inline const char* weather_icon_for_state(const std::string &state) {
  std::string normalized = normalize_weather_state(state);
  if (normalized == "sunny") return find_icon("Weather Sunny");
  if (normalized == "clear-night") return find_icon("Weather Night");
  if (normalized == "partlycloudy") return find_icon("Weather Partly Cloudy");
  if (normalized == "cloudy") return find_icon("Weather Cloudy");
  if (normalized == "cloudy-alert") return find_icon("Weather Cloudy Alert");
  if (normalized == "dust") return find_icon("Weather Dust");
  if (normalized == "fog") return find_icon("Weather Fog");
  if (normalized == "hail") return find_icon("Weather Hail");
  if (normalized == "hazy") return find_icon("Weather Hazy");
  if (normalized == "hurricane") return find_icon("Weather Hurricane");
  if (normalized == "lightning") return find_icon("Weather Lightning");
  if (normalized == "lightning-rainy") return find_icon("Weather Lightning Rainy");
  if (normalized == "night-partly-cloudy") return find_icon("Weather Night Cloudy");
  if (normalized == "partly-lightning") return find_icon("Weather Partly Lightning");
  if (normalized == "partly-rainy") return find_icon("Weather Partly Rainy");
  if (normalized == "partly-snowy") return find_icon("Weather Partly Snowy");
  if (normalized == "partly-snowy-rainy") return find_icon("Weather Partly Snowy Rainy");
  if (normalized == "pouring") return find_icon("Weather Pouring");
  if (normalized == "rainy") return find_icon("Weather Rainy");
  if (normalized == "snowy") return find_icon("Weather Snowy");
  if (normalized == "snowy-heavy") return find_icon("Weather Snowy Heavy");
  if (normalized == "snowy-rainy") return find_icon("Weather Snowy Rainy");
  if (normalized == "sunny-alert") return find_icon("Weather Sunny Alert");
  if (normalized == "sunset") return find_icon("Weather Sunset");
  if (normalized == "sunset-down") return find_icon("Weather Sunset Down");
  if (normalized == "sunset-up") return find_icon("Weather Sunset Up");
  if (normalized == "tornado") return find_icon("Weather Tornado");
  if (normalized == "windy") return find_icon("Weather Windy");
  if (normalized == "windy-variant") return find_icon("Weather Windy Variant");
  if (normalized == "unavailable" || normalized.empty()) return find_icon("Weather Sunny Off");
  return find_icon("Weather Cloudy Alert");
}

inline std::string weather_label_for_state(const std::string &state) {
  std::string normalized = normalize_weather_state(state);
  if (normalized == "sunny") return espcontrol_i18n(std::string("Sunny"));
  if (normalized == "clear-night") return espcontrol_i18n(std::string("Clear Night"));
  if (normalized == "partlycloudy") return espcontrol_i18n(std::string("Partly Cloudy"));
  if (normalized == "cloudy") return espcontrol_i18n(std::string("Cloudy"));
  if (normalized == "cloudy-alert") return espcontrol_i18n(std::string("Cloudy Alert"));
  if (normalized == "dust") return espcontrol_i18n(std::string("Dust"));
  if (normalized == "fog") return espcontrol_i18n(std::string("Fog"));
  if (normalized == "hail") return espcontrol_i18n(std::string("Hail"));
  if (normalized == "hazy") return espcontrol_i18n(std::string("Hazy"));
  if (normalized == "hurricane") return espcontrol_i18n(std::string("Hurricane"));
  if (normalized == "lightning") return espcontrol_i18n(std::string("Lightning"));
  if (normalized == "lightning-rainy") return espcontrol_i18n(std::string("Lightning And Rain"));
  if (normalized == "night-partly-cloudy") return espcontrol_i18n(std::string("Partly Cloudy Night"));
  if (normalized == "partly-lightning") return espcontrol_i18n(std::string("Partly Lightning"));
  if (normalized == "partly-rainy") return espcontrol_i18n(std::string("Partly Rainy"));
  if (normalized == "partly-snowy") return espcontrol_i18n(std::string("Partly Snowy"));
  if (normalized == "partly-snowy-rainy") return espcontrol_i18n(std::string("Partly Snow And Rain"));
  if (normalized == "pouring") return espcontrol_i18n(std::string("Pouring"));
  if (normalized == "rainy") return espcontrol_i18n(std::string("Rainy"));
  if (normalized == "snowy") return espcontrol_i18n(std::string("Snowy"));
  if (normalized == "snowy-heavy") return espcontrol_i18n(std::string("Heavy Snow"));
  if (normalized == "snowy-rainy") return espcontrol_i18n(std::string("Snowy And Rain"));
  if (normalized == "sunny-alert") return espcontrol_i18n(std::string("Sunny Alert"));
  if (normalized == "sunset") return espcontrol_i18n(std::string("Sunset"));
  if (normalized == "sunset-down") return espcontrol_i18n(std::string("Sunset Down"));
  if (normalized == "sunset-up") return espcontrol_i18n(std::string("Sunset Up"));
  if (normalized == "tornado") return espcontrol_i18n(std::string("Tornado"));
  if (normalized == "windy") return espcontrol_i18n(std::string("Windy"));
  if (normalized == "windy-variant") return espcontrol_i18n(std::string("Windy And Cloudy"));
  if (normalized == "exceptional") return espcontrol_i18n(std::string("Exceptional"));
  if (normalized == "unknown") return espcontrol_i18n(std::string("Unknown"));
  if (normalized == "unavailable" || normalized.empty()) return espcontrol_i18n(std::string("Unavailable"));

  return sentence_cap_text(state);
}

#if defined(ESPCONTROL_DISABLE_WEATHER_FORECAST) && ESPCONTROL_DISABLE_WEATHER_FORECAST

inline void reset_weather_forecast_cards() {}

inline void refresh_weather_forecast_card_visuals() {}

inline void register_weather_forecast_card(lv_obj_t *btn,
                                           lv_obj_t *value_lbl, lv_obj_t *unit_lbl,
                                           lv_obj_t *label_lbl,
                                           const std::string &entity_id,
                                           const std::string &day,
                                           const std::string &label) {
  (void) btn;
  (void) value_lbl;
  (void) unit_lbl;
  (void) label_lbl;
  (void) entity_id;
  (void) day;
  (void) label;
}

inline void weather_forecast_cancel_pending_requests() {}

inline bool weather_forecast_cancel_stale_requests() { return false; }

inline void weather_forecast_send_next_queued() {}

inline void refresh_weather_forecast_cards() {}

#else

struct WeatherForecastCardRef {
  lv_obj_t *btn;
  lv_obj_t *value_lbl;
  lv_obj_t *unit_lbl;
  lv_obj_t *label_lbl;
  std::string entity_id;
  std::string day;
  std::string label;
  std::string status_label;
  bool valid = false;
  float high = 0.0f;
  float low = 0.0f;
  std::string source_unit;
};

inline WeatherForecastCardRef *weather_forecast_card_refs() {
  static WeatherForecastCardRef refs[MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS];
  return refs;
}

inline int &weather_forecast_card_count() {
  static int count = 0;
  return count;
}

inline void reset_weather_forecast_cards() {
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  for (int i = 0; i < MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS; i++) {
    refs[i] = WeatherForecastCardRef();
  }
  weather_forecast_card_count() = 0;
}

constexpr float WEATHER_FORECAST_TEMP_MISSING = 32767.0f;
constexpr int WEATHER_FORECAST_PENDING_MAX = 8;
constexpr uint32_t WEATHER_FORECAST_REQUEST_TIMEOUT_MS = 60000;
constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;

struct WeatherForecastPendingRequest {
  uint32_t call_id = 0;
  uint32_t started_ms = 0;
  std::string entity_id;
  std::string day;
};

struct WeatherForecastQueuedRequest {
  std::string entity_id;
  std::string day;
};

struct WeatherForecastRetryRequest {
  std::string entity_id;
  std::string day;
  uint32_t due_ms = 0;
};

inline std::string weather_forecast_unit_symbol(const std::string &unit) {
  (void)unit;
  return display_temperature_unit_symbol();
}

inline int weather_forecast_display_temp(float value, const std::string &unit) {
  if (value == WEATHER_FORECAST_TEMP_MISSING) return value;
  float converted = convert_temperature_value_for_display_float(value, unit);
  return static_cast<int>(converted >= 0.0f ? converted + 0.5f : converted - 0.5f);
}

inline void apply_weather_forecast_card_text(const WeatherForecastCardRef &ref,
                                             bool valid, float high, float low,
                                             const std::string &unit) {
  if (ref.label_lbl) {
    std::string label = !ref.status_label.empty()
      ? ref.status_label
      : (ref.label.empty()
          ? (ref.day == "today" ? espcontrol_i18n(std::string("Today")) :
              (ref.day == "3day" ? espcontrol_i18n(std::string("3-Day Forecast")) : espcontrol_i18n(std::string("Tomorrow"))))
          : ref.label);
    lv_label_set_text(ref.label_lbl, label.c_str());
  }
  if (!ref.value_lbl || !ref.unit_lbl) return;
  if (!valid) {
    lv_label_set_text(ref.value_lbl, "--/--");
    std::string normalized_unit = weather_forecast_unit_symbol(unit);
    lv_label_set_text(ref.unit_lbl, normalized_unit.c_str());
    return;
  }
  char buf[24];
  char high_buf[12];
  char low_buf[12];
  if (high == WEATHER_FORECAST_TEMP_MISSING) snprintf(high_buf, sizeof(high_buf), "--");
  else snprintf(high_buf, sizeof(high_buf), "%d", weather_forecast_display_temp(high, unit));
  if (low == WEATHER_FORECAST_TEMP_MISSING) snprintf(low_buf, sizeof(low_buf), "--");
  else snprintf(low_buf, sizeof(low_buf), "%d", weather_forecast_display_temp(low, unit));
  snprintf(buf, sizeof(buf), "%s/%s", high_buf, low_buf);
  lv_label_set_text(ref.value_lbl, buf);
  std::string normalized_unit = weather_forecast_unit_symbol(unit);
  lv_label_set_text(ref.unit_lbl, normalized_unit.c_str());
}

inline bool weather_forecast_card_ref_ready(const WeatherForecastCardRef &ref) {
  if (!esphome::App.is_setup_complete()) return false;
  if (!lv_display_get_default()) return false;
  if (!ref.btn || !ref.value_lbl || !ref.unit_lbl) return false;
  if (!lv_obj_is_valid(ref.btn)) return false;
  if (!lv_obj_is_valid(ref.value_lbl)) return false;
  if (!lv_obj_is_valid(ref.unit_lbl)) return false;
  if (ref.label_lbl && !lv_obj_is_valid(ref.label_lbl)) return false;
  return true;
}

inline void refresh_weather_forecast_card_visuals() {
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  bool updated = false;
  for (int i = 0; i < count; i++) {
    if (!weather_forecast_card_ref_ready(refs[i])) continue;
    apply_weather_forecast_card_text(refs[i], refs[i].valid, refs[i].high,
                                     refs[i].low, refs[i].source_unit);
    updated = true;
  }
  if (updated) notify_dashboard_content_changed();
}

inline lv_timer_t *&weather_forecast_visual_refresh_timer() {
  static lv_timer_t *timer = nullptr;
  return timer;
}

inline void weather_forecast_apply_visuals_cb(lv_timer_t *timer) {
  lv_timer_t *&active_timer = weather_forecast_visual_refresh_timer();
  if (active_timer == timer) active_timer = nullptr;
  lv_timer_del(timer);
  refresh_weather_forecast_card_visuals();
}

inline void weather_forecast_schedule_visual_refresh() {
  lv_timer_t *&timer = weather_forecast_visual_refresh_timer();
  if (timer) lv_timer_reset(timer);
  else timer = lv_timer_create(weather_forecast_apply_visuals_cb, 25, nullptr);
}

inline void apply_weather_forecast_to_entity(const std::string &entity_id,
                                             const std::string &day,
                                             bool valid, float high, float low,
                                             const std::string &unit) {
  ESP_LOGI("weather_forecast", "Applying %s forecast for %s: %s high=%.1f low=%.1f unit=%s",
    day.c_str(), entity_id.c_str(), valid ? "valid" : "unavailable",
    high, low, unit.c_str());
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  for (int i = 0; i < count; i++) {
    if (refs[i].entity_id == entity_id && refs[i].day == day) {
      refs[i].valid = valid;
      refs[i].high = high;
      refs[i].low = low;
      refs[i].source_unit = unit;
      refs[i].status_label = "";
      weather_forecast_schedule_visual_refresh();
    }
  }
}

inline void apply_weather_forecast_unavailable_for_entity(const std::string &entity_id) {
  ESP_LOGW("weather_forecast", "Marking forecast unavailable for %s", entity_id.c_str());
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  for (int i = 0; i < count; i++) {
    if (refs[i].entity_id == entity_id) {
      refs[i].valid = false;
      refs[i].high = 0;
      refs[i].low = 0;
      refs[i].source_unit = "";
      refs[i].status_label = "";
      weather_forecast_schedule_visual_refresh();
    }
  }
}

inline void apply_weather_forecast_unavailable_all() {
  ESP_LOGW("weather_forecast", "Marking all forecast cards unavailable");
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  for (int i = 0; i < count; i++) {
    refs[i].valid = false;
    refs[i].high = 0;
    refs[i].low = 0;
    refs[i].source_unit = "";
    refs[i].status_label = "";
    weather_forecast_schedule_visual_refresh();
  }
}

inline void apply_weather_forecast_actions_required_for_entity(const std::string &entity_id) {
  ESP_LOGW("weather_forecast",
    "Forecast request timed out for %s; check that this ESPHome device is allowed to perform Home Assistant actions",
    entity_id.c_str());
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  for (int i = 0; i < count; i++) {
    if (refs[i].entity_id == entity_id) {
      refs[i].valid = false;
      refs[i].high = 0;
      refs[i].low = 0;
      refs[i].source_unit = "";
      refs[i].status_label = "";
      weather_forecast_schedule_visual_refresh();
    }
  }
}

inline bool weather_forecast_error_is_timeout(const std::string &message) {
  std::string lower;
  lower.reserve(message.size());
  for (char ch : message) {
    lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return lower.find("timeout") != std::string::npos ||
         lower.find("timed out") != std::string::npos;
}

inline bool weather_forecast_request_matches(const std::string &entity_id,
                                             const std::string &day,
                                             const std::string &other_entity_id,
                                             const std::string &other_day) {
  return entity_id == other_entity_id && day == other_day;
}

inline void register_weather_forecast_card(lv_obj_t *btn,
                                           lv_obj_t *value_lbl, lv_obj_t *unit_lbl,
                                           lv_obj_t *label_lbl,
                                           const std::string &entity_id,
                                           const std::string &day,
                                           const std::string &label) {
  int &count = weather_forecast_card_count();
  if (count >= MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("weather_forecast", "Too many forecast cards; skipping updates");
    return;
  }
  weather_forecast_card_refs()[count++] = {
    btn, value_lbl, unit_lbl, label_lbl, entity_id, day, label, "", false, 0, 0, ""
  };
  apply_weather_forecast_card_text(weather_forecast_card_refs()[count - 1], false, 0, 0, "");
}

inline bool weather_forecast_entity_id_safe(const std::string &entity_id) {
  if (entity_id.compare(0, 8, "weather.") != 0) return false;
  for (char ch : entity_id) {
    if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.')) return false;
  }
  return true;
}

inline bool parse_weather_forecast_temp(const std::string &value, float &out) {
  if (value.empty()) return false;
  char *end = nullptr;
  float parsed = strtof(value.c_str(), &end);
  if (end == value.c_str()) return false;
  if (!std::isfinite(parsed)) return false;
  out = parsed;
  return true;
}

struct WeatherForecastPayload {
  bool today_valid = false;
  float today_high = WEATHER_FORECAST_TEMP_MISSING;
  float today_low = WEATHER_FORECAST_TEMP_MISSING;
  bool tomorrow_valid = false;
  float tomorrow_high = WEATHER_FORECAST_TEMP_MISSING;
  float tomorrow_low = WEATHER_FORECAST_TEMP_MISSING;
  bool three_day_valid = false;
  float three_day_high = WEATHER_FORECAST_TEMP_MISSING;
  float three_day_low = WEATHER_FORECAST_TEMP_MISSING;
  std::string unit;
};

inline bool parse_weather_forecast_payload(const std::string &payload,
                                           WeatherForecastPayload &out) {
  size_t p1 = payload.find('|');
  if (p1 == std::string::npos) return false;
  size_t p2 = payload.find('|', p1 + 1);
  if (p2 == std::string::npos) return false;
  size_t p3 = payload.find('|', p2 + 1);
  if (p3 == std::string::npos) return false;
  size_t p4 = payload.find('|', p3 + 1);
  if (p4 == std::string::npos) return false;
  size_t p5 = payload.find('|', p4 + 1);
  if (p5 == std::string::npos) return false;
  size_t p6 = payload.find('|', p5 + 1);
  if (p6 == std::string::npos) return false;

  std::string today_high_text = payload.substr(0, p1);
  std::string today_low_text = payload.substr(p1 + 1, p2 - p1 - 1);
  std::string tomorrow_high_text = payload.substr(p2 + 1, p3 - p2 - 1);
  std::string tomorrow_low_text = payload.substr(p3 + 1, p4 - p3 - 1);
  std::string three_day_high_text = payload.substr(p4 + 1, p5 - p4 - 1);
  std::string three_day_low_text = payload.substr(p5 + 1, p6 - p5 - 1);
  out.unit = payload.substr(p6 + 1);

  bool today_has_high = parse_weather_forecast_temp(today_high_text, out.today_high);
  bool today_has_low = parse_weather_forecast_temp(today_low_text, out.today_low);
  bool tomorrow_has_high = parse_weather_forecast_temp(tomorrow_high_text, out.tomorrow_high);
  bool tomorrow_has_low = parse_weather_forecast_temp(tomorrow_low_text, out.tomorrow_low);
  bool three_day_has_high = parse_weather_forecast_temp(three_day_high_text, out.three_day_high);
  bool three_day_has_low = parse_weather_forecast_temp(three_day_low_text, out.three_day_low);
  out.today_valid = today_has_high || today_has_low;
  out.tomorrow_valid = tomorrow_has_high || tomorrow_has_low;
  out.three_day_valid = three_day_has_high || three_day_has_low;
  return out.today_valid || out.tomorrow_valid || out.three_day_valid;
}

inline void weather_forecast_extend_range(bool valid, float high, float low,
                                          bool &range_valid,
                                          float &range_high, float &range_low) {
  if (!valid) return;
  if (high != WEATHER_FORECAST_TEMP_MISSING) {
    if (!range_valid || range_high == WEATHER_FORECAST_TEMP_MISSING || high > range_high) {
      range_high = high;
    }
  }
  if (low != WEATHER_FORECAST_TEMP_MISSING) {
    if (!range_valid || range_low == WEATHER_FORECAST_TEMP_MISSING || low < range_low) {
      range_low = low;
    }
  }
  range_valid = true;
}

inline std::string weather_forecast_response_template(const std::string &entity_id) {
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set response_data = response if response is defined and response is not none else {} %}"
    "{% set entity_response = response_data if 'forecast' in response_data else (response_data[entity] if entity in response_data else {}) %}"
    "{% set forecasts = entity_response['forecast'] if 'forecast' in entity_response else [] %}"
    "{% set today_date = now().date() %}{% set tomorrow_date = (now() + timedelta(days=1)).date() %}{% set third_date = (now() + timedelta(days=2)).date() %}"
    "{% set ns = namespace(today=none, tomorrow=none, third=none) %}{% for item in forecasts %}"
    "{% set item_dt = as_datetime(item['datetime']) if 'datetime' in item else none %}{% set item_date = as_local(item_dt).date() if item_dt is not none else (as_datetime(item['date']).date() if 'date' in item else none) %}"
    "{% if item_date == today_date and ns.today is none %}{% set ns.today = item %}{% elif item_date == tomorrow_date and ns.tomorrow is none %}{% set ns.tomorrow = item %}{% elif item_date == third_date and ns.third is none %}{% set ns.third = item %}{% endif %}"
    "{% endfor %}"
    "{% set today = ns.today if ns.today is not none else (forecasts[0] if forecasts|length > 0 else none) %}"
    "{% set tomorrow = ns.tomorrow if ns.tomorrow is not none else (forecasts[1] if forecasts|length > 1 else none) %}"
    "{% set third = ns.third if ns.third is not none else (forecasts[2] if forecasts|length > 2 else none) %}"
    "{% set high_keys = ['temperature','native_temperature','temperature_high','native_temperature_high','high_temperature','max_temperature','temperature_max','temp_high','max_temp','high'] %}"
    "{% set low_keys = ['templow','native_templow','temperature_low','native_temperature_low','low_temperature','min_temperature','temperature_min','temp_low','min_temp','low'] %}"
    "{% set unit_keys = ['temperature_unit','native_temperature_unit','unit_of_measurement','native_unit_of_measurement','unit'] %}"
    "{% set out = namespace(today_high='', today_low='', tomorrow_high='', tomorrow_low='', third_high='', third_low='', unit='') %}"
    "{% for key in high_keys %}{% if out.today_high == '' and today is not none and key in today %}{% set out.today_high = today[key] %}{% endif %}{% if out.tomorrow_high == '' and tomorrow is not none and key in tomorrow %}{% set out.tomorrow_high = tomorrow[key] %}{% endif %}{% if out.third_high == '' and third is not none and key in third %}{% set out.third_high = third[key] %}{% endif %}{% endfor %}"
    "{% for key in low_keys %}{% if out.today_low == '' and today is not none and key in today %}{% set out.today_low = today[key] %}{% endif %}{% if out.tomorrow_low == '' and tomorrow is not none and key in tomorrow %}{% set out.tomorrow_low = tomorrow[key] %}{% endif %}{% if out.third_low == '' and third is not none and key in third %}{% set out.third_low = third[key] %}{% endif %}{% endfor %}"
    "{% for key in unit_keys %}{% if out.unit == '' and key in entity_response %}{% set out.unit = entity_response[key] %}{% endif %}{% if out.unit == '' and today is not none and key in today %}{% set out.unit = today[key] %}{% endif %}{% if out.unit == '' and tomorrow is not none and key in tomorrow %}{% set out.unit = tomorrow[key] %}{% endif %}{% if out.unit == '' and third is not none and key in third %}{% set out.unit = third[key] %}{% endif %}{% endfor %}"
    "{{ out.today_high }}|{{ out.today_low }}|{{ out.tomorrow_high }}|{{ out.tomorrow_low }}|{{ out.third_high }}|{{ out.third_low }}|"
    "{{ out.unit or state_attr(entity, 'temperature_unit') or state_attr(entity, 'native_temperature_unit') or state_attr(entity, 'unit_of_measurement') or '' }}";
}

inline uint32_t next_weather_forecast_call_id() {
  static uint32_t call_id = 1;
  return call_id++;
}

inline WeatherForecastPendingRequest *weather_forecast_pending_requests() {
  static WeatherForecastPendingRequest requests[WEATHER_FORECAST_PENDING_MAX];
  return requests;
}

inline WeatherForecastQueuedRequest *weather_forecast_queued_requests() {
  static WeatherForecastQueuedRequest requests[WEATHER_FORECAST_PENDING_MAX];
  return requests;
}

inline WeatherForecastRetryRequest *weather_forecast_retry_requests() {
  static WeatherForecastRetryRequest requests[WEATHER_FORECAST_PENDING_MAX];
  return requests;
}

inline uint32_t &weather_forecast_action_ready_ms() {
  static uint32_t due_ms = 0;
  return due_ms;
}

inline bool weather_forecast_actions_ready() {
  if (!ha_api_state_connected()) {
    weather_forecast_action_ready_ms() = 0;
    return false;
  }
  uint32_t &due_ms = weather_forecast_action_ready_ms();
  uint32_t now = esphome::millis();
  if (due_ms == 0) {
    due_ms = now + 10000;
    ESP_LOGI("weather_forecast",
      "Waiting 10 seconds for Home Assistant action subscription before requesting forecasts");
    return false;
  }
  return (int32_t) (now - due_ms) >= 0;
}

inline bool weather_forecast_pending_key(const std::string &entity_id,
                                         const std::string &day) {
  WeatherForecastPendingRequest *requests = weather_forecast_pending_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id != 0 &&
        weather_forecast_request_matches(entity_id, day, requests[i].entity_id, requests[i].day)) {
      return true;
    }
  }
  return false;
}

inline bool weather_forecast_queue_key(const std::string &entity_id,
                                       const std::string &day) {
  WeatherForecastQueuedRequest *requests = weather_forecast_queued_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (!requests[i].entity_id.empty() &&
        weather_forecast_request_matches(entity_id, day, requests[i].entity_id, requests[i].day)) {
      return true;
    }
  }
  return false;
}

inline bool weather_forecast_retry_key(const std::string &entity_id,
                                       const std::string &day) {
  WeatherForecastRetryRequest *requests = weather_forecast_retry_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (!requests[i].entity_id.empty() &&
        weather_forecast_request_matches(entity_id, day, requests[i].entity_id, requests[i].day)) {
      return true;
    }
  }
  return false;
}

inline bool weather_forecast_any_pending() {
  WeatherForecastPendingRequest *requests = weather_forecast_pending_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id != 0) return true;
  }
  return false;
}

inline bool weather_forecast_track_pending(uint32_t call_id,
                                           const std::string &entity_id,
                                           const std::string &day) {
  if (call_id == 0) return false;
  WeatherForecastPendingRequest *requests = weather_forecast_pending_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id == call_id) return true;
  }
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id == 0) {
      requests[i].call_id = call_id;
      requests[i].started_ms = esphome::millis();
      requests[i].entity_id = entity_id;
      requests[i].day = day;
      return true;
    }
  }
  return false;
}

inline void weather_forecast_clear_pending(uint32_t call_id) {
  if (call_id == 0) return;
  WeatherForecastPendingRequest *requests = weather_forecast_pending_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id == call_id) requests[i] = WeatherForecastPendingRequest();
  }
}

inline void weather_forecast_clear_queue() {
  WeatherForecastQueuedRequest *requests = weather_forecast_queued_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    requests[i] = WeatherForecastQueuedRequest();
  }
}

inline void weather_forecast_clear_retry(const std::string &entity_id,
                                         const std::string &day) {
  WeatherForecastRetryRequest *requests = weather_forecast_retry_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (!requests[i].entity_id.empty() &&
        weather_forecast_request_matches(entity_id, day, requests[i].entity_id, requests[i].day)) {
      requests[i] = WeatherForecastRetryRequest();
    }
  }
}

inline void weather_forecast_clear_retries() {
  WeatherForecastRetryRequest *requests = weather_forecast_retry_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    requests[i] = WeatherForecastRetryRequest();
  }
}

inline bool weather_forecast_schedule_retry(const std::string &entity_id,
                                            const std::string &day,
                                            const char *reason) {
  if (!weather_forecast_entity_id_safe(entity_id)) return false;
  if (weather_forecast_pending_key(entity_id, day) ||
      weather_forecast_queue_key(entity_id, day) ||
      weather_forecast_retry_key(entity_id, day)) {
    return true;
  }
  WeatherForecastRetryRequest *requests = weather_forecast_retry_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) {
      requests[i].entity_id = entity_id;
      requests[i].day = day;
      requests[i].due_ms = esphome::millis() + WEATHER_FORECAST_RETRY_DELAY_MS;
      ESP_LOGW("weather_forecast", "Retrying forecast request for %s in %u seconds: %s",
        entity_id.c_str(), (unsigned) (WEATHER_FORECAST_RETRY_DELAY_MS / 1000),
        reason ? reason : "failed");
      return true;
    }
  }
  ESP_LOGW("weather_forecast", "Too many delayed forecast retries; skipping %s",
    entity_id.c_str());
  return false;
}

inline bool weather_forecast_enqueue(const std::string &entity_id,
                                     const std::string &day) {
  if (!weather_forecast_entity_id_safe(entity_id)) return false;
  weather_forecast_clear_retry(entity_id, day);
  if (weather_forecast_pending_key(entity_id, day) ||
      weather_forecast_queue_key(entity_id, day)) {
    return true;
  }
  WeatherForecastQueuedRequest *requests = weather_forecast_queued_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) {
      requests[i].entity_id = entity_id;
      requests[i].day = day;
      return true;
    }
  }
  ESP_LOGW("weather_forecast", "Too many queued forecast requests; skipping %s",
    entity_id.c_str());
  return false;
}

inline bool weather_forecast_dequeue(std::string &entity_id,
                                     std::string &day) {
  WeatherForecastQueuedRequest *requests = weather_forecast_queued_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) continue;
    entity_id = requests[i].entity_id;
    day = requests[i].day;
    requests[i] = WeatherForecastQueuedRequest();
    return true;
  }
  return false;
}

inline bool weather_forecast_enqueue_due_retries() {
  if (!ha_api_state_connected()) return false;
  WeatherForecastRetryRequest *requests = weather_forecast_retry_requests();
  uint32_t now = esphome::millis();
  bool queued = false;
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) continue;
    if ((int32_t) (now - requests[i].due_ms) < 0) continue;
    std::string entity_id = requests[i].entity_id;
    std::string day = requests[i].day;
    requests[i] = WeatherForecastRetryRequest();
    queued = weather_forecast_enqueue(entity_id, day) || queued;
  }
  return queued;
}

inline void weather_forecast_send_next_queued();

inline void weather_forecast_cancel_pending_requests() {
  weather_forecast_action_ready_ms() = 0;
  weather_forecast_clear_queue();
  weather_forecast_clear_retries();
  WeatherForecastPendingRequest *requests = weather_forecast_pending_requests();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    uint32_t call_id = requests[i].call_id;
    if (call_id == 0) continue;
    requests[i] = WeatherForecastPendingRequest();
    ha_cancel_action_response_callback(call_id, "api disconnected");
  }
}

inline bool weather_forecast_cancel_stale_requests() {
  WeatherForecastPendingRequest *requests = weather_forecast_pending_requests();
  uint32_t now = esphome::millis();
  for (int i = 0; i < WEATHER_FORECAST_PENDING_MAX; i++) {
    uint32_t call_id = requests[i].call_id;
    if (call_id == 0) continue;
    if (now - requests[i].started_ms < WEATHER_FORECAST_REQUEST_TIMEOUT_MS) continue;
    std::string entity_id = requests[i].entity_id;
    requests[i] = WeatherForecastPendingRequest();
    ESP_LOGW("weather_forecast", "Cancelling forecast request %u for %s: timeout",
      (unsigned) call_id, entity_id.c_str());
    ha_cancel_action_response_callback(call_id, "timeout");
    return true;
  }
  return false;
}

inline void request_weather_forecast_entity(const std::string &entity_id,
                                            const std::string &day) {
  if (!weather_forecast_entity_id_safe(entity_id) ||
      !ha_api_state_connected() ||
      !weather_forecast_actions_ready()) {
    apply_weather_forecast_unavailable_for_entity(entity_id);
    return;
  }
#ifdef ESP_PLATFORM
  size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  if (internal_free < HA_ACTION_INTERNAL_FREE_MIN_BYTES ||
      internal_largest < HA_ACTION_INTERNAL_LARGEST_MIN_BYTES) {
    ESP_LOGW("weather_forecast",
             "Deferring forecast request for %s: internal heap free=%u largest=%u",
             entity_id.c_str(), (unsigned) internal_free, (unsigned) internal_largest);
    weather_forecast_schedule_retry(entity_id, day, "low internal heap");
    return;
  }
#endif

  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_weather_forecast_call_id();
  if (!ha_action_begin(req, "weather.get_forecasts", false, 2, call_id)) {
    apply_weather_forecast_unavailable_for_entity(entity_id);
    weather_forecast_schedule_retry(entity_id, day, "request setup failed");
    return;
  }
  req.wants_response = true;
  std::string response_template = weather_forecast_response_template(entity_id);
  req.response_template = decltype(req.response_template)(response_template);
  ha_action_add_entity(req, entity_id);
  ha_action_add_data(req, "type", "daily");
  uint32_t generation = ha_subscription_generation();

  if (!ha_register_action_response_callback(
    req.call_id,
    [entity_id, day, call_id = req.call_id, generation](const esphome::api::ActionResponse &response) {
      weather_forecast_clear_pending(call_id);
      if (generation != ha_subscription_generation()) {
        weather_forecast_send_next_queued();
        return;
      }
      if (!response.is_success()) {
        std::string error_message = response.get_error_message();
        ESP_LOGW("weather_forecast", "Forecast request failed for %s: %s",
          entity_id.c_str(), error_message.c_str());
        if (weather_forecast_error_is_timeout(error_message)) {
          apply_weather_forecast_actions_required_for_entity(entity_id);
        } else {
          apply_weather_forecast_unavailable_for_entity(entity_id);
        }
        weather_forecast_schedule_retry(entity_id, day, error_message.c_str());
        weather_forecast_send_next_queued();
        return;
      }
      auto json = response.get_json();
      const char *payload = json["response"].as<const char *>();
      if (payload == nullptr) {
        ESP_LOGW("weather_forecast", "Forecast response for %s did not include a rendered payload",
          entity_id.c_str());
        apply_weather_forecast_unavailable_for_entity(entity_id);
        weather_forecast_schedule_retry(entity_id, day, "empty response");
        weather_forecast_send_next_queued();
        return;
      }
      WeatherForecastPayload forecast;
      bool valid = parse_weather_forecast_payload(payload, forecast);
      if (!valid) {
        ESP_LOGW("weather_forecast", "No usable forecast temperatures for %s: %s",
          entity_id.c_str(), payload);
        weather_forecast_schedule_retry(entity_id, day, "no usable forecast temperatures");
      }
      apply_weather_forecast_to_entity(entity_id, "today", forecast.today_valid,
        forecast.today_high, forecast.today_low, forecast.unit);
      apply_weather_forecast_to_entity(entity_id, "tomorrow", forecast.tomorrow_valid,
        forecast.tomorrow_high, forecast.tomorrow_low, forecast.unit);
      bool three_day_valid = false;
      float three_day_high = WEATHER_FORECAST_TEMP_MISSING;
      float three_day_low = WEATHER_FORECAST_TEMP_MISSING;
      weather_forecast_extend_range(forecast.today_valid, forecast.today_high,
        forecast.today_low, three_day_valid, three_day_high, three_day_low);
      weather_forecast_extend_range(forecast.tomorrow_valid, forecast.tomorrow_high,
        forecast.tomorrow_low, three_day_valid, three_day_high, three_day_low);
      weather_forecast_extend_range(forecast.three_day_valid, forecast.three_day_high,
        forecast.three_day_low, three_day_valid, three_day_high, three_day_low);
      apply_weather_forecast_to_entity(entity_id, "3day", three_day_valid,
        three_day_high, three_day_low, forecast.unit);
      weather_forecast_send_next_queued();
    })) {
    apply_weather_forecast_unavailable_for_entity(entity_id);
    weather_forecast_schedule_retry(entity_id, day, "callback setup failed");
    return;
  }
  if (!weather_forecast_track_pending(req.call_id, entity_id, day)) {
    ha_cancel_action_response_callback(req.call_id, "too many pending forecasts");
    apply_weather_forecast_unavailable_for_entity(entity_id);
    return;
  }
  ESP_LOGI("weather_forecast", "Requesting daily forecast for %s", entity_id.c_str());
  if (!ha_action_send(req)) {
    weather_forecast_clear_pending(req.call_id);
    ha_cancel_action_response_callback(req.call_id, "send failed");
    apply_weather_forecast_unavailable_for_entity(entity_id);
    weather_forecast_schedule_retry(entity_id, day, "send failed");
    weather_forecast_send_next_queued();
  }
}

inline void weather_forecast_send_next_queued() {
  if (!weather_forecast_actions_ready() || weather_forecast_any_pending()) return;
  weather_forecast_enqueue_due_retries();
  std::string entity_id;
  std::string day;
  if (!weather_forecast_dequeue(entity_id, day)) return;
  request_weather_forecast_entity(entity_id, day);
}

inline void refresh_weather_forecast_cards() {
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  if (count <= 0) return;
  std::vector<std::string> requested;
  requested.reserve(count);
  for (int i = 0; i < count; i++) {
    const std::string &entity_id = refs[i].entity_id;
    if (entity_id.empty()) continue;
    std::string request_key = entity_id;
    bool already_requested = false;
    for (const auto &existing : requested) {
      if (existing == request_key) {
        already_requested = true;
        break;
      }
    }
    if (already_requested) continue;
    requested.push_back(request_key);
    weather_forecast_enqueue(entity_id, "");
  }
  weather_forecast_send_next_queued();
}

#endif
