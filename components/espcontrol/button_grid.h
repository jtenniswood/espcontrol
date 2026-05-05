// =============================================================================
// BUTTON GRID - LVGL button grid layout, parsing, and HA entity binding
// =============================================================================
// Shared C++ utilities included by each device's sensors.yaml lambda. Handles:
//   - Parsing semicolon-delimited button config strings into structured fields
//   - Grid layout with double-height (d), double-wide (w), and 2×2 big (b) cells
//   - LVGL visual setup for toggle buttons, sensor/time cards, and slider widgets
//   - Home Assistant state subscriptions and action dispatch
//   - Subpage creation (nested grid screens with back button)
// =============================================================================
#pragma once
#include <string>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <cmath>
#include <vector>
#include <functional>
#include "esphome/components/api/homeassistant_service.h"
#include "esphome/core/string_ref.h"
#include "icons.h"
#include "backlight.h"

constexpr uint32_t DEFAULT_SLIDER_COLOR = 0xFF8C00;
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

// Structured view of a button config string: entity;label;icon;icon_on;sensor;unit;type;precision
struct ParsedCfg {
  std::string entity;      // 0  HA entity_id, internal relay key, or timezone option
  std::string label;       // 1  display name (blank = use HA friendly_name)
  std::string icon;        // 2  icon name for off/default state
  std::string icon_on;     // 3  icon name for on state (blank = no swap)
  std::string sensor;      // 4  sensor entity, cover mode, or action name for Action cards
  std::string unit;        // 5  unit suffix for sensor display
  std::string type;        // 6  button type: "" (toggle), action, sensor, calendar, timezone, climate, weather_forecast, slider, cover, garage, lock, media, push, internal, subpage
  std::string precision;   // 7  decimal places for sensors; "text" = text sensor mode
};

inline ParsedCfg normalize_parsed_cfg(ParsedCfg p) {
  // Slider cards used to store "h" here for horizontal layout. Sliders are
  // now always vertical, so treat any saved slider sensor value as legacy.
  if (p.type == "slider" && !p.sensor.empty()) p.sensor.clear();
  if (p.type == "weather_forecast") {
    p.type = "weather";
    p.precision = "tomorrow";
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
  return normalize_parsed_cfg(p);
}

inline int parse_precision(const std::string &s) {
  if (s.empty()) return 0;
  int v = atoi(s.c_str());
  return (v < 0) ? 0 : (v > 3) ? 3 : v;
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
  if (ref.label_lbl) lv_label_set_text(ref.label_lbl, "Tomorrow");
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
                                             bool valid, int high, int low,
                                             const std::string &unit) {
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  for (int i = 0; i < count; i++) {
    if (refs[i].entity_id == entity_id) {
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
                                           const std::string &entity_id) {
  int &count = weather_forecast_card_count();
  if (count >= MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("weather_forecast", "Too many forecast cards; skipping updates");
    return;
  }
  weather_forecast_card_refs()[count++] = {value_lbl, unit_lbl, label_lbl, entity_id, false, 0, 0, ""};
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

inline std::string weather_forecast_response_template(const std::string &entity_id) {
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set forecasts = response.get(entity, {}).get('forecast', []) %}"
    "{% set tomorrow = (now().date() + timedelta(days=1)).isoformat() %}"
    "{% set ns = namespace(forecast=none) %}"
    "{% for item in forecasts %}"
    "{% if ns.forecast is none and item.datetime is defined and item.datetime[:10] == tomorrow %}"
    "{% set ns.forecast = item %}"
    "{% endif %}"
    "{% endfor %}"
    "{% set f = ns.forecast if ns.forecast is not none else (forecasts[1] if forecasts|length > 1 else (forecasts[0] if forecasts|length > 0 else none)) %}"
    "{% set high = f.temperature if f is not none and f.temperature is defined else (f.temperature_high if f is not none and f.temperature_high is defined else (f.high_temperature if f is not none and f.high_temperature is defined else (f.high if f is not none and f.high is defined else ''))) %}"
    "{% set low = f.templow if f is not none and f.templow is defined else (f.temperature_low if f is not none and f.temperature_low is defined else (f.low_temperature if f is not none and f.low_temperature is defined else (f.low if f is not none and f.low is defined else ''))) %}"
    "{{ high }}|{{ low }}|"
    "{{ state_attr(entity, 'temperature_unit') or '' }}";
}

inline uint32_t next_weather_forecast_call_id() {
  static uint32_t call_id = 100000;
  return call_id++;
}

inline void request_weather_forecast_entity(const std::string &entity_id) {
  if (!weather_forecast_entity_id_safe(entity_id) || esphome::api::global_api_server == nullptr) {
    apply_weather_forecast_to_entity(entity_id, false, 0, 0, "");
    return;
  }

  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)("weather.get_forecasts");
  req.is_event = false;
  req.call_id = next_weather_forecast_call_id();
  req.wants_response = true;
  std::string response_template = weather_forecast_response_template(entity_id);
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
    [entity_id](const esphome::api::ActionResponse &response) {
      if (!response.is_success()) {
        ESP_LOGW("weather_forecast", "Forecast request failed for %s: %s",
          entity_id.c_str(), response.get_error_message().c_str());
        apply_weather_forecast_to_entity(entity_id, false, 0, 0, "");
        return;
      }
      auto json = response.get_json();
      const char *payload = json["response"].as<const char *>();
      if (payload == nullptr) {
        apply_weather_forecast_to_entity(entity_id, false, 0, 0, "");
        return;
      }
      int high = 0;
      int low = 0;
      std::string unit;
      bool valid = parse_weather_forecast_payload(payload, high, low, unit);
      if (!valid) {
        ESP_LOGW("weather_forecast", "No usable forecast temperatures for %s", entity_id.c_str());
      }
      apply_weather_forecast_to_entity(entity_id, valid, high, low, unit);
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
    bool already_requested = false;
    for (const auto &existing : requested) {
      if (existing == entity_id) {
        already_requested = true;
        break;
      }
    }
    if (already_requested) continue;
    requested.push_back(entity_id);
    request_weather_forecast_entity(entity_id);
  }
}

inline const char* garage_closed_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Garage") : find_icon(icon.c_str());
}

inline const char* garage_open_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Garage Open") : find_icon(icon_on.c_str());
}

inline const char* lock_locked_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Lock") : find_icon(icon.c_str());
}

inline const char* lock_unlocked_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Lock Open") : find_icon(icon_on.c_str());
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

// Parse a 6-char hex color string (no # prefix) into a uint32_t RGB value
inline uint32_t parse_hex_color(const std::string &hex, bool &valid) {
  valid = hex.length() == 6;
  if (!valid) return 0;
  return strtoul(hex.c_str(), nullptr, 16);
}

// Reduce green channel to 80% to compensate for display panel color shift
inline uint32_t correct_color(uint32_t rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (uint8_t)(((rgb >> 8) & 0xFF) * 80 / 100);
  uint8_t b = rgb & 0xFF;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// ── Climate card helpers ──────────────────────────────────────────────

constexpr uint32_t CLIMATE_HEAT_COLOR = 0xA44A1C;
constexpr uint32_t CLIMATE_COOL_COLOR = 0x1565C0;
constexpr uint32_t CLIMATE_NEUTRAL_COLOR = 0x313131;
constexpr uint32_t CLIMATE_DETAIL_CARD_COLOR = 0x252525;
constexpr uint32_t CLIMATE_DETAIL_TEXT_COLOR = 0xD8D8D8;

struct ClimateCardCtx;

struct ClimateDetailUi {
  lv_obj_t *page = nullptr;
  lv_obj_t *card = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *arc = nullptr;
  lv_obj_t *state_label = nullptr;
  lv_obj_t *target_value = nullptr;
  lv_obj_t *target_unit = nullptr;
  lv_obj_t *target_hint = nullptr;
  lv_obj_t *minus_btn = nullptr;
  lv_obj_t *plus_btn = nullptr;
  lv_obj_t *low_btn = nullptr;
  lv_obj_t *high_btn = nullptr;
  lv_obj_t *preset_chip = nullptr;
  lv_obj_t *fan_chip = nullptr;
  lv_obj_t *swing_chip = nullptr;
  lv_obj_t *mode_tabs = nullptr;
  lv_obj_t *overlay = nullptr;
  lv_obj_t *popup = nullptr;
  lv_obj_t *popup_title = nullptr;
  std::string mode_tabs_key;
  ClimateCardCtx *mode_tabs_ctx = nullptr;
  bool updating_arc = false;
  ClimateCardCtx *active = nullptr;
  lv_obj_t *return_page = nullptr;
};

inline ClimateDetailUi &climate_detail_ui() {
  static ClimateDetailUi ui;
  return ui;
}

struct ClimateCardCtx {
  std::string entity_id;
  std::string label;
  std::string friendly_name;
  std::string hvac_mode = "off";
  std::string hvac_action = "idle";
  std::string fan_mode;
  std::string swing_mode;
  std::string preset_mode;
  std::vector<std::string> hvac_modes;
  std::vector<std::string> fan_modes;
  std::vector<std::string> swing_modes;
  std::vector<std::string> preset_modes;
  bool available = false;
  bool has_target = false;
  bool has_low = false;
  bool has_high = false;
  bool edit_high = false;
  float target = 20.0f;
  float low = 18.0f;
  float high = 22.0f;
  float min_temp = 5.0f;
  float max_temp = 35.0f;
  float step = 0.5f;
  lv_obj_t *card_btn = nullptr;
  lv_obj_t *sensor_container = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  const lv_font_t *value_font = nullptr;
  const lv_font_t *target_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *unit_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  const lv_font_t *climate_control_icon_font = nullptr;
  uint32_t on_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = CLIMATE_NEUTRAL_COLOR;
  int precision = 0;
  lv_timer_t *send_timer = nullptr;
};

struct ClimateOptionCtx {
  ClimateCardCtx *ctx = nullptr;
  std::string kind;
  std::string value;
};

inline ClimateCardCtx **climate_card_contexts() {
  static ClimateCardCtx *contexts[MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS];
  return contexts;
}

inline int &climate_card_context_count() {
  static int count = 0;
  return count;
}

inline void register_climate_context(ClimateCardCtx *ctx) {
  if (!ctx) return;
  int &count = climate_card_context_count();
  if (count >= MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("climate", "Too many climate cards; skipping unit refresh registration");
    return;
  }
  climate_card_contexts()[count++] = ctx;
}

inline void reset_climate_contexts() {
  climate_card_context_count() = 0;
}

struct ClimateHomeGridMetrics {
  lv_obj_t *page = nullptr;
  int cols = 3;
  int rows = 3;
};

inline ClimateHomeGridMetrics &climate_home_grid_metrics() {
  static ClimateHomeGridMetrics metrics;
  return metrics;
}

inline void set_climate_home_grid_metrics(lv_obj_t *page, int cols, int rows) {
  ClimateHomeGridMetrics &metrics = climate_home_grid_metrics();
  metrics.page = page;
  metrics.cols = cols > 0 ? cols : 3;
  metrics.rows = rows > 0 ? rows : 3;
}

inline std::string climate_mode_label(const std::string &mode) {
  if (mode == "off") return "Off";
  if (mode == "heat") return "Heat";
  if (mode == "cool") return "Cool";
  if (mode == "heat_cool") return "Heat/Cool";
  if (mode == "auto") return "Auto";
  if (mode == "dry") return "Dry";
  if (mode == "fan_only") return "Fan";
  return sentence_cap_text(mode);
}

inline std::string climate_action_label(const ClimateCardCtx *ctx) {
  if (!ctx || !ctx->available) return "Unavailable";
  if (ctx->hvac_action == "heating") return "Heating";
  if (ctx->hvac_action == "cooling") return "Cooling";
  if (ctx->hvac_action == "drying") return "Drying";
  if (ctx->hvac_action == "fan") return "Fan";
  if (ctx->hvac_mode == "off") return "Off";
  return "Idle";
}

inline bool climate_action_text_is_active(const std::string &action) {
  return !(action.empty() ||
           action == "idle" ||
           action == "off" ||
           action == "unavailable" ||
           action == "unknown");
}

inline bool climate_action_is_active(const ClimateCardCtx *ctx) {
  if (!ctx || !ctx->available || ctx->hvac_mode == "off") return false;
  return climate_action_text_is_active(ctx->hvac_action);
}

inline void climate_format_service_temp(char *buf, size_t size, float value) {
  format_fixed_decimal(buf, size, value, 1);
}

inline int climate_display_precision(const ClimateCardCtx *ctx) {
  if (!ctx) return 0;
  return ctx->precision < 0 ? 0 : (ctx->precision > 3 ? 3 : ctx->precision);
}

inline void climate_format_temp(char *buf, size_t size, const ClimateCardCtx *ctx, float value) {
  format_fixed_decimal(buf, size, value, climate_display_precision(ctx));
}

inline void climate_format_temp_unit(char *buf, size_t size, const ClimateCardCtx *ctx, float value) {
  format_fixed_decimal_unit(buf, size, value, climate_display_precision(ctx),
                            display_temperature_unit_symbol());
}

inline std::string climate_dashboard_target_value_text(const ClimateCardCtx *ctx) {
  if (!ctx || !ctx->available) return "--";
  char buf[32];
  if (ctx->has_low && ctx->has_high) {
    char low_buf[16];
    char high_buf[16];
    climate_format_temp(low_buf, sizeof(low_buf), ctx, ctx->low);
    climate_format_temp(high_buf, sizeof(high_buf), ctx, ctx->high);
    snprintf(buf, sizeof(buf), "%s-%s", low_buf, high_buf);
    return std::string(buf);
  }
  if (ctx->has_target) {
    climate_format_temp(buf, sizeof(buf), ctx, ctx->target);
    return std::string(buf);
  }
  if (ctx->has_low) {
    climate_format_temp(buf, sizeof(buf), ctx, ctx->low);
    return std::string(buf);
  }
  if (ctx->has_high) {
    climate_format_temp(buf, sizeof(buf), ctx, ctx->high);
    return std::string(buf);
  }
  return "--";
}

inline bool climate_dual_target(const ClimateCardCtx *ctx) {
  return ctx && ctx->hvac_mode == "heat_cool" && ctx->has_low && ctx->has_high;
}

inline float climate_step(const ClimateCardCtx *ctx) {
  return (ctx && ctx->step > 0.0f && ctx->step < 10.0f) ? ctx->step : 0.5f;
}

inline float climate_clamp(float value, float lo, float hi) {
  if (hi < lo) hi = lo;
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

inline float climate_round_to_step(const ClimateCardCtx *ctx, float value) {
  float step = climate_step(ctx);
  float base = ctx ? ctx->min_temp : 0.0f;
  float steps = floorf(((value - base) / step) + 0.5f);
  return base + steps * step;
}

inline float climate_selected_target(const ClimateCardCtx *ctx) {
  if (!ctx) return 20.0f;
  if (climate_dual_target(ctx)) return ctx->edit_high ? ctx->high : ctx->low;
  if (ctx->has_target) return ctx->target;
  if (ctx->has_low) return ctx->low;
  if (ctx->has_high) return ctx->high;
  return climate_clamp(20.0f, ctx->min_temp, ctx->max_temp);
}

inline std::string climate_dashboard_label(const ClimateCardCtx *ctx) {
  if (!ctx) return "Climate";
  if (climate_action_is_active(ctx)) return climate_action_label(ctx);
  if (!ctx->label.empty()) return ctx->label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  return ctx->entity_id.empty() ? std::string("Climate") : ctx->entity_id;
}

inline uint32_t climate_state_color(const ClimateCardCtx *ctx) {
  if (!ctx) return CLIMATE_NEUTRAL_COLOR;
  return climate_action_is_active(ctx) ? ctx->on_color : ctx->off_color;
}

inline uint32_t climate_detail_accent_color(const ClimateCardCtx *ctx) {
  if (!ctx || !ctx->available || ctx->hvac_mode == "off") return 0xBDBDBD;
  if (ctx->hvac_action == "heating") return CLIMATE_HEAT_COLOR;
  if (ctx->hvac_action == "cooling") return CLIMATE_COOL_COLOR;
  return ctx->on_color;
}

inline void climate_apply_btn_color(lv_obj_t *btn, uint32_t color) {
  if (!btn) return;
  lv_obj_set_style_bg_color(btn, lv_color_hex(color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  lv_obj_set_style_bg_color(btn, lv_color_hex(color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
  lv_obj_set_style_bg_color(btn, lv_color_hex(color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
}

inline void climate_update_dashboard(ClimateCardCtx *ctx) {
  if (!ctx) return;
  climate_apply_btn_color(ctx->card_btn, climate_state_color(ctx));
  if (climate_action_is_active(ctx)) lv_obj_add_state(ctx->card_btn, LV_STATE_CHECKED);
  else lv_obj_clear_state(ctx->card_btn, LV_STATE_CHECKED);

  if (ctx->value_lbl) {
    std::string target = climate_dashboard_target_value_text(ctx);
    lv_label_set_text(ctx->value_lbl, target.c_str());
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, target == "--" ? "" : display_temperature_unit_symbol());
  }
  if (ctx->text_lbl) {
    std::string label = climate_dashboard_label(ctx);
    lv_label_set_text(ctx->text_lbl, label.c_str());
  }
}

inline std::vector<std::string> climate_parse_list(const std::string &raw) {
  std::vector<std::string> out;
  auto trim_token = [](std::string token) {
    while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) token.erase(token.begin());
    while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) token.pop_back();
    return token;
  };
  auto lowercase_token = [](const std::string &token) {
    std::string lower;
    lower.reserve(token.size());
    for (char ch : token) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    return lower;
  };
  auto push_option = [&](std::string token) {
    token = trim_token(token);
    if (token.empty()) return;
    std::string lower = lowercase_token(token);
    if (lower == "hvacmode" || lower == "fanmode" || lower == "swingmode" || lower == "presetmode") return;
    bool has_alpha = false;
    bool has_lower = false;
    for (char ch : token) {
      if (!std::isalpha(static_cast<unsigned char>(ch))) continue;
      has_alpha = true;
      if (std::islower(static_cast<unsigned char>(ch))) has_lower = true;
    }
    if (has_alpha && !has_lower) token = lower;
    for (const auto &existing : out) {
      if (lowercase_token(existing) == lower) return;
    }
    out.push_back(token);
  };

  std::string quoted_token;
  bool reading_quote = false;
  char quote_char = 0;
  for (char ch : raw) {
    if (reading_quote) {
      if (ch == quote_char) {
        push_option(quoted_token);
        quoted_token.clear();
        reading_quote = false;
      } else {
        quoted_token.push_back(ch);
      }
    } else if (ch == '\'' || ch == '"') {
      reading_quote = true;
      quote_char = ch;
    }
  }
  if (!out.empty()) return out;

  std::string token;
  auto push_token = [&]() {
    push_option(token);
    token.clear();
  };
  for (char ch : raw) {
    if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-' || ch == '/' || ch == ' ') {
      token.push_back(ch);
    } else {
      push_token();
    }
  }
  push_token();
  return out;
}

inline bool climate_has_options(const std::vector<std::string> &options) {
  return !options.empty();
}

inline void climate_send_string_action(ClimateCardCtx *ctx,
                                       const char *service,
                                       const char *key,
                                       const std::string &value) {
  if (!ctx || ctx->entity_id.empty() || esphome::api::global_api_server == nullptr) return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  req.data.init(2);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(ctx->entity_id.c_str());
  auto &value_kv = req.data.emplace_back();
  value_kv.key = decltype(value_kv.key)(key);
  value_kv.value = decltype(value_kv.value)(value.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void climate_send_temperature_action(ClimateCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || esphome::api::global_api_server == nullptr) return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)("climate.set_temperature");
  req.is_event = false;
  bool dual = climate_dual_target(ctx);
  req.data.init(dual ? 3 : 2);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(ctx->entity_id.c_str());
  char low_buf[16];
  char high_buf[16];
  char target_buf[16];
  if (dual) {
    climate_format_service_temp(low_buf, sizeof(low_buf), ctx->low);
    climate_format_service_temp(high_buf, sizeof(high_buf), ctx->high);
    auto &low_kv = req.data.emplace_back();
    low_kv.key = decltype(low_kv.key)("target_temp_low");
    low_kv.value = decltype(low_kv.value)(low_buf);
    auto &high_kv = req.data.emplace_back();
    high_kv.key = decltype(high_kv.key)("target_temp_high");
    high_kv.value = decltype(high_kv.value)(high_buf);
  } else {
    climate_format_service_temp(target_buf, sizeof(target_buf), climate_selected_target(ctx));
    auto &temp_kv = req.data.emplace_back();
    temp_kv.key = decltype(temp_kv.key)("temperature");
    temp_kv.value = decltype(temp_kv.value)(target_buf);
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void climate_send_timer_cb(lv_timer_t *timer) {
  ClimateCardCtx *ctx = static_cast<ClimateCardCtx *>(lv_timer_get_user_data(timer));
  if (!ctx) return;
  climate_send_temperature_action(ctx);
  lv_timer_pause(timer);
}

inline void climate_schedule_temperature_send(ClimateCardCtx *ctx) {
  if (!ctx) return;
  if (!ctx->send_timer) {
    ctx->send_timer = lv_timer_create(climate_send_timer_cb, 450, ctx);
  }
  lv_timer_reset(ctx->send_timer);
  lv_timer_resume(ctx->send_timer);
}

inline void climate_hide_popup() {
  ClimateDetailUi &ui = climate_detail_ui();
  if (ui.overlay) lv_obj_add_flag(ui.overlay, LV_OBJ_FLAG_HIDDEN);
  if (ui.popup) lv_obj_add_flag(ui.popup, LV_OBJ_FLAG_HIDDEN);
}

inline void climate_update_detail(ClimateCardCtx *ctx);

inline void climate_apply_selected_target(ClimateCardCtx *ctx, float value,
                                          bool send_debounced) {
  if (!ctx) return;
  float step = climate_step(ctx);
  value = climate_round_to_step(ctx, value);
  if (climate_dual_target(ctx)) {
    if (ctx->edit_high) {
      float min_high = ctx->has_low ? ctx->low + step : ctx->min_temp + step;
      value = climate_clamp(value, min_high, ctx->max_temp);
      ctx->high = value;
      ctx->has_high = true;
    } else {
      float max_low = ctx->has_high ? ctx->high - step : ctx->max_temp - step;
      value = climate_clamp(value, ctx->min_temp, max_low);
      ctx->low = value;
      ctx->has_low = true;
    }
  } else {
    value = climate_clamp(value, ctx->min_temp, ctx->max_temp);
    ctx->target = value;
    ctx->has_target = true;
  }
  climate_update_detail(ctx);
  if (send_debounced) climate_schedule_temperature_send(ctx);
}

inline void climate_update_arc(ClimateCardCtx *ctx) {
  ClimateDetailUi &ui = climate_detail_ui();
  if (!ctx || !ui.arc) return;
  int min_v = static_cast<int>(roundf(ctx->min_temp * 10.0f));
  int max_v = static_cast<int>(roundf(ctx->max_temp * 10.0f));
  if (max_v <= min_v) max_v = min_v + 1;
  int value = static_cast<int>(roundf(climate_selected_target(ctx) * 10.0f));
  if (value < min_v) value = min_v;
  if (value > max_v) value = max_v;
  ui.updating_arc = true;
  lv_arc_set_range(ui.arc, min_v, max_v);
  lv_arc_set_value(ui.arc, value);
  ui.updating_arc = false;
}

inline void climate_style_chip(lv_obj_t *btn, bool active) {
  if (!btn) return;
  lv_obj_set_style_bg_color(btn, lv_color_hex(active ? 0x303030 : 0x222222), LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(active ? 0xA0A0A0 : 0x333333), LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, active ? 2 : 1, LV_PART_MAIN);
}

inline void climate_set_button_label(lv_obj_t *btn, const std::string &text) {
  if (!btn) return;
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) lv_label_set_text(label, text.c_str());
}

inline void climate_set_visible(lv_obj_t *obj, bool visible) {
  if (!obj) return;
  if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

inline void climate_option_click(lv_event_t *e);

inline std::string climate_tabs_key(const std::vector<std::string> &options) {
  std::string key;
  for (const auto &option : options) {
    key += option;
    key.push_back('\n');
  }
  return key;
}

inline bool climate_value_equals(const std::string &a, const std::string &b) {
  if (a.empty() && (b.empty() || b == "none")) return true;
  if (b.empty() && a == "none") return true;
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); i++) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i]))) return false;
  }
  return true;
}

inline void climate_style_mode_tab(lv_obj_t *btn, bool active, uint32_t accent_color) {
  if (!btn) return;
  lv_obj_set_style_bg_color(btn, lv_color_hex(active ? 0x303030 : 0x202020), LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(active ? accent_color : 0x333333), LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, active ? 2 : 1, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) lv_obj_set_style_text_color(label, lv_color_hex(active ? accent_color : 0xD8D8D8), LV_PART_MAIN);
}

inline void climate_render_mode_tabs(ClimateCardCtx *ctx) {
  ClimateDetailUi &ui = climate_detail_ui();
  if (!ctx || !ui.mode_tabs) return;
  bool show = climate_has_options(ctx->hvac_modes);
  climate_set_visible(ui.mode_tabs, show);
  if (!show) {
    if (ui.mode_tabs_key.empty() && ui.mode_tabs_ctx == nullptr) return;
    lv_obj_clean(ui.mode_tabs);
    ui.mode_tabs_key.clear();
    ui.mode_tabs_ctx = nullptr;
    return;
  }

  std::string key = climate_tabs_key(ctx->hvac_modes);
  if (ui.mode_tabs_ctx != ctx || ui.mode_tabs_key != key) {
    lv_obj_clean(ui.mode_tabs);
    ui.mode_tabs_key = key;
    ui.mode_tabs_ctx = ctx;
    for (const auto &value : ctx->hvac_modes) {
      lv_obj_t *btn = lv_btn_create(ui.mode_tabs);
      lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
      lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
      lv_obj_t *label = lv_label_create(btn);
      lv_label_set_text(label, climate_mode_label(value).c_str());
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (ctx->label_font) lv_obj_set_style_text_font(label, ctx->label_font, LV_PART_MAIN);
      lv_obj_center(label);
      ClimateOptionCtx *opt = new ClimateOptionCtx();
      opt->ctx = ctx;
      opt->kind = "hvac";
      opt->value = value;
      lv_obj_add_event_cb(btn, climate_option_click, LV_EVENT_CLICKED, opt);
      lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        delete static_cast<ClimateOptionCtx *>(lv_event_get_user_data(e));
      }, LV_EVENT_DELETE, opt);
    }
  }

  uint32_t accent_color = climate_detail_accent_color(ctx);
  uint32_t child_count = lv_obj_get_child_count(ui.mode_tabs);
  int count = static_cast<int>(child_count);
  lv_coord_t tabs_w = lv_obj_get_width(ui.mode_tabs);
  lv_coord_t gap = 8;
  lv_coord_t tab_w = count > 0 ? (tabs_w - gap * (count - 1)) / count : tabs_w;
  if (tab_w < 62) tab_w = 62;
  for (uint32_t i = 0; i < child_count && i < ctx->hvac_modes.size(); i++) {
    lv_obj_t *btn = lv_obj_get_child(ui.mode_tabs, i);
    if (!btn) continue;
    lv_obj_set_size(btn, tab_w, lv_pct(100));
    climate_style_mode_tab(btn, climate_value_equals(ctx->hvac_modes[i], ctx->hvac_mode), accent_color);
  }
}

inline void climate_update_detail(ClimateCardCtx *ctx) {
  ClimateDetailUi &ui = climate_detail_ui();
  if (!ctx || ui.active != ctx) return;

  if (ui.state_label) {
    std::string state = climate_action_label(ctx);
    lv_label_set_text(ui.state_label, state.c_str());
    lv_obj_set_style_text_color(ui.state_label, lv_color_hex(CLIMATE_DETAIL_TEXT_COLOR), LV_PART_MAIN);
  }
  if (ui.target_value) {
    char tbuf[16];
    climate_format_temp(tbuf, sizeof(tbuf), ctx, climate_selected_target(ctx));
    lv_label_set_text(ui.target_value, tbuf);
  }
  if (ui.target_unit && ui.target_value) {
    lv_label_set_text(ui.target_unit, display_temperature_unit_symbol());
    lv_obj_set_style_text_color(ui.target_unit, lv_color_hex(CLIMATE_DETAIL_TEXT_COLOR), LV_PART_MAIN);
    lv_obj_update_layout(ui.target_value);
    lv_obj_align_to(ui.target_unit, ui.target_value, LV_ALIGN_OUT_RIGHT_TOP, 2, 8);
    if (ui.state_label) lv_obj_align_to(ui.state_label, ui.target_value, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
  }
  if (ui.target_hint) {
    if (climate_dual_target(ctx)) {
      lv_label_set_text(ui.target_hint, ctx->edit_high ? "High target" : "Low target");
      climate_set_visible(ui.target_hint, true);
    } else {
      climate_set_visible(ui.target_hint, false);
    }
  }
  bool dual = climate_dual_target(ctx);
  climate_set_visible(ui.low_btn, dual);
  climate_set_visible(ui.high_btn, dual);
  climate_style_chip(ui.low_btn, dual && !ctx->edit_high);
  climate_style_chip(ui.high_btn, dual && ctx->edit_high);

  climate_set_visible(ui.preset_chip, climate_has_options(ctx->hvac_modes));
  climate_set_visible(ui.fan_chip, climate_has_options(ctx->fan_modes));
  climate_set_visible(ui.swing_chip, climate_has_options(ctx->swing_modes));
  climate_set_button_label(ui.preset_chip, find_icon("Dots Horizontal"));
  climate_set_button_label(ui.fan_chip, "Fan\n" + (ctx->fan_mode.empty() ? std::string("None") : climate_mode_label(ctx->fan_mode)));
  climate_set_button_label(ui.swing_chip, "Swing\n" + (ctx->swing_mode.empty() ? std::string("None") : climate_mode_label(ctx->swing_mode)));
  climate_set_visible(ui.mode_tabs, false);

  uint32_t active_color = climate_detail_accent_color(ctx);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(ctx ? ctx->off_color : CLIMATE_NEUTRAL_COLOR), LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(active_color), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui.arc, lv_color_hex(0xF4F4F4), LV_PART_KNOB);
  lv_obj_set_style_border_color(ui.arc, lv_color_hex(active_color), LV_PART_KNOB);
  climate_update_arc(ctx);
}

inline void refresh_temperature_unit_labels() {
  WeatherForecastCardRef *weather_refs = weather_forecast_card_refs();
  int weather_count = weather_forecast_card_count();
  for (int i = 0; i < weather_count; i++) {
    apply_weather_forecast_card_text(weather_refs[i], weather_refs[i].valid,
                                     weather_refs[i].high, weather_refs[i].low,
                                     weather_refs[i].source_unit);
  }

  ClimateCardCtx **climate_refs = climate_card_contexts();
  int climate_count = climate_card_context_count();
  for (int i = 0; i < climate_count; i++) {
    climate_update_dashboard(climate_refs[i]);
    climate_update_detail(climate_refs[i]);
  }
}

inline lv_obj_t *climate_create_label(lv_obj_t *parent, const char *text,
                                      lv_align_t align, lv_coord_t x, lv_coord_t y,
                                      const lv_font_t *font = nullptr,
                                      uint32_t color = 0xE8E8E8) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_align(label, align, x, y);
  return label;
}

inline lv_obj_t *climate_create_round_button(lv_obj_t *parent, lv_coord_t size,
                                             const char *text, const lv_font_t *font = nullptr,
                                             uint32_t border_color = 0xFFFFFF,
                                             uint32_t text_color = 0xD8D8D8,
                                             uint32_t bg_color = CLIMATE_DETAIL_CARD_COLOR) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, size, size);
  lv_obj_set_style_radius(btn, size / 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(border_color), LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(text_color), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_center(label);
  return btn;
}

inline void climate_set_button_label_font(lv_obj_t *btn, const lv_font_t *font) {
  if (!btn || !font) return;
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
}

inline void climate_center_button_label(lv_obj_t *btn) {
  if (!btn) return;
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (!label) return;
  lv_obj_set_style_transform_zoom(label, 256, LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_center(label);
}

inline lv_obj_t *climate_create_chip(lv_obj_t *parent, const char *text,
                                     const lv_font_t *font = nullptr) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(0xD8D8D8), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_center(label);
  return btn;
}

inline void climate_layout_detail_ui(ClimateCardCtx *ctx) {
  ClimateDetailUi &ui = climate_detail_ui();
  if (!ui.page) return;
  lv_disp_t *disp = lv_disp_get_default();
  lv_coord_t sw = disp ? lv_disp_get_hor_res(disp) : 480;
  lv_coord_t sh = disp ? lv_disp_get_ver_res(disp) : 480;
  lv_coord_t short_side = sw < sh ? sw : sh;
  ClimateHomeGridMetrics &home_metrics = climate_home_grid_metrics();
  lv_obj_t *home_page = home_metrics.page;
  lv_coord_t card_x = home_page ? lv_obj_get_style_pad_left(home_page, LV_PART_MAIN) : (short_side < 520 ? 12 : 28);
  lv_coord_t card_y = home_page ? lv_obj_get_style_pad_top(home_page, LV_PART_MAIN) : (sw < sh ? 58 : (short_side < 520 ? 48 : 104));
  lv_coord_t card_right = home_page ? lv_obj_get_style_pad_right(home_page, LV_PART_MAIN) : card_x;
  lv_coord_t card_bottom = home_page ? lv_obj_get_style_pad_bottom(home_page, LV_PART_MAIN) : (short_side < 520 ? 12 : 28);
  lv_coord_t card_w = sw - card_x - card_right;
  lv_coord_t card_h = sh - card_y - card_bottom;
  if (card_w < sw / 2) {
    card_x = 0;
    card_w = sw;
  }
  if (card_h < sh / 2) {
    card_y = 0;
    card_h = sh;
  }
  lv_coord_t card_pad = short_side < 520 ? 18 : 42;
  lv_coord_t frame_w = card_w - card_pad * 2;
  lv_coord_t frame_h = card_h - card_pad * 2;
  if (frame_w < 1) frame_w = card_w;
  if (frame_h < 1) frame_h = card_h;
  lv_coord_t frame_x = card_x + card_pad;
  lv_coord_t frame_y = card_y + card_pad;
  lv_coord_t frame_cx = frame_x + frame_w / 2 - sw / 2;
  lv_coord_t frame_cy = frame_y + frame_h / 2 - sh / 2;
  lv_coord_t arc_size = (frame_w < frame_h ? frame_w : frame_h) * (short_side < 520 ? 90 : 96) / 100;
  if (arc_size < 300) arc_size = (frame_w < frame_h ? frame_w : frame_h) * 92 / 100;
  lv_coord_t arc_cx = frame_cx;
  lv_coord_t arc_cy = frame_cy + (sh < 520 ? 24 : 60);
  lv_coord_t round_btn = short_side * 15 / 100;
  if (round_btn < 66) round_btn = 66;
  if (round_btn > 116) round_btn = 116;
  lv_coord_t top_btn = round_btn * 86 / 100;
  if (top_btn < 56) top_btn = 56;
  lv_coord_t chip_w = (frame_w - 56) / 4;
  if (chip_w > 140) chip_w = 140;
  if (chip_w < 86) chip_w = 86;
  lv_coord_t chip_h = sh < 520 ? 56 : 64;
  lv_coord_t control_y = frame_y + frame_h - chip_h;

  if (ui.card) {
    lv_obj_set_size(ui.card, card_w, card_h);
    lv_obj_set_style_radius(ui.card, short_side < 520 ? 12 : 28, LV_PART_MAIN);
    lv_obj_align(ui.card, LV_ALIGN_TOP_LEFT, card_x, card_y);
    lv_obj_move_background(ui.card);
  }
  lv_obj_set_size(ui.back_btn, top_btn, top_btn);
  lv_obj_set_style_radius(ui.back_btn, top_btn / 2, LV_PART_MAIN);
  lv_coord_t menu_inset = short_side < 520 ? 3 : 4;
  lv_obj_align(ui.back_btn, LV_ALIGN_TOP_LEFT, frame_x + menu_inset, frame_y + menu_inset);
  lv_obj_move_foreground(ui.back_btn);
  lv_obj_set_size(ui.preset_chip, top_btn, top_btn);
  lv_obj_set_style_radius(ui.preset_chip, top_btn / 2, LV_PART_MAIN);
  lv_obj_align(ui.preset_chip, LV_ALIGN_TOP_LEFT,
    frame_x + frame_w - top_btn - menu_inset, frame_y + menu_inset);
  lv_obj_move_foreground(ui.preset_chip);

  lv_obj_set_size(ui.arc, arc_size, arc_size);
  lv_obj_align(ui.arc, LV_ALIGN_CENTER, arc_cx, arc_cy);
  lv_obj_set_style_arc_width(ui.arc, short_side < 520 ? 18 : 28, LV_PART_MAIN);
  lv_obj_set_style_arc_width(ui.arc, short_side < 520 ? 18 : 28, LV_PART_INDICATOR);
  lv_obj_set_style_pad_all(ui.arc, short_side < 520 ? 8 : 12, LV_PART_KNOB);
  lv_obj_set_size(ui.minus_btn, round_btn, round_btn);
  lv_obj_set_style_radius(ui.minus_btn, round_btn / 2, LV_PART_MAIN);
  lv_obj_set_size(ui.plus_btn, round_btn, round_btn);
  lv_obj_set_style_radius(ui.plus_btn, round_btn / 2, LV_PART_MAIN);
  lv_obj_t *controls[2] = {ui.fan_chip, ui.swing_chip};
  bool visible[2] = {
    ctx && climate_has_options(ctx->fan_modes),
    ctx && climate_has_options(ctx->swing_modes),
  };
  int visible_count = 0;
  for (bool is_visible : visible) {
    if (is_visible) visible_count++;
  }
  lv_coord_t adjust_btn_x = (round_btn + 18) / 2;
  lv_coord_t adjust_btn_y_abs = sh / 2 + arc_cy + arc_size / 2 - round_btn / 2;
  lv_coord_t max_btn_y_abs = (visible_count > 0 ? control_y - 12 : frame_y + frame_h) - round_btn / 2;
  if (adjust_btn_y_abs > max_btn_y_abs) adjust_btn_y_abs = max_btn_y_abs;
  lv_coord_t adjust_btn_y = adjust_btn_y_abs - sh / 2;
  lv_obj_align(ui.minus_btn, LV_ALIGN_CENTER, frame_cx - adjust_btn_x, adjust_btn_y);
  lv_obj_align(ui.plus_btn, LV_ALIGN_CENTER, frame_cx + adjust_btn_x, adjust_btn_y);

  lv_obj_set_size(ui.fan_chip, chip_w, chip_h);
  lv_obj_set_size(ui.swing_chip, chip_w, chip_h);
  lv_obj_set_size(ui.mode_tabs, frame_w, chip_h);
  lv_obj_align(ui.mode_tabs, LV_ALIGN_TOP_LEFT, frame_x, control_y);
  climate_set_visible(ui.mode_tabs, false);

  lv_coord_t gap = 8;
  lv_coord_t row_w = visible_count > 0 ? visible_count * chip_w + (visible_count - 1) * gap : chip_w;
  lv_coord_t x = frame_x + (frame_w - row_w) / 2;
  for (int i = 0; i < 2; i++) {
    if (!visible[i]) continue;
    lv_obj_align(controls[i], LV_ALIGN_TOP_LEFT, x, control_y);
    x += chip_w + gap;
  }

  lv_obj_align(ui.target_value, LV_ALIGN_CENTER, frame_cx, arc_cy - arc_size / 18);
  lv_obj_update_layout(ui.target_value);
  lv_obj_align_to(ui.target_unit, ui.target_value, LV_ALIGN_OUT_RIGHT_TOP, 2, 8);
  lv_obj_align_to(ui.state_label, ui.target_value, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
  lv_obj_align(ui.target_hint, LV_ALIGN_CENTER, frame_cx, arc_cy + arc_size / 3);
  lv_obj_align(ui.low_btn, LV_ALIGN_CENTER, frame_cx - 44, arc_cy + arc_size / 3 + 32);
  lv_obj_align(ui.high_btn, LV_ALIGN_CENTER, frame_cx + 44, arc_cy + arc_size / 3 + 32);
  if (ctx && ctx->target_font) {
    lv_obj_set_style_text_font(ui.target_value, ctx->target_font, LV_PART_MAIN);
  }
  if (ctx && ctx->label_font) {
    lv_obj_set_style_text_font(ui.state_label, ctx->label_font, LV_PART_MAIN);
    lv_obj_set_style_text_font(ui.target_hint, ctx->label_font, LV_PART_MAIN);
    climate_set_button_label_font(ui.low_btn, ctx->label_font);
    climate_set_button_label_font(ui.high_btn, ctx->label_font);
    climate_set_button_label_font(ui.fan_chip, ctx->label_font);
    climate_set_button_label_font(ui.swing_chip, ctx->label_font);
  }
  const lv_font_t *unit_font = ctx && ctx->unit_font ? ctx->unit_font : (ctx ? ctx->label_font : nullptr);
  if (unit_font) lv_obj_set_style_text_font(ui.target_unit, unit_font, LV_PART_MAIN);
  const lv_font_t *control_icon_font = ctx && ctx->climate_control_icon_font
    ? ctx->climate_control_icon_font : (ctx ? ctx->icon_font : nullptr);
  if (control_icon_font) {
    climate_set_button_label_font(ui.back_btn, control_icon_font);
    climate_set_button_label_font(ui.preset_chip, control_icon_font);
    climate_set_button_label_font(ui.minus_btn, control_icon_font);
    climate_set_button_label_font(ui.plus_btn, control_icon_font);
  }
  climate_center_button_label(ui.back_btn);
  climate_center_button_label(ui.preset_chip);
  climate_center_button_label(ui.minus_btn);
  climate_center_button_label(ui.plus_btn);
}

inline void climate_open_options(ClimateCardCtx *ctx, const char *kind,
                                 const char *title,
                                 const std::vector<std::string> &options);

inline void climate_ensure_detail_ui(ClimateCardCtx *ctx) {
  ClimateDetailUi &ui = climate_detail_ui();
  if (ui.page) {
    climate_layout_detail_ui(ctx);
    return;
  }

  ui.page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(ui.page, lv_color_hex(0x111111), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.page, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(ui.page, LV_OBJ_FLAG_SCROLLABLE);

  ui.card = lv_obj_create(ui.page);
  lv_obj_set_style_bg_color(ui.card, lv_color_hex(CLIMATE_DETAIL_CARD_COLOR), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.card, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.card, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.card, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.card, LV_OBJ_FLAG_SCROLLABLE);

  const lv_font_t *control_icon_font = ctx && ctx->climate_control_icon_font
    ? ctx->climate_control_icon_font : (ctx ? ctx->icon_font : nullptr);
  ui.back_btn = climate_create_round_button(ui.page, 72, "\U000F0141", control_icon_font,
    0x555555, 0xBDBDBD, CLIMATE_DETAIL_CARD_COLOR);
  lv_obj_add_event_cb(ui.back_btn, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    climate_hide_popup();
    lv_obj_t *target = ui.return_page ? ui.return_page : (lv_obj_t *)lv_event_get_user_data(e);
    if (target) lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  }, LV_EVENT_CLICKED, nullptr);

  ui.arc = lv_arc_create(ui.page);
  lv_arc_set_bg_angles(ui.arc, 135, 45);
  lv_arc_set_range(ui.arc, 50, 350);
  lv_arc_set_value(ui.arc, 200);
  lv_obj_set_style_bg_opa(ui.arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_border_width(ui.arc, 4, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_add_flag(ui.arc, LV_OBJ_FLAG_ADV_HITTEST);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (ui.updating_arc || !ui.active) return;
    lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
    float temp = lv_arc_get_value(arc) / 10.0f;
    climate_apply_selected_target(ui.active, temp, false);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (ui.active) climate_send_temperature_action(ui.active);
  }, LV_EVENT_RELEASED, nullptr);

  const lv_font_t *unit_font = ctx && ctx->unit_font ? ctx->unit_font : (ctx ? ctx->label_font : nullptr);
  ui.state_label = climate_create_label(ui.page, "Idle", LV_ALIGN_CENTER, 0, -38, ctx ? ctx->label_font : nullptr, CLIMATE_DETAIL_TEXT_COLOR);
  ui.target_value = climate_create_label(ui.page, "20.0", LV_ALIGN_CENTER, 0, 14, ctx ? ctx->target_font : nullptr);
  ui.target_unit = climate_create_label(ui.page, display_temperature_unit_symbol(), LV_ALIGN_CENTER, 66, -2, unit_font, CLIMATE_DETAIL_TEXT_COLOR);
  ui.target_hint = climate_create_label(ui.page, "Target", LV_ALIGN_CENTER, 0, 78, ctx ? ctx->label_font : nullptr, 0xBDBDBD);
  ui.minus_btn = climate_create_round_button(ui.page, 72, find_icon("Minus"), control_icon_font,
    0xBDBDBD, 0xD8D8D8, CLIMATE_DETAIL_CARD_COLOR);
  ui.plus_btn = climate_create_round_button(ui.page, 72, find_icon("Plus"), control_icon_font,
    0xBDBDBD, 0xD8D8D8, CLIMATE_DETAIL_CARD_COLOR);
  ui.low_btn = climate_create_chip(ui.page, "Low", ctx ? ctx->label_font : nullptr);
  ui.high_btn = climate_create_chip(ui.page, "High", ctx ? ctx->label_font : nullptr);
  lv_obj_set_size(ui.low_btn, 76, 36);
  lv_obj_set_size(ui.high_btn, 76, 36);
  lv_obj_add_event_cb(ui.minus_btn, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (!ui.active) return;
    climate_apply_selected_target(ui.active, climate_selected_target(ui.active) - climate_step(ui.active), true);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.plus_btn, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (!ui.active) return;
    climate_apply_selected_target(ui.active, climate_selected_target(ui.active) + climate_step(ui.active), true);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.low_btn, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (!ui.active) return;
    ui.active->edit_high = false;
    climate_update_detail(ui.active);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.high_btn, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (!ui.active) return;
    ui.active->edit_high = true;
    climate_update_detail(ui.active);
  }, LV_EVENT_CLICKED, nullptr);

  ui.preset_chip = climate_create_round_button(ui.page, 48, find_icon("Dots Horizontal"), control_icon_font,
    0x555555, 0xBDBDBD, CLIMATE_DETAIL_CARD_COLOR);
  ui.fan_chip = climate_create_chip(ui.page, "Fan\nNone", ctx ? ctx->label_font : nullptr);
  ui.swing_chip = climate_create_chip(ui.page, "Swing\nNone", ctx ? ctx->label_font : nullptr);
  ui.mode_tabs = lv_obj_create(ui.page);
  lv_obj_set_style_bg_opa(ui.mode_tabs, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.mode_tabs, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.mode_tabs, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.mode_tabs, 8, LV_PART_MAIN);
  lv_obj_set_layout(ui.mode_tabs, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.mode_tabs, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.mode_tabs, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.mode_tabs, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_add_flag(ui.mode_tabs, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(ui.preset_chip, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (ui.active) climate_open_options(ui.active, "hvac", "Mode", ui.active->hvac_modes);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.fan_chip, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (ui.active) climate_open_options(ui.active, "fan", "Fan", ui.active->fan_modes);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.swing_chip, [](lv_event_t *e) {
    ClimateDetailUi &ui = climate_detail_ui();
    if (ui.active) climate_open_options(ui.active, "swing", "Swing", ui.active->swing_modes);
  }, LV_EVENT_CLICKED, nullptr);

  ui.overlay = lv_obj_create(ui.page);
  lv_obj_set_size(ui.overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(ui.overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_60, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_add_flag(ui.overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(ui.overlay, [](lv_event_t *e) { climate_hide_popup(); }, LV_EVENT_CLICKED, nullptr);

  ui.popup = lv_obj_create(ui.page);
  lv_obj_set_size(ui.popup, lv_pct(76), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(ui.popup, lv_color_hex(0x242424), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.popup, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.popup, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(ui.popup, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.popup, 16, LV_PART_MAIN);
  lv_obj_set_layout(ui.popup, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.popup, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.popup, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui.popup, 10, LV_PART_MAIN);
  lv_obj_align(ui.popup, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(ui.popup, LV_OBJ_FLAG_HIDDEN);

  climate_layout_detail_ui(ctx);
}

inline void climate_option_click(lv_event_t *e) {
  ClimateOptionCtx *opt = static_cast<ClimateOptionCtx *>(lv_event_get_user_data(e));
  if (!opt || !opt->ctx) return;
  if (opt->kind == "hvac") {
    opt->ctx->hvac_mode = opt->value;
    opt->ctx->available = true;
    climate_send_string_action(opt->ctx, "climate.set_hvac_mode", "hvac_mode", opt->value);
  } else if (opt->kind == "fan") {
    opt->ctx->fan_mode = opt->value;
    climate_send_string_action(opt->ctx, "climate.set_fan_mode", "fan_mode", opt->value);
  } else if (opt->kind == "swing") {
    opt->ctx->swing_mode = opt->value;
    climate_send_string_action(opt->ctx, "climate.set_swing_mode", "swing_mode", opt->value);
  } else if (opt->kind == "preset") {
    opt->ctx->preset_mode = opt->value;
    climate_send_string_action(opt->ctx, "climate.set_preset_mode", "preset_mode", opt->value);
  }
  climate_hide_popup();
  climate_update_dashboard(opt->ctx);
  climate_update_detail(opt->ctx);
}

inline void climate_open_options(ClimateCardCtx *ctx, const char *kind,
                                 const char *title,
                                 const std::vector<std::string> &options) {
  ClimateDetailUi &ui = climate_detail_ui();
  if (!ctx || !ui.overlay || !ui.popup || options.empty()) return;
  lv_obj_clean(ui.popup);
  bool compact_menu = std::strcmp(kind, "preset") == 0 || std::strcmp(kind, "hvac") == 0;
  lv_obj_set_style_pad_all(ui.popup, compact_menu ? 12 : 16, LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui.popup, compact_menu ? 8 : 10, LV_PART_MAIN);
  if (!compact_menu) {
    lv_obj_t *title_lbl = lv_label_create(ui.popup);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(title_lbl, ctx->label_font, LV_PART_MAIN);
  }

  for (const auto &value : options) {
    lv_obj_t *btn = climate_create_chip(ui.popup, climate_mode_label(value).c_str(), ctx->label_font);
    lv_obj_set_width(btn, compact_menu ? lv_pct(100) : lv_pct(92));
    lv_obj_set_height(btn, compact_menu ? 58 : 44);
    ClimateOptionCtx *opt = new ClimateOptionCtx();
    opt->ctx = ctx;
    opt->kind = kind;
    opt->value = value;
    lv_obj_add_event_cb(btn, climate_option_click, LV_EVENT_CLICKED, opt);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      delete static_cast<ClimateOptionCtx *>(lv_event_get_user_data(e));
    }, LV_EVENT_DELETE, opt);
  }
  if (!compact_menu) {
    lv_obj_t *cancel = climate_create_chip(ui.popup, "Cancel", ctx->label_font);
    lv_obj_set_width(cancel, lv_pct(72));
    lv_obj_set_height(cancel, 40);
    lv_obj_add_event_cb(cancel, [](lv_event_t *e) { climate_hide_popup(); }, LV_EVENT_CLICKED, nullptr);
  }

  lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui.popup, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(ui.overlay);
  lv_obj_move_foreground(ui.popup);
  if (compact_menu) {
    lv_disp_t *disp = lv_disp_get_default();
    lv_coord_t sw = disp ? lv_disp_get_hor_res(disp) : 480;
    lv_coord_t sh = disp ? lv_disp_get_ver_res(disp) : 480;
    lv_coord_t short_side = sw < sh ? sw : sh;
    lv_coord_t top_clearance = short_side < 520 ? 44 : 56;
    lv_coord_t menu_size = short_side < 520 ? 44 : 48;
    lv_coord_t menu_w = sw < 420 ? sw - 32 : 280;
    if (menu_w < 220) menu_w = 220;
    lv_obj_set_width(ui.popup, menu_w);
    lv_obj_align(ui.popup, LV_ALIGN_TOP_RIGHT, -12, top_clearance + menu_size + 8);
  } else {
    lv_obj_set_width(ui.popup, lv_pct(76));
    lv_obj_align(ui.popup, LV_ALIGN_CENTER, 0, 0);
  }
}

inline void climate_open_detail(ClimateCardCtx *ctx, lv_obj_t *return_page) {
  if (!ctx) return;
  climate_ensure_detail_ui(ctx);
  ClimateDetailUi &ui = climate_detail_ui();
  ui.active = ctx;
  ui.return_page = return_page ? return_page : lv_scr_act();
  climate_hide_popup();
  climate_layout_detail_ui(ctx);
  climate_update_detail(ctx);
  lv_scr_load_anim(ui.page, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

inline void setup_climate_card(BtnSlot &s, const ParsedCfg &p,
                               const lv_font_t *value_font,
                               uint32_t off_color) {
  lv_label_set_text(s.icon_lbl, find_icon("Thermostat"));
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (value_font) lv_obj_set_style_text_font(s.sensor_lbl, value_font, LV_PART_MAIN);
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Climate" : p.label.c_str());
  climate_apply_btn_color(s.btn, off_color);
}

inline void climate_subscribe_attribute_float(ClimateCardCtx *ctx,
                                              const std::string &attribute,
                                              std::function<void(float)> on_value,
                                              std::function<void()> on_invalid = nullptr) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, attribute,
    std::function<void(esphome::StringRef)>(
      [ctx, on_value, on_invalid](esphome::StringRef state) {
        float value = 0.0f;
        if (parse_float_ref(state, value)) {
          on_value(value);
        } else if (on_invalid) {
          on_invalid();
        }
        climate_update_dashboard(ctx);
        climate_update_detail(ctx);
      })
  );
}

inline ClimateCardCtx *create_climate_context(lv_obj_t *card_btn,
                                              lv_obj_t *sensor_container,
                                              lv_obj_t *value_lbl,
                                              lv_obj_t *unit_lbl,
                                              lv_obj_t *text_lbl,
                                              lv_obj_t *icon_lbl,
                                              const ParsedCfg &p,
                                              uint32_t on_color,
                                              uint32_t off_color,
                                              const lv_font_t *value_font,
                                              const lv_font_t *target_font,
                                              const lv_font_t *icon_font,
                                              const lv_font_t *climate_control_icon_font) {
  ClimateCardCtx *ctx = new ClimateCardCtx();
  ctx->entity_id = p.entity;
  ctx->label = p.label;
  ctx->card_btn = card_btn;
  ctx->sensor_container = sensor_container;
  ctx->value_lbl = value_lbl;
  ctx->unit_lbl = unit_lbl;
  ctx->text_lbl = text_lbl;
  ctx->value_font = value_font;
  ctx->target_font = target_font ? target_font : value_font;
  ctx->label_font = text_lbl ? lv_obj_get_style_text_font(text_lbl, LV_PART_MAIN) : nullptr;
  ctx->unit_font = unit_lbl ? lv_obj_get_style_text_font(unit_lbl, LV_PART_MAIN) : ctx->label_font;
  ctx->icon_font = icon_font ? icon_font : (icon_lbl ? lv_obj_get_style_text_font(icon_lbl, LV_PART_MAIN) : nullptr);
  ctx->climate_control_icon_font = climate_control_icon_font ? climate_control_icon_font : ctx->icon_font;
  ctx->on_color = on_color;
  ctx->off_color = off_color;
  ctx->precision = parse_precision(p.precision);
  ctx->send_timer = lv_timer_create(climate_send_timer_cb, 450, ctx);
  lv_timer_pause(ctx->send_timer);
  lv_obj_set_user_data(card_btn, (void *)ctx);
  register_climate_context(ctx);
  climate_update_dashboard(ctx);
  return ctx;
}

inline void subscribe_climate_card(ClimateCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || esphome::api::global_api_server == nullptr) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, {},
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        std::string mode = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->available = !(mode.empty() || mode == "unavailable" || mode == "unknown");
        ctx->hvac_mode = ctx->available ? mode : "off";
        if (!climate_dual_target(ctx)) ctx->edit_high = false;
        climate_update_dashboard(ctx);
        climate_update_detail(ctx);
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      ctx->friendly_name = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      climate_update_dashboard(ctx);
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("hvac_action"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef action) {
      ctx->hvac_action = string_ref_limited(action, HA_SHORT_STATE_MAX_LEN);
      climate_update_dashboard(ctx);
      climate_update_detail(ctx);
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("hvac_modes"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef modes) {
      ctx->hvac_modes = climate_parse_list(string_ref_limited(modes, HA_STATE_TEXT_MAX_LEN));
      climate_update_detail(ctx);
    })
  );
  climate_subscribe_attribute_float(ctx, "temperature",
    [ctx](float value) { ctx->target = value; ctx->has_target = true; },
    [ctx]() { ctx->has_target = false; });
  climate_subscribe_attribute_float(ctx, "target_temp_low",
    [ctx](float value) { ctx->low = value; ctx->has_low = true; },
    [ctx]() { ctx->has_low = false; });
  climate_subscribe_attribute_float(ctx, "target_temp_high",
    [ctx](float value) { ctx->high = value; ctx->has_high = true; },
    [ctx]() { ctx->has_high = false; });
  climate_subscribe_attribute_float(ctx, "min_temp",
    [ctx](float value) { ctx->min_temp = value; });
  climate_subscribe_attribute_float(ctx, "max_temp",
    [ctx](float value) { ctx->max_temp = value; });
  climate_subscribe_attribute_float(ctx, "target_temp_step",
    [ctx](float value) { ctx->step = value; });
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("fan_mode"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->fan_mode = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
      climate_update_detail(ctx);
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("fan_modes"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->fan_modes = climate_parse_list(string_ref_limited(value, HA_STATE_TEXT_MAX_LEN));
      climate_update_detail(ctx);
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("swing_mode"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->swing_mode = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
      climate_update_detail(ctx);
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("swing_modes"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->swing_modes = climate_parse_list(string_ref_limited(value, HA_STATE_TEXT_MAX_LEN));
      climate_update_detail(ctx);
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("preset_mode"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->preset_mode = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
      climate_update_detail(ctx);
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("preset_modes"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->preset_modes = climate_parse_list(string_ref_limited(value, HA_STATE_TEXT_MAX_LEN));
      climate_update_detail(ctx);
    })
  );
}

// ── Grid layout parsing ───────────────────────────────────────────────

// Result of parsing a button_order CSV string into grid cell positions
struct OrderResult {
  int positions[MAX_GRID_SLOTS] = {};    // slot number at each grid position (1-based, 0=empty)
  bool is_double[MAX_GRID_SLOTS] = {};   // slot uses double height (suffix "d" or "b")
  bool is_wide[MAX_GRID_SLOTS] = {};     // slot uses double width (suffix "w" or "b")
};

// Parse "1,2d,3w,4b,..." into positions + double/wide flags
inline void parse_order_string(const std::string &order_str, int num_slots, OrderResult &result) {
  memset(result.positions, 0, sizeof(result.positions));
  memset(result.is_double, 0, sizeof(result.is_double));
  memset(result.is_wide, 0, sizeof(result.is_wide));
  int slot_limit = bounded_grid_slots(num_slots);
  if (order_str.empty()) return;
  size_t gpos = 0, start = 0;
  while (start <= order_str.length() && gpos < (size_t)slot_limit) {
    size_t comma = order_str.find(',', start);
    if (comma == std::string::npos) comma = order_str.length();
    if (comma > start) {
      std::string token = order_str.substr(start, comma - start);
      bool dbl = !token.empty() && token.back() == 'd';
      bool wide = !token.empty() && token.back() == 'w';
      bool big = !token.empty() && token.back() == 'b';
      if (big) { dbl = true; wide = true; }
      if (dbl || wide) token.pop_back();
      int v = atoi(token.c_str());
      if (v >= 1 && v <= slot_limit) {
        result.positions[gpos] = v;
        result.is_double[v - 1] = dbl;
        result.is_wide[v - 1] = wide;
      }
    }
    gpos++;
    start = comma + 1;
  }
}

// Zero out grid cells that are covered by a neighbouring double/wide/big button
inline void clear_spanned_cells(const OrderResult &order, int num_slots, int cols, OrderResult &result) {
  int slot_limit = bounded_grid_slots(num_slots);
  for (int p = 0; p < slot_limit; p++) {
    result.positions[p] = order.positions[p];
    result.is_double[p] = order.is_double[p];
    result.is_wide[p] = order.is_wide[p];
  }
  for (int p = 0; p < slot_limit; p++) {
    if (result.positions[p] <= 0) continue;
    int idx = result.positions[p] - 1;
    if (result.is_double[idx] && p + cols < slot_limit) {
      result.positions[p + cols] = 0;
    }
    if (result.is_wide[idx] && (p + 1) % cols != 0 && p + 1 < slot_limit) {
      result.positions[p + 1] = 0;
    }
    if (result.is_double[idx] && result.is_wide[idx] && (p + 1) % cols != 0 && p + cols + 1 < slot_limit) {
      result.positions[p + cols + 1] = 0;
    }
  }
}

// ── Button visuals ────────────────────────────────────────────────────

// Apply on/off background colors to a button's checked/pressed/default states
inline void apply_button_colors(lv_obj_t *btn, bool has_on, uint32_t on_val,
                                bool has_off, uint32_t off_val) {
  if (has_on) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(on_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
    lv_obj_set_style_bg_color(btn, lv_color_hex(on_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  }
  if (has_off) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(off_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
}

// Match the main-page button widget label behavior so longer titles wrap
// instead of running off the edge of the tile.
inline void configure_button_label_wrap(lv_obj_t *label) {
  if (!label) return;
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, lv_pct(100));
}

inline void set_wrapped_button_label_text(lv_obj_t *label, const std::string &text) {
  if (!label) return;
  configure_button_label_wrap(label);
  lv_label_set_text(label, text.c_str());
  lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

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
    lv_label_set_text(s.unit_lbl, p.unit.c_str());
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

inline bool weather_card_shows_tomorrow(const ParsedCfg &p) {
  return p.type == "weather_forecast" || (p.type == "weather" && p.precision == "tomorrow");
}

inline void setup_weather_forecast_card(BtnSlot &s, const ParsedCfg &p,
                                        bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.sensor_lbl, "--/--");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, "Tomorrow");
  register_weather_forecast_card(s.sensor_lbl, s.unit_lbl, s.text_lbl, p.entity);
}

inline void setup_garage_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, garage_closed_icon(p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Garage Door" : p.label.c_str());
}

inline void setup_lock_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, lock_locked_icon(p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Lock" : p.label.c_str());
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
        lv_label_set_text(s.unit_lbl, p.unit.c_str());
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
  lv_label_set_text(s.unit_lbl, p.unit.c_str());
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Subpage" : p.label.c_str());
}

// ── Home Assistant subscriptions ──────────────────────────────────────

struct ToggleTextSensorCtx {
  lv_obj_t *text_lbl = nullptr;
  std::string steady_text;
  std::string sensor_text = "--";
  bool on = false;
};

inline std::string label_text_or_empty(lv_obj_t *label) {
  if (!label) return "";
  const char *text = lv_label_get_text(label);
  return text ? std::string(text) : "";
}

inline void apply_toggle_text_sensor_label(ToggleTextSensorCtx *ctx) {
  if (!ctx || !ctx->text_lbl) return;
  set_wrapped_button_label_text(ctx->text_lbl, ctx->on ? ctx->sensor_text : ctx->steady_text);
}

// Subscribe to a HA sensor entity and update an LVGL label with its value
inline void subscribe_sensor_value(lv_obj_t *sensor_lbl, const std::string &sensor_id, int precision = 0) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(esphome::StringRef)>([sensor_lbl, precision](esphome::StringRef state) {
      float val = 0.0f;
      if (parse_float_ref(state, val)) {
        char buf[16];
        format_fixed_decimal(buf, sizeof(buf), val, precision);
        lv_label_set_text(sensor_lbl, buf);
      } else {
        std::string text = text_sensor_display_text(state, HA_SHORT_STATE_MAX_LEN);
        lv_label_set_text(sensor_lbl, text.c_str());
      }
    })
  );
}

inline void subscribe_toggle_text_sensor_value(ToggleTextSensorCtx *ctx, const std::string &sensor_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      if (!ctx) return;
      ctx->sensor_text = text_sensor_display_text(state);
      apply_toggle_text_sensor_label(ctx);
    })
  );
}

inline void subscribe_text_sensor_value(lv_obj_t *text_lbl, const std::string &sensor_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(esphome::StringRef)>([text_lbl](esphome::StringRef state) {
      set_wrapped_button_label_text(text_lbl, text_sensor_display_text(state));
    })
  );
}

inline void subscribe_weather_state(lv_obj_t *icon_lbl, lv_obj_t *text_lbl, const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>([icon_lbl, text_lbl](esphome::StringRef state) {
      std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
      lv_label_set_text(icon_lbl, weather_icon_for_state(state_text));
      lv_label_set_text(text_lbl, weather_label_for_state(state_text).c_str());
    })
  );
}

inline void subscribe_garage_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                   TransientStatusLabel *status_label,
                                   const char *closed_icon, const char *open_icon,
                                   const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool active = garage_state_is_active(state_text);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state_text) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state_text), garage_state_releases_label(state_text));
      })
  );
}

inline void subscribe_cover_toggle_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                         TransientStatusLabel *status_label,
                                         const char *closed_icon, const char *open_icon,
                                         const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool active = cover_toggle_state_is_active(state_text);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state_text) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state_text), garage_state_releases_label(state_text));
      })
  );
}

inline void subscribe_lock_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                 TransientStatusLabel *status_label,
                                 const char *locked_icon, const char *unlocked_icon,
                                 LockCardCtx *ctx) {
  if (!ctx) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, status_label, locked_icon, unlocked_icon, ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->state = state_text;
        bool active = lock_state_is_active(state_text);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl,
          lock_state_uses_unlocked_icon(state_text) ? unlocked_icon : locked_icon);
        transient_status_label_show_if_changed(
          status_label, lock_state_label(state_text), lock_state_releases_label(state_text));
      })
  );
}

// Subscribe to an entity's friendly_name attribute and use it as the button label
inline void subscribe_friendly_name(TransientStatusLabel *status_label,
                                    const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([status_label](esphome::StringRef name) {
      transient_status_label_set_steady(status_label, string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN));
    })
  );
}

inline void subscribe_friendly_name(ToggleTextSensorCtx *ctx,
                                    const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      if (!ctx) return;
      ctx->steady_text = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      if (!ctx->on) apply_toggle_text_sensor_label(ctx);
    })
  );
}

inline void subscribe_friendly_name(lv_obj_t *text_lbl, const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([text_lbl](esphome::StringRef name) {
      lv_label_set_text_limited(text_lbl, name, HA_FRIENDLY_NAME_MAX_LEN);
    })
  );
}

// Subscribe to a toggle entity's state; updates checked visual, icon swap, sensor overlay
inline void subscribe_toggle_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                   lv_obj_t *sensor_ctr,
                                   bool *slot_has_sensor, bool *slot_sensor_text_mode,
                                   bool *slot_has_icon_on,
                                   const char **slot_icon_off, const char **slot_icon_on,
                                   ToggleTextSensorCtx *text_sensor_ctx,
                                   const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, icon_lbl, sensor_ctr, slot_has_sensor, slot_sensor_text_mode,
       slot_has_icon_on, slot_icon_off, slot_icon_on, text_sensor_ctx](esphome::StringRef state) {
        bool on = is_entity_on_ref(state);
        if (on) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);

        if (text_sensor_ctx) {
          text_sensor_ctx->on = on;
          apply_toggle_text_sensor_label(text_sensor_ctx);
        }

        bool show_numeric_sensor = *slot_has_sensor && !*slot_sensor_text_mode;
        if (show_numeric_sensor && sensor_ctr) {
          if (on) {
            lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          }
        } else {
          if (icon_lbl) lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
          if (sensor_ctr) lv_obj_add_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          if (*slot_has_icon_on)
            lv_label_set_text(icon_lbl, on ? *slot_icon_on : *slot_icon_off);
        }
      })
  );
}

// ── Home Assistant actions ────────────────────────────────────────────

inline bool is_button_entity(const std::string &entity_id) {
  return entity_id.size() > 7 && entity_id.compare(0, 7, "button.") == 0;
}

// Press HA button entities; toggle other bound entities.
inline void send_toggle_action(const std::string &entity_id) {
  esphome::api::HomeassistantActionRequest req;
  req.service = is_button_entity(entity_id)
    ? decltype(req.service)("button.press")
    : decltype(req.service)("homeassistant.toggle");
  req.is_event = false;
  req.data.init(1);
  auto &kv = req.data.emplace_back();
  kv.key = decltype(kv.key)("entity_id");
  kv.value = decltype(kv.value)(entity_id.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline bool action_card_requires_value(const std::string &action) {
  return action == "input_number.set_value" || action == "input_select.select_option";
}

inline const char *action_card_value_key(const std::string &action) {
  if (action == "input_number.set_value") return "value";
  if (action == "input_select.select_option") return "option";
  return nullptr;
}

inline bool action_card_action_allowed(const std::string &action) {
  return action == "scene.turn_on" ||
         action == "script.turn_on" ||
         action == "automation.trigger" ||
         action == "button.press" ||
         action == "input_button.press" ||
         action == "lock.open" ||
         action == "input_boolean.toggle" ||
         action == "input_boolean.turn_on" ||
         action == "input_boolean.turn_off" ||
         action_card_requires_value(action);
}

inline void send_action_card_action(const ParsedCfg &p) {
  if (p.entity.empty() || p.sensor.empty() || !action_card_action_allowed(p.sensor)) return;
  const char *value_key = action_card_value_key(p.sensor);
  if (value_key && p.unit.empty()) return;

  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(p.sensor.c_str());
  req.is_event = false;
  req.data.init(value_key ? 2 : 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(p.entity.c_str());
  if (value_key) {
    auto &value_kv = req.data.emplace_back();
    value_kv.key = decltype(value_kv.key)(value_key);
    value_kv.value = decltype(value_kv.value)(p.unit.c_str());
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void send_lock_action(const std::string &entity_id, const std::string &state) {
  if (entity_id.empty()) return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(state == "locked" ? "lock.unlock" : "lock.lock");
  req.is_event = false;
  req.data.init(1);
  auto &kv = req.data.emplace_back();
  kv.key = decltype(kv.key)("entity_id");
  kv.value = decltype(kv.value)(entity_id.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void send_lock_action(LockCardCtx *ctx) {
  if (!ctx) return;
  send_lock_action(ctx->entity_id, ctx->state);
}

// ── Slider card helpers ────────────────────────────────────────────────

inline bool is_cover_entity(const std::string &entity_id) {
  return entity_id.size() > 6 && entity_id.compare(0, 6, "cover.") == 0;
}

inline bool is_fan_entity(const std::string &entity_id) {
  return entity_id.size() > 4 && entity_id.compare(0, 4, "fan.") == 0;
}

inline bool cover_toggle_mode(const std::string &sensor) {
  return sensor == "toggle";
}

inline bool cover_tilt_mode(const std::string &sensor) {
  return sensor == "tilt";
}

inline bool cover_command_mode(const std::string &sensor) {
  return sensor == "open" || sensor == "close" ||
         sensor == "stop" || sensor == "set_position";
}

inline const char *cover_command_service(const std::string &sensor) {
  if (sensor == "open") return "cover.open_cover";
  if (sensor == "close") return "cover.close_cover";
  if (sensor == "stop") return "cover.stop_cover";
  if (sensor == "set_position") return "cover.set_cover_position";
  return nullptr;
}

inline int cover_position_value(const std::string &value) {
  char *end = nullptr;
  long pos = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) pos = 50;
  if (pos < 0) pos = 0;
  if (pos > 100) pos = 100;
  return static_cast<int>(pos);
}

inline uint32_t next_cover_stop_call_id() {
  static uint32_t call_id = 200000;
  return call_id++;
}

inline void send_cover_command_action(const ParsedCfg &p) {
  const char *service = cover_command_service(p.sensor);
  if (p.entity.empty() || service == nullptr || esphome::api::global_api_server == nullptr) return;

  bool has_position = p.sensor == "set_position";
  bool wants_stop_response = p.sensor == "stop";
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  if (wants_stop_response) {
    req.call_id = next_cover_stop_call_id();
    req.wants_response = true;
  }
  req.data.init(has_position ? 2 : 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(p.entity.c_str());
  if (has_position) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", cover_position_value(p.unit));
    auto &position_kv = req.data.emplace_back();
    position_kv.key = decltype(position_kv.key)("position");
    position_kv.value = decltype(position_kv.value)(buf);
  }
  if (wants_stop_response) {
    std::string entity_id = p.entity;
    esphome::api::global_api_server->register_action_response_callback(
      req.call_id,
      [entity_id](const esphome::api::ActionResponse &response) {
        if (response.is_success()) return;
        ESP_LOGW("cover", "cover.stop_cover failed for %s: %s; falling back to cover toggle",
                 entity_id.c_str(), response.get_error_message().c_str());
        send_toggle_action(entity_id);
      });
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

// Send HA action for a slider change: toggle (value<0), brightness, or cover position/tilt
inline void send_slider_action(const std::string &entity_id, int value, bool cover_tilt = false) {
  esphome::api::HomeassistantActionRequest req;
  req.is_event = false;
  if (value < 0) {
    req.service = decltype(req.service)("homeassistant.toggle");
    req.data.init(1);
    auto &kv = req.data.emplace_back();
    kv.key = decltype(kv.key)("entity_id");
    kv.value = decltype(kv.value)(entity_id.c_str());
  } else if (is_cover_entity(entity_id)) {
    req.service = decltype(req.service)(
      cover_tilt ? "cover.set_cover_tilt_position" : "cover.set_cover_position");
    req.data.init(2);
    auto &kv1 = req.data.emplace_back();
    kv1.key = decltype(kv1.key)("entity_id");
    kv1.value = decltype(kv1.value)(entity_id.c_str());
    auto &kv2 = req.data.emplace_back();
    kv2.key = decltype(kv2.key)(cover_tilt ? "tilt_position" : "position");
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    kv2.value = decltype(kv2.value)(buf);
  } else if (is_fan_entity(entity_id)) {
    if (value == 0) {
      req.service = decltype(req.service)("fan.turn_off");
      req.data.init(1);
      auto &kv = req.data.emplace_back();
      kv.key = decltype(kv.key)("entity_id");
      kv.value = decltype(kv.value)(entity_id.c_str());
    } else {
      req.service = decltype(req.service)("fan.turn_on");
      req.data.init(2);
      auto &kv1 = req.data.emplace_back();
      kv1.key = decltype(kv1.key)("entity_id");
      kv1.value = decltype(kv1.value)(entity_id.c_str());
      auto &kv2 = req.data.emplace_back();
      kv2.key = decltype(kv2.key)("percentage");
      char buf[8];
      snprintf(buf, sizeof(buf), "%d", value);
      kv2.value = decltype(kv2.value)(buf);
    }
  } else if (value == 0) {
    req.service = decltype(req.service)("light.turn_off");
    req.data.init(1);
    auto &kv = req.data.emplace_back();
    kv.key = decltype(kv.key)("entity_id");
    kv.value = decltype(kv.value)(entity_id.c_str());
  } else {
    req.service = decltype(req.service)("light.turn_on");
    req.data.init(2);
    auto &kv1 = req.data.emplace_back();
    kv1.key = decltype(kv1.key)("entity_id");
    kv1.value = decltype(kv1.value)(entity_id.c_str());
    auto &kv2 = req.data.emplace_back();
    kv2.key = decltype(kv2.key)("brightness_pct");
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    kv2.value = decltype(kv2.value)(buf);
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

// Parse "min-max" kelvin range from the unit config field (e.g. "2000-6500").
inline void parse_kelvin_range(const std::string &unit, int &min_k, int &max_k) {
  min_k = 2000; max_k = 6500;
  if (unit.empty()) return;
  auto dash = unit.find('-');
  if (dash == std::string::npos || dash == 0) return;
  int a = atoi(unit.substr(0, dash).c_str());
  int b = atoi(unit.substr(dash + 1).c_str());
  if (a >= 1000 && b > a) { min_k = a; max_k = b; }
}

// Map a kelvin value to an lv_color_t by lerping between warm amber (low K) and
// cool blue-white (high K). Used when "Show light colour on card" is enabled.
//
// The interpolation is anchored to an absolute reference range
// (KELVIN_REF_MIN..KELVIN_REF_MAX) rather than the user's configured slider
// range. That way a narrow range (e.g. 5000-6000K) shows the actual subtle
// shift between two cool-white shades — instead of stretching the full
// amber-to-blue gradient across two values that should both look cool.
inline lv_color_t kelvin_to_fill_color(int k, int /*min_k*/, int /*max_k*/) {
  constexpr int KELVIN_REF_MIN = 2000;
  constexpr int KELVIN_REF_MAX = 6500;
  float t = (float)(k - KELVIN_REF_MIN) /
            (float)(KELVIN_REF_MAX - KELVIN_REF_MIN);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  // warm = 0xFF8012, cool = 0xB8CCFF
  uint8_t r = (uint8_t)(0xFF + t * (float)(0xB8 - 0xFF) + 0.5f);
  uint8_t g = (uint8_t)(0x80 + t * (float)(0xCC - 0x80) + 0.5f);
  uint8_t b = (uint8_t)(0x12 + t * (float)(0xFF - 0x12) + 0.5f);
  return lv_color_make(r, g, b);
}

// Send light.turn_on with color_temp_kelvin mapped from 0-100 pct over [min_k, max_k].
inline void send_light_temp_action(const std::string &entity_id, int pct, int min_k, int max_k) {
  if (entity_id.empty()) return;
  esphome::api::HomeassistantActionRequest req;
  req.is_event = false;
  req.service = decltype(req.service)("light.turn_on");
  req.data.init(2);
  auto &kv1 = req.data.emplace_back();
  kv1.key = decltype(kv1.key)("entity_id");
  kv1.value = decltype(kv1.value)(entity_id.c_str());
  auto &kv2 = req.data.emplace_back();
  kv2.key = decltype(kv2.key)("color_temp_kelvin");
  int kelvin = min_k + (pct * (max_k - min_k)) / 100;
  if (kelvin < min_k) kelvin = min_k;
  if (kelvin > max_k) kelvin = max_k;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", kelvin);
  kv2.value = decltype(kv2.value)(buf);
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline const char *light_temp_icon(const std::string &icon) {
  return (!icon.empty() && icon != "Auto") ? find_icon(icon.c_str()) : find_icon("Lightbulb");
}

inline std::string media_card_mode(const std::string &sensor) {
  if (sensor == "volume" || sensor == "position" ||
      sensor == "now_playing" || sensor == "play_pause" ||
      sensor == "previous" || sensor == "next")
    return sensor;
  if (sensor == "controls") return "play_pause";
  return "play_pause";
}

inline bool media_playback_button_mode(const std::string &mode) {
  return mode == "play_pause" || mode == "previous" || mode == "next";
}

inline const char *media_service_for_mode(const std::string &mode) {
  if (mode == "previous") return "media_player.media_previous_track";
  if (mode == "next") return "media_player.media_next_track";
  return "media_player.media_play_pause";
}

inline void send_media_player_action(const std::string &entity_id,
                                     const char *service,
                                     const char *value_key = nullptr,
                                     const char *value = nullptr) {
  if (entity_id.empty() || service == nullptr || service[0] == '\0') return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  req.data.init(value_key && value ? 2 : 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(entity_id.c_str());
  if (value_key && value) {
    auto &value_kv = req.data.emplace_back();
    value_kv.key = decltype(value_kv.key)(value_key);
    value_kv.value = decltype(value_kv.value)(value);
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void send_media_volume_action(const std::string &entity_id, int value) {
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  char buf[12];
  snprintf(buf, sizeof(buf), "%d.%02d", value / 100, value % 100);
  send_media_player_action(entity_id, "media_player.volume_set", "volume_level", buf);
}

inline void send_media_seek_action(const std::string &entity_id, int value, float duration) {
  if (duration <= 0.0f) return;
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  int seconds = (int)((duration * value / 100.0f) + 0.5f);
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", seconds);
  send_media_player_action(entity_id, "media_player.media_seek", "seek_position", buf);
}

inline void send_media_playback_action(const std::string &entity_id,
                                       const std::string &mode) {
  if (entity_id.empty()) return;
  send_media_player_action(entity_id, media_service_for_mode(mode));
}

// ── Button click dispatch ─────────────────────────────────────────────

inline bool experimental_card_enabled(const ParsedCfg &p, bool developer_experimental_features);
struct MediaVolumeCtx;
inline void media_volume_open_modal(MediaVolumeCtx *ctx);

// Handle a main-grid button press: dispatch push event, subpage nav,
// slider toggle, or entity toggle based on the config string.
inline void handle_button_click(const std::string &cfg, int slot_num,
                                lv_obj_t *btn_obj,
                                bool developer_experimental_features = false) {
  ParsedCfg p = parse_cfg(cfg);
  if (!experimental_card_enabled(p, developer_experimental_features)) return;
  if (p.type == "sensor" || p.type == "text_sensor" ||
      p.type == "calendar" || p.type == "timezone" ||
      p.type == "weather_forecast") return;
  if (p.type == "push") {
    std::string label = p.label;
    if (label.empty()) {
      char buf[16];
      snprintf(buf, sizeof(buf), "Push %d", slot_num);
      label = buf;
    }
    esphome::api::HomeassistantActionRequest req;
    req.service = decltype(req.service)("esphome.push_button_pressed");
    req.is_event = true;
    req.data.init(2);
    auto &kv1 = req.data.emplace_back();
    kv1.key = decltype(kv1.key)("label");
    kv1.value = decltype(kv1.value)(label.c_str());
    auto &kv2 = req.data.emplace_back();
    kv2.key = decltype(kv2.key)("slot");
    char slot_buf[8];
    snprintf(slot_buf, sizeof(slot_buf), "%d", slot_num);
    kv2.value = decltype(kv2.value)(slot_buf);
    esphome::api::global_api_server->send_homeassistant_action(req);
  } else if (p.type == "subpage") {
    lv_obj_t *sub_scr = (lv_obj_t *)lv_obj_get_user_data(btn_obj);
    if (sub_scr)
      lv_scr_load_anim(sub_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  } else if (p.type == "climate") {
    ClimateCardCtx *ctx = (ClimateCardCtx *)lv_obj_get_user_data(btn_obj);
    if (ctx) climate_open_detail(ctx, lv_scr_act());
  } else if (p.type == "garage") {
    if (!p.entity.empty()) {
      lv_obj_add_state(btn_obj, LV_STATE_CHECKED);
      send_toggle_action(p.entity);
    }
  } else if (p.type == "lock") {
    LockCardCtx *ctx = (LockCardCtx *)lv_obj_get_user_data(btn_obj);
    if (ctx) send_lock_action(ctx);
    else send_lock_action(p.entity, "");
  } else if (p.type == "cover" && cover_command_mode(p.sensor)) {
    send_cover_command_action(p);
  } else if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
    if (!p.entity.empty()) {
      lv_obj_add_state(btn_obj, LV_STATE_CHECKED);
      send_toggle_action(p.entity);
    }
  } else if (p.type == "internal") {
    if (!p.entity.empty()) send_internal_relay_action(p);
  } else if (p.type == "action") {
    send_action_card_action(p);
  } else if (p.type == "media") {
    std::string mode = media_card_mode(p.sensor);
    if (mode == "volume") {
      MediaVolumeCtx *ctx = (MediaVolumeCtx *)lv_obj_get_user_data(btn_obj);
      if (ctx) media_volume_open_modal(ctx);
    } else if (media_playback_button_mode(mode)) {
      send_media_playback_action(p.entity, mode);
    }
  } else if (p.type == "light_temperature") {
    // Tap does nothing; only dragging the slider sends commands.
  } else if (p.type == "slider" || p.type == "cover") {
    if (!p.entity.empty()) send_slider_action(p.entity, -1, cover_tilt_mode(p.sensor));
  } else {
    if (!p.entity.empty()) send_toggle_action(p.entity);
  }
}

// ── Slider widgets ───────────────────────────────────────────────────

// Context attached to each LVGL slider via user_data
struct SliderCtx {
  std::string entity_id;
  lv_obj_t *fill;
  bool horizontal;
  bool cover_tilt;
  bool inverted;
  lv_coord_t radius;
  bool media_position = false;
  float media_duration = 0.0f;
  float media_position_seconds = 0.0f;
  uint32_t media_position_updated_ms = 0;
  bool media_seek_pending = false;
  float media_seek_target_seconds = 0.0f;
  uint32_t media_seek_pending_ms = 0;
  bool media_playing = false;
  lv_obj_t *media_slider = nullptr;
  lv_obj_t *media_track_bg = nullptr;
  lv_obj_t *media_value_lbl = nullptr;
  lv_obj_t *media_status_lbl = nullptr;
  lv_timer_t *media_timer = nullptr;
  // light_temperature fields
  bool light_temp = false;
  int kelvin_min = 2000;
  int kelvin_max = 6500;
  bool kelvin_color = false;
  bool light_on = false;
  bool light_state_known = false;
  bool light_temp_has_kelvin = false;
  int light_temp_last_kelvin = 2000;
  bool show_kelvin = false;
  lv_obj_t *text_lbl = nullptr;
  std::string cached_label;
};

constexpr uint32_t MEDIA_SEEK_PENDING_TIMEOUT_MS = 3000;
constexpr float MEDIA_SEEK_MATCH_TOLERANCE_SECONDS = 2.0f;

struct MediaVolumeCtx {
  std::string entity_id;
  std::string label;
  int current_pct = 0;
  int pending_pct = -1;
  uint32_t pending_until_ms = 0;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  lv_obj_t *btn = nullptr;
  lv_obj_t *label_lbl = nullptr;
  const lv_font_t *value_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
};

struct MediaVolumeModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *arc = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *pct_lbl = nullptr;
  lv_obj_t *minus_btn = nullptr;
  lv_obj_t *plus_btn = nullptr;
  MediaVolumeCtx *active = nullptr;
  bool updating_arc = false;
};

inline MediaVolumeModalUi &media_volume_modal_ui() {
  static MediaVolumeModalUi ui;
  return ui;
}

inline void slider_fit_to_button(lv_obj_t *slider, lv_obj_t *btn, bool horizontal) {
  if (!slider || !btn) return;
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  if (bw <= 0 || bh <= 0) return;

  if (horizontal) {
    lv_coord_t h = bh >= bw ? bw - 1 : bh;
    if (h < 1) h = 1;
    lv_obj_set_size(slider, bw, h);
  } else {
    lv_coord_t w = bw >= bh ? bh - 1 : bw;
    if (w < 1) w = 1;
    lv_obj_set_size(slider, w, bh);
  }
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
}

// Resize the colored fill overlay to reflect the current slider percentage
inline void slider_update_fill(lv_obj_t *fill, lv_obj_t *btn, int pct, bool horizontal, bool inverted, lv_coord_t r) {
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  lv_obj_set_style_radius(fill, r, LV_PART_MAIN);
  if (horizontal) {
    lv_coord_t w = (lv_coord_t)((int32_t)bw * pct / 100);
    lv_obj_set_size(fill, w, bh);
    lv_obj_align(fill, inverted ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);
  } else {
    lv_coord_t h = (lv_coord_t)((int32_t)bh * pct / 100);
    lv_obj_set_size(fill, bw, h);
    lv_obj_align(fill, inverted ? LV_ALIGN_TOP_MID : LV_ALIGN_BOTTOM_MID, 0, 0);
  }
}

inline void slider_horizontal_track_geometry(lv_obj_t *btn, lv_coord_t &x,
                                             lv_coord_t &y, lv_coord_t &w,
                                             lv_coord_t &h) {
  x = y = 0;
  w = h = 0;
  if (!btn) return;
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  if (bw <= 0 || bh <= 0) return;

  w = (lv_coord_t)((int32_t)bw * 84 / 100);
  h = (lv_coord_t)(((int32_t)bh * 15 + 100) / 200);
  if (w < 1) w = 1;
  if (h < 4) h = 4;
  if (h > 12) h = 12;
  x = (bw - w) / 2;
  lv_coord_t bottom_pad = (lv_coord_t)((int32_t)bh * 10 / 100);
  if (bottom_pad < 8) bottom_pad = 8;
  y = -bottom_pad;
}

inline void slider_update_horizontal_track_bg(lv_obj_t *track, lv_obj_t *btn) {
  if (!track || !btn) return;
  lv_coord_t x, y, w, h;
  slider_horizontal_track_geometry(btn, x, y, w, h);
  if (w <= 0 || h <= 0) return;
  lv_obj_set_style_radius(track, h / 2, LV_PART_MAIN);
  lv_obj_set_size(track, w, h);
  lv_obj_align(track, LV_ALIGN_BOTTOM_LEFT, x, y);
}

inline void slider_update_horizontal_track_fill(lv_obj_t *fill, lv_obj_t *btn, int pct) {
  if (!fill || !btn) return;
  lv_coord_t x, y, track_w, track_h;
  slider_horizontal_track_geometry(btn, x, y, track_w, track_h);
  if (track_w <= 0 || track_h <= 0) return;
  lv_coord_t fill_w = (lv_coord_t)((int32_t)track_w * pct / 100);
  if (fill_w < 0) fill_w = 0;

  lv_obj_set_style_radius(fill, track_h / 2, LV_PART_MAIN);
  lv_obj_set_size(fill, fill_w, track_h);
  lv_obj_align(fill, LV_ALIGN_BOTTOM_LEFT, x, y);
}

inline void slider_update_ctx_fill(SliderCtx *c, lv_obj_t *btn, int pct) {
  if (!c || !c->fill || !btn) return;
  if (c->media_position) {
    if (c->media_track_bg) slider_update_horizontal_track_bg(c->media_track_bg, btn);
    slider_update_horizontal_track_fill(c->fill, btn, pct);
  } else {
    slider_update_fill(c->fill, btn, pct, c->horizontal, c->inverted, c->radius);
  }
}

inline void slider_refresh_geometry(lv_obj_t *slider) {
  if (!slider) return;
  SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *btn = lv_obj_get_parent(slider);
  if (!c || !btn) return;

  slider_fit_to_button(slider, btn, c->horizontal);
  int val = lv_slider_get_value(slider);
  int fill_val = c->inverted ? 100 - val : val;
  slider_update_ctx_fill(c, btn, fill_val);
}

inline void slider_bind_geometry_refresh(lv_obj_t *btn, lv_obj_t *slider) {
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_user_data(e);
    slider_refresh_geometry(sl);
  }, LV_EVENT_SIZE_CHANGED, slider);
  slider_refresh_geometry(slider);
}

// Create an invisible LVGL slider with a colored fill overlay inside a button
inline lv_obj_t *setup_slider_widget(lv_obj_t *btn, uint32_t on_color, bool horizontal) {
  lv_obj_set_style_pad_all(btn, 0,
    static_cast<lv_style_selector_t>(LV_PART_MAIN));
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *fill = lv_obj_create(btn);
  lv_obj_set_size(fill, 0, 0);
  lv_obj_set_style_bg_color(fill, lv_color_hex(on_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(fill, 0, LV_PART_MAIN);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *slider = lv_slider_create(btn);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 0, LV_ANIM_OFF);
  lv_obj_update_layout(btn);
  slider_fit_to_button(slider, btn, horizontal);

  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(LV_PART_INDICATOR));
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_border_width(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_shadow_width(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_pad_all(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_width(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_height(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));

  lv_obj_move_to_index(fill, 0);
  lv_obj_move_to_index(slider, 1);

  return slider;
}

inline bool slider_has_alt_icon(const std::string &type, const std::string &icon_on) {
  return type == "slider" || type == "cover" || (!icon_on.empty() && icon_on != "Auto");
}

inline const char *slider_icon_off(const std::string &type, const std::string &entity_id,
                                   const std::string &icon) {
  if (type == "cover" && (icon.empty() || icon == "Auto"))
    return find_icon("Blinds");
  if (icon.empty() || icon == "Auto")
    return domain_default_icon(entity_id.substr(0, entity_id.find('.')));
  return find_icon(icon.c_str());
}

inline const char *slider_icon_on(const std::string &type, const std::string &entity_id,
                                  const std::string &icon, const std::string &icon_on) {
  if (type == "cover" && (icon_on.empty() || icon_on == "Auto"))
    return find_icon("Blinds Open");
  if (type == "slider" && (icon_on.empty() || icon_on == "Auto"))
    return slider_icon_off(type, entity_id, icon);
  return find_icon(icon_on.c_str());
}

inline void setup_cover_toggle_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Cover" : p.label.c_str());
}

inline void setup_cover_command_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Cover" : p.label.c_str());
  apply_push_button_transition(s.btn);
}

// Full slider button setup: visual + event handlers + HA action on release
inline void setup_slider_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color) {
  setup_toggle_visual(s, p);
  if (p.type == "cover")
    lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));

  bool horizontal = false;
  lv_obj_t *slider = setup_slider_widget(s.btn, on_color, horizontal);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);

  lv_obj_t *fill = lv_obj_get_child(s.btn, 0);
  // Intentionally leaked -- lives for the lifetime of the display
  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = p.entity;
  ctx->fill = fill;
  ctx->horizontal = horizontal;
  ctx->cover_tilt = p.type == "cover" && cover_tilt_mode(p.sensor);
  ctx->inverted = is_cover_entity(p.entity);
  ctx->radius = lv_obj_get_style_radius(s.btn, LV_PART_MAIN);
  lv_obj_set_user_data(slider, (void *)ctx);
  slider_bind_geometry_refresh(s.btn, slider);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    int fill_val = c->inverted ? 100 - val : val;
    slider_update_ctx_fill(c, lv_obj_get_parent(sl), fill_val);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (c && !c->entity_id.empty()) {
      int val = lv_slider_get_value(sl);
      send_slider_action(c->entity_id, val, c->cover_tilt);
    }
  }, LV_EVENT_RELEASED, nullptr);
}

// Subscribe to HA state for a slider entity (light brightness, fan percentage, or cover position/tilt)
inline void subscribe_slider_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                  lv_obj_t *slider,
                                  bool has_icon_on,
                                  const char *icon_off, const char *icon_on,
                                  const std::string &entity_id,
                                  bool cover_tilt = false) {
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *fill = sctx ? sctx->fill : nullptr;
  bool horiz = sctx ? sctx->horizontal : false;
  bool inv = sctx ? sctx->inverted : false;
  lv_coord_t rad = sctx ? sctx->radius : 0;
  bool is_cover = is_cover_entity(entity_id);
  bool is_fan = is_fan_entity(entity_id);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on](esphome::StringRef state) {
        bool on = is_entity_on_ref(state);
        if (!on) {
          lv_slider_set_value(slider, 0, LV_ANIM_OFF);
          if (fill) slider_update_fill(fill, btn_ptr, inv ? 100 : 0, horiz, inv, rad);
        }
        if (has_icon_on)
          lv_label_set_text(icon_lbl, on ? icon_on : icon_off);
      })
  );
  if (is_cover) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string(cover_tilt ? "current_tilt_position" : "current_position"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on](esphome::StringRef val) {
          float pos = 0.0f;
          if (parse_float_ref(val, pos)) {
            int pct = (int)(pos + 0.5f);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
            if (has_icon_on) {
              lv_label_set_text(icon_lbl, pct > 0 ? icon_on : icon_off);
            }
          }
        })
    );
  } else if (is_fan) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("percentage"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad](esphome::StringRef val) {
          float pct_f = 0.0f;
          if (parse_float_ref(val, pct_f)) {
            int pct = (int)(pct_f + 0.5f);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
          }
        })
    );
  } else {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("brightness"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad](esphome::StringRef val) {
          float bri = 0.0f;
          if (parse_float_ref(val, bri)) {
            int pct = (int)((bri * 100.0f + 127.0f) / 255.0f);
            if (pct < 1) pct = 1;
            if (pct > 100) pct = 100;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
          }
        })
    );
  }
}

// ── Light temperature card helpers ───────────────────────────────────

// Apply the resolved label for a light_temperature card. When show_kelvin is
// enabled and the light is on, displays "<K>K"; otherwise restores the cached
// configured label (user-set label or last known friendly_name).
//
// Bulbs store color temperature as integer mireds, so a 5500K command echoes
// back from HA as ~5494K. Rounding the displayed value to 50K makes the drag
// preview and post-release echo render identically.
inline void light_temp_apply_label(SliderCtx *ctx, int kelvin) {
  if (!ctx || !ctx->text_lbl) return;
  if (ctx->show_kelvin && ctx->light_on) {
    int rounded = ((kelvin + 25) / 50) * 50;
    if (rounded < ctx->kelvin_min) rounded = ctx->kelvin_min;
    if (rounded > ctx->kelvin_max) rounded = ctx->kelvin_max;
    char buf[16];
    snprintf(buf, sizeof(buf), "%dK", rounded);
    lv_label_set_text(ctx->text_lbl, buf);
  } else {
    lv_label_set_text(ctx->text_lbl, ctx->cached_label.c_str());
  }
}

// Subscribe to friendly_name and keep the SliderCtx cached_label in sync;
// only writes to the visible label when not currently displaying kelvin.
inline void subscribe_friendly_name_for_light_temp(lv_obj_t *text_lbl,
                                                    SliderCtx *ctx,
                                                    const std::string &entity_id) {
  if (entity_id.empty() || !text_lbl) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [text_lbl, ctx](esphome::StringRef name) {
        if (ctx) ctx->cached_label = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
        if (!ctx || !ctx->show_kelvin || !ctx->light_on) {
          lv_label_set_text_limited(text_lbl, name, HA_FRIENDLY_NAME_MAX_LEN);
        }
      })
  );
}

inline void light_temp_apply_kelvin_state(SliderCtx *ctx, lv_obj_t *btn_ptr,
                                          lv_obj_t *slider, int kelvin,
                                          bool kelvin_color) {
  if (!ctx || !slider || !btn_ptr) return;
  int k = kelvin;
  if (k < ctx->kelvin_min) k = ctx->kelvin_min;
  if (k > ctx->kelvin_max) k = ctx->kelvin_max;
  int range = ctx->kelvin_max - ctx->kelvin_min;
  int pct = range > 0 ? (k - ctx->kelvin_min) * 100 / range : 50;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  lv_slider_set_value(slider, pct, LV_ANIM_OFF);
  if (ctx->fill)
    slider_update_fill(ctx->fill, btn_ptr, pct, false, false, ctx->radius);
  if (kelvin_color && ctx->fill)
    lv_obj_set_style_bg_color(ctx->fill,
      kelvin_to_fill_color(k, ctx->kelvin_min, ctx->kelvin_max), LV_PART_MAIN);
  light_temp_apply_label(ctx, k);
}

// Subscribe to on/off state plus color_temp_kelvin for a light temperature slider.
// When the light is off, the slider renders empty (value 0, no fill).
inline void subscribe_light_temp_state(lv_obj_t *btn_ptr, lv_obj_t *slider,
                                        const std::string &entity_id,
                                        int /*min_k*/, int /*max_k*/, bool kelvin_color) {
  if (!slider || entity_id.empty()) return;
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  // Track on/off so kelvin updates can be ignored once the light is known off
  // while still handling the initial case where HA sends color_temp before state.
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, kelvin_color, sctx](esphome::StringRef state) {
        bool on = is_entity_on_ref(state);
        bool was_on = sctx ? sctx->light_on : false;
        if (sctx) {
          sctx->light_state_known = true;
          sctx->light_on = on;
        }
        bool applied_cached = false;
        if (!on) {
          lv_slider_set_value(slider, 0, LV_ANIM_OFF);
          if (sctx && sctx->fill)
            slider_update_fill(sctx->fill, btn_ptr, 0, false, false, sctx->radius);
        } else if (sctx && sctx->light_temp_has_kelvin) {
          light_temp_apply_kelvin_state(
            sctx, btn_ptr, slider, sctx->light_temp_last_kelvin, kelvin_color);
          applied_cached = true;
        }
        // Refresh label on any on↔off transition so kelvin/cached_label swaps.
        if (sctx && sctx->show_kelvin && was_on != on && !applied_cached) {
          int cur_k = sctx->kelvin_min + lv_slider_get_value(slider) *
                      (sctx->kelvin_max - sctx->kelvin_min) / 100;
          light_temp_apply_label(sctx, cur_k);
        }
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("color_temp_kelvin"),
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, kelvin_color, sctx](esphome::StringRef val) {
        float k_f = 0.0f;
        if (!parse_float_ref(val, k_f)) return;
        int k = (int)(k_f + 0.5f);
        if (!sctx) return;
        sctx->light_temp_last_kelvin = k;
        sctx->light_temp_has_kelvin = true;
        if (!sctx->light_state_known || !sctx->light_on) return;
        // HA can report values outside the configured display range. Clamp in
        // the renderer so slider and label agree.
        light_temp_apply_kelvin_state(sctx, btn_ptr, slider, k, kelvin_color);
      })
  );
}

// Build the visual for a light temperature slider card.
inline void setup_light_temp_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color) {
  setup_toggle_visual(s, p);
  lv_label_set_text(s.icon_lbl, light_temp_icon(p.icon));
  int min_k = 2000, max_k = 6500;
  parse_kelvin_range(p.unit, min_k, max_k);
  bool kcolor = (p.precision == "color");

  lv_obj_t *slider = setup_slider_widget(s.btn, on_color, false);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);

  lv_obj_t *fill = lv_obj_get_child(s.btn, 0);
  // Intentionally leaked -- lives for the lifetime of the display
  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = p.entity;
  ctx->fill = fill;
  ctx->horizontal = false;
  ctx->cover_tilt = false;
  ctx->inverted = false;
  ctx->radius = lv_obj_get_style_radius(s.btn, LV_PART_MAIN);
  ctx->light_temp = true;
  ctx->kelvin_min = min_k;
  ctx->kelvin_max = max_k;
  ctx->kelvin_color = kcolor;
  ctx->light_on = false;
  ctx->show_kelvin = (p.sensor == "kelvin");
  ctx->text_lbl = s.text_lbl;
  ctx->cached_label = p.label;  // may be empty; friendly_name sub fills it later
  lv_obj_set_user_data(slider, (void *)ctx);
  slider_bind_geometry_refresh(s.btn, slider);

  if (kcolor && fill) {
    int mid_k = min_k + (max_k - min_k) / 2;
    lv_obj_set_style_bg_color(fill, kelvin_to_fill_color(mid_k, min_k, max_k), LV_PART_MAIN);
  }

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    slider_update_fill(c->fill, lv_obj_get_parent(sl), val, false, false, c->radius);
    int k = c->kelvin_min + val * (c->kelvin_max - c->kelvin_min) / 100;
    if (c->kelvin_color && c->fill) {
      lv_obj_set_style_bg_color(c->fill, kelvin_to_fill_color(k, c->kelvin_min, c->kelvin_max), LV_PART_MAIN);
    }
    // Treat dragging as the light coming on so following HA attribute updates
    // are not discarded before the on/off state echo arrives.
    c->light_on = true;
    c->light_state_known = true;
    c->light_temp_has_kelvin = true;
    c->light_temp_last_kelvin = k;
    if (c->show_kelvin) {
      light_temp_apply_label(c, k);
    }
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (c && !c->entity_id.empty())
      send_light_temp_action(c->entity_id, lv_slider_get_value(sl), c->kelvin_min, c->kelvin_max);
  }, LV_EVENT_RELEASED, nullptr);
}

// ── Media player card helpers ─────────────────────────────────────────

inline const char *media_default_icon(const std::string &mode,
                                      const std::string &icon) {
  if (!icon.empty() && icon != "Auto") return find_icon(icon.c_str());
  if (mode == "previous") return find_icon("Skip Previous");
  if (mode == "next") return find_icon("Skip Next");
  if (mode == "play_pause") return find_icon("Play Pause");
  if (mode == "volume") return find_icon("Volume High");
  if (mode == "position") return find_icon("Progress Clock");
  if (mode == "now_playing") return find_icon("Music");
  return find_icon("Play Pause");
}

inline std::string media_default_label(const std::string &mode) {
  if (mode == "previous") return "Previous";
  if (mode == "next") return "Next";
  if (mode == "volume") return "Volume";
  if (mode == "play_pause") return "Play/Pause";
  return "Media";
}

inline std::string media_label(const ParsedCfg &p) {
  return p.label.empty() ? std::string("Volume") : p.label;
}

inline std::string media_action_label(const ParsedCfg &p, const std::string &mode) {
  return p.label.empty() ? media_default_label(mode) : p.label;
}

inline bool media_play_pause_show_state(const ParsedCfg &p) {
  return media_card_mode(p.sensor) == "play_pause" && p.precision == "state";
}

inline void media_format_time(float seconds, char *buf, size_t size) {
  if (!buf || size == 0) return;
  if (seconds < 0.0f || !std::isfinite(seconds)) seconds = 0.0f;
  int total = (int)(seconds + 0.5f);
  int h = total / 3600;
  int m = (total / 60) % 60;
  int s = total % 60;
  if (h > 0) snprintf(buf, size, "%d:%02d:%02d", h, m, s);
  else snprintf(buf, size, "%d:%02d", m, s);
}

inline void media_format_percent(int percent, char *buf, size_t size) {
  if (!buf || size == 0) return;
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  snprintf(buf, size, "%d%%", percent);
}

inline int media_clamp_percent(int value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return value;
}

inline bool media_volume_pending_active(MediaVolumeCtx *ctx) {
  return ctx && ctx->pending_until_ms != 0 &&
         (int32_t)(ctx->pending_until_ms - esphome::millis()) > 0;
}

inline void media_volume_set_modal_value(MediaVolumeCtx *ctx, int pct);

inline void media_volume_apply_percent(MediaVolumeCtx *ctx, int pct,
                                       bool from_user, bool send_action) {
  if (!ctx) return;
  pct = media_clamp_percent(pct);
  ctx->current_pct = pct;
  if (from_user) {
    ctx->pending_pct = pct;
    ctx->pending_until_ms = esphome::millis() + 1500;
  }
  media_volume_set_modal_value(ctx, pct);
  if (send_action) send_media_volume_action(ctx->entity_id, pct);
}

inline void media_volume_hide_modal() {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui.overlay = nullptr;
  ui.panel = nullptr;
  ui.arc = nullptr;
  ui.title_lbl = nullptr;
  ui.pct_lbl = nullptr;
  ui.minus_btn = nullptr;
  ui.plus_btn = nullptr;
  ui.active = nullptr;
  ui.updating_arc = false;
}

inline lv_obj_t *media_volume_create_round_button(lv_obj_t *parent, lv_coord_t size,
                                                  const char *text,
                                                  const lv_font_t *font,
                                                  uint32_t border_color,
                                                  uint32_t bg_color) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, size, size);
  lv_obj_set_style_radius(btn, size / 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(border_color), LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_center(label);
  return btn;
}

inline void media_volume_layout_modal(MediaVolumeCtx *ctx) {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  if (!ctx || !ui.overlay || !ui.panel) return;
  lv_disp_t *disp = lv_disp_get_default();
  lv_coord_t sw = disp ? lv_disp_get_hor_res(disp) : 480;
  lv_coord_t sh = disp ? lv_disp_get_ver_res(disp) : 480;
  lv_coord_t short_side = sw < sh ? sw : sh;
  lv_coord_t panel_w = sw * 78 / 100;
  lv_coord_t panel_h = sh * 78 / 100;
  if (panel_w > 620) panel_w = 620;
  if (panel_h > 620) panel_h = 620;
  if (panel_w < short_side * 78 / 100) panel_w = short_side * 78 / 100;
  if (panel_h < short_side * 78 / 100) panel_h = short_side * 78 / 100;
  lv_coord_t arc_size = panel_w < panel_h ? panel_w : panel_h;
  arc_size = arc_size * 78 / 100;
  if (arc_size < 220) arc_size = short_side * 62 / 100;
  lv_coord_t btn_size = short_side * 15 / 100;
  if (btn_size < 54) btn_size = 54;
  if (btn_size > 108) btn_size = 108;

  lv_obj_set_size(ui.overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_size(ui.panel, panel_w, panel_h);
  lv_obj_align(ui.panel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_size(ui.arc, arc_size, arc_size);
  lv_obj_align(ui.arc, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_arc_width(ui.arc, short_side < 520 ? 18 : 28, LV_PART_MAIN);
  lv_obj_set_style_arc_width(ui.arc, short_side < 520 ? 18 : 28, LV_PART_INDICATOR);
  lv_obj_set_style_pad_all(ui.arc, short_side < 520 ? 8 : 12, LV_PART_KNOB);
  lv_obj_set_size(ui.minus_btn, btn_size, btn_size);
  lv_obj_set_style_radius(ui.minus_btn, btn_size / 2, LV_PART_MAIN);
  lv_obj_set_size(ui.plus_btn, btn_size, btn_size);
  lv_obj_set_style_radius(ui.plus_btn, btn_size / 2, LV_PART_MAIN);
  lv_obj_align(ui.title_lbl, LV_ALIGN_CENTER, 0, -arc_size / 7);
  lv_obj_align(ui.pct_lbl, LV_ALIGN_CENTER, 0, 18);
  lv_obj_align(ui.minus_btn, LV_ALIGN_CENTER, -(btn_size + 18) / 2, arc_size / 2 - btn_size / 3);
  lv_obj_align(ui.plus_btn, LV_ALIGN_CENTER, (btn_size + 18) / 2, arc_size / 2 - btn_size / 3);
}

inline void media_volume_set_modal_value(MediaVolumeCtx *ctx, int pct) {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  if (!ctx || ui.active != ctx) return;
  pct = media_clamp_percent(pct);
  if (ui.arc) {
    ui.updating_arc = true;
    lv_arc_set_value(ui.arc, pct);
    ui.updating_arc = false;
  }
  if (ui.pct_lbl) {
    char buf[12];
    media_format_percent(pct, buf, sizeof(buf));
    lv_label_set_text(ui.pct_lbl, buf);
  }
}

inline void media_volume_open_modal(MediaVolumeCtx *ctx) {
  if (!ctx) return;
  media_volume_hide_modal();
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  ui.active = ctx;

  lv_obj_t *parent = lv_scr_act();
  ui.overlay = lv_obj_create(parent);
  lv_obj_set_size(ui.overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(ui.overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_60, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui.overlay, [](lv_event_t *) { media_volume_hide_modal(); },
    LV_EVENT_CLICKED, nullptr);

  ui.panel = lv_obj_create(ui.overlay);
  lv_obj_set_style_bg_color(ui.panel, lv_color_hex(0x252525), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.panel, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.panel, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(ui.panel, 18, LV_PART_MAIN);
  lv_obj_clear_flag(ui.panel, LV_OBJ_FLAG_SCROLLABLE);

  ui.arc = lv_arc_create(ui.panel);
  lv_arc_set_bg_angles(ui.arc, 135, 45);
  lv_arc_set_range(ui.arc, 0, 100);
  lv_arc_set_value(ui.arc, ctx->current_pct);
  lv_obj_set_style_bg_opa(ui.arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(ctx->accent_color), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui.arc, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_add_flag(ui.arc, LV_OBJ_FLAG_ADV_HITTEST);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.updating_arc || !ui.active) return;
    lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
    media_volume_apply_percent(ui.active, lv_arc_get_value(arc), true, true);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  ui.title_lbl = lv_label_create(ui.panel);
  lv_label_set_text(ui.title_lbl, "Volume");
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(0xA0A0A0), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.title_lbl, ctx->label_font, LV_PART_MAIN);

  ui.pct_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.pct_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.pct_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->value_font) lv_obj_set_style_text_font(ui.pct_lbl, ctx->value_font, LV_PART_MAIN);

  ui.minus_btn = media_volume_create_round_button(ui.panel, 72, find_icon("Minus"),
    ctx->icon_font, 0xBDBDBD, 0x252525);
  ui.plus_btn = media_volume_create_round_button(ui.panel, 72, find_icon("Plus"),
    ctx->icon_font, 0xBDBDBD, 0x252525);
  lv_obj_add_event_cb(ui.minus_btn, [](lv_event_t *) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.active) media_volume_apply_percent(ui.active, ui.active->current_pct - 1, true, true);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.plus_btn, [](lv_event_t *) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.active) media_volume_apply_percent(ui.active, ui.active->current_pct + 1, true, true);
  }, LV_EVENT_CLICKED, nullptr);

  media_volume_layout_modal(ctx);
  media_volume_set_modal_value(ctx, ctx->current_pct);
  lv_obj_move_foreground(ui.overlay);
}

inline std::string media_status_text(const std::string &state) {
  if (state == "playing") return "Playing";
  if (state == "paused") return "Paused";
  if (state == "idle") return "Idle";
  if (state == "off") return "Off";
  if (state == "unavailable") return "Unavailable";
  if (state == "unknown" || state.empty()) return "Unknown";
  return sentence_cap_text(state);
}

inline void media_set_metadata_text(lv_obj_t *label, esphome::StringRef value,
                                    const char *fallback) {
  if (!label) return;
  std::string text = string_ref_limited(value, HA_STATE_TEXT_MAX_LEN);
  if (text.empty() || text == "unknown" || text == "unavailable")
    text = fallback ? fallback : "--";
  lv_label_set_text(label, text.c_str());
}

inline bool media_seek_pending_active(SliderCtx *ctx) {
  return ctx && ctx->media_seek_pending &&
         (esphome::millis() - ctx->media_seek_pending_ms) < MEDIA_SEEK_PENDING_TIMEOUT_MS;
}

inline void media_apply_position(SliderCtx *ctx) {
  if (!ctx || !ctx->media_slider) return;
  float seconds = ctx->media_position_seconds;
  if (ctx->media_playing && ctx->media_position_updated_ms > 0) {
    uint32_t elapsed_ms = esphome::millis() - ctx->media_position_updated_ms;
    seconds += elapsed_ms / 1000.0f;
  }
  if (ctx->media_duration > 0.0f && seconds > ctx->media_duration) {
    seconds = ctx->media_duration;
  }

  if (ctx->media_value_lbl) {
    char time_buf[16];
    media_format_time(seconds, time_buf, sizeof(time_buf));
    lv_label_set_text(ctx->media_value_lbl, time_buf);
  }

  int pct = 0;
  if (ctx->media_duration > 0.0f) {
    pct = (int)((seconds * 100.0f / ctx->media_duration) + 0.5f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
  }
  lv_slider_set_value(ctx->media_slider, pct, LV_ANIM_OFF);
  if (ctx->fill) {
    lv_obj_t *btn = lv_obj_get_parent(ctx->media_slider);
    int fill_pct = ctx->inverted ? 100 - pct : pct;
    slider_update_ctx_fill(ctx, btn, fill_pct);
  }
}

inline void media_set_pending_seek_position(SliderCtx *ctx, int value) {
  if (!ctx || ctx->media_duration <= 0.0f) return;
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  float seconds = ctx->media_duration * value / 100.0f;
  ctx->media_seek_pending = true;
  ctx->media_seek_target_seconds = seconds;
  ctx->media_seek_pending_ms = esphome::millis();
  ctx->media_position_seconds = seconds;
  ctx->media_position_updated_ms = ctx->media_seek_pending_ms;
  media_apply_position(ctx);
}

inline void media_position_timer_cb(lv_timer_t *timer) {
  SliderCtx *ctx = static_cast<SliderCtx *>(lv_timer_get_user_data(timer));
  if (!ctx || !ctx->media_position || !ctx->media_playing) return;
  media_apply_position(ctx);
}

inline void setup_media_action_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                      lv_obj_t *text_lbl,
                                      const ParsedCfg &p) {
  std::string mode = media_card_mode(p.sensor);
  if (icon_lbl) {
    lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(icon_lbl, media_default_icon(mode, p.icon));
    lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
  }
  if (text_lbl) {
    std::string label = media_play_pause_show_state(p)
      ? std::string("Paused")
      : media_action_label(p, mode);
    lv_label_set_text(text_lbl, label.c_str());
    lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    configure_button_label_wrap(text_lbl);
  }
  apply_push_button_transition(btn);
}

inline void setup_media_now_playing_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                           lv_obj_t *title_lbl,
                                           lv_obj_t *artist_lbl,
                                           const lv_font_t *title_font,
                                           lv_coord_t pad) {
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  if (icon_lbl) lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (title_lbl) {
    if (title_font) lv_obj_set_style_text_font(title_lbl, title_font, LV_PART_MAIN);
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(title_lbl, lv_pct(100));
    lv_label_set_text(title_lbl, "--");
    lv_obj_move_foreground(title_lbl);
  }
  if (artist_lbl) {
    lv_label_set_text(artist_lbl, "--");
    lv_obj_align(artist_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
    configure_button_label_wrap(artist_lbl);
    lv_obj_move_foreground(artist_lbl);
  }
}

inline void setup_media_volume_button(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                      lv_obj_t *text_lbl,
                                      const ParsedCfg &p) {
  lv_coord_t pad_left = lv_obj_get_style_pad_left(btn, LV_PART_MAIN);
  lv_coord_t pad_top = lv_obj_get_style_pad_top(btn, LV_PART_MAIN);
  lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(btn, LV_PART_MAIN);
  if (icon_lbl) {
    lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(icon_lbl, media_default_icon("volume", p.icon));
    lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, pad_left, pad_top);
    lv_obj_move_foreground(icon_lbl);
  }
  if (text_lbl) {
    lv_label_set_text(text_lbl, media_label(p).c_str());
    lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, pad_left, -pad_bottom);
    configure_button_label_wrap(text_lbl);
    lv_obj_move_foreground(text_lbl);
  }
  apply_push_button_transition(btn);
}

inline lv_obj_t *setup_media_slider_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                           lv_obj_t *text_lbl, lv_obj_t *value_lbl,
                                           const ParsedCfg &p,
                                           uint32_t on_color,
                                           uint32_t track_color,
                                           lv_coord_t pad) {
  std::string mode = media_card_mode(p.sensor);
  bool position = mode == "position";
  bool horizontal = true;

  if (position) {
    if (icon_lbl) lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    if (value_lbl) {
      lv_label_set_text(value_lbl, "0:00");
      lv_obj_move_foreground(value_lbl);
    }
    if (text_lbl) {
      lv_label_set_text(text_lbl, "");
      lv_obj_add_flag(text_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    if (icon_lbl) {
      lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(icon_lbl, media_default_icon(mode, p.icon));
      lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
      lv_obj_move_foreground(icon_lbl);
    }
    if (text_lbl) {
      lv_label_set_text(text_lbl, media_label(p).c_str());
      lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
      configure_button_label_wrap(text_lbl);
      lv_obj_move_foreground(text_lbl);
    }
  }

  lv_obj_t *slider = setup_slider_widget(btn, on_color, horizontal);
  lv_obj_t *fill = lv_obj_get_child(btn, 0);
  lv_obj_t *track = nullptr;
  if (position) {
    track = lv_obj_create(btn);
    lv_obj_set_style_bg_color(track, lv_color_hex(track_color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(track, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_to_index(track, 0);
    lv_obj_move_to_index(fill, 1);
    lv_obj_move_to_index(slider, 2);
  }

  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = p.entity;
  ctx->fill = fill;
  ctx->horizontal = horizontal;
  ctx->cover_tilt = false;
  ctx->inverted = false;
  ctx->radius = lv_obj_get_style_radius(btn, LV_PART_MAIN);
  ctx->media_position = position;
  ctx->media_slider = slider;
  ctx->media_track_bg = track;
  ctx->media_value_lbl = value_lbl;
  ctx->media_status_lbl = nullptr;
  lv_obj_set_user_data(slider, (void *)ctx);
  slider_bind_geometry_refresh(btn, slider);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!ctx) return;
    int val = lv_slider_get_value(sl);
    int fill_val = ctx->inverted ? 100 - val : val;
    slider_update_ctx_fill(ctx, lv_obj_get_parent(sl), fill_val);
    if (ctx->media_position && ctx->media_duration > 0.0f && ctx->media_value_lbl) {
      char time_buf[16];
      media_format_time(ctx->media_duration * val / 100.0f, time_buf, sizeof(time_buf));
      lv_label_set_text(ctx->media_value_lbl, time_buf);
    }
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!ctx || ctx->entity_id.empty()) return;
    int val = lv_slider_get_value(sl);
    if (ctx->media_position) {
      media_set_pending_seek_position(ctx, val);
      send_media_seek_action(ctx->entity_id, val, ctx->media_duration);
    }
  }, LV_EVENT_RELEASED, nullptr);

  if (position) {
    ctx->media_timer = lv_timer_create(media_position_timer_cb, 1000, ctx);
    if (ctx->media_timer) lv_timer_pause(ctx->media_timer);
  }
  return slider;
}

inline lv_obj_t *setup_media_position_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                             lv_obj_t *text_lbl,
                                             const ParsedCfg &p,
                                             uint32_t on_color,
                                             uint32_t track_color,
                                             const lv_font_t *value_font,
                                             lv_color_t text_color,
                                             lv_coord_t pad) {
  lv_obj_t *value_lbl = lv_label_create(btn);
  if (value_font) lv_obj_set_style_text_font(value_lbl, value_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(value_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(value_lbl, "0:00");
  lv_obj_align(value_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  return setup_media_slider_layout(
    btn, icon_lbl, text_lbl, value_lbl, p, on_color, track_color, pad);
}

inline void setup_media_card(BtnSlot &s, const ParsedCfg &p, uint32_t on_color,
                             uint32_t tertiary_color,
                             const lv_font_t *value_font) {
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  std::string mode = media_card_mode(p.sensor);
  if (media_playback_button_mode(mode)) {
    setup_media_action_layout(s.btn, s.icon_lbl, s.text_lbl, p);
    return;
  }
  if (mode == "volume") {
    setup_media_volume_button(s.btn, s.icon_lbl, s.text_lbl, p);
    return;
  }
  if (mode == "now_playing") {
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(s.sensor_container, lv_pct(100));
    lv_label_set_text(s.unit_lbl, "");
    setup_media_now_playing_layout(
      s.btn, s.icon_lbl, s.sensor_lbl, s.text_lbl, value_font, pad);
    return;
  }
  if (mode == "position") {
    lv_coord_t position_pad = lv_obj_get_style_pad_top(s.btn, LV_PART_MAIN);
    lv_color_t text_color = lv_obj_get_style_text_color(s.sensor_lbl, LV_PART_MAIN);
    lv_obj_t *slider = setup_media_position_layout(
      s.btn, s.icon_lbl, s.text_lbl, p, on_color, tertiary_color,
      value_font, text_color, position_pad);
    lv_obj_set_user_data(s.sensor_container, (void *)slider);
    return;
  }
  lv_obj_t *slider = setup_media_slider_layout(s.btn, s.icon_lbl, s.text_lbl,
    nullptr, p, on_color, tertiary_color, pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);
}

inline void subscribe_media_state(lv_obj_t *btn_ptr,
                                  lv_obj_t *status_lbl,
                                  const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, status_lbl](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool playing = state_text == "playing";
        if (playing) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        if (status_lbl) {
          std::string label = media_status_text(state_text);
          lv_label_set_text(status_lbl, label.c_str());
        }
      })
  );
}

inline void subscribe_media_now_playing_state(lv_obj_t *title_lbl,
                                              lv_obj_t *artist_lbl,
                                              const std::string &entity_id) {
  if (entity_id.empty()) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_title"),
    std::function<void(esphome::StringRef)>(
      [title_lbl](esphome::StringRef title) {
        media_set_metadata_text(title_lbl, title, "--");
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [artist_lbl](esphome::StringRef artist) {
        media_set_metadata_text(artist_lbl, artist, "--");
      })
  );
}

inline MediaVolumeCtx *create_media_volume_context(lv_obj_t *btn,
                                                   lv_obj_t *label_lbl,
                                                   const ParsedCfg &p,
                                                   uint32_t accent_color,
                                                   const lv_font_t *value_font,
                                                   const lv_font_t *label_font,
                                                   const lv_font_t *icon_font) {
  MediaVolumeCtx *ctx = new MediaVolumeCtx();
  ctx->entity_id = p.entity;
  ctx->label = media_label(p);
  ctx->accent_color = accent_color;
  ctx->btn = btn;
  ctx->label_lbl = label_lbl;
  ctx->value_font = value_font;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  if (btn) lv_obj_set_user_data(btn, ctx);
  return ctx;
}

inline void subscribe_media_volume_state(MediaVolumeCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("volume_level"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float level = 0.0f;
        if (!parse_float_ref(val, level)) return;
        int pct = media_clamp_percent((int)(level * 100.0f + 0.5f));
        if (media_volume_pending_active(ctx)) {
          if (pct != ctx->pending_pct) {
            media_volume_set_modal_value(ctx, ctx->pending_pct);
            return;
          }
          ctx->pending_pct = -1;
          ctx->pending_until_ms = 0;
        } else {
          ctx->pending_pct = -1;
          ctx->pending_until_ms = 0;
        }
        ctx->current_pct = pct;
        media_volume_set_modal_value(ctx, pct);
      })
  );
}

inline void subscribe_media_slider_state(lv_obj_t *btn_ptr,
                                         lv_obj_t *slider,
                                         const std::string &entity_id) {
  SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(slider);
  if (!ctx) return;

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->media_playing = state_text == "playing";
        if (ctx->media_status_lbl) {
          std::string label = media_status_text(state_text);
          lv_label_set_text(ctx->media_status_lbl, label.c_str());
        }
        if (ctx->media_timer) {
          if (ctx->media_playing) lv_timer_resume(ctx->media_timer);
          else lv_timer_pause(ctx->media_timer);
        }
      })
  );

  if (!ctx->media_position) return;

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_duration"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float duration = 0.0f;
        if (!parse_float_ref(val, duration) || duration < 0.0f) duration = 0.0f;
        ctx->media_duration = duration;
        media_apply_position(ctx);
      })
  );

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_position"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float pos = 0.0f;
        if (!parse_float_ref(val, pos) || pos < 0.0f) pos = 0.0f;
        if (media_seek_pending_active(ctx)) {
          if (std::fabs(pos - ctx->media_seek_target_seconds) > MEDIA_SEEK_MATCH_TOLERANCE_SECONDS) {
            media_apply_position(ctx);
            return;
          }
          ctx->media_seek_pending = false;
        } else {
          ctx->media_seek_pending = false;
        }
        ctx->media_position_seconds = pos;
        ctx->media_position_updated_ms = esphome::millis();
        media_apply_position(ctx);
      })
  );

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_position_updated_at"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef) {
        if (media_seek_pending_active(ctx)) {
          media_apply_position(ctx);
          return;
        }
        ctx->media_seek_pending = false;
        ctx->media_position_updated_ms = esphome::millis();
        media_apply_position(ctx);
      })
  );
}

// ── Subpage helpers ───────────────────────────────────────────────────

// Button definition parsed from a subpage config (pipe+colon delimited)
struct SubpageBtn {
  std::string entity;
  std::string label;
  std::string icon;
  std::string icon_on;
  std::string sensor;     // sensor entity, cover/internal mode, or action name
  std::string unit;
  std::string type;       // button type: "" (toggle), action, sensor, calendar, timezone, climate, weather_forecast, slider, cover, garage, lock, media, push, internal, subpage
  std::string precision;  // decimal places for sensor display; "text" = text sensor mode
};

inline std::vector<std::string> split_subpage_fields(const std::string &value, char delim) {
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

inline std::string compact_subpage_type(const std::string &code) {
  if (code == "A") return "action";
  if (code == "D") return "calendar";
  if (code == "T") return "timezone";
  if (code == "H") return "climate";
  if (code == "S") return "sensor";
  if (code == "W") return "weather";
  if (code == "F") return "weather_forecast";
  if (code == "L") return "slider";
  if (code == "C") return "cover";
  if (code == "N") return "light_temperature";
  if (code == "R") return "garage";
  if (code == "K") return "lock";
  if (code == "M") return "media";
  if (code == "P") return "push";
  if (code == "I") return "internal";
  if (code == "G") return "subpage";
  return code;
}

inline std::string decode_compact_subpage_field(const std::string &value) {
  return decode_compact_field(value);
}

inline SubpageBtn normalize_subpage_btn(SubpageBtn b) {
  if (b.type == "slider" && !b.sensor.empty()) b.sensor.clear();
  if (b.type == "weather_forecast") {
    b.type = "weather";
    b.precision = "tomorrow";
  }
  if (b.type == "media") {
    if (b.sensor == "controls") {
      if (b.icon.empty() || b.icon == "Speaker") b.icon = "Auto";
      b.sensor = "play_pause";
    } else if (b.sensor.empty()) {
      b.sensor = "play_pause";
    } else if (b.sensor != "play_pause" && b.sensor != "previous" &&
               b.sensor != "next" && b.sensor != "volume" &&
               b.sensor != "position" && b.sensor != "now_playing") {
      b.sensor = "play_pause";
    }
    if (b.sensor == "previous" && b.label == "Skip Previous") b.label = "Previous";
    if (b.sensor == "next" && b.label == "Skip Next") b.label = "Next";
    if (b.sensor == "volume") {
      if (b.label.empty() || b.label == "Media") b.label = "Volume";
      b.icon = "Auto";
    }
  }
  return b;
}

inline ParsedCfg parsed_cfg_from_subpage_btn(const SubpageBtn &b) {
  ParsedCfg p;
  p.entity = b.entity;
  p.label = b.label;
  p.icon = b.icon;
  p.icon_on = b.icon_on;
  p.sensor = b.sensor;
  p.unit = b.unit;
  p.type = b.type;
  p.precision = b.precision;
  return normalize_parsed_cfg(p);
}

inline lv_obj_t *create_grid_card_button(lv_obj_t *parent, lv_coord_t radius,
                                         lv_coord_t pad,
                                         const lv_font_t *label_font,
                                         lv_color_t text_color) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, pad, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(btn, label_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(btn, text_color, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  return btn;
}

inline lv_obj_t *create_card_sensor_container(lv_obj_t *parent,
                                              const lv_font_t *value_font,
                                              const lv_font_t *unit_font,
                                              lv_color_t text_color,
                                              lv_obj_t **value_lbl,
                                              lv_obj_t **unit_lbl) {
  lv_obj_t *container = lv_obj_create(parent);
  lv_obj_set_align(container, LV_ALIGN_TOP_LEFT);
  lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(container, 0, LV_PART_MAIN);
  lv_obj_set_layout(container, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(container, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(container, LV_FLEX_ALIGN_END, LV_PART_MAIN);

  lv_obj_t *value = lv_label_create(container);
  if (value_font) lv_obj_set_style_text_font(value, value_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(value, text_color, LV_PART_MAIN);
  lv_label_set_text(value, "--");

  lv_obj_t *unit = lv_label_create(container);
  if (unit_font) lv_obj_set_style_text_font(unit, unit_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(unit, text_color, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(unit, 6, LV_PART_MAIN);
  lv_label_set_text(unit, "");

  if (value_lbl) *value_lbl = value;
  if (unit_lbl) *unit_lbl = unit;
  return container;
}

inline BtnSlot create_dynamic_card_slot(lv_obj_t *btn,
                                        const lv_font_t *icon_font,
                                        const lv_font_t *value_font,
                                        const lv_font_t *label_font,
                                        lv_color_t text_color) {
  BtnSlot slot{};
  slot.config = nullptr;
  slot.btn = btn;
  slot.icon_lbl = lv_label_create(btn);
  if (icon_font) lv_obj_set_style_text_font(slot.icon_lbl, icon_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(slot.icon_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(slot.icon_lbl, "\U000F0493");
  lv_obj_align(slot.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

  slot.sensor_container = create_card_sensor_container(
    btn, value_font, label_font, text_color, &slot.sensor_lbl, &slot.unit_lbl);

  slot.text_lbl = lv_label_create(btn);
  if (label_font) lv_obj_set_style_text_font(slot.text_lbl, label_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(slot.text_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(slot.text_lbl, "Configure");
  lv_obj_align(slot.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  configure_button_label_wrap(slot.text_lbl);
  return slot;
}

// Parse "order|entity:label:icon:...|entity:label:..." into a vector of SubpageBtns
inline std::vector<SubpageBtn> parse_subpage_config(const std::string &sp_cfg) {
  std::vector<SubpageBtn> btns;
  if (sp_cfg.empty()) return btns;

  bool compact = sp_cfg[0] == '~';
  std::vector<std::string> pipes = split_subpage_fields(compact ? sp_cfg.substr(1) : sp_cfg, '|');
  if (pipes.size() < 2) return btns;

  for (size_t pi = 1; pi < pipes.size(); pi++) {
    if (compact) {
      std::vector<std::string> flds = split_subpage_fields(pipes[pi], ',');
      std::string tp = flds.size() > 0 ? compact_subpage_type(flds[0]) : "";
      std::string e = flds.size() > 1 ? decode_compact_subpage_field(flds[1]) : "";
      std::string l = flds.size() > 2 ? decode_compact_subpage_field(flds[2]) : "";
      std::string ic = flds.size() > 3 ? decode_compact_subpage_field(flds[3]) : "Auto";
      if (ic.empty()) ic = "Auto";
      std::string io = flds.size() > 4 ? decode_compact_subpage_field(flds[4]) : "Auto";
      if (io.empty()) io = "Auto";
      std::string sn = flds.size() > 5 ? decode_compact_subpage_field(flds[5]) : "";
      std::string un = flds.size() > 6 ? decode_compact_subpage_field(flds[6]) : "";
      std::string pr = flds.size() > 7 ? decode_compact_subpage_field(flds[7]) : "";
      btns.push_back(normalize_subpage_btn({e, l, ic, io, sn, un, tp, pr}));
      continue;
    }
    std::vector<std::string> flds = split_subpage_fields(pipes[pi], ':');
    std::string e = flds.size() > 0 ? flds[0] : "";
    std::string l = flds.size() > 1 ? flds[1] : "";
    std::string ic = flds.size() > 2 ? flds[2] : "Auto";
    if (ic.empty()) ic = "Auto";
    std::string io = flds.size() > 3 ? flds[3] : "Auto";
    if (io.empty()) io = "Auto";
    std::string sn = flds.size() > 4 ? flds[4] : "";
    std::string un = flds.size() > 5 ? flds[5] : "";
    std::string tp = flds.size() > 6 ? flds[6] : "";
    std::string pr = flds.size() > 7 ? flds[7] : "";
    btns.push_back(normalize_subpage_btn({e, l, ic, io, sn, un, tp, pr}));
  }
  return btns;
}

// Extract the order string (everything before the first pipe) from subpage config
inline std::string get_subpage_order(const std::string &sp_cfg) {
  if (sp_cfg.empty()) return "";
  size_t start = sp_cfg[0] == '~' ? 1 : 0;
  size_t pe = sp_cfg.find('|', start);
  if (pe == std::string::npos) return sp_cfg.substr(start);
  return sp_cfg.substr(start, pe - start);
}

// Subpage grid layout with support for a back button token ("B")
struct SubpageOrder {
  int positions[MAX_GRID_SLOTS] = {};
  bool is_double[MAX_GRID_SLOTS] = {};
  bool is_wide[MAX_GRID_SLOTS] = {};
  int back_pos = 0;
  bool back_dbl = false;
  bool back_wide = false;
  bool has_back_token = false;
};

inline void subscribe_subpage_parent_indicator(
    const std::string &entity_id,
    lv_obj_t *parent_btn, lv_obj_t *parent_icon,
    int parent_idx, bool *child_was_on,
    bool has_alt_icon, const char *off_glyph, const char *on_glyph,
    int *sp_on_count) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [parent_btn, parent_icon, parent_idx, child_was_on,
       has_alt_icon, off_glyph, on_glyph, sp_on_count](esphome::StringRef state) {
        bool is_on = is_entity_on_ref(state);
        if (is_on && !*child_was_on) {
          sp_on_count[parent_idx]++;
          *child_was_on = true;
        } else if (!is_on && *child_was_on) {
          sp_on_count[parent_idx]--;
          *child_was_on = false;
        }
        if (sp_on_count[parent_idx] > 0) {
          lv_obj_add_state(parent_btn, LV_STATE_CHECKED);
          if (has_alt_icon) lv_label_set_text(parent_icon, on_glyph);
        } else {
          lv_obj_clear_state(parent_btn, LV_STATE_CHECKED);
          if (has_alt_icon) lv_label_set_text(parent_icon, off_glyph);
        }
      })
  );
}

inline void subscribe_subpage_parent_climate_indicator(
    const std::string &entity_id,
    lv_obj_t *parent_btn, lv_obj_t *parent_icon,
    int parent_idx, bool *child_was_on,
    bool has_alt_icon, const char *off_glyph, const char *on_glyph,
    int *sp_on_count) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("hvac_action"),
    std::function<void(esphome::StringRef)>(
      [parent_btn, parent_icon, parent_idx, child_was_on,
       has_alt_icon, off_glyph, on_glyph, sp_on_count](esphome::StringRef action) {
        bool is_on = climate_action_text_is_active(string_ref_limited(action, HA_SHORT_STATE_MAX_LEN));
        if (is_on && !*child_was_on) {
          sp_on_count[parent_idx]++;
          *child_was_on = true;
        } else if (!is_on && *child_was_on) {
          sp_on_count[parent_idx]--;
          *child_was_on = false;
        }
        if (sp_on_count[parent_idx] > 0) {
          lv_obj_add_state(parent_btn, LV_STATE_CHECKED);
          if (has_alt_icon) lv_label_set_text(parent_icon, on_glyph);
        } else {
          lv_obj_clear_state(parent_btn, LV_STATE_CHECKED);
          if (has_alt_icon) lv_label_set_text(parent_icon, off_glyph);
        }
      })
  );
}

// Parse subpage order CSV; "B"/"Bd"/"Bw"/"Bb" tokens mark the back button position
inline void parse_subpage_order(const std::string &order_str, int num_slots, int num_btns,
                                SubpageOrder &result) {
  int slot_limit = bounded_grid_slots(num_slots);
  int btn_limit = bounded_grid_slots(num_btns);
  if (order_str.empty()) return;
  size_t gp2 = 0, st2 = 0;
  while (st2 <= order_str.length() && gp2 < (size_t)slot_limit) {
    size_t cm = order_str.find(',', st2);
    if (cm == std::string::npos) cm = order_str.length();
    if (cm > st2) {
      std::string tk = order_str.substr(st2, cm - st2);
      if (tk == "B" || tk == "Bd" || tk == "Bw" || tk == "Bb") {
        result.back_pos = gp2;
        result.back_dbl = (tk == "Bd" || tk == "Bb");
        result.back_wide = (tk == "Bw" || tk == "Bb");
        result.has_back_token = true;
      } else {
        bool d = !tk.empty() && tk.back() == 'd';
        bool w = !tk.empty() && tk.back() == 'w';
        bool bg = !tk.empty() && tk.back() == 'b';
        if (bg) { d = true; w = true; }
        if (d || w) tk.pop_back();
        int v = atoi(tk.c_str());
        if (v >= 1 && v <= btn_limit) {
          result.positions[gp2] = v;
          result.is_double[v - 1] = d;
          result.is_wide[v - 1] = w;
        }
      }
    }
    gp2++;
    st2 = cm + 1;
  }
}

// =============================================================================
// GRID BOOT PHASES - Consolidated on_boot logic for all devices
// =============================================================================
// Each sensors.yaml builds id()-based arrays and calls these three functions.
// Device-specific behavior is controlled by GridConfig fields.
// =============================================================================

struct GridConfig {
  int num_slots;
  int cols;
  bool color_correction;
  bool wrap_tall_labels;
  const lv_font_t *icon_font;
  const lv_font_t *climate_control_icon_font;
  const lv_font_t *sp_sensor_font;
  const lv_font_t *climate_target_font;
  std::string temperature_unit;
  std::string timezone;
  bool developer_experimental_features;
};

inline bool experimental_card_enabled(const ParsedCfg &p, bool developer_experimental_features) {
  if (p.type == "climate" || p.type == "light_temperature") return developer_experimental_features;
  return true;
}

inline bool experimental_card_enabled(const ParsedCfg &p, const GridConfig &cfg) {
  return experimental_card_enabled(p, cfg.developer_experimental_features);
}

struct CardPalette {
  bool has_on = false;
  bool has_off = false;
  bool has_sensor_color = false;
  uint32_t on_val = DEFAULT_SLIDER_COLOR;
  uint32_t off_val = CLIMATE_NEUTRAL_COLOR;
  uint32_t sensor_val = DEFAULT_TERTIARY_COLOR;
};

inline void setup_card_visual(BtnSlot &s, const ParsedCfg &p,
                              const GridConfig &cfg,
                              const CardPalette &palette) {
  apply_button_colors(s.btn, palette.has_on, palette.on_val,
    palette.has_off, palette.off_val);

  if (!experimental_card_enabled(p, cfg)) {
    setup_toggle_visual(s, ParsedCfg{});
    return;
  }
  if (is_text_sensor_card(p)) {
    setup_text_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "sensor") {
    if (p.sensor.empty()) return;
    setup_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "calendar") {
    setup_calendar_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "timezone") {
    setup_timezone_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (weather_card_shows_tomorrow(p)) {
    setup_weather_forecast_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "weather") {
    setup_weather_card(s, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "climate") {
    setup_climate_card(s, p, cfg.sp_sensor_font,
      palette.has_off ? palette.off_val : CLIMATE_NEUTRAL_COLOR);
    return;
  }
  if (p.type == "garage") {
    setup_garage_card(s, p);
    return;
  }
  if (subpage_parent_sensor_state_enabled(p)) {
    setup_subpage_parent_state_card(s, p, cfg.sp_sensor_font);
    return;
  }
  if (p.type == "lock") {
    setup_lock_card(s, p);
    return;
  }
  if (p.type == "cover" && cover_command_mode(p.sensor)) {
    setup_cover_command_card(s, p);
    return;
  }
  if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
    setup_cover_toggle_card(s, p);
    return;
  }
  if (p.type == "internal") {
    setup_internal_relay_card(s, p);
    return;
  }
  if (p.type == "action") {
    setup_action_card(s, p);
    return;
  }
  if (p.type == "media") {
    setup_media_card(s, p,
      palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR,
      palette.has_sensor_color ? palette.sensor_val : DEFAULT_TERTIARY_COLOR,
      cfg.sp_sensor_font);
    return;
  }
  if (p.type == "slider" || p.type == "cover") {
    setup_slider_visual(s, p, palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    return;
  }
  if (p.type == "light_temperature") {
    setup_light_temp_visual(s, p, palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    return;
  }
  setup_toggle_visual(s, p);
}

// ── Phase 1: Visual setup ────────────────────────────────────────────

inline void grid_phase1(
    BtnSlot *slots, const GridConfig &cfg,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex) {
  ESP_LOGI("sensors", "Phase 1: visual setup start (%lu ms)", esphome::millis());
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  if (NS != cfg.num_slots) {
    ESP_LOGW("sensors", "Grid slot count %d exceeds max %d; ignoring extra slots",
      cfg.num_slots, MAX_GRID_SLOTS);
  }

  if (!order_str.empty()) {
    bool all_empty = true;
    for (int i = 0; i < NS; i++) {
      if (!slots[i].config->state.empty()) { all_empty = false; break; }
    }
    if (all_empty) {
      ESP_LOGW("sensors", "Button order is set but all configs are empty. "
        "If upgrading from the old per-field format, export your config "
        "from the old firmware's web UI and import it after upgrading.");
    }
  }

  OrderResult parsed, order;
  parse_order_string(order_str, NS, parsed);
  clear_spanned_cells(parsed, NS, COLS, order);

  bool has_on, has_off, has_sensor_color;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = parse_hex_color(off_hex, has_off);
  uint32_t sensor_val = parse_hex_color(sensor_hex, has_sensor_color);

  if (cfg.color_correction) {
    if (has_on) on_val = correct_color(on_val);
    if (has_off) off_val = correct_color(off_val);
    if (has_sensor_color) sensor_val = correct_color(sensor_val);
  }

  CardPalette palette;
  palette.has_on = has_on;
  palette.has_off = has_off;
  palette.has_sensor_color = has_sensor_color;
  palette.on_val = has_on ? on_val : DEFAULT_SLIDER_COLOR;
  palette.off_val = has_off ? off_val : CLIMATE_NEUTRAL_COLOR;
  palette.sensor_val = has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR;

  reset_calendar_cards();
  reset_timezone_cards();
  reset_weather_forecast_cards();
  reset_climate_contexts();

  for (int i = 0; i < NS; i++)
    lv_obj_add_flag(slots[i].btn, LV_OBJ_FLAG_HIDDEN);

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
    int col = pos % COLS, row = pos / COLS;
    int row_span = order.is_double[idx - 1] ? 2 : 1;
    int col_span = order.is_wide[idx - 1] ? 2 : 1;
    lv_obj_set_grid_cell(s.btn,
      LV_GRID_ALIGN_STRETCH, col, col_span,
      LV_GRID_ALIGN_STRETCH, row, row_span);

    if (cfg.wrap_tall_labels && row_span > 1) {
      lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(s.text_lbl, lv_pct(100));
    }

    ParsedCfg p = parse_cfg(scfg);
    setup_card_visual(s, p, cfg, palette);
  }
  ESP_LOGI("sensors", "Phase 1: done (%lu ms)", esphome::millis());
}

// ── Phase 2: HA subscriptions + subpage creation ─────────────────────

inline std::string optional_text_state(esphome::text::Text **configs, int index) {
  return (configs != nullptr && configs[index] != nullptr) ? configs[index]->state : "";
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    esphome::text::Text **sp_ext2_configs,
    esphome::text::Text **sp_ext3_configs,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj) {
  ESP_LOGI("sensors", "Phase 2: subscriptions + subpages start (%lu ms)", esphome::millis());
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  if (NS != cfg.num_slots) {
    ESP_LOGW("sensors", "Grid slot count %d exceeds max %d; ignoring extra slots",
      cfg.num_slots, MAX_GRID_SLOTS);
  }
  int ROWS = (NS + COLS - 1) / COLS;
  set_climate_home_grid_metrics(main_page_obj, COLS, ROWS);

  static bool has_sensor[MAX_GRID_SLOTS] = {};
  static bool sensor_text_mode[MAX_GRID_SLOTS] = {};
  static bool has_icon_on[MAX_GRID_SLOTS] = {};
  static const char* icon_off_cp[MAX_GRID_SLOTS] = {};
  static const char* icon_on_cp[MAX_GRID_SLOTS] = {};

  static bool sp_child_was_on[MAX_SUBPAGE_ITEMS] = {};
  static std::string sp_entity_ids[MAX_SUBPAGE_ITEMS];
  static int sp_child_alloc_idx = 0;
  static int sp_entity_alloc_idx = 0;
  sp_child_alloc_idx = 0;
  sp_entity_alloc_idx = 0;
  memset(has_sensor, 0, sizeof(has_sensor));
  memset(sensor_text_mode, 0, sizeof(sensor_text_mode));
  memset(has_icon_on, 0, sizeof(has_icon_on));
  clear_internal_relay_watchers();

  bool has_on, has_off, has_sensor_color;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = parse_hex_color(off_hex, has_off);
  uint32_t sensor_val = parse_hex_color(sensor_hex, has_sensor_color);

  if (cfg.color_correction) {
    if (has_on) on_val = correct_color(on_val);
    if (has_off) off_val = correct_color(off_val);
    if (has_sensor_color) sensor_val = correct_color(sensor_val);
  }

  CardPalette palette;
  palette.has_on = has_on;
  palette.has_off = has_off;
  palette.has_sensor_color = has_sensor_color;
  palette.on_val = has_on ? on_val : DEFAULT_SLIDER_COLOR;
  palette.off_val = has_off ? off_val : CLIMATE_NEUTRAL_COLOR;
  palette.sensor_val = has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR;

  OrderResult parsed;
  parse_order_string(order_str, NS, parsed);

  for (int pos = 0; pos < NS; pos++) {
    int idx = parsed.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;

    ParsedCfg p = parse_cfg(scfg);
    if (!experimental_card_enabled(p, cfg)) continue;
    if (is_text_sensor_card(p)) {
      if (!p.sensor.empty())
        subscribe_text_sensor_value(s.text_lbl, p.sensor);
      continue;
    }
    if (p.type == "sensor") {
      if (p.sensor.empty()) continue;
      subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision));
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.sensor);
      continue;
    }
    if (p.type == "calendar") {
      subscribe_calendar_date_source(p.entity);
      continue;
    }
    if (p.type == "timezone") {
      continue;
    }
    if (weather_card_shows_tomorrow(p)) {
      continue;
    }
    if (p.type == "weather") {
      if (!p.entity.empty())
        subscribe_weather_state(s.icon_lbl, s.text_lbl, p.entity);
      continue;
    }
    if (p.type == "climate") {
      if (!p.entity.empty()) {
        ClimateCardCtx *climate_ctx = create_climate_context(
          s.btn, s.sensor_container, s.sensor_lbl, s.unit_lbl, s.text_lbl, s.icon_lbl, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          has_off ? off_val : CLIMATE_NEUTRAL_COLOR,
          cfg.sp_sensor_font,
          cfg.climate_target_font ? cfg.climate_target_font : cfg.sp_sensor_font,
          cfg.icon_font,
          cfg.climate_control_icon_font ? cfg.climate_control_icon_font : cfg.icon_font);
        subscribe_climate_card(climate_ctx);
      }
      continue;
    }
    if (p.type == "garage") {
      if (!p.entity.empty()) {
        TransientStatusLabel *status_label = create_transient_status_label(
          s.text_lbl, p.label.empty() ? "Garage Door" : p.label);
        subscribe_garage_state(s.btn, s.icon_lbl, status_label,
          garage_closed_icon(p.icon), garage_open_icon(p.icon_on), p.entity);
        if (p.label.empty())
          subscribe_friendly_name(status_label, p.entity);
      }
      continue;
    }
    if (subpage_parent_sensor_state_enabled(p)) {
      if (subpage_parent_text_state_enabled(p)) {
        subscribe_text_sensor_value(s.text_lbl, p.sensor);
      } else {
        subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision));
        if (p.label.empty())
          subscribe_friendly_name(s.text_lbl, p.sensor);
      }
      continue;
    }
    if (subpage_parent_icon_entity_state_enabled(p)) {
      has_sensor[idx - 1] = false;
      sensor_text_mode[idx - 1] = false;
      has_icon_on[idx - 1] = !p.icon_on.empty() && p.icon_on != "Auto";
      if (has_icon_on[idx - 1])
        icon_on_cp[idx - 1] = find_icon(p.icon_on.c_str());

      if (p.icon.empty() || p.icon == "Auto") {
        icon_off_cp[idx - 1] = domain_default_icon(p.entity.substr(0, p.entity.find('.')));
      } else {
        icon_off_cp[idx - 1] = find_icon(p.icon.c_str());
      }

      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.entity);

      subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,
        &has_sensor[idx - 1], &sensor_text_mode[idx - 1],
        &has_icon_on[idx - 1], &icon_off_cp[idx - 1], &icon_on_cp[idx - 1],
        nullptr, p.entity);
      continue;
    }
    if (p.type == "lock") {
      if (!p.entity.empty()) {
        LockCardCtx *ctx = new LockCardCtx();
        ctx->entity_id = p.entity;
        lv_obj_set_user_data(s.btn, ctx);
        TransientStatusLabel *status_label = create_transient_status_label(
          s.text_lbl, p.label.empty() ? "Lock" : p.label);
        subscribe_lock_state(s.btn, s.icon_lbl, status_label,
          lock_locked_icon(p.icon), lock_unlocked_icon(p.icon_on), ctx);
        if (p.label.empty())
          subscribe_friendly_name(status_label, p.entity);
      }
      continue;
    }
    if (p.type == "cover" && cover_command_mode(p.sensor)) {
      if (!p.entity.empty() && p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.entity);
      continue;
    }
    if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
      if (!p.entity.empty()) {
        TransientStatusLabel *status_label = create_transient_status_label(
          s.text_lbl, p.label.empty() ? "Cover" : p.label);
        subscribe_cover_toggle_state(s.btn, s.icon_lbl, status_label,
          slider_icon_off(p.type, p.entity, p.icon), slider_icon_on(p.type, p.entity, p.icon, p.icon_on), p.entity);
        if (p.label.empty())
          subscribe_friendly_name(status_label, p.entity);
      }
      continue;
    }
    if (p.type == "internal") {
      if (!p.entity.empty() && !internal_relay_push_mode(p)) {
        bool internal_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
        const char *internal_icon_on = internal_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
        watch_internal_relay_state(p.entity, s.btn, s.icon_lbl,
          internal_has_icon_on, internal_relay_icon(p, false), internal_icon_on);
      }
      continue;
    }
    if (p.type == "action") {
      continue;
    }
    if (p.type == "media") {
      if (!p.entity.empty()) {
        std::string mode = media_card_mode(p.sensor);
        if (mode == "play_pause") {
          subscribe_media_state(s.btn, media_play_pause_show_state(p) ? s.text_lbl : nullptr, p.entity);
        } else if (media_playback_button_mode(mode)) {
          // Previous/next are momentary actions and do not reflect player state.
        } else if (mode == "volume") {
          MediaVolumeCtx *ctx = create_media_volume_context(
            s.btn, s.text_lbl, p, has_on ? on_val : DEFAULT_SLIDER_COLOR,
            cfg.sp_sensor_font, lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
            cfg.icon_font);
          subscribe_media_volume_state(ctx);
          if (p.label.empty()) subscribe_friendly_name(s.text_lbl, p.entity);
        } else if (mode == "now_playing") {
          subscribe_media_now_playing_state(s.sensor_lbl, s.text_lbl, p.entity);
        } else {
          lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
          if (slider) subscribe_media_slider_state(s.btn, slider, p.entity);
          if (p.label.empty() && mode != "position")
            subscribe_friendly_name(s.text_lbl, p.entity);
        }
      }
      continue;
    }

    if (p.entity.empty()) continue;

    if (p.type == "slider" || p.type == "cover") {
      lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
      bool sl_has_icon_on = slider_has_alt_icon(p.type, p.icon_on);
      const char *sl_icon_on_cp = sl_has_icon_on ? slider_icon_on(p.type, p.entity, p.icon, p.icon_on) : nullptr;
      const char *sl_icon_off_cp = sl_has_icon_on ? slider_icon_off(p.type, p.entity, p.icon) : nullptr;
      subscribe_slider_state(s.btn, s.icon_lbl, slider,
        sl_has_icon_on, sl_icon_off_cp, sl_icon_on_cp, p.entity,
        p.type == "cover" && cover_tilt_mode(p.sensor));
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.entity);
      continue;
    }
    if (p.type == "light_temperature") {
      lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
      if (slider) {
        int min_k = 2000, max_k = 6500;
        parse_kelvin_range(p.unit, min_k, max_k);
        subscribe_light_temp_state(s.btn, slider, p.entity, min_k, max_k, p.precision == "color");
      }
      if (p.label.empty()) {
        SliderCtx *lctx = slider ? (SliderCtx *)lv_obj_get_user_data(slider) : nullptr;
        subscribe_friendly_name_for_light_temp(s.text_lbl, lctx, p.entity);
      }
      continue;
    }

    has_sensor[idx - 1] = !p.sensor.empty();
    sensor_text_mode[idx - 1] = has_sensor[idx - 1] && p.precision == "text";

    has_icon_on[idx - 1] = !p.icon_on.empty() && p.icon_on != "Auto";
    if (has_icon_on[idx - 1])
      icon_on_cp[idx - 1] = find_icon(p.icon_on.c_str());

    const char* icon_cp = "\U000F0493";
    if (p.icon.empty() || p.icon == "Auto") {
      icon_cp = domain_default_icon(p.entity.substr(0, p.entity.find('.')));
    } else {
      icon_cp = find_icon(p.icon.c_str());
    }
    icon_off_cp[idx - 1] = icon_cp;

    ToggleTextSensorCtx *text_sensor_ctx = nullptr;
    if (sensor_text_mode[idx - 1]) {
      text_sensor_ctx = new ToggleTextSensorCtx();
      text_sensor_ctx->text_lbl = s.text_lbl;
      text_sensor_ctx->steady_text = label_text_or_empty(s.text_lbl);
    }

    if (p.label.empty()) {
      if (text_sensor_ctx)
        subscribe_friendly_name(text_sensor_ctx, p.entity);
      else
        subscribe_friendly_name(s.text_lbl, p.entity);
    }

    subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,
      &has_sensor[idx - 1], &sensor_text_mode[idx - 1],
      &has_icon_on[idx - 1], &icon_off_cp[idx - 1], &icon_on_cp[idx - 1],
      text_sensor_ctx, p.entity);

    if (has_sensor[idx - 1]) {
      if (sensor_text_mode[idx - 1])
        subscribe_toggle_text_sensor_value(text_sensor_ctx, p.sensor);
      else
        subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision));
    }
  }

  // --- Subpage creation ---
  // Heap-allocated grid descriptors (never freed -- display lifetime)
  lv_coord_t *sp_col_dsc = new lv_coord_t[COLS + 1];
  for (int i = 0; i < COLS; i++) sp_col_dsc[i] = LV_GRID_FR(1);
  sp_col_dsc[COLS] = LV_GRID_TEMPLATE_LAST;
  lv_coord_t *sp_row_dsc = new lv_coord_t[ROWS + 1];
  for (int i = 0; i < ROWS; i++) sp_row_dsc[i] = LV_GRID_FR(1);
  sp_row_dsc[ROWS] = LV_GRID_TEMPLATE_LAST;

  const lv_font_t *sp_icon_fnt = lv_obj_get_style_text_font(slots[0].icon_lbl, LV_PART_MAIN);

  lv_obj_t *ref_btn = slots[0].btn;
  for (int i = 0; i < NS; i++) {
    ParsedCfg pc = parse_cfg(slots[i].config->state);
    if (pc.type != "slider" && pc.type != "cover") {
      ref_btn = slots[i].btn;
      break;
    }
  }
  lv_coord_t sp_radius = lv_obj_get_style_radius(ref_btn, LV_PART_MAIN);
  lv_coord_t sp_pad = lv_obj_get_style_pad_top(ref_btn, LV_PART_MAIN);
  const lv_font_t *sp_btn_fnt = lv_obj_get_style_text_font(ref_btn, LV_PART_MAIN);
  lv_color_t sp_txt_color = lv_obj_get_style_text_color(ref_btn, LV_PART_MAIN);

  lv_coord_t mp_pad_top = lv_obj_get_style_pad_top(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_bottom = lv_obj_get_style_pad_bottom(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_left = lv_obj_get_style_pad_left(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_right = lv_obj_get_style_pad_right(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_row = lv_obj_get_style_pad_row(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_col = lv_obj_get_style_pad_column(main_page_obj, LV_PART_MAIN);

  static int sp_on_count[MAX_GRID_SLOTS] = {};
  memset(sp_on_count, 0, sizeof(sp_on_count));

  for (int si = 0; si < NS; si++) {
    ParsedCfg p = parse_cfg(slots[si].config->state);
    if (p.type != "subpage") continue;
    bool sp_indicator = p.sensor == "indicator" && p.entity.empty();

    bool sp_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
    const char* sp_icon_on_glyph = sp_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
    const char* sp_icon_off_glyph = nullptr;
    if (sp_has_icon_on) {
      sp_icon_off_glyph = (p.icon.empty() || p.icon == "Auto")
        ? "\U000F024B" : find_icon(p.icon.c_str());
    }

    std::string sp_cfg = optional_text_state(sp_configs, si) +
      optional_text_state(sp_ext_configs, si) +
      optional_text_state(sp_ext2_configs, si) +
      optional_text_state(sp_ext3_configs, si);
    if (sp_cfg.empty()) continue;

    auto sp_btns = parse_subpage_config(sp_cfg);
    if (sp_btns.empty()) continue;
    std::string sp_order_str = get_subpage_order(sp_cfg);

    SubpageOrder sp_ord;
    parse_subpage_order(sp_order_str, NS, sp_btns.size(), sp_ord);

    lv_obj_t *sub_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(sub_scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sub_scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_layout(sub_scr, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(sub_scr, sp_col_dsc, sp_row_dsc);
    lv_obj_set_style_pad_top(sub_scr, mp_pad_top, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(sub_scr, mp_pad_bottom, LV_PART_MAIN);
    lv_obj_set_style_pad_left(sub_scr, mp_pad_left, LV_PART_MAIN);
    lv_obj_set_style_pad_right(sub_scr, mp_pad_right, LV_PART_MAIN);
    lv_obj_set_style_pad_row(sub_scr, mp_pad_row, LV_PART_MAIN);
    lv_obj_set_style_pad_column(sub_scr, mp_pad_col, LV_PART_MAIN);
    lv_obj_clear_flag(sub_scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_btn = create_grid_card_button(
      sub_scr, sp_radius, sp_pad, sp_btn_fnt, sp_txt_color);
    apply_button_colors(back_btn, false, DEFAULT_SLIDER_COLOR, has_off, off_val);
    lv_obj_set_grid_cell(back_btn, LV_GRID_ALIGN_STRETCH, sp_ord.back_pos % COLS, sp_ord.back_wide ? 2 : 1,
      LV_GRID_ALIGN_STRETCH, sp_ord.back_pos / COLS, sp_ord.back_dbl ? 2 : 1);
    BtnSlot back_slot = create_dynamic_card_slot(
      back_btn, sp_icon_fnt, cfg.sp_sensor_font, sp_btn_fnt, sp_txt_color);
    lv_label_set_text(back_slot.icon_lbl, "\U000F0141");
    lv_label_set_text(back_slot.text_lbl, "Back");

    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
      lv_scr_load_anim((lv_obj_t *)lv_event_get_user_data(e), LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }, LV_EVENT_CLICKED, main_page_obj);

    auto add_parent_indicator = [&](const std::string &entity_id, bool climate_indicator) {
      if (!sp_indicator || entity_id.empty()) return;
      lv_obj_t *parent_btn = slots[si].btn;
      lv_obj_t *parent_icon = slots[si].icon_lbl;
      int parent_idx = si;
      int cwi = sp_child_alloc_idx++;
      if (cwi >= MAX_SUBPAGE_ITEMS) {
        ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", entity_id.c_str());
        return;
      }
      sp_child_was_on[cwi] = false;
      if (climate_indicator) {
        subscribe_subpage_parent_climate_indicator(
          entity_id, parent_btn, parent_icon, parent_idx,
          &sp_child_was_on[cwi], sp_has_icon_on,
          sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
      } else {
        subscribe_subpage_parent_indicator(
          entity_id, parent_btn, parent_icon, parent_idx,
          &sp_child_was_on[cwi], sp_has_icon_on,
          sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
      }
    };

    auto add_subpage_toggle_click = [&](lv_obj_t *btn, const std::string &entity_id, bool set_checked) {
      if (entity_id.empty()) return;
      int eid_idx = sp_entity_alloc_idx++;
      if (eid_idx >= MAX_SUBPAGE_ITEMS) {
        ESP_LOGW("sensors", "Too many subpage click handlers; skipping %s", entity_id.c_str());
        return;
      }
      sp_entity_ids[eid_idx] = entity_id;
      if (set_checked) {
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
          lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
          lv_obj_add_state(target, LV_STATE_CHECKED);
          std::string *en = (std::string *)lv_event_get_user_data(e);
          if (en && !en->empty()) send_toggle_action(*en);
        }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
      } else {
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
          std::string *en = (std::string *)lv_event_get_user_data(e);
          if (en && !en->empty()) send_toggle_action(*en);
        }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
      }
    };

    for (int gp = 0; gp < NS; gp++) {
      int bn = sp_ord.positions[gp];
      if (bn < 1 || bn > (int)sp_btns.size()) continue;
      auto &sb = sp_btns[bn - 1];
      ParsedCfg sb_cfg = parsed_cfg_from_subpage_btn(sb);
      if (!experimental_card_enabled(sb_cfg, cfg)) continue;
      int col, row;
      if (sp_ord.has_back_token) { col = gp % COLS; row = gp / COLS; }
      else { int op = gp + 1; col = op % COLS; row = op / COLS; }
      int rs = sp_ord.is_double[bn - 1] ? 2 : 1;

      lv_obj_t *sb_btn = create_grid_card_button(
        sub_scr, sp_radius, sp_pad, sp_btn_fnt, sp_txt_color);
      int cs = sp_ord.is_wide[bn - 1] ? 2 : 1;
      lv_obj_set_grid_cell(sb_btn, LV_GRID_ALIGN_STRETCH, col, cs, LV_GRID_ALIGN_STRETCH, row, rs);
      BtnSlot sub_slot = create_dynamic_card_slot(
        sb_btn, sp_icon_fnt, cfg.sp_sensor_font, sp_btn_fnt, sp_txt_color);
      setup_card_visual(sub_slot, sb_cfg, cfg, palette);

      if (is_text_sensor_card(sb_cfg)) {
        if (!sb_cfg.sensor.empty())
          subscribe_text_sensor_value(sub_slot.text_lbl, sb_cfg.sensor);
        continue;
      }
      if (sb_cfg.type == "sensor") {
        if (sb_cfg.sensor.empty()) continue;
        subscribe_sensor_value(sub_slot.sensor_lbl, sb_cfg.sensor, parse_precision(sb_cfg.precision));
        if (sb_cfg.label.empty())
          subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.sensor);
        continue;
      }
      if (sb_cfg.type == "calendar") {
        subscribe_calendar_date_source(sb_cfg.entity);
        continue;
      }
      if (sb_cfg.type == "timezone" || weather_card_shows_tomorrow(sb_cfg)) {
        continue;
      }
      if (sb_cfg.type == "weather") {
        if (!sb_cfg.entity.empty())
          subscribe_weather_state(sub_slot.icon_lbl, sub_slot.text_lbl, sb_cfg.entity);
        continue;
      }
      if (sb_cfg.type == "climate") {
        if (!sb_cfg.entity.empty()) {
          ClimateCardCtx *climate_ctx = create_climate_context(
            sub_slot.btn, sub_slot.sensor_container, sub_slot.sensor_lbl,
            sub_slot.unit_lbl, sub_slot.text_lbl, sub_slot.icon_lbl, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            has_off ? off_val : CLIMATE_NEUTRAL_COLOR,
            cfg.sp_sensor_font,
            cfg.climate_target_font ? cfg.climate_target_font : cfg.sp_sensor_font,
            cfg.icon_font,
            cfg.climate_control_icon_font ? cfg.climate_control_icon_font : cfg.icon_font);
          subscribe_climate_card(climate_ctx);
          add_parent_indicator(sb_cfg.entity, true);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ClimateCardCtx *ctx = (ClimateCardCtx *)lv_event_get_user_data(e);
            if (ctx) climate_open_detail(ctx, lv_scr_act());
          }, LV_EVENT_CLICKED, climate_ctx);
        }
        continue;
      }
      if (sb_cfg.type == "cover" && cover_command_mode(sb_cfg.sensor)) {
        if (!sb_cfg.entity.empty()) {
          if (sb_cfg.label.empty())
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
          ParsedCfg *ctx = new ParsedCfg(sb_cfg);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
            if (c) send_cover_command_action(*c);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "cover" && cover_toggle_mode(sb_cfg.sensor)) {
        if (!sb_cfg.entity.empty()) {
          TransientStatusLabel *status_label = create_transient_status_label(
            sub_slot.text_lbl, sb_cfg.label.empty() ? "Cover" : sb_cfg.label);
          subscribe_cover_toggle_state(sub_slot.btn, sub_slot.icon_lbl, status_label,
            slider_icon_off(sb_cfg.type, sb_cfg.entity, sb_cfg.icon),
            slider_icon_on(sb_cfg.type, sb_cfg.entity, sb_cfg.icon, sb_cfg.icon_on),
            sb_cfg.entity);
          if (sb_cfg.label.empty())
            subscribe_friendly_name(status_label, sb_cfg.entity);
          add_parent_indicator(sb_cfg.entity, false);
          add_subpage_toggle_click(sb_btn, sb_cfg.entity, true);
        }
        continue;
      }
      if (sb_cfg.type == "garage") {
        if (!sb_cfg.entity.empty()) {
          TransientStatusLabel *status_label = create_transient_status_label(
            sub_slot.text_lbl, sb_cfg.label.empty() ? "Garage Door" : sb_cfg.label);
          subscribe_garage_state(sub_slot.btn, sub_slot.icon_lbl, status_label,
            garage_closed_icon(sb_cfg.icon), garage_open_icon(sb_cfg.icon_on), sb_cfg.entity);
          if (sb_cfg.label.empty())
            subscribe_friendly_name(status_label, sb_cfg.entity);
          add_parent_indicator(sb_cfg.entity, false);
          add_subpage_toggle_click(sb_btn, sb_cfg.entity, true);
        }
        continue;
      }
      if (sb_cfg.type == "lock") {
        if (!sb_cfg.entity.empty()) {
          LockCardCtx *lock_ctx = new LockCardCtx();
          lock_ctx->entity_id = sb_cfg.entity;
          lv_obj_set_user_data(sb_btn, lock_ctx);
          TransientStatusLabel *status_label = create_transient_status_label(
            sub_slot.text_lbl, sb_cfg.label.empty() ? "Lock" : sb_cfg.label);
          subscribe_lock_state(sub_slot.btn, sub_slot.icon_lbl, status_label,
            lock_locked_icon(sb_cfg.icon), lock_unlocked_icon(sb_cfg.icon_on), lock_ctx);
          if (sb_cfg.label.empty())
            subscribe_friendly_name(status_label, sb_cfg.entity);
          add_parent_indicator(sb_cfg.entity, false);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            LockCardCtx *ctx = (LockCardCtx *)lv_event_get_user_data(e);
            if (ctx) send_lock_action(ctx);
          }, LV_EVENT_CLICKED, lock_ctx);
        }
        continue;
      }
      if (sb_cfg.type == "push") {
        std::string push_label = sb_cfg.label.empty() ? "Push" : sb_cfg.label;
        std::string *label = new std::string(push_label);
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          std::string *label = (std::string *)lv_event_get_user_data(e);
          esphome::api::HomeassistantActionRequest req;
          req.service = decltype(req.service)("esphome.push_button_pressed");
          req.is_event = true;
          req.data.init(1);
          auto &kv = req.data.emplace_back();
          kv.key = decltype(kv.key)("label");
          kv.value = decltype(kv.value)(label->c_str());
          esphome::api::global_api_server->send_homeassistant_action(req);
        }, LV_EVENT_CLICKED, label);
        continue;
      }
      if (sb_cfg.type == "action") {
        if (!sb_cfg.entity.empty() && !sb_cfg.sensor.empty()) {
          ParsedCfg *ctx = new ParsedCfg(sb_cfg);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
            if (c) send_action_card_action(*c);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "media") {
        std::string mode = media_card_mode(sb_cfg.sensor);
        if (!sb_cfg.entity.empty()) {
          if (media_playback_button_mode(mode)) {
            ParsedCfg *ctx = new ParsedCfg(sb_cfg);
            ctx->sensor = mode;
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_media_playback_action(c->entity, media_card_mode(c->sensor));
            }, LV_EVENT_CLICKED, ctx);
            if (mode == "play_pause")
              subscribe_media_state(sub_slot.btn,
                media_play_pause_show_state(sb_cfg) ? sub_slot.text_lbl : nullptr,
                sb_cfg.entity);
          } else if (mode == "volume") {
            MediaVolumeCtx *ctx = create_media_volume_context(
              sub_slot.btn, sub_slot.text_lbl, sb_cfg,
              has_on ? on_val : DEFAULT_SLIDER_COLOR,
              cfg.sp_sensor_font, lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
              cfg.icon_font);
            subscribe_media_volume_state(ctx);
            if (sb_cfg.label.empty()) subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              MediaVolumeCtx *ctx = (MediaVolumeCtx *)lv_event_get_user_data(e);
              if (ctx) media_volume_open_modal(ctx);
            }, LV_EVENT_CLICKED, ctx);
          } else if (mode == "now_playing") {
            subscribe_media_now_playing_state(sub_slot.sensor_lbl, sub_slot.text_lbl, sb_cfg.entity);
          } else {
            lv_obj_t *media_slider = (lv_obj_t *)lv_obj_get_user_data(sub_slot.sensor_container);
            if (media_slider) subscribe_media_slider_state(sub_slot.btn, media_slider, sb_cfg.entity);
            if (sb_cfg.label.empty() && mode != "position")
              subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
          }
          add_parent_indicator(sb_cfg.entity, false);
        }
        continue;
      }
      if (sb_cfg.type == "internal") {
        bool push_mode = internal_relay_push_mode(sb_cfg);
        if (!push_mode && !sb_cfg.entity.empty()) {
          bool internal_has_icon_on = !sb_cfg.icon_on.empty() && sb_cfg.icon_on != "Auto";
          const char *internal_icon_on = internal_has_icon_on ? find_icon(sb_cfg.icon_on.c_str()) : nullptr;
          bool *child_was_on = nullptr;
          lv_obj_t *parent_btn = nullptr;
          lv_obj_t *parent_icon = nullptr;
          if (sp_indicator) {
            int cwi = sp_child_alloc_idx++;
            if (cwi >= MAX_SUBPAGE_ITEMS) {
              ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", sb_cfg.entity.c_str());
            } else {
              sp_child_was_on[cwi] = false;
              child_was_on = &sp_child_was_on[cwi];
              parent_btn = slots[si].btn;
              parent_icon = slots[si].icon_lbl;
            }
          }
          watch_internal_relay_state(
            sb_cfg.entity, sub_slot.btn, sub_slot.icon_lbl,
            internal_has_icon_on, internal_relay_icon(sb_cfg, push_mode), internal_icon_on,
            child_was_on, parent_btn, parent_icon, si,
            sp_has_icon_on, sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
        }
        if (!sb_cfg.entity.empty()) {
          InternalRelayClickCtx *ctx = new InternalRelayClickCtx();
          ctx->key = sb_cfg.entity;
          ctx->push_mode = push_mode;
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            InternalRelayClickCtx *c = (InternalRelayClickCtx *)lv_event_get_user_data(e);
            if (c && !c->key.empty()) send_internal_relay_action(c->key, c->push_mode);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "light_temperature") {
        if (!sb_cfg.entity.empty()) {
          lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(sub_slot.sensor_container);
          if (slider) {
            int min_k = 2000, max_k = 6500;
            parse_kelvin_range(sb_cfg.unit, min_k, max_k);
            subscribe_light_temp_state(sub_slot.btn, slider, sb_cfg.entity,
              min_k, max_k, sb_cfg.precision == "color");
            if (sb_cfg.label.empty()) {
              SliderCtx *lctx = (SliderCtx *)lv_obj_get_user_data(slider);
              subscribe_friendly_name_for_light_temp(sub_slot.text_lbl, lctx, sb_cfg.entity);
            }
          }
          add_parent_indicator(sb_cfg.entity, false);
        }
        continue;
      }
      if (sb_cfg.type == "slider" || sb_cfg.type == "cover") {
        if (!sb_cfg.entity.empty()) {
          lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(sub_slot.sensor_container);
          bool sl_has_icon_on = slider_has_alt_icon(sb_cfg.type, sb_cfg.icon_on);
          const char *sl_icon_on = sl_has_icon_on
            ? slider_icon_on(sb_cfg.type, sb_cfg.entity, sb_cfg.icon, sb_cfg.icon_on) : nullptr;
          const char *sl_icon_off = sl_has_icon_on
            ? slider_icon_off(sb_cfg.type, sb_cfg.entity, sb_cfg.icon) : nullptr;
          if (slider) {
            subscribe_slider_state(sub_slot.btn, sub_slot.icon_lbl, slider,
              sl_has_icon_on, sl_icon_off, sl_icon_on, sb_cfg.entity,
              sb_cfg.type == "cover" && cover_tilt_mode(sb_cfg.sensor));
          }
          if (sb_cfg.label.empty())
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
          add_parent_indicator(sb_cfg.entity, false);
          std::string *eid = new std::string(sb_cfg.entity);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            std::string *en = (std::string *)lv_event_get_user_data(e);
            if (en && !en->empty()) send_slider_action(*en, -1);
          }, LV_EVENT_CLICKED, eid);
        }
        continue;
      }
      if (!sb_cfg.entity.empty()) {
        bool switch_has_sensor = !sb_cfg.sensor.empty();
        bool switch_sensor_text_mode = switch_has_sensor && sb_cfg.precision == "text";
        bool switch_has_icon_on = !sb_cfg.icon_on.empty() && sb_cfg.icon_on != "Auto";
        const char *switch_icon_on = switch_has_icon_on ? find_icon(sb_cfg.icon_on.c_str()) : nullptr;
        const char *switch_icon_off = (sb_cfg.icon.empty() || sb_cfg.icon == "Auto")
          ? domain_default_icon(sb_cfg.entity.substr(0, sb_cfg.entity.find('.')))
          : find_icon(sb_cfg.icon.c_str());

        ToggleTextSensorCtx *switch_text_ctx = nullptr;
        if (switch_sensor_text_mode) {
          switch_text_ctx = new ToggleTextSensorCtx();
          switch_text_ctx->text_lbl = sub_slot.text_lbl;
          switch_text_ctx->steady_text = label_text_or_empty(sub_slot.text_lbl);
        }

        if (sb_cfg.label.empty()) {
          if (switch_text_ctx)
            subscribe_friendly_name(switch_text_ctx, sb_cfg.entity);
          else
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
        }

        bool *switch_has_sensor_ptr = new bool(switch_has_sensor);
        bool *switch_sensor_text_ptr = new bool(switch_sensor_text_mode);
        bool *switch_has_icon_on_ptr = new bool(switch_has_icon_on);
        const char **switch_icon_off_ptr = new const char*(switch_icon_off);
        const char **switch_icon_on_ptr = new const char*(switch_icon_on);
        subscribe_toggle_state(sub_slot.btn, sub_slot.icon_lbl, sub_slot.sensor_container,
          switch_has_sensor_ptr, switch_sensor_text_ptr, switch_has_icon_on_ptr,
          switch_icon_off_ptr, switch_icon_on_ptr, switch_text_ctx, sb_cfg.entity);

        if (switch_has_sensor) {
          if (switch_sensor_text_mode)
            subscribe_toggle_text_sensor_value(switch_text_ctx, sb_cfg.sensor);
          else
            subscribe_sensor_value(sub_slot.sensor_lbl, sb_cfg.sensor, parse_precision(sb_cfg.precision));
        }

        add_parent_indicator(sb_cfg.entity, false);
        add_subpage_toggle_click(sb_btn, sb_cfg.entity, false);
      }
    }

    lv_obj_set_user_data(slots[si].btn, (void *)sub_scr);
  }
  refresh_weather_forecast_cards();
  ESP_LOGI("sensors", "Phase 2: done (%lu ms)", esphome::millis());
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj) {
  grid_phase2(slots, cfg, sp_configs, sp_ext_configs, nullptr, nullptr,
    order_str, on_hex, off_hex, sensor_hex, main_page_obj);
}

// ── Phase 3: Temperature + presence subscriptions ────────────────────

inline void grid_phase3(
    bool indoor_on, bool outdoor_on,
    const std::string &indoor_entity, const std::string &outdoor_entity,
    float *indoor_temp_ptr, float *outdoor_temp_ptr,
    lv_obj_t *temp_label,
    const std::string &presence_entity,
    bool *presence_detected_ptr,
    std::function<void()> wake_callback,
    std::function<void()> sleep_callback) {
  ESP_LOGI("sensors", "Phase 3: temp/presence subscriptions start (%lu ms)", esphome::millis());

  if (indoor_on && outdoor_on) {
    char buf[32];
    format_clock_bar_temperature_pair(buf, sizeof(buf), "-", "-");
    lv_label_set_text(temp_label, buf);
  } else if (indoor_on || outdoor_on) {
    char buf[16];
    format_clock_bar_temperature_single(buf, sizeof(buf), "-");
    lv_label_set_text(temp_label, buf);
  }

  if (indoor_on && !indoor_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      indoor_entity, {},
      std::function<void(esphome::StringRef)>(
        [indoor_temp_ptr, outdoor_temp_ptr, temp_label](esphome::StringRef state) {
          float val = 0.0f;
          if (parse_float_ref(state, val)) {
            *indoor_temp_ptr = val;
            float outdoor = *outdoor_temp_ptr;
            char buf[40];
            if (std::isnan(outdoor)) {
              char indoor_buf[16];
              format_fixed_decimal(indoor_buf, sizeof(indoor_buf), val, 0);
              format_clock_bar_temperature_single(buf, sizeof(buf), indoor_buf);
            } else {
              char outdoor_buf[16];
              char indoor_buf[16];
              format_fixed_decimal(outdoor_buf, sizeof(outdoor_buf), outdoor, 0);
              format_fixed_decimal(indoor_buf, sizeof(indoor_buf), val, 0);
              format_clock_bar_temperature_pair(buf, sizeof(buf), outdoor_buf, indoor_buf);
            }
            lv_label_set_text(temp_label, buf);
          }
        })
    );
  }

  if (outdoor_on && !outdoor_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      outdoor_entity, {},
      std::function<void(esphome::StringRef)>(
        [indoor_temp_ptr, outdoor_temp_ptr, temp_label](esphome::StringRef state) {
          float val = 0.0f;
          if (parse_float_ref(state, val)) {
            *outdoor_temp_ptr = val;
            float indoor = *indoor_temp_ptr;
            char buf[40];
            if (std::isnan(indoor)) {
              char outdoor_buf[16];
              format_fixed_decimal(outdoor_buf, sizeof(outdoor_buf), val, 0);
              format_clock_bar_temperature_single(buf, sizeof(buf), outdoor_buf);
            } else {
              char outdoor_buf[16];
              char indoor_buf[16];
              format_fixed_decimal(outdoor_buf, sizeof(outdoor_buf), val, 0);
              format_fixed_decimal(indoor_buf, sizeof(indoor_buf), indoor, 0);
              format_clock_bar_temperature_pair(buf, sizeof(buf), outdoor_buf, indoor_buf);
            }
            lv_label_set_text(temp_label, buf);
          }
        })
    );
  }

  if (!presence_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      presence_entity, {},
      std::function<void(esphome::StringRef)>(
        [presence_detected_ptr, wake_callback, sleep_callback](esphome::StringRef state) {
          if (state == "on") {
            *presence_detected_ptr = true;
            lv_disp_trig_activity(NULL);
            wake_callback();
          } else if (state == "off") {
            *presence_detected_ptr = false;
            sleep_callback();
          }
        })
    );
  }
  ESP_LOGI("sensors", "Phase 3: done (%lu ms)", esphome::millis());
}
