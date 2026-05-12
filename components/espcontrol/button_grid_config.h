#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_WEBSERVER
#include <esp_http_server.h>
#include "esphome/components/web_server_idf/web_server_idf.h"
#endif

constexpr uint32_t DEFAULT_SLIDER_COLOR = 0xFF8C00;
constexpr uint32_t DEFAULT_OFF_COLOR = 0x313131;
constexpr uint32_t DEFAULT_TERTIARY_COLOR = 0x212121;
constexpr int MAX_GRID_SLOTS = 25;
constexpr int MAX_SUBPAGE_ITEMS = MAX_GRID_SLOTS * MAX_GRID_SLOTS;

inline int bounded_grid_slots(int num_slots) {
  if (num_slots < 0) return 0;
  return num_slots > MAX_GRID_SLOTS ? MAX_GRID_SLOTS : num_slots;
}

// LVGL widget handles for one button slot on the main grid
struct BtnSlot {
  esphome::text::Text *config;       // persisted config string (entity;label;icon;...)
  lv_obj_t *btn;                     // button container
  lv_obj_t *icon_lbl;               // icon label (MDI glyph)
  lv_obj_t *text_lbl;               // entity name / custom label
  lv_obj_t *sensor_container;       // flex row shown when sensor overlay is active
  lv_obj_t *sensor_lbl;             // numeric sensor value
  lv_obj_t *unit_lbl;               // unit suffix (°C, %, etc.)
};

// Extract the Nth semicolon-delimited field from a config string
inline std::string cfg_field(const std::string &cfg, int idx) {
  size_t start = 0;
  for (int i = 0; i < idx; i++) {
    size_t pos = cfg.find(';', start);
    if (pos == std::string::npos) return "";
    start = pos + 1;
  }
  size_t end = cfg.find(';', start);
  return (end == std::string::npos) ? cfg.substr(start) : cfg.substr(start, end - start);
}

inline std::vector<std::string> split_config_fields(const std::string &value, char delim) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= value.length()) {
    size_t end = value.find(delim, start);
    if (end == std::string::npos) end = value.length();
    out.push_back(value.substr(start, end - start));
    start = end + 1;
  }
  return out;
}

inline int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

inline std::string decode_compact_field(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (size_t i = 0; i < value.size(); i++) {
    if (value[i] == '%' && i + 2 < value.size()) {
      int hi = hex_digit(value[i + 1]);
      int lo = hex_digit(value[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    out.push_back(value[i]);
  }
  return out;
}

// Structured view of a button config string: entity;label;icon;icon_on;sensor;unit;type;precision;options
struct ParsedCfg {
  std::string entity;      // 0  HA entity_id, internal relay key, or timezone option
  std::string label;       // 1  display name (blank = use HA friendly_name)
  std::string icon;        // 2  icon name for off/default state
  std::string icon_on;     // 3  icon name for on state (blank = no swap)
  std::string sensor;      // 4  sensor entity, cover mode, or action name for Action cards
  std::string unit;        // 5  unit suffix for sensor display
  std::string type;        // 6  button type: "" (toggle), action, sensor, calendar, timezone, weather_forecast, slider, light_brightness, cover, garage, lock, media, climate, push, internal, subpage
  std::string precision;   // 7  decimal places for sensors; "text" = text sensor mode
  std::string options;     // 8  comma-delimited card options
};

inline bool card_large_numbers_supported(const ParsedCfg &p) {
  return (p.type == "sensor" && p.precision != "text") ||
    (p.type == "weather" && (p.precision == "today" || p.precision == "tomorrow")) ||
    p.type == "calendar" ||
    p.type == "timezone";
}

inline bool brightness_slider_type(const std::string &type) {
  return type == "slider" || type == "light_brightness";
}

inline ParsedCfg normalize_parsed_cfg(ParsedCfg p) {
  // Slider cards used to store "h" here for horizontal layout. Sliders are
  // now always vertical, so treat any saved slider sensor value as legacy.
  if (brightness_slider_type(p.type) && !p.sensor.empty()) p.sensor.clear();
  if (p.type == "weather_forecast") {
    p.type = "weather";
    p.precision = "tomorrow";
    if (p.label == "Weather") p.label.clear();
  }
  if (p.type == "media") {
    if (p.sensor == "controls") {
      if (p.icon.empty() || p.icon == "Speaker") p.icon = "Auto";
      p.sensor = "play_pause";
    } else if (p.sensor.empty()) {
      p.sensor = "play_pause";
    } else if (p.sensor != "play_pause" && p.sensor != "previous" &&
               p.sensor != "next" && p.sensor != "volume" &&
               p.sensor != "position" && p.sensor != "now_playing") {
      p.sensor = "play_pause";
    }
    if (p.sensor == "previous" && p.label == "Skip Previous") p.label = "Previous";
    if (p.sensor == "next" && p.label == "Skip Next") p.label = "Next";
    if (p.sensor == "volume") {
      if (p.label.empty() || p.label == "Media") p.label = "Volume";
      p.icon = "Auto";
    }
    if (p.sensor == "position" && (p.label.empty() || p.label == "Track")) p.label = "Position";
    if (p.sensor == "now_playing") {
      p.precision = (p.precision == "progress" || p.precision == "play_pause") ? p.precision : "";
    } else if ((p.sensor == "play_pause" || p.sensor == "position") && p.precision == "state") {
      p.precision = "state";
    } else {
      p.precision.clear();
    }
  }
  if (p.type == "climate") {
    p.sensor.clear();
    p.unit.clear();
    p.icon_on.clear();
  }
  if (p.type == "garage") {
    if (p.sensor != "open" && p.sensor != "close") p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    if (!p.sensor.empty()) p.icon_on.clear();
  }
  if (!p.type.empty() && !card_large_numbers_supported(p)) {
    p.options.clear();
  }
  return p;
}

inline ParsedCfg parse_cfg(const std::string &cfg) {
  ParsedCfg p;
  if (!cfg.empty() && cfg[0] == '~') {
    std::vector<std::string> f = split_config_fields(cfg.substr(1), ',');
    p.entity    = f.size() > 0 ? decode_compact_field(f[0]) : "";
    p.label     = f.size() > 1 ? decode_compact_field(f[1]) : "";
    p.icon      = f.size() > 2 ? decode_compact_field(f[2]) : "";
    p.icon_on   = f.size() > 3 ? decode_compact_field(f[3]) : "";
    p.sensor    = f.size() > 4 ? decode_compact_field(f[4]) : "";
    p.unit      = f.size() > 5 ? decode_compact_field(f[5]) : "";
    p.type      = f.size() > 6 ? decode_compact_field(f[6]) : "";
    p.precision = f.size() > 7 ? decode_compact_field(f[7]) : "";
    p.options   = f.size() > 8 ? decode_compact_field(f[8]) : "";
    return normalize_parsed_cfg(p);
  }
  p.entity    = cfg_field(cfg, 0);
  p.label     = cfg_field(cfg, 1);
  p.icon      = cfg_field(cfg, 2);
  p.icon_on   = cfg_field(cfg, 3);
  p.sensor    = cfg_field(cfg, 4);
  p.unit      = cfg_field(cfg, 5);
  p.type      = cfg_field(cfg, 6);
  p.precision = cfg_field(cfg, 7);
  p.options   = cfg_field(cfg, 8);
  return normalize_parsed_cfg(p);
}

inline bool cfg_option_enabled(const std::string &options, const char *name) {
  if (!name || !*name || options.empty()) return false;
  size_t start = 0;
  while (start <= options.length()) {
    size_t end = options.find(',', start);
    if (end == std::string::npos) end = options.length();
    if (options.compare(start, end - start, name) == 0) return true;
    start = end + 1;
  }
  return false;
}

inline std::string cfg_option_value(const std::string &options, const char *name) {
  if (!name || !*name || options.empty()) return "";
  std::string prefix = std::string(name) + "=";
  size_t start = 0;
  while (start <= options.length()) {
    size_t end = options.find(',', start);
    if (end == std::string::npos) end = options.length();
    if (options.compare(start, prefix.length(), prefix) == 0) {
      return decode_compact_field(options.substr(start + prefix.length(), end - start - prefix.length()));
    }
    start = end + 1;
  }
  return "";
}

inline bool card_large_numbers_enabled(const ParsedCfg &p) {
  return card_large_numbers_supported(p) && cfg_option_enabled(p.options, "large_numbers");
}

inline bool sensor_large_numbers_enabled(const ParsedCfg &p) {
  return card_large_numbers_enabled(p);
}

inline bool switch_confirmation_enabled(const ParsedCfg &p) {
  return p.type.empty() && cfg_option_enabled(p.options, "confirm_off");
}

inline std::string switch_confirmation_message(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_message");
  return value.empty() ? std::string("Turn off this device?") : value;
}

inline std::string switch_confirmation_yes_text(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_yes");
  return value.empty() ? std::string("Turn Off") : value;
}

inline std::string switch_confirmation_no_text(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_no");
  return value.empty() ? std::string("Cancel") : value;
}

inline int parse_precision(const std::string &s) {
  if (s.empty()) return 0;
  int v = atoi(s.c_str());
  return (v < 0) ? 0 : (v > 3) ? 3 : v;
}

inline std::string trim_display_unit(const std::string &unit) {
  size_t start = 0;
  while (start < unit.size() &&
         std::isspace(static_cast<unsigned char>(unit[start]))) {
    start++;
  }
  size_t end = unit.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(unit[end - 1]))) {
    end--;
  }
  return unit.substr(start, end - start);
}

inline bool is_text_sensor_card(const std::string &type, const std::string &precision) {
  return (type == "sensor" && precision == "text") || type == "text_sensor";
}

inline bool is_text_sensor_card(const ParsedCfg &p) {
  return is_text_sensor_card(p.type, p.precision);
}

constexpr size_t HA_STATE_TEXT_MAX_LEN = 96;
constexpr size_t HA_TEXT_SENSOR_STATE_MAX_LEN = 256;
constexpr size_t HA_SHORT_STATE_MAX_LEN = 32;
constexpr size_t HA_FRIENDLY_NAME_MAX_LEN = 64;

inline std::string string_ref_limited(esphome::StringRef value, size_t max_len) {
  size_t len = value.size();
  if (len > max_len) len = max_len;
  return std::string(value.c_str(), len);
}

inline std::string text_sensor_display_text(esphome::StringRef value,
                                            size_t max_len = HA_TEXT_SENSOR_STATE_MAX_LEN) {
  std::string raw = string_ref_limited(value, max_len);
  std::string out;
  out.reserve(raw.size());
  bool cap_next = true;
  bool last_space = false;
  for (size_t i = 0; i < raw.size(); i++) {
    char ch = raw[i];
    unsigned char c = static_cast<unsigned char>(ch);
    if (ch == '\r' || ch == '\n') {
      if (ch == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n') continue;
      if (!out.empty() && out.back() == ' ') out.pop_back();
      if (!out.empty() && out.back() != '\n') out.push_back('\n');
      cap_next = true;
      last_space = false;
      continue;
    }
    if (ch == '_' || ch == '-' || std::isspace(c)) {
      if (!out.empty() && !last_space && out.back() != '\n') {
        out.push_back(' ');
        last_space = true;
      }
      cap_next = true;
      continue;
    }
    if (std::isalpha(c)) {
      out.push_back(static_cast<char>(cap_next ? std::toupper(c) : std::tolower(c)));
      cap_next = false;
    } else {
      out.push_back(ch);
    }
    last_space = false;
  }
  while (!out.empty() && (out.back() == ' ' || out.back() == '\n')) out.pop_back();
  return out;
}

inline void lv_label_set_text_limited(lv_obj_t *label, esphome::StringRef value, size_t max_len) {
  std::string text = string_ref_limited(value, max_len);
  lv_label_set_text(label, text.c_str());
}

inline bool parse_float_ref(esphome::StringRef value, float &out) {
  char *end;
  out = strtof(value.c_str(), &end);
  return end != value.c_str();
}

inline bool is_entity_on_ref(esphome::StringRef state) {
  return state == "on" || state == "home" || state == "playing" ||
         state == "open" || state == "opening" || state == "closing" ||
         state == "unlocked" || state == "unlocking" || state == "jammed";
}

inline bool ha_state_unavailable_ref(esphome::StringRef state) {
  return state.size() == 0 || state == "unavailable" || state == "unknown";
}

inline void apply_control_availability(lv_obj_t *visual_obj, lv_obj_t *input_obj,
                                       bool available, bool disable_interaction = true) {
  if (visual_obj) {
    lv_obj_set_style_opa(visual_obj, available ? LV_OPA_COVER : LV_OPA_50, LV_PART_MAIN);
    if (disable_interaction) {
      if (available) lv_obj_clear_state(visual_obj, LV_STATE_DISABLED);
      else lv_obj_add_state(visual_obj, LV_STATE_DISABLED);
    }
  }
  if (!disable_interaction || !input_obj) return;
  if (input_obj != visual_obj) {
    if (available) lv_obj_clear_state(input_obj, LV_STATE_DISABLED);
    else lv_obj_add_state(input_obj, LV_STATE_DISABLED);
  }
  if (available) lv_obj_add_flag(input_obj, LV_OBJ_FLAG_CLICKABLE);
  else lv_obj_clear_flag(input_obj, LV_OBJ_FLAG_CLICKABLE);
}

inline std::string sentence_cap_text(const std::string &state) {
  std::string out;
  out.reserve(state.size());
  bool cap_next = true;
  bool last_space = false;
  for (char ch : state) {
    unsigned char c = static_cast<unsigned char>(ch);
    if (ch == '_' || ch == '-' || std::isspace(c)) {
      if (!out.empty() && !last_space) {
        out.push_back(' ');
        last_space = true;
      }
      cap_next = true;
      continue;
    }
    if (std::isalpha(c)) {
      out.push_back(static_cast<char>(cap_next ? std::toupper(c) : std::tolower(c)));
      cap_next = false;
    } else {
      out.push_back(ch);
    }
    last_space = false;
  }
  if (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

inline const char* weather_icon_for_state(const std::string &state) {
  if (state == "sunny") return find_icon("Weather Sunny");
  if (state == "clear-night") return find_icon("Weather Night");
  if (state == "partlycloudy") return find_icon("Weather Partly Cloudy");
  if (state == "cloudy") return find_icon("Weather Cloudy");
  if (state == "fog") return find_icon("Weather Fog");
  if (state == "hail") return find_icon("Weather Hail");
  if (state == "lightning") return find_icon("Weather Lightning");
  if (state == "lightning-rainy") return find_icon("Weather Lightning Rainy");
  if (state == "pouring") return find_icon("Weather Pouring");
  if (state == "rainy") return find_icon("Weather Rainy");
  if (state == "snowy") return find_icon("Weather Snowy");
  if (state == "snowy-rainy") return find_icon("Weather Snowy Rainy");
  if (state == "windy") return find_icon("Weather Windy");
  if (state == "windy-variant") return find_icon("Weather Windy Variant");
  if (state == "unavailable" || state.empty()) return find_icon("Weather Sunny Off");
  return find_icon("Weather Cloudy Alert");
}

inline std::string weather_label_for_state(const std::string &state) {
  if (state == "sunny") return "Sunny";
  if (state == "clear-night") return "Clear Night";
  if (state == "partlycloudy") return "Partly Cloudy";
  if (state == "cloudy") return "Cloudy";
  if (state == "fog") return "Fog";
  if (state == "hail") return "Hail";
  if (state == "lightning") return "Lightning";
  if (state == "lightning-rainy") return "Lightning And Rain";
  if (state == "pouring") return "Pouring";
  if (state == "rainy") return "Rainy";
  if (state == "snowy") return "Snowy";
  if (state == "snowy-rainy") return "Snowy And Rain";
  if (state == "windy") return "Windy";
  if (state == "windy-variant") return "Windy And Cloudy";
  if (state == "exceptional") return "Exceptional";
  if (state == "unknown") return "Unknown";
  if (state == "unavailable" || state.empty()) return "Unavailable";

  return sentence_cap_text(state);
}

struct WeatherForecastCardRef {
  lv_obj_t *value_lbl;
  lv_obj_t *unit_lbl;
  lv_obj_t *label_lbl;
  std::string entity_id;
  std::string day;
  std::string label;
  bool valid = false;
  int high = 0;
  int low = 0;
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
  weather_forecast_card_count() = 0;
}

constexpr int WEATHER_FORECAST_TEMP_MISSING = 32767;

inline std::string weather_forecast_unit_symbol(const std::string &unit) {
  (void)unit;
  return display_temperature_unit_symbol();
}

inline void apply_weather_forecast_card_text(const WeatherForecastCardRef &ref,
                                             bool valid, int high, int low,
                                             const std::string &unit) {
  if (ref.label_lbl) {
    std::string label = ref.label.empty()
      ? (ref.day == "today" ? "Today" : "Tomorrow")
      : ref.label;
    lv_label_set_text(ref.label_lbl, label.c_str());
  }
  if (!ref.value_lbl || !ref.unit_lbl) return;
  if (!valid) {
    lv_label_set_text(ref.value_lbl, "--/--");
    lv_label_set_text(ref.unit_lbl, "");
    return;
  }
  char buf[24];
  char high_buf[12];
  char low_buf[12];
  if (high == WEATHER_FORECAST_TEMP_MISSING) snprintf(high_buf, sizeof(high_buf), "--");
  else snprintf(high_buf, sizeof(high_buf), "%d", high);
  if (low == WEATHER_FORECAST_TEMP_MISSING) snprintf(low_buf, sizeof(low_buf), "--");
  else snprintf(low_buf, sizeof(low_buf), "%d", low);
  snprintf(buf, sizeof(buf), "%s/%s", high_buf, low_buf);
  lv_label_set_text(ref.value_lbl, buf);
  std::string normalized_unit = weather_forecast_unit_symbol(unit);
  lv_label_set_text(ref.unit_lbl, normalized_unit.c_str());
}

inline void apply_weather_forecast_to_entity(const std::string &entity_id,
                                             const std::string &day,
                                             bool valid, int high, int low,
                                             const std::string &unit) {
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  for (int i = 0; i < count; i++) {
    if (refs[i].entity_id == entity_id && refs[i].day == day) {
      refs[i].valid = valid;
      refs[i].high = high;
      refs[i].low = low;
      refs[i].source_unit = unit;
      apply_weather_forecast_card_text(refs[i], valid, high, low, unit);
    }
  }
}

inline void register_weather_forecast_card(lv_obj_t *value_lbl, lv_obj_t *unit_lbl,
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
    value_lbl, unit_lbl, label_lbl, entity_id, day, label, false, 0, 0, ""
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

inline bool parse_weather_forecast_temp(const std::string &value, int &out) {
  if (value.empty()) return false;
  char *end = nullptr;
  float parsed = strtof(value.c_str(), &end);
  if (end == value.c_str()) return false;
  out = static_cast<int>(parsed >= 0 ? parsed + 0.5f : parsed - 0.5f);
  return true;
}

inline bool parse_weather_forecast_payload(const std::string &payload,
                                           int &high, int &low,
                                           std::string &unit) {
  size_t p1 = payload.find('|');
  if (p1 == std::string::npos) return false;
  size_t p2 = payload.find('|', p1 + 1);
  if (p2 == std::string::npos) return false;
  std::string high_text = payload.substr(0, p1);
  std::string low_text = payload.substr(p1 + 1, p2 - p1 - 1);
  unit = payload.substr(p2 + 1);
  high = WEATHER_FORECAST_TEMP_MISSING;
  low = WEATHER_FORECAST_TEMP_MISSING;
  bool has_high = parse_weather_forecast_temp(high_text, high);
  bool has_low = parse_weather_forecast_temp(low_text, low);
  return has_high || has_low;
}

inline std::string weather_forecast_response_template(const std::string &entity_id,
                                                      const std::string &day) {
  const char *target_date_template = day == "today"
    ? "now().date().isoformat()"
    : "(now().date() + timedelta(days=1)).isoformat()";
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set forecasts = response.get(entity, {}).get('forecast', []) %}"
    "{% set target_date = " + target_date_template + " %}"
    "{% set ns = namespace(forecast=none) %}"
    "{% for item in forecasts %}"
    "{% if ns.forecast is none and item.datetime is defined and item.datetime[:10] == target_date %}"
    "{% set ns.forecast = item %}"
    "{% endif %}"
    "{% endfor %}"
    "{% set fallback_index = 0 if target_date == now().date().isoformat() else 1 %}"
    "{% set f = ns.forecast if ns.forecast is not none else (forecasts[fallback_index] if forecasts|length > fallback_index else (forecasts[0] if forecasts|length > 0 else none)) %}"
    "{% set high = f.temperature if f is not none and f.temperature is defined else (f.temperature_high if f is not none and f.temperature_high is defined else (f.high_temperature if f is not none and f.high_temperature is defined else (f.high if f is not none and f.high is defined else ''))) %}"
    "{% set low = f.templow if f is not none and f.templow is defined else (f.temperature_low if f is not none and f.temperature_low is defined else (f.low_temperature if f is not none and f.low_temperature is defined else (f.low if f is not none and f.low is defined else ''))) %}"
    "{{ high }}|{{ low }}|"
    "{{ state_attr(entity, 'temperature_unit') or '' }}";
}

inline uint32_t next_weather_forecast_call_id() {
  static uint32_t call_id = 100000;
  return call_id++;
}

inline void request_weather_forecast_entity(const std::string &entity_id,
                                            const std::string &day) {
  if (!weather_forecast_entity_id_safe(entity_id) || esphome::api::global_api_server == nullptr) {
    apply_weather_forecast_to_entity(entity_id, day, false, 0, 0, "");
    return;
  }

  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)("weather.get_forecasts");
  req.is_event = false;
  req.call_id = next_weather_forecast_call_id();
  req.wants_response = true;
  std::string response_template = weather_forecast_response_template(entity_id, day);
  req.response_template = decltype(req.response_template)(response_template);
  req.data.init(2);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(entity_id.c_str());
  auto &type_kv = req.data.emplace_back();
  type_kv.key = decltype(type_kv.key)("type");
  type_kv.value = decltype(type_kv.value)("daily");

  esphome::api::global_api_server->register_action_response_callback(
    req.call_id,
    [entity_id, day](const esphome::api::ActionResponse &response) {
      if (!response.is_success()) {
        ESP_LOGW("weather_forecast", "Forecast request failed for %s: %s",
          entity_id.c_str(), response.get_error_message().c_str());
        apply_weather_forecast_to_entity(entity_id, day, false, 0, 0, "");
        return;
      }
      auto json = response.get_json();
      const char *payload = json["response"].as<const char *>();
      if (payload == nullptr) {
        apply_weather_forecast_to_entity(entity_id, day, false, 0, 0, "");
        return;
      }
      int high = 0;
      int low = 0;
      std::string unit;
      bool valid = parse_weather_forecast_payload(payload, high, low, unit);
      if (!valid) {
        ESP_LOGW("weather_forecast", "No usable forecast temperatures for %s", entity_id.c_str());
      }
      apply_weather_forecast_to_entity(entity_id, day, valid, high, low, unit);
    });
  esphome::api::global_api_server->send_homeassistant_action(req);
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
    const std::string &day = refs[i].day;
    std::string request_key = entity_id + "|" + day;
    bool already_requested = false;
    for (const auto &existing : requested) {
      if (existing == request_key) {
        already_requested = true;
        break;
      }
    }
    if (already_requested) continue;
    requested.push_back(request_key);
    request_weather_forecast_entity(entity_id, day);
  }
}

struct ClimateControlCtx;
inline ClimateControlCtx **climate_control_refs();
inline int &climate_control_ref_count();
inline void climate_update_card(ClimateControlCtx *ctx);
inline void climate_control_set_modal_value(ClimateControlCtx *ctx);

inline void refresh_temperature_unit_labels() {
  WeatherForecastCardRef *weather_refs = weather_forecast_card_refs();
  int weather_count = weather_forecast_card_count();
  for (int i = 0; i < weather_count; i++) {
    apply_weather_forecast_card_text(weather_refs[i], weather_refs[i].valid,
                                     weather_refs[i].high, weather_refs[i].low,
                                     weather_refs[i].source_unit);
  }
  ClimateControlCtx **climate_refs = climate_control_refs();
  int climate_count = climate_control_ref_count();
  for (int i = 0; i < climate_count; i++) {
    climate_update_card(climate_refs[i]);
    climate_control_set_modal_value(climate_refs[i]);
  }
}

inline const char* garage_closed_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Garage") : find_icon(icon.c_str());
}

inline const char* garage_open_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Garage Open") : find_icon(icon_on.c_str());
}

inline bool garage_command_mode(const std::string &sensor) {
  return sensor == "open" || sensor == "close";
}

inline const char *garage_command_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(p.sensor == "open" ? "Garage Open" : "Garage");
}

inline const char *garage_card_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label.c_str();
  if (p.sensor == "open") return "Open";
  if (p.sensor == "close") return "Close";
  return "Garage Door";
}

inline const char* lock_locked_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Lock") : find_icon(icon.c_str());
}

inline const char* lock_unlocked_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Lock Open") : find_icon(icon_on.c_str());
}

inline bool lock_command_mode(const std::string &sensor) {
  return sensor == "lock" || sensor == "unlock";
}

inline const char *lock_command_icon(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(p.sensor == "unlock" ? "Lock Open" : "Lock");
}

inline const char *lock_card_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label.c_str();
  if (p.sensor == "lock") return "Lock";
  if (p.sensor == "unlock") return "Unlock";
  return "Lock";
}

// ── Internal relay controls ───────────────────────────────────────────
//
// Only devices that actually have relays register entries here. The shared
// grid code can then control those relays locally without referencing device-
// specific ids, so non-relay devices still compile and simply have no relays.

struct InternalRelayControl {
  std::string key;
  std::string label;
  std::function<void(bool)> set_state;
  std::function<void()> pulse;
  std::function<bool()> is_on;
};

struct InternalRelayWatcher {
  std::string key;
  lv_obj_t *btn;
  lv_obj_t *icon_lbl;
  bool has_icon_on;
  const char *icon_off;
  const char *icon_on;
  bool *child_was_on;
  lv_obj_t *parent_btn;
  lv_obj_t *parent_icon;
  int parent_idx;
  bool parent_has_alt_icon;
  const char *parent_off_glyph;
  const char *parent_on_glyph;
  int *sp_on_count;
};

struct InternalRelayClickCtx {
  std::string key;
  bool push_mode;
};

// ── Local sensor controls ─────────────────────────────────────────────
//
// Displays a live value from any ESPHome sensor/text_sensor on the device.
// The device auto-subscribes to sensor callbacks; send_local_sensor_update()
// is available as a fallback for computed/non-entity values.

struct LocalSensorControl {
  std::string key;
  bool is_text;
  int precision;
  lv_obj_t *sensor_lbl;
  lv_obj_t *text_lbl;
};

inline std::vector<LocalSensorControl> &local_sensor_registry() {
  static std::vector<LocalSensorControl> sensors;
  return sensors;
}

#ifdef USE_WEBSERVER
class LocalSensorHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  static std::string json_escape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
      if (c == '"') out += "\\\"";
      else if (c == '\\') out += "\\\\";
      else out += c;
    }
    return out;
  }

  static std::string build_json() {
    std::string json;
    json.reserve(512);
    json = "[";
    bool first = true;
    auto append = [&](const std::string &key, const std::string &name,
                      const std::string &unit, const char *type, bool internal) {
      if (!first) json += ",";
      first = false;
      json += "{\"key\":\"" + json_escape(key) + "\",\"name\":\"" + json_escape(name) +
              "\",\"unit\":\"" + json_escape(unit) + "\",\"type\":\"" + type + "\"";
      if (internal) json += ",\"internal\":true";
      json += "}";
    };
    char oid_buf[128];
#ifdef USE_SENSOR
    for (auto *s : esphome::App.get_sensors()) {
      bool internal = (int) s->get_entity_category() != 0;
      append(std::string(s->get_object_id_to(oid_buf).c_str()), std::string(s->get_name()),
             std::string(s->get_unit_of_measurement_ref()), "numeric", internal);
    }
#endif
#ifdef USE_TEXT_SENSOR
    for (auto *ts : esphome::App.get_text_sensors()) {
      bool internal = (int) ts->get_entity_category() != 0;
      append(std::string(ts->get_object_id_to(oid_buf).c_str()), std::string(ts->get_name()),
             "", "text", internal);
    }
#endif
    json += "]";
    return json;
  }

  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    esphome::StringRef url = request->url_to(url_buf);
    return strncmp(url.c_str(), "/local_sensors", 14) == 0;
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    std::string json = build_json();
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK) ESP_LOGE("sensors", "httpd_resp_send failed: %d", err);
  }
};

inline void register_local_sensor_endpoint() {
  static bool registered = false;
  if (registered) return;
  auto *server = esphome::web_server_idf::global_async_web_server();
  if (!server) {
    ESP_LOGW("sensors", "register_local_sensor_endpoint: server not ready");
    return;
  }
  server->addHandler(new LocalSensorHandler());
  registered = true;
  ESP_LOGI("sensors", "Local sensor endpoint registered");
}
#endif  // USE_WEBSERVER

inline std::vector<InternalRelayControl> &internal_relay_registry() {
  static std::vector<InternalRelayControl> relays;
  return relays;
}

inline std::vector<InternalRelayWatcher> &internal_relay_watchers() {
  static std::vector<InternalRelayWatcher> watchers;
  return watchers;
}

inline void clear_internal_relay_watchers() {
  internal_relay_watchers().clear();
}

inline void register_internal_relay(
    const std::string &key, const std::string &label,
    std::function<void(bool)> set_state,
    std::function<void()> pulse,
    std::function<bool()> is_on) {
  if (key.empty()) return;
  InternalRelayControl r;
  r.key = key;
  r.label = label;
  r.set_state = set_state;
  r.pulse = pulse;
  r.is_on = is_on;

  auto &relays = internal_relay_registry();
  for (auto &existing : relays) {
    if (existing.key == key) {
      existing = r;
      return;
    }
  }
  relays.push_back(r);
}

inline InternalRelayControl *find_internal_relay(const std::string &key) {
  auto &relays = internal_relay_registry();
  for (auto &relay : relays) {
    if (relay.key == key) return &relay;
  }
  return nullptr;
}

inline bool internal_relay_push_mode(const ParsedCfg &p) {
  return p.sensor == "push";
}

inline bool internal_relay_state(const std::string &key) {
  InternalRelayControl *relay = find_internal_relay(key);
  return relay && relay->is_on ? relay->is_on() : false;
}

inline std::string internal_relay_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label;
  InternalRelayControl *relay = find_internal_relay(p.entity);
  if (relay && !relay->label.empty()) return relay->label;
  return p.entity.empty() ? std::string("Relay") : sentence_cap_text(p.entity);
}

inline const char *internal_relay_icon(const ParsedCfg &p, bool push_mode) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(push_mode ? "Gesture Tap" : "Power Plug");
}

inline void apply_internal_relay_state(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                       bool on, bool has_icon_on,
                                       const char *icon_off, const char *icon_on) {
  if (btn) {
    if (on) lv_obj_add_state(btn, LV_STATE_CHECKED);
    else lv_obj_clear_state(btn, LV_STATE_CHECKED);
  }
  if (icon_lbl && has_icon_on)
    lv_label_set_text(icon_lbl, on ? icon_on : icon_off);
}

inline void apply_internal_relay_parent_indicator(InternalRelayWatcher &w, bool on) {
  if (!w.child_was_on || !w.parent_btn || !w.sp_on_count) return;
  if (on && !*w.child_was_on) {
    w.sp_on_count[w.parent_idx]++;
    *w.child_was_on = true;
  } else if (!on && *w.child_was_on) {
    w.sp_on_count[w.parent_idx]--;
    *w.child_was_on = false;
  }
  if (w.sp_on_count[w.parent_idx] > 0) {
    lv_obj_add_state(w.parent_btn, LV_STATE_CHECKED);
    if (w.parent_has_alt_icon && w.parent_icon)
      lv_label_set_text(w.parent_icon, w.parent_on_glyph);
  } else {
    lv_obj_clear_state(w.parent_btn, LV_STATE_CHECKED);
    if (w.parent_has_alt_icon && w.parent_icon)
      lv_label_set_text(w.parent_icon, w.parent_off_glyph);
  }
}

inline void notify_internal_relay_changed(const std::string &key, bool on) {
  auto &watchers = internal_relay_watchers();
  for (auto &w : watchers) {
    if (w.key != key) continue;
    apply_internal_relay_state(w.btn, w.icon_lbl, on, w.has_icon_on, w.icon_off, w.icon_on);
    apply_internal_relay_parent_indicator(w, on);
  }
}

inline void watch_internal_relay_state(
    const std::string &key, lv_obj_t *btn, lv_obj_t *icon_lbl,
    bool has_icon_on, const char *icon_off, const char *icon_on,
    bool *child_was_on = nullptr, lv_obj_t *parent_btn = nullptr,
    lv_obj_t *parent_icon = nullptr, int parent_idx = 0,
    bool parent_has_alt_icon = false, const char *parent_off_glyph = nullptr,
    const char *parent_on_glyph = nullptr, int *sp_on_count = nullptr) {
  if (key.empty()) return;
  InternalRelayWatcher w;
  w.key = key;
  w.btn = btn;
  w.icon_lbl = icon_lbl;
  w.has_icon_on = has_icon_on;
  w.icon_off = icon_off;
  w.icon_on = icon_on;
  w.child_was_on = child_was_on;
  w.parent_btn = parent_btn;
  w.parent_icon = parent_icon;
  w.parent_idx = parent_idx;
  w.parent_has_alt_icon = parent_has_alt_icon;
  w.parent_off_glyph = parent_off_glyph;
  w.parent_on_glyph = parent_on_glyph;
  w.sp_on_count = sp_on_count;
  internal_relay_watchers().push_back(w);

  bool on = internal_relay_state(key);
  apply_internal_relay_state(btn, icon_lbl, on, has_icon_on, icon_off, icon_on);
  InternalRelayWatcher &stored = internal_relay_watchers().back();
  apply_internal_relay_parent_indicator(stored, on);
}

inline void send_internal_relay_action(const std::string &key, bool push_mode) {
  InternalRelayControl *relay = find_internal_relay(key);
  if (!relay) return;
  if (push_mode) {
    if (relay->pulse) relay->pulse();
    return;
  }
  bool next = !internal_relay_state(key);
  if (relay->set_state) relay->set_state(next);
  notify_internal_relay_changed(key, next);
}

inline void send_internal_relay_action(const ParsedCfg &p) {
  send_internal_relay_action(p.entity, internal_relay_push_mode(p));
}

inline std::string garage_state_label(const std::string &state) {
  if (state.empty()) return "--";
  return sentence_cap_text(state);
}

inline bool garage_state_is_active(const std::string &state) {
  return state == "open" || state == "opening" || state == "closing";
}

inline bool cover_toggle_state_is_active(const std::string &state) {
  return state == "closed" || state == "closing";
}

inline bool garage_state_uses_open_icon(const std::string &state) {
  return state == "open" || state == "opening";
}

inline bool garage_state_releases_label(const std::string &state) {
  return state == "open" || state == "closed";
}

struct LockCardCtx {
  std::string entity_id;
  std::string state;
};

inline std::string lock_state_label(const std::string &state) {
  if (state.empty()) return "--";
  return sentence_cap_text(state);
}

inline bool lock_state_is_active(const std::string &state) {
  return state == "unlocked" || state == "unlocking" ||
         state == "open" || state == "opening" ||
         state == "jammed";
}

inline bool lock_state_uses_unlocked_icon(const std::string &state) {
  return lock_state_is_active(state);
}

inline bool lock_state_releases_label(const std::string &state) {
  return state == "locked" || state == "unlocked" || state == "open";
}

// Reusable label helper: show changed status, then optionally return to steady text.
static const uint32_t STATUS_LABEL_STABLE_MS = 3000;

struct TransientStatusLabel {
  lv_obj_t *label = nullptr;
  std::string steady_text;
  std::string last_status_text;
  bool has_status = false;
  bool showing_status = false;
  lv_timer_t *revert_timer = nullptr;
};

inline void transient_status_label_revert_cb(lv_timer_t *timer) {
  TransientStatusLabel *ctx = static_cast<TransientStatusLabel *>(lv_timer_get_user_data(timer));
  if (!ctx) return;
  ctx->showing_status = false;
  if (ctx->label) lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  lv_timer_pause(timer);
}

inline TransientStatusLabel *create_transient_status_label(
    lv_obj_t *label, const std::string &steady_text,
    uint32_t stable_ms = STATUS_LABEL_STABLE_MS) {
  // Intentionally leaked -- lives for the lifetime of the display.
  TransientStatusLabel *ctx = new TransientStatusLabel();
  ctx->label = label;
  ctx->steady_text = steady_text;
  if (ctx->label) lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  ctx->revert_timer = lv_timer_create(transient_status_label_revert_cb, stable_ms, ctx);
  if (ctx->revert_timer) lv_timer_pause(ctx->revert_timer);
  return ctx;
}

inline void transient_status_label_set_steady(TransientStatusLabel *ctx,
                                              const std::string &steady_text) {
  if (!ctx) return;
  ctx->steady_text = steady_text;
  if (!ctx->showing_status && ctx->label) {
    lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  }
}

inline void transient_status_label_show_if_changed(TransientStatusLabel *ctx,
                                                   const std::string &status_text,
                                                   bool release_to_steady = true) {
  if (!ctx) return;
  if (!ctx->has_status) {
    ctx->last_status_text = status_text;
    ctx->has_status = true;
    if (!release_to_steady) {
      ctx->showing_status = true;
      if (ctx->label) lv_label_set_text(ctx->label, status_text.c_str());
      if (ctx->revert_timer) lv_timer_pause(ctx->revert_timer);
    }
    return;
  }
  if (ctx->last_status_text == status_text) return;
  ctx->last_status_text = status_text;
  ctx->showing_status = true;
  if (ctx->label) lv_label_set_text(ctx->label, status_text.c_str());
  if (ctx->revert_timer) {
    if (release_to_steady) {
      lv_timer_reset(ctx->revert_timer);
      lv_timer_resume(ctx->revert_timer);
    } else {
      lv_timer_pause(ctx->revert_timer);
    }
  }
}
