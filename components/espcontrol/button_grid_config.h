#pragma once

#ifndef ESPCONTROL_HA_SUBSCRIPTION_SCOPE_CONSTANTS_DEFINED
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_ALL = 0;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_DEFAULT = 1u << 0;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_COVER_ART = 1u << 1;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_PHASE3 = 1u << 2;
#define ESPCONTROL_HA_SUBSCRIPTION_SCOPE_CONSTANTS_DEFINED 1
#endif

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.
#include "esphome/core/defines.h"
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
#include "display_color.h"

inline std::function<void()> &dashboard_content_changed_callback() {
  static std::function<void()> callback;
  return callback;
}

inline void set_dashboard_content_changed_callback(std::function<void()> callback) {
  dashboard_content_changed_callback() = std::move(callback);
}

inline void notify_dashboard_content_changed() {
  auto &callback = dashboard_content_changed_callback();
  if (callback) callback();
}

#ifndef ESPCONTROL_MAX_GRID_SLOTS
#define ESPCONTROL_MAX_GRID_SLOTS 25
#endif

constexpr int MAX_GRID_SLOTS = ESPCONTROL_MAX_GRID_SLOTS;
static_assert(MAX_GRID_SLOTS > 0, "ESPCONTROL_MAX_GRID_SLOTS must be positive");
constexpr int MAX_SUBPAGE_ITEMS = MAX_GRID_SLOTS * MAX_GRID_SLOTS;
#include "button_grid_card_runtime.h"
#include <cstdlib>

constexpr const char *SENSOR_STATE_LABELS_OPTION = card_runtime_option_name_state_labels();
constexpr const char *SENSOR_STATE_INPUT_OPTION = card_runtime_option_name_state_input();
constexpr const char *SENSOR_STATE_OUTPUT_OPTION = card_runtime_option_name_state_output();
constexpr const char *SENSOR_STATE_INPUT_2_OPTION = card_runtime_option_name_state_input_2();
constexpr const char *SENSOR_STATE_OUTPUT_2_OPTION = card_runtime_option_name_state_output_2();
constexpr const char *SENSOR_STATE_LOW_LABEL_OPTION = card_runtime_option_name_state_low_label();
constexpr const char *SENSOR_STATE_HIGH_LABEL_OPTION = card_runtime_option_name_state_high_label();
constexpr const char *IMAGE_LABEL_OPTION = card_runtime_option_name_image_label();
constexpr const char *IMAGE_ICON_OPTION = card_runtime_option_name_image_icon();
constexpr const char *IMAGE_MODAL_MODE_OPTION = card_runtime_option_name_image_modal_mode();
constexpr const char *IMAGE_REFRESH_OPTION = card_runtime_option_name_image_refresh();
constexpr const char *IMAGE_REFRESH_MODE_OPTION = card_runtime_option_name_image_refresh_mode();
constexpr const char *MEDIA_COVER_ART_OPTION = card_runtime_option_name_media_cover_art();
constexpr const char *LIGHT_CONTROL_TABS_OPTION = card_runtime_option_name_light_tabs();
constexpr const char *LIGHT_CONTROL_DEFAULT_TABS_VALUE = "power|brightness|temperature|color";
constexpr const char *COVER_CONTROL_TABS_OPTION = card_runtime_option_name_cover_tabs();
constexpr const char *COVER_CONTROL_DEFAULT_TABS_VALUE = "position|controls|tilt";
constexpr const char *CLIMATE_CONTROL_TABS_OPTION = "climate_tabs";
constexpr const char *CLIMATE_CONTROL_DEFAULT_TABS_VALUE = "temperature|mode|preset|fan|swing";
constexpr const char *FAN_CONTROL_TABS_OPTION = "fan_tabs";
constexpr const char *FAN_CONTROL_DEFAULT_TABS_VALUE = "power|speed|preset|oscillation|direction";
constexpr const char *LABEL_DISPLAY_OPTION = card_runtime_option_name_label_display();
constexpr const char *NUMBER_DISPLAY_OPTION = card_runtime_option_name_number_display();
constexpr const char *TEMPERATURE_STEP_OPTION = card_runtime_option_name_temperature_step();
constexpr const char *VOLUME_MAX_OPTION = card_runtime_option_name_volume_max();
constexpr const char *MEDIA_PLAYLIST_CONTENT_ID_OPTION = card_runtime_option_name_playlist_content_id();
constexpr const char *MEDIA_PLAYLIST_CONTENT_TYPE_OPTION = card_runtime_option_name_playlist_content_type();
constexpr const char *MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION = card_runtime_option_name_playlist_player_source();

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
  lv_obj_t *subpage_lbl = nullptr;  // small chevron marker for subpage cards
};

struct ParsedCfg;
inline void set_card_checked_state(lv_obj_t *btn, bool checked);
#include "screen_lock_state.h"

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

inline std::string decode_compact_field(const std::string &value, size_t start, size_t len) {
  if (start > value.size()) return "";
  size_t end = start + len;
  if (end < start || end > value.size()) end = value.size();
  std::string out;
  out.reserve(end - start);
  for (size_t i = start; i < end; i++) {
    if (value[i] == '%' && i + 2 < end) {
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

inline std::string decode_compact_field(const std::string &value) {
  return decode_compact_field(value, 0, value.size());
}

inline char compact_hex_char(uint8_t value) {
  return value < 10 ? static_cast<char>('0' + value)
                    : static_cast<char>('A' + value - 10);
}

inline std::string encode_compact_field(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (unsigned char ch : value) {
    if (ch == '%' || ch == ',' || ch == ';' || ch == '|' || ch == ':') {
      out.push_back('%');
      out.push_back(compact_hex_char((ch >> 4) & 0x0F));
      out.push_back(compact_hex_char(ch & 0x0F));
    } else {
      out.push_back(static_cast<char>(ch));
    }
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
  std::string type;        // 6  button type: "" (toggle), action, sensor, calendar, timezone, weather_forecast, slider, light_brightness, light_switch, fan_*, cover, garage, gate, lock, alarm, alarm_action, media, climate, push, webhook, todo, internal, subpage
  std::string precision;   // 7  decimal places for sensors; "text" = text sensor mode
  std::string options;     // 8  comma-delimited card options
};

inline bool brightness_slider_type(const std::string &type) {
  return card_runtime_brightness_slider_type(type);
}

inline bool fan_card_type(const std::string &type) {
  return card_runtime_fan_card_type(type);
}

inline const char *fan_card_default_icon_name(const std::string &type) {
  return card_runtime_fan_default_icon_name(type);
}

inline bool action_card_option_select_action(const std::string &action) {
  return card_runtime_option_select_action(action);
}

inline bool action_card_option_select(const ParsedCfg &p) {
  return p.type == "action" && action_card_option_select_action(p.sensor);
}

inline bool action_card_local_action(const ParsedCfg &p) {
  return p.type == "action" && p.sensor == "local";
}

inline bool sensor_card_local_sensor(const ParsedCfg &p) {
  return p.type == "sensor" && p.sensor == "local";
}

inline bool cfg_option_token_present(const std::string &options, const char *name) {
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

inline bool large_numbers_explicitly_disabled(const std::string &options) {
  return cfg_option_value(options, "large_numbers") == "off";
}

inline void append_large_numbers_option(std::string &out, const std::string &options) {
  std::string value;
  if (large_numbers_explicitly_disabled(options)) {
    value = "large_numbers=off";
  } else if (cfg_option_token_present(options, "large_numbers")) {
    value = "large_numbers";
  }
  if (value.empty()) return;
  if (!out.empty()) out += ",";
  out += value;
}

inline int normalize_media_volume_max_percent(const std::string &value) {
  if (value.empty()) return card_runtime_media_volume_max_default();
  char *end = nullptr;
  long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) return card_runtime_media_volume_max_default();
  if (parsed < card_runtime_media_volume_max_min()) return card_runtime_media_volume_max_min();
  if (parsed > card_runtime_media_volume_max_max()) return card_runtime_media_volume_max_max();
  return static_cast<int>(parsed);
}

inline std::string media_card_options_normalized(const std::string &options,
                                                 const std::string &mode) {
  if (mode == "control_modal") {
    std::string out;
    if (cfg_option_value(options, "label_display") == "label") {
      out = "label_display=label";
    }
    if (cfg_option_value(options, "number_display") == "volume") {
      if (!out.empty()) out += ",";
      out += "number_display=volume";
    }
    int max_pct = normalize_media_volume_max_percent(
      cfg_option_value(options, VOLUME_MAX_OPTION));
    if (max_pct < card_runtime_media_volume_max_default()) {
      if (!out.empty()) out += ",";
      out += std::string(VOLUME_MAX_OPTION) + "=" + std::to_string(max_pct);
    }
    return out;
  }
  if (mode == "playlist") {
    std::string out;
    std::string content_id = cfg_option_value(options, MEDIA_PLAYLIST_CONTENT_ID_OPTION);
    if (!content_id.empty()) {
      out = std::string(MEDIA_PLAYLIST_CONTENT_ID_OPTION) + "=" + encode_compact_field(content_id);
    }
    std::string content_type = cfg_option_value(options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION);
    if (content_type.empty()) content_type = "playlist";
    if (content_type != "playlist") {
      if (!out.empty()) out += ",";
      out += std::string(MEDIA_PLAYLIST_CONTENT_TYPE_OPTION) + "=" + encode_compact_field(content_type);
    }
    std::string player_source = cfg_option_value(options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION);
    if (!player_source.empty()) {
      if (!out.empty()) out += ",";
      out += std::string(MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION) + "=" + encode_compact_field(player_source);
    }
    return out;
  }
  if (mode == "now_playing") {
    return cfg_option_token_present(options, MEDIA_COVER_ART_OPTION)
      ? std::string(MEDIA_COVER_ART_OPTION)
      : "";
  }
  if (mode != "volume" && mode != "position") return "";
  std::string out;
  int max_pct = normalize_media_volume_max_percent(
    cfg_option_value(options, VOLUME_MAX_OPTION));
  if (mode == "volume" && max_pct < card_runtime_media_volume_max_default()) {
    out = std::string(VOLUME_MAX_OPTION) + "=" + std::to_string(max_pct);
  }
  append_large_numbers_option(out, options);
  return out;
}

inline std::string weather_card_options_normalized(const std::string &options,
                                                   const ParsedCfg &p) {
  if (!card_runtime_weather_forecast_precision(p.precision)) return "";
  std::string out;
  append_large_numbers_option(out, options);
  return out;
}

inline std::string normalize_image_refresh_interval(const std::string &value) {
  return value == "10" || value == "30" || value == "60" || value == "300"
    ? value
    : "off";
}

inline std::string normalize_image_refresh_mode(const std::string &value) {
  return value == "timer" ? "timer" : "changes_timer";
}

inline std::string normalize_image_modal_mode(const std::string &value) {
  return card_runtime_image_modal_mode(value);
}

inline std::string image_card_options_normalized(const std::string &options) {
  std::string out;
  if (cfg_option_token_present(options, IMAGE_LABEL_OPTION)) {
    out = IMAGE_LABEL_OPTION;
  }
  if (cfg_option_token_present(options, IMAGE_ICON_OPTION)) {
    if (!out.empty()) out += ",";
    out += IMAGE_ICON_OPTION;
  }
  std::string modal_mode = normalize_image_modal_mode(
    cfg_option_value(options, IMAGE_MODAL_MODE_OPTION));
  if (modal_mode != card_runtime_image_modal_mode_default()) {
    if (!out.empty()) out += ",";
    out += std::string(IMAGE_MODAL_MODE_OPTION) + "=" + modal_mode;
  }
  return out;
}

inline bool light_control_tab_token_valid(const std::string &value) {
  return card_runtime_light_control_tab_valid(value);
}

inline std::string normalize_light_control_tabs_value(const std::string &value) {
  std::vector<std::string> parts = split_config_fields(
    value.empty() ? std::string(card_runtime_light_control_tabs_default()) : value, '|');
  std::vector<std::string> tabs;
  for (const auto &part : parts) {
    if (!light_control_tab_token_valid(part)) continue;
    if (std::find(tabs.begin(), tabs.end(), part) == tabs.end()) {
      tabs.push_back(part);
    }
  }
  if (tabs.empty()) tabs.push_back("power");
  std::string out;
  for (const auto &tab : tabs) {
    if (!out.empty()) out += "|";
    out += tab;
  }
  return out;
}

inline std::string light_control_card_options_normalized(const std::string &options) {
  std::string tabs = normalize_light_control_tabs_value(
    cfg_option_value(options, LIGHT_CONTROL_TABS_OPTION));
  if (tabs == card_runtime_light_control_tabs_default()) return "";
  return std::string(LIGHT_CONTROL_TABS_OPTION) + "=" + encode_compact_field(tabs);
}

inline bool cover_control_tab_token_valid(const std::string &value) {
  return card_runtime_cover_control_tab_valid(value);
}

inline std::string normalize_cover_control_tabs_value(const std::string &value) {
  std::vector<std::string> parts = split_config_fields(
    value.empty() ? std::string(card_runtime_cover_control_tabs_default()) : value, '|');
  std::vector<std::string> tabs;
  for (const auto &part : parts) {
    if (!cover_control_tab_token_valid(part)) continue;
    if (std::find(tabs.begin(), tabs.end(), part) == tabs.end()) {
      tabs.push_back(part);
    }
  }
  if (tabs.empty()) tabs.push_back("position");
  std::string out;
  for (const auto &tab : tabs) {
    if (!out.empty()) out += "|";
    out += tab;
  }
  return out;
}

inline std::string cover_card_options_normalized(const std::string &options,
                                                 const std::string &mode) {
  if (!card_runtime_cover_modal_mode(mode)) return "";
  std::string tabs = normalize_cover_control_tabs_value(
    cfg_option_value(options, COVER_CONTROL_TABS_OPTION));
  if (tabs == card_runtime_cover_control_tabs_default()) return "";
  return std::string(COVER_CONTROL_TABS_OPTION) + "=" + encode_compact_field(tabs);
}

inline bool climate_control_tab_token_valid(const std::string &value) {
  return value == "temperature" || value == "mode" || value == "preset" ||
         value == "fan" || value == "swing";
}

inline std::string normalize_climate_control_tabs_value(const std::string &value) {
  std::vector<std::string> parts = split_config_fields(
    value.empty() ? std::string(CLIMATE_CONTROL_DEFAULT_TABS_VALUE) : value, '|');
  std::vector<std::string> tabs;
  for (const auto &part : parts) {
    if (!climate_control_tab_token_valid(part)) continue;
    if (std::find(tabs.begin(), tabs.end(), part) == tabs.end()) {
      tabs.push_back(part);
    }
  }
  if (tabs.empty()) tabs.push_back("temperature");
  std::string out;
  for (const auto &tab : tabs) {
    if (!out.empty()) out += "|";
    out += tab;
  }
  return out;
}

inline bool fan_control_tab_token_valid(const std::string &value) {
  return value == "power" || value == "speed" || value == "preset" ||
         value == "oscillation" || value == "direction";
}

inline std::string normalize_fan_control_tabs_value(const std::string &value) {
  std::vector<std::string> parts = split_config_fields(
    value.empty() ? std::string(FAN_CONTROL_DEFAULT_TABS_VALUE) : value, '|');
  std::vector<std::string> tabs;
  for (const auto &part : parts) {
    if (!fan_control_tab_token_valid(part)) continue;
    if (std::find(tabs.begin(), tabs.end(), part) == tabs.end()) {
      tabs.push_back(part);
    }
  }
  if (tabs.empty()) tabs.push_back("power");
  std::string out;
  for (const auto &tab : tabs) {
    if (!out.empty()) out += "|";
    out += tab;
  }
  return out;
}

inline std::string fan_control_card_options_normalized(const std::string &options) {
  std::string tabs = normalize_fan_control_tabs_value(
    cfg_option_value(options, FAN_CONTROL_TABS_OPTION));
  if (tabs == FAN_CONTROL_DEFAULT_TABS_VALUE) return "";
  return std::string(FAN_CONTROL_TABS_OPTION) + "=" + encode_compact_field(tabs);
}

inline uint32_t image_card_refresh_interval_ms(const ParsedCfg &p) {
  (void) p;
  return 0;
}

inline bool image_card_timer_only_refresh(const ParsedCfg &p) {
  (void) p;
  return false;
}

inline bool image_card_label_enabled(const ParsedCfg &p) {
  return cfg_option_token_present(p.options, IMAGE_LABEL_OPTION);
}

inline bool image_card_icon_enabled(const ParsedCfg &p) {
  return cfg_option_token_present(p.options, IMAGE_ICON_OPTION);
}

inline bool image_card_modal_fit_enabled(const ParsedCfg &p) {
  return normalize_image_modal_mode(
    cfg_option_value(p.options, IMAGE_MODAL_MODE_OPTION)) == "fit";
}

inline bool media_cover_art_enabled(const ParsedCfg &p) {
  return cfg_option_token_present(p.options, MEDIA_COVER_ART_OPTION);
}

inline std::string sensor_card_options_normalized(const std::string &options,
                                                  const std::string &precision) {
  std::string out;
  if (precision != "icon" && precision != "text" &&
      (cfg_option_token_present(options, "large_numbers") ||
       large_numbers_explicitly_disabled(options))) {
    append_large_numbers_option(out, options);
  }
  if (precision == "text" && cfg_option_token_present(options, SENSOR_STATE_LABELS_OPTION)) {
    if (!out.empty()) out += ",";
    out += SENSOR_STATE_LABELS_OPTION;
    std::string input = cfg_option_value(options, SENSOR_STATE_INPUT_OPTION);
    std::string output = cfg_option_value(options, SENSOR_STATE_OUTPUT_OPTION);
    if (input.empty() && !cfg_option_value(options, SENSOR_STATE_HIGH_LABEL_OPTION).empty()) {
      input = "high";
      output = cfg_option_value(options, SENSOR_STATE_HIGH_LABEL_OPTION);
    } else if (input.empty() && !cfg_option_value(options, SENSOR_STATE_LOW_LABEL_OPTION).empty()) {
      input = "low";
      output = cfg_option_value(options, SENSOR_STATE_LOW_LABEL_OPTION);
    }
    if (!input.empty()) {
      out += ",";
      out += std::string(SENSOR_STATE_INPUT_OPTION) + "=" + encode_compact_field(input);
    }
    if (!output.empty()) {
      out += ",";
      out += std::string(SENSOR_STATE_OUTPUT_OPTION) + "=" + encode_compact_field(output);
    }
    std::string input_2 = cfg_option_value(options, SENSOR_STATE_INPUT_2_OPTION);
    std::string output_2 = cfg_option_value(options, SENSOR_STATE_OUTPUT_2_OPTION);
    if (!input_2.empty()) {
      out += ",";
      out += std::string(SENSOR_STATE_INPUT_2_OPTION) + "=" + encode_compact_field(input_2);
    }
    if (!output_2.empty()) {
      out += ",";
      out += std::string(SENSOR_STATE_OUTPUT_2_OPTION) + "=" + encode_compact_field(output_2);
    }
  }
  return out;
}

inline std::string normalize_subpage_kind(const std::string &value) {
  return value == "lights" || value == "media" ||
    value == "climate" || value == "presence" ||
    value == "switch" || value == "alarm" ||
    value == "cover" || value == "garage" || value == "gate" ||
    value == "lock" || value == "vacuum" ||
    value == "lawn_mower" ||
    value == "weather" || value == "sensor" ||
    value == "image" ? value : "";
}

inline std::string subpage_card_options_normalized(const std::string &options,
                                                   const std::string &sensor,
                                                   const std::string &precision) {
  std::string out;
  std::string kind = normalize_subpage_kind(cfg_option_value(options, "subpage_kind"));
  if (!kind.empty()) out = "subpage_kind=" + kind;
  if (!sensor.empty() && sensor != "indicator" && precision != "text" &&
      (cfg_option_token_present(options, "large_numbers") ||
       large_numbers_explicitly_disabled(options))) {
    append_large_numbers_option(out, options);
  }
  return out;
}

inline std::string normalize_door_window_subtype(const std::string &value) {
  return value == "window" ? "window" : "door";
}

inline const char *door_window_closed_icon_name(const std::string &subtype) {
  return normalize_door_window_subtype(subtype) == "window" ? "Window Closed" : "Door";
}

inline const char *door_window_open_icon_name(const std::string &subtype) {
  return normalize_door_window_subtype(subtype) == "window" ? "Window Open" : "Door Open";
}

inline std::string door_window_card_options_normalized(const std::string &options) {
  return cfg_option_token_present(options, "active_color") ? "active_color" : "";
}

inline std::string presence_card_options_normalized(const std::string &options) {
  return cfg_option_token_present(options, "active_color") ? "active_color" : "";
}

inline std::string normalize_todo_count_display(const std::string &value) {
  return value == "icon" ? "icon" : "count";
}

inline std::string normalize_todo_label_display(const std::string &value) {
  (void) value;
  return "label";
}

inline std::string normalize_todo_completed_display(const std::string &value) {
  (void) value;
  return "hide";
}

inline std::string todo_card_options_normalized(const std::string &options) {
  bool show_count = normalize_todo_count_display(cfg_option_value(options, "count_display")) == "count";
  std::string out = show_count ? "" : "count_display=icon";
  if (show_count && (cfg_option_token_present(options, "large_numbers") ||
      large_numbers_explicitly_disabled(options))) {
    append_large_numbers_option(out, options);
  }
  return out;
}

inline bool todo_card_show_count(const ParsedCfg &p) {
  return normalize_todo_count_display(cfg_option_value(p.options, "count_display")) == "count";
}

inline bool todo_card_shows_top_task(const ParsedCfg &p) {
  (void) p;
  return false;
}

inline bool todo_card_label_shows_count(const ParsedCfg &p) {
  (void) p;
  return false;
}

inline bool todo_card_shows_completed_items(const ParsedCfg &p) {
  (void) p;
  return false;
}

inline std::string normalize_climate_label_display(const std::string &value) {
  return card_runtime_climate_label_display(value);
}

inline std::string normalize_climate_number_display(const std::string &value) {
  return card_runtime_climate_number_display(value);
}

inline std::string normalize_climate_temperature_step(const std::string &value) {
  return card_runtime_climate_temperature_step(value);
}

inline bool climate_card_type(const std::string &type) {
  return type == "climate" || type == "climate_control";
}

inline std::string sanitize_climate_range_value(const std::string &value) {
  const char *start = value.c_str();
  char *end = nullptr;
  double parsed = std::strtod(start, &end);
  if (end == start) return "";
  while (*end != '\0') {
    if (!std::isspace(static_cast<unsigned char>(*end))) return "";
    end++;
  }
  double rounded = std::floor(parsed * 10.0 + 0.5) / 10.0;
  char buffer[24];
  std::snprintf(buffer, sizeof(buffer), "%.1f", rounded);
  std::string out(buffer);
  if (out.size() > 2 && out.substr(out.size() - 2) == ".0") {
    out.erase(out.size() - 2);
  }
  return out;
}

inline std::string normalize_climate_precision_config(const std::string &value) {
  std::vector<std::string> parts = split_config_fields(value, ':');
  std::string precision = parts.empty() ? "" : parts[0];
  if (precision == "0") precision.clear();
  if (!card_runtime_climate_precision_valid(precision)) precision.clear();
  std::string min = parts.size() > 1 ? sanitize_climate_range_value(parts[1]) : "";
  std::string max = parts.size() > 2 ? sanitize_climate_range_value(parts[2]) : "";
  if (min.empty() && max.empty()) return precision;
  return (precision.empty() ? std::string("0") : precision) + ":" + min + ":" + max;
}

inline std::string climate_card_options_normalized(const std::string &options,
                                                   bool include_control_tabs = false) {
  std::string label_display = normalize_climate_label_display(cfg_option_value(options, LABEL_DISPLAY_OPTION));
  std::string number_display = normalize_climate_number_display(cfg_option_value(options, NUMBER_DISPLAY_OPTION));
  std::string temperature_step = normalize_climate_temperature_step(cfg_option_value(options, TEMPERATURE_STEP_OPTION));
  std::string out;
  if (label_display != card_runtime_climate_label_display_default()) {
    out += std::string(LABEL_DISPLAY_OPTION) + "=" + label_display;
  }
  if (number_display != card_runtime_climate_number_display_default()) {
    if (!out.empty()) out += ",";
    out += std::string(NUMBER_DISPLAY_OPTION) + "=" + number_display;
  }
  if (temperature_step != card_runtime_climate_temperature_step_default()) {
    if (!out.empty()) out += ",";
    out += std::string(TEMPERATURE_STEP_OPTION) + "=" + temperature_step;
  }
  if (number_display != "icon" &&
      (cfg_option_token_present(options, "large_numbers") ||
       large_numbers_explicitly_disabled(options))) {
    append_large_numbers_option(out, options);
  }
  if (include_control_tabs) {
    std::string tabs = normalize_climate_control_tabs_value(
      cfg_option_value(options, CLIMATE_CONTROL_TABS_OPTION));
    if (tabs != CLIMATE_CONTROL_DEFAULT_TABS_VALUE) {
      if (!out.empty()) out += ",";
      out += std::string(CLIMATE_CONTROL_TABS_OPTION) + "=" + encode_compact_field(tabs);
    }
  }
  return out;
}

inline bool action_card_large_numbers_supported(const ParsedCfg &p) {
  if (p.type != "action" || action_card_local_action(p)) return false;
  std::string precision = cfg_option_value(p.options, "state_precision");
  return precision == "0" || precision == "1" || precision == "2" ||
         !cfg_option_value(p.options, "state_unit").empty();
}

inline bool card_large_numbers_supported(const ParsedCfg &p) {
  if (p.type.empty()) return !p.sensor.empty() && p.precision != "text";
  if (p.type == "action") return action_card_large_numbers_supported(p);
  if (sensor_card_local_sensor(p)) return false;
  if (p.type == "media") return p.sensor == "volume" || p.sensor == "position";
  if (climate_card_type(p.type)) {
    return normalize_climate_number_display(cfg_option_value(p.options, "number_display")) != "icon";
  }
  if (p.type == "todo") {
    return normalize_todo_count_display(cfg_option_value(p.options, "count_display")) == "count";
  }
  if (p.type == "subpage") return !p.sensor.empty() && p.sensor != "indicator" && p.precision != "text";
  return card_runtime_large_numbers_supported(p.type, p.precision);
}

inline std::string date_time_card_options_normalized(const std::string &options,
                                                     const ParsedCfg &p) {
  if (!card_large_numbers_supported(p)) return "";
  if (cfg_option_token_present(options, "large_numbers") ||
      large_numbers_explicitly_disabled(options)) {
    std::string out;
    append_large_numbers_option(out, options);
    return out;
  }
  return "";
}

inline std::string normalize_garage_label_display(const std::string &value) {
  return card_runtime_garage_label_display(value);
}

inline std::string garage_card_options_normalized(const std::string &options,
                                                  const std::string &sensor) {
  (void)sensor;
  return normalize_garage_label_display(cfg_option_value(options, LABEL_DISPLAY_OPTION)) == "status"
    ? std::string(LABEL_DISPLAY_OPTION) + "=status"
    : "";
}

inline std::string normalize_gate_label_display(const std::string &value) {
  return card_runtime_gate_label_display(value);
}

inline std::string gate_card_options_normalized(const std::string &options,
                                                 const std::string &sensor) {
  (void)sensor;
  return normalize_gate_label_display(cfg_option_value(options, LABEL_DISPLAY_OPTION)) == "status"
    ? std::string(LABEL_DISPLAY_OPTION) + "=status"
    : "";
}

inline bool alarm_action_mode_valid(const std::string &mode) {
  return card_runtime_alarm_action_mode_valid(mode);
}

inline const char *alarm_action_icon_name(const std::string &mode) {
  return card_runtime_alarm_action_icon_name(mode);
}

inline bool alarm_action_legacy_icon_name(const std::string &mode, const std::string &icon) {
  return card_runtime_alarm_action_legacy_icon_name(mode, icon);
}

inline std::string normalize_alarm_icon_display(const std::string &value) {
  return card_runtime_alarm_icon_display(value);
}

inline std::string normalize_alarm_label_display(const std::string &value) {
  return card_runtime_alarm_label_display(value);
}

inline bool alarm_action_list_is_default(const std::vector<std::string> &actions) {
  if (actions.size() != card_runtime_alarm_default_action_count()) return false;
  for (size_t i = 0; i < actions.size(); i++) {
    if (actions[i] != card_runtime_alarm_default_action_at(i)) return false;
  }
  return true;
}

inline std::string alarm_card_options_normalized(const std::string &options) {
  std::string out;
  if (cfg_option_value(options, "pin_arm") == "0") out = "pin_arm=0";
  if (cfg_option_value(options, "pin_disarm") == "0") {
    if (!out.empty()) out += ",";
    out += "pin_disarm=0";
  }
  std::string actions = cfg_option_value(options, "actions");
  if (!actions.empty()) {
    std::vector<std::string> filtered;
    size_t start = 0;
    while (start <= actions.length()) {
      size_t end = actions.find('|', start);
      if (end == std::string::npos) end = actions.length();
      std::string action = actions.substr(start, end - start);
      if (alarm_action_mode_valid(action) &&
          std::find(filtered.begin(), filtered.end(), action) == filtered.end()) {
        filtered.push_back(action);
        if (filtered.size() >= card_runtime_alarm_max_visible_actions()) break;
      }
      start = end + 1;
    }
    if (!filtered.empty() && !alarm_action_list_is_default(filtered)) {
      std::string joined;
      for (const auto &action : filtered) {
        if (!joined.empty()) joined += "|";
        joined += action;
      }
      if (!out.empty()) out += ",";
      out += "actions=" + encode_compact_field(joined);
    }
  }
  std::string icon_display = normalize_alarm_icon_display(
    cfg_option_value(options, "icon_display"));
  if (icon_display != "status") {
    if (!out.empty()) out += ",";
    out += "icon_display=" + icon_display;
  }
  std::string label_display = normalize_alarm_label_display(
    cfg_option_value(options, "label_display"));
  if (label_display != "status") {
    if (!out.empty()) out += ",";
    out += "label_display=" + label_display;
  }
  return out;
}

inline std::string normalize_webhook_method(const std::string &value) {
  std::string method;
  method.reserve(value.size());
  for (char ch : value) {
    method.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  if (method == "POST" || method == "PUT" || method == "PATCH" ||
      method == "DELETE") return method;
  return "GET";
}

inline std::string webhook_card_options_normalized(const std::string &options) {
  std::string headers = cfg_option_value(options, "webhook_headers");
  return headers.empty() ? std::string() : "webhook_headers=" + encode_compact_field(headers);
}

inline std::string normalize_card_on_pattern(const std::string &value) {
  return value == "stripes" ? std::string("stripes") : std::string();
}

inline std::string switch_card_options_normalized(const std::string &options) {
  std::string out;
  append_large_numbers_option(out, options);
  std::string pattern = normalize_card_on_pattern(cfg_option_value(options, "on_pattern"));
  if (!pattern.empty()) {
    if (!out.empty()) out += ",";
    out += "on_pattern=" + pattern;
  }
  if (cfg_option_token_present(options, "confirm_off")) {
    if (!out.empty()) out += ",";
    out += "confirm_off";
  }
  if (cfg_option_token_present(options, "confirm_on")) {
    if (!out.empty()) out += ",";
    out += "confirm_on";
  }
  std::string mode;
  if (cfg_option_token_present(options, "confirm_off") &&
      cfg_option_token_present(options, "confirm_on")) {
    mode = "both";
  } else if (cfg_option_token_present(options, "confirm_on")) {
    mode = "on";
  } else if (cfg_option_token_present(options, "confirm_off")) {
    mode = "off";
  }
  if (!mode.empty()) {
    std::string message = cfg_option_value(options, "confirm_message");
    std::string yes = cfg_option_value(options, "confirm_yes");
    std::string no = cfg_option_value(options, "confirm_no");
    std::string default_message = mode == "on" ? "Turn on this device?"
      : mode == "both" ? "Toggle this device?"
      : "Turn off this device?";
    if (!message.empty() && message != default_message) {
      if (!out.empty()) out += ",";
      out += "confirm_message=" + encode_compact_field(message);
    }
    if (!yes.empty() && yes != "Yes") {
      if (!out.empty()) out += ",";
      out += "confirm_yes=" + encode_compact_field(yes);
    }
    if (!no.empty() && no != "No") {
      if (!out.empty()) out += ",";
      out += "confirm_no=" + encode_compact_field(no);
    }
  }
  return out;
}

inline void append_config_token(std::string &out, const std::string &token) {
  if (token.empty()) return;
  if (!out.empty()) out += ",";
  out += token;
}

inline std::string action_card_options_normalized(const std::string &options,
                                                  const std::string &action) {
  std::string out;
  std::string state_entity = cfg_option_value(options, "state_entity");
  if (!state_entity.empty()) {
    append_config_token(out, "state_entity=" + encode_compact_field(state_entity));
    std::string state_precision = cfg_option_value(options, "state_precision");
    if (state_precision == "icon" || state_precision == "text") {
      append_config_token(out, "state_precision=" + state_precision);
    } else {
      std::string state_unit = cfg_option_value(options, "state_unit");
      if (!state_unit.empty()) {
        append_config_token(out, "state_unit=" + encode_compact_field(state_unit));
      }
      if (state_precision == "1" || state_precision == "2") {
        append_config_token(out, "state_precision=" + state_precision);
      }
      append_large_numbers_option(out, options);
    }
  }

  if (action == "script.turn_on") {
    std::string fields = cfg_option_value(options, "script_fields");
    if (!fields.empty()) {
      append_config_token(out, "script_fields=" + encode_compact_field(fields));
    }
  }

  if (action == "script.turn_on" && cfg_option_token_present(options, "confirm_on")) {
    append_config_token(out, "confirm_on");
    std::string message = cfg_option_value(options, "confirm_message");
    std::string yes = cfg_option_value(options, "confirm_yes");
    std::string no = cfg_option_value(options, "confirm_no");
    if (!message.empty() && message != "Run this script?") {
      append_config_token(out, "confirm_message=" + encode_compact_field(message));
    }
    if (!yes.empty() && yes != "Yes") {
      append_config_token(out, "confirm_yes=" + encode_compact_field(yes));
    }
    if (!no.empty() && no != "No") {
      append_config_token(out, "confirm_no=" + encode_compact_field(no));
    }
  }
  return out;
}

inline ParsedCfg normalize_parsed_cfg(ParsedCfg p) {
  if (p.type == "local") {
    p.type = "action";
    p.sensor = "local";
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto" || p.icon == "Flash") p.icon = "Gesture Tap";
  }
  if (p.type == "local_sensor") {
    p.type = "sensor";
    p.sensor = "local";
    p.icon_on = "Auto";
    p.options.clear();
    if (p.precision != "text" && p.precision != "1" && p.precision != "2") p.precision.clear();
    if (p.precision != "text" && (p.icon.empty() || p.icon == "Auto")) p.icon = "Auto";
  }
  // Slider cards used to store "h" here for horizontal layout. Sliders are
  // now always vertical, so treat any saved slider sensor value as legacy.
  if (brightness_slider_type(p.type) && !p.sensor.empty()) p.sensor.clear();
  if (fan_card_type(p.type)) {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options = p.type == "fan_control" ? fan_control_card_options_normalized(p.options) : "";
    if (p.icon.empty() || p.icon == "Auto") p.icon = fan_card_default_icon_name(p.type);
    if (p.type == "fan_switch") {
      if (p.icon_on.empty() || p.icon_on == "Auto") p.icon_on = "Fan";
    } else {
      p.icon_on.clear();
    }
  }
  if (p.type == "weather_forecast") {
    p.type = "weather";
    p.precision = "tomorrow";
    if (p.label == "Weather") p.label.clear();
  }
  if (p.type == "weather" && !card_runtime_weather_forecast_precision(p.precision)) {
    p.precision.clear();
  }
  if (p.type == "weather") {
    p.sensor.clear();
    p.options = weather_card_options_normalized(p.options, p);
  }
  if (p.type == "media") {
    if (p.sensor == "controls") {
      if (p.icon.empty() || p.icon == "Speaker") p.icon = "Auto";
      p.sensor = card_runtime_media_mode(p.sensor);
    } else if (p.sensor.empty()) {
      p.sensor = card_runtime_media_mode(p.sensor);
    } else {
      p.sensor = card_runtime_media_mode(p.sensor);
    }
    if (p.sensor == "previous" && p.label == "Skip Previous") p.label = "Previous";
    if (p.sensor == "next" && p.label == "Skip Next") p.label = "Next";
    if (p.sensor == "volume") {
      if (p.label.empty() || p.label == "Media") p.label = "Volume";
      p.icon = "Auto";
    }
    if (p.sensor == "playlist") {
      if (p.label.empty() || p.label == "Media") p.label = "Playlist";
      if (p.icon.empty() || p.icon == "Auto") p.icon = "Music";
    }
    if (p.sensor == "position" && (p.label.empty() || p.label == "Track")) p.label = "Position";
    if (p.sensor == "now_playing") {
      p.precision = card_runtime_media_now_playing_control(p.precision) ? p.precision : "";
    } else if (card_runtime_media_state_display_mode(p.sensor) && p.precision == "state") {
      p.precision = "state";
    } else {
      p.precision.clear();
    }
    p.options = media_card_options_normalized(p.options, p.sensor);
  }
  if (climate_card_type(p.type)) {
    p.type = "climate_control";
    p.sensor.clear();
    p.unit.clear();
    if (p.icon.empty()) p.icon = "Thermostat";
    p.precision = normalize_climate_precision_config(p.precision);
    p.options = climate_card_options_normalized(p.options, true);
  }
  if (p.type == "garage") {
    if (!card_runtime_garage_mode_valid(p.sensor)) p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    if (!p.sensor.empty()) p.icon_on = "Auto";
    p.options = garage_card_options_normalized(p.options, p.sensor);
  }
  if (p.type == "gate") {
    if (!card_runtime_gate_mode_valid(p.sensor)) p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    if (!p.sensor.empty()) p.icon_on = "Auto";
    p.options = gate_card_options_normalized(p.options, p.sensor);
  }
  if (p.type == "lock") {
    if (!card_runtime_lock_mode_valid(p.sensor)) p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    if (!p.sensor.empty()) {
      p.icon_on = "Auto";
    } else if (p.icon_on.empty() || p.icon_on == "Auto") {
      p.icon_on = "Lock Open";
    }
  }
  if (p.type == "cover") {
    if (!card_runtime_cover_mode_valid(p.sensor)) p.sensor.clear();
    p.precision.clear();
    if (p.sensor != "set_position") p.unit.clear();
    p.options = cover_card_options_normalized(p.options, p.sensor);
  }
  if (p.type == "alarm") {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Security";
    p.options = alarm_card_options_normalized(p.options);
  }
  if (p.type == "alarm_action") {
    if (!alarm_action_mode_valid(p.sensor)) p.sensor = "away";
    p.unit.clear();
    p.precision.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto" || alarm_action_legacy_icon_name(p.sensor, p.icon)) {
      p.icon = alarm_action_icon_name(p.sensor);
    }
    p.options = alarm_card_options_normalized(p.options);
  }
  if (p.type == "webhook") {
    p.sensor = normalize_webhook_method(p.sensor);
    if (p.sensor == "GET" || p.sensor == "DELETE") p.unit.clear();
    p.precision.clear();
    p.icon_on = "Auto";
    if (p.icon.empty()) p.icon = "Auto";
    p.options = webhook_card_options_normalized(p.options);
  }
  if (p.type == "image") {
    p.icon_on = "Auto";
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options = image_card_options_normalized(p.options);
    p.icon = image_card_icon_enabled(p)
      ? (p.icon.empty() || p.icon == "Auto" ? "Camera" : p.icon)
      : "Auto";
    if (!image_card_label_enabled(p)) p.label.clear();
  }
  if (p.type == "screen_lock") {
    p.entity.clear();
    p.label.clear();
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon = "Lock";
    p.icon_on = "Lock Open";
  }
  if (p.type == "calendar") {
    if (p.entity.empty()) p.entity = "sensor.date";
    p.label.clear();
    p.icon = "Auto";
    p.icon_on = "Auto";
    p.sensor.clear();
    p.unit.clear();
    if (p.precision != "datetime") p.precision.clear();
    p.options = date_time_card_options_normalized(p.options, p);
  }
  if (p.type == "clock") {
    p.entity.clear();
    p.label.clear();
    p.icon = "Auto";
    p.icon_on = "Auto";
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options = date_time_card_options_normalized(p.options, p);
  }
  if (p.type == "timezone") {
    if (p.entity.empty()) p.entity = "UTC (GMT+0)";
    p.label.clear();
    p.icon = "Auto";
    p.icon_on = "Auto";
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options = date_time_card_options_normalized(p.options, p);
  }
  if (p.type == "todo") {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Check";
    p.options = todo_card_options_normalized(p.options);
  }
  if (p.type == "light_switch") {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
  }
  if (p.type == "light_control") {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options = light_control_card_options_normalized(p.options);
  }
  if (p.type == "subpage") {
    p.options = subpage_card_options_normalized(p.options, p.sensor, p.precision);
  }
  if (p.type == "option_select") {
    p.type = "action";
    p.sensor = card_runtime_option_select_canonical_action();
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on.clear();
    if (p.icon.empty() || p.icon == "Auto" || p.icon == "Chevron Down") p.icon = "Flash";
  }
  if (action_card_option_select(p)) {
    p.sensor = card_runtime_option_select_canonical_action();
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on.clear();
    if (p.icon.empty() || p.icon == "Auto" || p.icon == "Chevron Down") p.icon = "Flash";
  }
  if (action_card_local_action(p)) {
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto" || p.icon == "Flash") p.icon = "Gesture Tap";
  }
  if (p.type == "action" && p.sensor == "vacuum.start") {
    p.type = "vacuum";
    p.sensor = "start_stop";
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Robot Vacuum";
  }
  if (p.type == "action" && p.sensor == "vacuum.return_to_base") {
    p.type = "vacuum";
    p.sensor = "dock";
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Robot Vacuum Variant";
  }
  if (p.type == "action") {
    p.precision.clear();
    p.options = action_card_options_normalized(p.options, p.sensor);
  }
  if (p.type == "vacuum") {
    p.sensor = card_runtime_vacuum_mode(p.sensor);
    if (p.sensor != "clean_area") p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto") p.icon = card_runtime_vacuum_default_icon_name(p.sensor);
  }
  if (p.type == "lawn_mower") {
    p.sensor = card_runtime_lawn_mower_mode(p.sensor);
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto") p.icon = card_runtime_lawn_mower_default_icon_name(p.sensor);
  }
  if (p.type.empty()) {
    p.options = switch_card_options_normalized(p.options);
  }
  if (p.type == "door_window") {
    p.entity.clear();
    p.unit.clear();
    p.precision = normalize_door_window_subtype(p.precision);
    if (p.icon.empty() || p.icon == "Auto") p.icon = door_window_closed_icon_name(p.precision);
    if (p.icon_on.empty() || p.icon_on == "Auto") p.icon_on = door_window_open_icon_name(p.precision);
    p.options = door_window_card_options_normalized(p.options);
  }
  if (p.type == "presence") {
    p.entity.clear();
    p.unit.clear();
    p.precision.clear();
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Motion Sensor Off";
    if (p.icon_on.empty() || p.icon_on == "Auto") p.icon_on = "Motion Sensor";
    p.options = presence_card_options_normalized(p.options);
  }
  if (!p.type.empty() && p.type != "action" && p.type != "alarm" && p.type != "alarm_action" && !climate_card_type(p.type) && p.type != "cover" && p.type != "garage" && p.type != "gate" && p.type != "webhook" && p.type != "screen_lock" && p.type != "todo" && p.type != "sensor" && p.type != "door_window" && p.type != "presence" && p.type != "media" && p.type != "subpage" && p.type != "image" && p.type != "light_control" && p.type != "vacuum" && p.type != "lawn_mower" && !fan_card_type(p.type) && !card_large_numbers_supported(p)) {
    p.options.clear();
  }
  if (sensor_card_local_sensor(p)) {
    p.icon_on = "Auto";
    p.options.clear();
    if (p.precision != "text" && p.precision != "1" && p.precision != "2") p.precision.clear();
    if (p.precision != "text" && (p.icon.empty() || p.icon == "Auto")) p.icon = "Auto";
  } else if (p.type == "sensor") {
    p.options = sensor_card_options_normalized(p.options, p.precision);
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
  return cfg_option_token_present(options, name);
}

inline int media_volume_max_percent(const ParsedCfg &p) {
  return p.type == "media" && (p.sensor == "volume" || p.sensor == "control_modal")
    ? normalize_media_volume_max_percent(cfg_option_value(p.options, VOLUME_MAX_OPTION))
    : card_runtime_media_volume_max_default();
}

inline bool media_control_card_show_status_label(const ParsedCfg &p) {
  return p.type == "media" && p.sensor == "control_modal" &&
         cfg_option_value(p.options, "label_display") != "label";
}

inline bool media_control_card_show_volume_number(const ParsedCfg &p) {
  return p.type == "media" && p.sensor == "control_modal" &&
         cfg_option_value(p.options, "number_display") == "volume";
}

inline std::string action_card_state_entity(const ParsedCfg &p) {
  if (action_card_local_action(p)) return "";
  return p.type == "action" ? cfg_option_value(p.options, "state_entity") : "";
}

inline std::string action_card_state_unit(const ParsedCfg &p) {
  if (action_card_local_action(p)) return "";
  return p.type == "action" ? cfg_option_value(p.options, "state_unit") : "";
}

inline std::string action_card_state_precision(const ParsedCfg &p) {
  if (action_card_local_action(p)) return "";
  return p.type == "action" ? cfg_option_value(p.options, "state_precision") : "";
}

inline bool action_card_state_display_enabled(const ParsedCfg &p) {
  if (action_card_state_entity(p).empty()) return false;
  std::string precision = action_card_state_precision(p);
  return precision == "icon" || precision == "text" || precision == "0" ||
         precision == "1" || precision == "2" ||
         !action_card_state_unit(p).empty();
}

inline bool action_card_state_icon_mode(const ParsedCfg &p) {
  return action_card_state_display_enabled(p) &&
         action_card_state_precision(p) == "icon";
}

inline bool action_card_state_text_mode(const ParsedCfg &p) {
  return action_card_state_display_enabled(p) &&
         action_card_state_precision(p) == "text";
}

inline std::string webhook_card_headers(const ParsedCfg &p) {
  return p.type == "webhook" ? cfg_option_value(p.options, "webhook_headers") : "";
}

inline bool action_card_state_numeric_mode(const ParsedCfg &p) {
  return action_card_state_display_enabled(p) &&
         action_card_state_precision(p) != "icon" &&
         action_card_state_precision(p) != "text";
}

inline bool card_large_numbers_enabled(const ParsedCfg &p) {
  return card_large_numbers_supported(p) && cfg_option_enabled(p.options, "large_numbers");
}

inline bool card_large_numbers_disabled(const ParsedCfg &p) {
  return card_large_numbers_supported(p) && large_numbers_explicitly_disabled(p.options);
}

inline bool sensor_large_numbers_enabled(const ParsedCfg &p) {
  return card_large_numbers_enabled(p);
}

inline bool sensor_active_color_enabled(const ParsedCfg &p) {
  return p.type == "sensor" && cfg_option_enabled(p.options, "active_color");
}

inline bool sensor_state_labels_enabled(const ParsedCfg &p) {
  return p.type == "sensor" && p.precision == "text" &&
         cfg_option_enabled(p.options, SENSOR_STATE_LABELS_OPTION);
}

inline bool door_window_active_color_enabled(const ParsedCfg &p) {
  return p.type == "door_window" && cfg_option_enabled(p.options, "active_color");
}

inline bool presence_active_color_enabled(const ParsedCfg &p) {
  return p.type == "presence" && cfg_option_enabled(p.options, "active_color");
}

inline bool switch_confirmation_enabled(const ParsedCfg &p) {
  return p.type.empty() &&
         (cfg_option_enabled(p.options, "confirm_off") ||
          cfg_option_enabled(p.options, "confirm_on"));
}

inline bool action_script_confirmation_enabled(const ParsedCfg &p) {
  return p.type == "action" && p.sensor == "script.turn_on" &&
         cfg_option_enabled(p.options, "confirm_on");
}

inline std::string action_script_fields(const ParsedCfg &p) {
  return p.type == "action" && p.sensor == "script.turn_on"
    ? cfg_option_value(p.options, "script_fields")
    : "";
}

inline bool switch_confirmation_required(const ParsedCfg &p, bool currently_on) {
  if (p.type.empty()) {
    return currently_on
      ? cfg_option_enabled(p.options, "confirm_off")
      : cfg_option_enabled(p.options, "confirm_on");
  }
  return false;
}

inline std::string switch_confirmation_default_message(const ParsedCfg &p) {
  if (action_script_confirmation_enabled(p)) {
    return espcontrol_i18n(std::string("Run this script?"));
  }
  bool confirm_off = cfg_option_enabled(p.options, "confirm_off");
  bool confirm_on = cfg_option_enabled(p.options, "confirm_on");
  if (confirm_off && confirm_on) return espcontrol_i18n(std::string("Toggle this device?"));
  if (confirm_on) return espcontrol_i18n(std::string("Turn on this device?"));
  return espcontrol_i18n(std::string("Turn off this device?"));
}

inline std::string switch_confirmation_message(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_message");
  return value.empty() ? switch_confirmation_default_message(p) : value;
}

inline std::string switch_confirmation_yes_text(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_yes");
  return value.empty() ? espcontrol_i18n(std::string("Yes")) : value;
}

inline std::string switch_confirmation_no_text(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_no");
  return value.empty() ? espcontrol_i18n(std::string("No")) : value;
}

inline int parse_precision(const std::string &s) {
  if (s.empty()) return 0;
  int v = atoi(s.c_str());
  return (v < 0) ? 0 : (v > 3) ? 3 : v;
}

inline int clamp_percent_value(int pct) {
  if (pct < 0) return 0;
  if (pct > 100) return 100;
  return pct;
}

inline bool light_brightness_to_percent(float brightness, int &pct) {
  if (!std::isfinite(brightness)) return false;
  if (brightness <= 0.0f) {
    pct = 0;
    return true;
  }
  pct = clamp_percent_value((int)((brightness * 100.0f + 127.0f) / 255.0f));
  if (pct < 1) pct = 1;
  return true;
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
  if (sensor_card_local_sensor(p)) return false;
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

inline std::string normalized_state_text(esphome::StringRef value,
                                         size_t max_len = HA_SHORT_STATE_MAX_LEN) {
  std::string text = trim_display_unit(string_ref_limited(value, max_len));
  for (char &ch : text) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return text;
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
    if (ch == '-' && !out.empty() && out.back() != '\n' && out.back() != ' ') {
      out.push_back(ch);
      cap_next = true;
      last_space = false;
      continue;
    }
    if (ch == '_' || std::isspace(c)) {
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

inline std::string sensor_state_translation_key(const std::string &value) {
  std::string text = trim_display_unit(value);
  for (char &ch : text) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return text;
}

inline std::string sensor_state_display_text(const ParsedCfg &p,
                                             esphome::StringRef value,
                                             size_t max_len = HA_TEXT_SENSOR_STATE_MAX_LEN) {
  if (sensor_state_labels_enabled(p)) {
    std::string state = normalized_state_text(value, max_len);
    std::string input = cfg_option_value(p.options, SENSOR_STATE_INPUT_OPTION);
    std::string output = cfg_option_value(p.options, SENSOR_STATE_OUTPUT_OPTION);
    if (input.empty() && !cfg_option_value(p.options, SENSOR_STATE_HIGH_LABEL_OPTION).empty()) {
      input = "high";
      output = cfg_option_value(p.options, SENSOR_STATE_HIGH_LABEL_OPTION);
    } else if (input.empty() && !cfg_option_value(p.options, SENSOR_STATE_LOW_LABEL_OPTION).empty()) {
      input = "low";
      output = cfg_option_value(p.options, SENSOR_STATE_LOW_LABEL_OPTION);
    }
    if (!input.empty() && state == sensor_state_translation_key(input)) {
      return output;
    }
    input = cfg_option_value(p.options, SENSOR_STATE_INPUT_2_OPTION);
    output = cfg_option_value(p.options, SENSOR_STATE_OUTPUT_2_OPTION);
    if (!input.empty() && state == sensor_state_translation_key(input)) {
      return output;
    }
  }
  return text_sensor_display_text(value, max_len);
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

inline bool numeric_state_positive_ref(esphome::StringRef state) {
  float value = 0.0f;
  return parse_float_ref(state, value) && std::isfinite(value) && value > 0.0f;
}

inline bool is_entity_on_ref(esphome::StringRef state) {
  std::string value = normalized_state_text(state);
  return value == "on" || value == "true" || value == "1" ||
         value == "home" || value == "playing" ||
         value == "open" || value == "opened" ||
         value == "opening" || value == "closing" ||
         value == "unlocked" || value == "unlocking" || value == "jammed";
}

inline bool presence_detected_ref(esphome::StringRef state) {
  std::string value = normalized_state_text(state);
  return value == "detected" || is_entity_on_ref(state);
}

inline bool ha_state_unavailable_ref(esphome::StringRef state) {
  std::string value = normalized_state_text(state);
  return value.empty() || value == "unavailable" || value == "unknown";
}

inline bool ha_entity_accepts_unknown_state(const std::string &entity_id) {
  return (entity_id.size() > 7 && entity_id.compare(0, 7, "button.") == 0) ||
         (entity_id.size() > 13 && entity_id.compare(0, 13, "input_button.") == 0);
}

inline bool ha_entity_state_unavailable_ref(const std::string &entity_id,
                                            esphome::StringRef state) {
  std::string value = normalized_state_text(state);
  if (value.empty() || value == "unavailable") return true;
  if (value == "unknown") return !ha_entity_accepts_unknown_state(entity_id);
  return false;
}

#ifndef ESPCONTROL_HA_DEFERRED_HELPERS_DEFINED
inline void ha_reset_deferred_state_requests() {}
#endif

#ifndef ESPCONTROL_HA_SUBSCRIPTION_HELPERS_DEFINED
inline void ha_reset_subscription_callbacks(uint32_t scope = 0) { (void) scope; }
#endif

inline uint32_t &ha_subscription_generation() {
  static uint32_t generation = 1;
  return generation;
}

inline void bump_ha_subscription_generation() {
  uint32_t &generation = ha_subscription_generation();
  generation++;
  if (generation == 0) generation = 1;
  ha_reset_deferred_state_requests();
  ha_reset_subscription_callbacks(HA_SUBSCRIPTION_SCOPE_DEFAULT);
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

#include "button_grid_weather_forecast.h"

struct ClimateControlCtx;
inline ClimateControlCtx **climate_control_refs();
inline int &climate_control_ref_count();
inline void climate_update_card(ClimateControlCtx *ctx);
inline void climate_control_set_modal_value(ClimateControlCtx *ctx);

inline void refresh_temperature_unit_labels() {
  ClimateControlCtx **climate_refs = climate_control_refs();
  int climate_count = climate_control_ref_count();
  for (int i = 0; i < climate_count; i++) {
    if (!climate_refs[i]) continue;
    climate_update_card(climate_refs[i]);
    climate_control_set_modal_value(climate_refs[i]);
  }
  refresh_weather_forecast_card_visuals();
  if (climate_count > 0) notify_dashboard_content_changed();
}

#include "button_grid_access_cards.h"

#include "button_grid_local_controls.h"
