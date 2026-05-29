#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.
#ifdef USE_WEBSERVER
#include <esp_http_server.h>
#include "esphome/components/web_server_idf/web_server_idf.h"
#endif

// RGB multipliers for display calibration; 100 leaves a channel unchanged.
constexpr int COLOR_CORRECTION_RED_PERCENT = 100;
constexpr int COLOR_CORRECTION_GREEN_PERCENT = 100;
constexpr int COLOR_CORRECTION_BLUE_PERCENT = 100;

constexpr uint32_t clamp_color_channel(uint32_t value) {
  return value > 255 ? 255 : value;
}

constexpr uint32_t correct_display_color(
    uint32_t rgb, int red_percent, int green_percent, int blue_percent) {
  uint32_t red = clamp_color_channel(((rgb >> 16) & 0xFF) * red_percent / 100);
  uint32_t green = clamp_color_channel(((rgb >> 8) & 0xFF) * green_percent / 100);
  uint32_t blue = clamp_color_channel((rgb & 0xFF) * blue_percent / 100);
  return (red << 16) | (green << 8) | blue;
}

constexpr uint32_t correct_display_color(uint32_t rgb) {
  return correct_display_color(
    rgb, COLOR_CORRECTION_RED_PERCENT, COLOR_CORRECTION_GREEN_PERCENT,
    COLOR_CORRECTION_BLUE_PERCENT);
}

static_assert(correct_display_color(0x123456, 100, 100, 100) == 0x123456,
              "neutral colour correction must not change RGB values");
static_assert(correct_display_color(0x123456, 0, 100, 100) == 0x003456,
              "red correction must only adjust the red channel");
static_assert(correct_display_color(0x123456, 100, 0, 100) == 0x120056,
              "green correction must only adjust the green channel");
static_assert(correct_display_color(0x123456, 100, 100, 0) == 0x123400,
              "blue correction must only adjust the blue channel");
static_assert(correct_display_color(0xF0F0F0, 200, 200, 200) == 0xFFFFFF,
              "colour correction must clamp channels at 255");

constexpr uint32_t DEFAULT_SLIDER_COLOR = correct_display_color(0xFF8C00);
constexpr uint32_t DEFAULT_OFF_COLOR = correct_display_color(0x313131);
constexpr uint32_t DEFAULT_TERTIARY_COLOR = correct_display_color(0x212121);
constexpr uint32_t DARK_BACKGROUND_SECONDARY = DEFAULT_OFF_COLOR;
constexpr uint32_t DARK_BACKGROUND_TERTIARY = DEFAULT_TERTIARY_COLOR;
constexpr uint32_t DARK_TEXT_PRIMARY = 0xFFFFFF;
constexpr uint32_t DARK_TEXT_MUTED = 0xA0A0A0;
constexpr uint32_t DARK_TEXT_SOFT = 0xE8E8E8;
constexpr uint32_t DARK_BORDER = correct_display_color(0x454545);
constexpr uint32_t DARK_CONTROL_NEUTRAL = correct_display_color(0xBDBDBD);
constexpr uint32_t DARK_OVERLAY = 0x000000;
constexpr uint32_t DARK_TRACK_BACKGROUND = correct_display_color(0x333333);
constexpr int MAX_GRID_SLOTS = 25;
constexpr int MAX_SUBPAGE_ITEMS = MAX_GRID_SLOTS * MAX_GRID_SLOTS;
constexpr const char *SENSOR_STATE_LABELS_OPTION = "state_labels";
constexpr const char *SENSOR_STATE_INPUT_OPTION = "state_input";
constexpr const char *SENSOR_STATE_OUTPUT_OPTION = "state_output";
constexpr const char *SENSOR_STATE_LOW_LABEL_OPTION = "state_low_label";
constexpr const char *SENSOR_STATE_HIGH_LABEL_OPTION = "state_high_label";

#include "button_grid_contract_generated.h"
#include "button_grid_card_runtime.h"
#include <cstdlib>

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
  std::string type;        // 6  button type: "" (toggle), action, sensor, calendar, timezone, weather_forecast, slider, light_brightness, light_switch, fan_*, cover, garage, lock, alarm, alarm_action, media, climate, push, webhook, todo, internal, subpage
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

inline int normalize_media_volume_max_percent(const std::string &value) {
  if (value.empty()) return 100;
  char *end = nullptr;
  long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) return 100;
  if (parsed < 1) return 1;
  if (parsed > 100) return 100;
  return static_cast<int>(parsed);
}

inline std::string media_card_options_normalized(const std::string &options,
                                                 const std::string &mode) {
  if (mode != "volume" && mode != "position") return "";
  std::string out;
  int max_pct = normalize_media_volume_max_percent(
    cfg_option_value(options, "volume_max"));
  if (mode == "volume" && max_pct < 100) {
    out = "volume_max=" + std::to_string(max_pct);
  }
  if (cfg_option_token_present(options, "large_numbers")) {
    if (!out.empty()) out += ",";
    out += "large_numbers";
  }
  return out;
}

inline std::string sensor_card_options_normalized(const std::string &options,
                                                  const std::string &precision) {
  std::string out;
  if (precision != "text" && cfg_option_token_present(options, "large_numbers")) {
    out = "large_numbers";
  }
  if (cfg_option_token_present(options, "active_color")) {
    if (!out.empty()) out += ",";
    out += "active_color";
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
  }
  return out;
}

inline std::string normalize_subpage_kind(const std::string &value) {
  return value == "lights" || value == "media" ||
    value == "climate" || value == "presence" ? value : "";
}

inline std::string subpage_card_options_normalized(const std::string &options,
                                                   const std::string &sensor,
                                                   const std::string &precision) {
  std::string out;
  std::string kind = normalize_subpage_kind(cfg_option_value(options, "subpage_kind"));
  if (!kind.empty()) out = "subpage_kind=" + kind;
  if (!sensor.empty() && sensor != "indicator" && precision != "text" &&
      cfg_option_token_present(options, "large_numbers")) {
    if (!out.empty()) out += ",";
    out += "large_numbers";
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
  if (show_count && cfg_option_token_present(options, "large_numbers")) {
    out = "large_numbers";
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

inline std::string climate_card_options_normalized(const std::string &options) {
  std::string label_display = normalize_climate_label_display(cfg_option_value(options, "label_display"));
  std::string number_display = normalize_climate_number_display(cfg_option_value(options, "number_display"));
  std::string out;
  if (label_display != "label") out += "label_display=" + label_display;
  if (number_display != "target") {
    if (!out.empty()) out += ",";
    out += "number_display=" + number_display;
  }
  if (number_display != "icon" && cfg_option_token_present(options, "large_numbers")) {
    if (!out.empty()) out += ",";
    out += "large_numbers";
  }
  return out;
}

inline bool action_card_large_numbers_supported(const ParsedCfg &p) {
  if (p.type != "action") return false;
  std::string precision = cfg_option_value(p.options, "state_precision");
  return precision == "0" || precision == "1" || precision == "2" ||
         !cfg_option_value(p.options, "state_unit").empty();
}

inline bool card_large_numbers_supported(const ParsedCfg &p) {
  if (p.type.empty()) return !p.sensor.empty() && p.precision != "text";
  if (p.type == "action") return action_card_large_numbers_supported(p);
  if (p.type == "media") return p.sensor == "volume" || p.sensor == "position";
  if (p.type == "climate") {
    return normalize_climate_number_display(cfg_option_value(p.options, "number_display")) != "icon";
  }
  if (p.type == "todo") {
    return normalize_todo_count_display(cfg_option_value(p.options, "count_display")) == "count";
  }
  if (p.type == "subpage") return !p.sensor.empty() && p.sensor != "indicator" && p.precision != "text";
  return card_runtime_large_numbers_supported(p.type, p.precision);
}

inline std::string normalize_garage_label_display(const std::string &value) {
  return card_runtime_garage_label_display(value);
}

inline std::string garage_card_options_normalized(const std::string &options,
                                                  const std::string &sensor) {
  if (sensor == "open" || sensor == "close") return "";
  return normalize_garage_label_display(cfg_option_value(options, "label_display")) == "status"
    ? "label_display=status"
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

inline std::string alarm_card_options_normalized(const std::string &options) {
  std::string out;
  if (cfg_option_value(options, "pin_arm") == "0") out = "pin_arm=0";
  if (cfg_option_value(options, "pin_disarm") == "0") {
    if (!out.empty()) out += ",";
    out += "pin_disarm=0";
  }
  std::string actions = cfg_option_value(options, "actions");
  if (!actions.empty()) {
    std::string filtered;
    bool saw_valid = false;
    size_t start = 0;
    while (start <= actions.length()) {
      size_t end = actions.find('|', start);
      if (end == std::string::npos) end = actions.length();
      std::string action = actions.substr(start, end - start);
      if (alarm_action_mode_valid(action)) {
        if (!filtered.empty()) filtered += "|";
        filtered += action;
        saw_valid = true;
      }
      start = end + 1;
    }
    if (saw_valid && filtered != "away|home|disarm") {
      if (!out.empty()) out += ",";
      out += "actions=" + filtered;
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

inline ParsedCfg normalize_parsed_cfg(ParsedCfg p) {
  // Slider cards used to store "h" here for horizontal layout. Sliders are
  // now always vertical, so treat any saved slider sensor value as legacy.
  if (brightness_slider_type(p.type) && !p.sensor.empty()) p.sensor.clear();
  if (fan_card_type(p.type)) {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
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
  if (p.type == "climate") {
    p.sensor.clear();
    p.unit.clear();
    if (p.icon.empty()) p.icon = "Thermostat";
    p.options = climate_card_options_normalized(p.options);
  }
  if (p.type == "garage") {
    if (!card_runtime_garage_mode_valid(p.sensor)) p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    if (!p.sensor.empty()) p.icon_on.clear();
    p.options = garage_card_options_normalized(p.options, p.sensor);
  }
  if (p.type == "alarm") {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.icon_on.clear();
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Security";
    p.options = alarm_card_options_normalized(p.options);
  }
  if (p.type == "alarm_action") {
    if (!alarm_action_mode_valid(p.sensor)) p.sensor = "away";
    p.unit.clear();
    p.precision.clear();
    p.icon_on.clear();
    if (p.icon.empty() || p.icon == "Auto" || alarm_action_legacy_icon_name(p.sensor, p.icon)) {
      p.icon = alarm_action_icon_name(p.sensor);
    }
    p.options = alarm_card_options_normalized(p.options);
  }
  if (p.type == "webhook") {
    p.sensor = normalize_webhook_method(p.sensor);
    if (p.sensor == "GET" || p.sensor == "DELETE") p.unit.clear();
    p.precision.clear();
    p.icon_on.clear();
    if (p.icon.empty()) p.icon = "Auto";
    p.options = webhook_card_options_normalized(p.options);
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
  if (!p.type.empty() && p.type != "action" && p.type != "alarm" && p.type != "alarm_action" && p.type != "climate" && p.type != "garage" && p.type != "webhook" && p.type != "todo" && p.type != "sensor" && p.type != "door_window" && p.type != "presence" && p.type != "media" && p.type != "subpage" && !fan_card_type(p.type) && !card_large_numbers_supported(p)) {
    p.options.clear();
  }
  if (p.type == "sensor") {
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
  return p.type == "media" && p.sensor == "volume"
    ? normalize_media_volume_max_percent(cfg_option_value(p.options, "volume_max"))
    : 100;
}

inline std::string action_card_state_entity(const ParsedCfg &p) {
  return p.type == "action" ? cfg_option_value(p.options, "state_entity") : "";
}

inline std::string action_card_state_unit(const ParsedCfg &p) {
  return p.type == "action" ? cfg_option_value(p.options, "state_unit") : "";
}

inline std::string action_card_state_precision(const ParsedCfg &p) {
  return p.type == "action" ? cfg_option_value(p.options, "state_precision") : "";
}

inline bool action_card_state_display_enabled(const ParsedCfg &p) {
  if (action_card_state_entity(p).empty()) return false;
  std::string precision = action_card_state_precision(p);
  return precision == "text" || precision == "0" ||
         precision == "1" || precision == "2" ||
         !action_card_state_unit(p).empty();
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
         action_card_state_precision(p) != "text";
}

inline bool card_large_numbers_enabled(const ParsedCfg &p) {
  return card_large_numbers_supported(p) && cfg_option_enabled(p.options, "large_numbers");
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

inline bool switch_confirmation_required(const ParsedCfg &p, bool currently_on) {
  if (p.type.empty()) {
    return currently_on
      ? cfg_option_enabled(p.options, "confirm_off")
      : cfg_option_enabled(p.options, "confirm_on");
  }
  return false;
}

inline std::string switch_confirmation_default_message(const ParsedCfg &p) {
  bool confirm_off = cfg_option_enabled(p.options, "confirm_off");
  bool confirm_on = cfg_option_enabled(p.options, "confirm_on");
  if (confirm_off && confirm_on) return "Toggle this device?";
  if (confirm_on) return "Turn on this device?";
  return "Turn off this device?";
}

inline std::string switch_confirmation_message(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_message");
  return value.empty() ? switch_confirmation_default_message(p) : value;
}

inline std::string switch_confirmation_yes_text(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_yes");
  return value.empty() ? std::string("Yes") : value;
}

inline std::string switch_confirmation_no_text(const ParsedCfg &p) {
  std::string value = cfg_option_value(p.options, "confirm_no");
  return value.empty() ? std::string("No") : value;
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
constexpr int WEATHER_FORECAST_PENDING_MAX = 8;
constexpr uint32_t WEATHER_FORECAST_REQUEST_TIMEOUT_MS = 60000;

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

inline void apply_weather_forecast_unavailable_for_entity(const std::string &entity_id) {
  WeatherForecastCardRef *refs = weather_forecast_card_refs();
  int count = weather_forecast_card_count();
  for (int i = 0; i < count; i++) {
    if (refs[i].entity_id == entity_id) {
      refs[i].valid = false;
      refs[i].high = 0;
      refs[i].low = 0;
      refs[i].source_unit = "";
      apply_weather_forecast_card_text(refs[i], false, 0, 0, "");
    }
  }
}

inline bool weather_forecast_request_matches(const std::string &entity_id,
                                             const std::string &day,
                                             const std::string &other_entity_id,
                                             const std::string &other_day) {
  return entity_id == other_entity_id && day == other_day;
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

struct WeatherForecastPayload {
  bool today_valid = false;
  int today_high = WEATHER_FORECAST_TEMP_MISSING;
  int today_low = WEATHER_FORECAST_TEMP_MISSING;
  bool tomorrow_valid = false;
  int tomorrow_high = WEATHER_FORECAST_TEMP_MISSING;
  int tomorrow_low = WEATHER_FORECAST_TEMP_MISSING;
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

  std::string today_high_text = payload.substr(0, p1);
  std::string today_low_text = payload.substr(p1 + 1, p2 - p1 - 1);
  std::string tomorrow_high_text = payload.substr(p2 + 1, p3 - p2 - 1);
  std::string tomorrow_low_text = payload.substr(p3 + 1, p4 - p3 - 1);
  out.unit = payload.substr(p4 + 1);

  bool today_has_high = parse_weather_forecast_temp(today_high_text, out.today_high);
  bool today_has_low = parse_weather_forecast_temp(today_low_text, out.today_low);
  bool tomorrow_has_high = parse_weather_forecast_temp(tomorrow_high_text, out.tomorrow_high);
  bool tomorrow_has_low = parse_weather_forecast_temp(tomorrow_low_text, out.tomorrow_low);
  out.today_valid = today_has_high || today_has_low;
  out.tomorrow_valid = tomorrow_has_high || tomorrow_has_low;
  return out.today_valid || out.tomorrow_valid;
}

inline std::string weather_forecast_response_template(const std::string &entity_id) {
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set forecasts = response.get(entity, {}).get('forecast', []) %}"
    "{% set today_date = now().date().isoformat() %}"
    "{% set tomorrow_date = (now().date() + timedelta(days=1)).isoformat() %}"
    "{% set ns = namespace(today=none, tomorrow=none) %}"
    "{% for item in forecasts %}"
    "{% if item.datetime is defined and item.datetime[:10] == today_date and ns.today is none %}"
    "{% set ns.today = item %}{% endif %}"
    "{% if item.datetime is defined and item.datetime[:10] == tomorrow_date and ns.tomorrow is none %}"
    "{% set ns.tomorrow = item %}{% endif %}"
    "{% endfor %}"
    "{% set today = ns.today if ns.today is not none else (forecasts[0] if forecasts|length > 0 else none) %}"
    "{% set tomorrow = ns.tomorrow if ns.tomorrow is not none else (forecasts[1] if forecasts|length > 1 else (forecasts[0] if forecasts|length > 0 else none)) %}"
    "{% set today_high = today.temperature if today is not none and today.temperature is defined else (today.temperature_high if today is not none and today.temperature_high is defined else (today.high_temperature if today is not none and today.high_temperature is defined else (today.high if today is not none and today.high is defined else ''))) %}"
    "{% set today_low = today.templow if today is not none and today.templow is defined else (today.temperature_low if today is not none and today.temperature_low is defined else (today.low_temperature if today is not none and today.low_temperature is defined else (today.low if today is not none and today.low is defined else ''))) %}"
    "{% set tomorrow_high = tomorrow.temperature if tomorrow is not none and tomorrow.temperature is defined else (tomorrow.temperature_high if tomorrow is not none and tomorrow.temperature_high is defined else (tomorrow.high_temperature if tomorrow is not none and tomorrow.high_temperature is defined else (tomorrow.high if tomorrow is not none and tomorrow.high is defined else ''))) %}"
    "{% set tomorrow_low = tomorrow.templow if tomorrow is not none and tomorrow.templow is defined else (tomorrow.temperature_low if tomorrow is not none and tomorrow.temperature_low is defined else (tomorrow.low_temperature if tomorrow is not none and tomorrow.low_temperature is defined else (tomorrow.low if tomorrow is not none and tomorrow.low is defined else ''))) %}"
    "{{ today_high }}|{{ today_low }}|{{ tomorrow_high }}|{{ tomorrow_low }}|"
    "{{ state_attr(entity, 'temperature_unit') or '' }}";
}

inline uint32_t next_weather_forecast_call_id() {
  static uint32_t call_id = 100000;
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

inline bool weather_forecast_enqueue(const std::string &entity_id,
                                     const std::string &day) {
  if (!weather_forecast_entity_id_safe(entity_id)) return false;
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

inline void weather_forecast_send_next_queued();

inline void weather_forecast_cancel_pending_requests() {
  weather_forecast_clear_queue();
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
  if (!weather_forecast_entity_id_safe(entity_id) || !ha_api_state_connected()) {
    apply_weather_forecast_unavailable_for_entity(entity_id);
    return;
  }

  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_weather_forecast_call_id();
  if (!ha_action_begin(req, "weather.get_forecasts", false, 2, call_id)) {
    apply_weather_forecast_unavailable_for_entity(entity_id);
    return;
  }
  req.wants_response = true;
  std::string response_template = weather_forecast_response_template(entity_id);
  req.response_template = decltype(req.response_template)(response_template);
  ha_action_add_entity(req, entity_id);
  ha_action_add_data(req, "type", "daily");

  if (!ha_register_action_response_callback(
    req.call_id,
    [entity_id, call_id = req.call_id](const esphome::api::ActionResponse &response) {
      weather_forecast_clear_pending(call_id);
      if (!response.is_success()) {
        ESP_LOGW("weather_forecast", "Forecast request failed for %s: %s",
          entity_id.c_str(), response.get_error_message().c_str());
        apply_weather_forecast_unavailable_for_entity(entity_id);
        weather_forecast_send_next_queued();
        return;
      }
      auto json = response.get_json();
      const char *payload = json["response"].as<const char *>();
      if (payload == nullptr) {
        apply_weather_forecast_unavailable_for_entity(entity_id);
        weather_forecast_send_next_queued();
        return;
      }
      WeatherForecastPayload forecast;
      bool valid = parse_weather_forecast_payload(payload, forecast);
      if (!valid) {
        ESP_LOGW("weather_forecast", "No usable forecast temperatures for %s", entity_id.c_str());
      }
      apply_weather_forecast_to_entity(entity_id, "today", forecast.today_valid,
        forecast.today_high, forecast.today_low, forecast.unit);
      apply_weather_forecast_to_entity(entity_id, "tomorrow", forecast.tomorrow_valid,
        forecast.tomorrow_high, forecast.tomorrow_low, forecast.unit);
      weather_forecast_send_next_queued();
    })) {
    apply_weather_forecast_unavailable_for_entity(entity_id);
    return;
  }
  if (!weather_forecast_track_pending(req.call_id, entity_id, day)) {
    ha_cancel_action_response_callback(req.call_id, "too many pending forecasts");
    apply_weather_forecast_unavailable_for_entity(entity_id);
    return;
  }
  if (!ha_action_send(req)) {
    weather_forecast_clear_pending(req.call_id);
    ha_cancel_action_response_callback(req.call_id, "send failed");
    weather_forecast_send_next_queued();
  }
}

inline void weather_forecast_send_next_queued() {
  if (!ha_api_state_connected() || weather_forecast_any_pending()) return;
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
  return card_runtime_garage_command_mode(sensor);
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

inline bool garage_card_show_status(const ParsedCfg &p) {
  return !garage_command_mode(p.sensor) &&
    normalize_garage_label_display(cfg_option_value(p.options, "label_display")) == "status";
}

inline bool alarm_card_show_status_icon(const ParsedCfg &p) {
  return normalize_alarm_icon_display(cfg_option_value(p.options, "icon_display")) == "status";
}

inline bool alarm_card_show_status_label(const ParsedCfg &p) {
  return normalize_alarm_label_display(cfg_option_value(p.options, "label_display")) == "status";
}

inline const char* lock_locked_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Lock") : find_icon(icon.c_str());
}

inline const char* lock_unlocked_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Lock Open") : find_icon(icon_on.c_str());
}

inline bool lock_command_mode(const std::string &sensor) {
  return card_runtime_lock_command_mode(sensor);
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

// ── Local action controls ─────────────────────────────────────────────
//
// Devices register named one-shot callbacks here at boot. The button type
// "local" dispatches to these by key, so device-specific addons (e.g. BLE
// keyboard) can be triggered from the grid without going through HA.

struct LocalActionControl {
  std::string key;
  std::string label;
  std::function<void()> action;
};

inline std::vector<LocalActionControl> &local_action_registry() {
  static std::vector<LocalActionControl> actions;
  return actions;
}

inline void register_local_action(
    const std::string &key, const std::string &label,
    std::function<void()> action) {
  if (key.empty()) return;
  LocalActionControl a;
  a.key = key;
  a.label = label;
  a.action = action;
  auto &reg = local_action_registry();
  for (auto &existing : reg) {
    if (existing.key == key) {
      existing = a;
      return;
    }
  }
  reg.push_back(a);
}

inline void send_local_action(const std::string &key) {
  for (auto &a : local_action_registry()) {
    if (a.key == key) {
      if (a.action) a.action();
      return;
    }
  }
  ESP_LOGW("espcontrol", "Local action '%s' not registered", key.c_str());
}

#ifdef USE_WEBSERVER
class LocalActionHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    esphome::StringRef url = request->url_to(url_buf);
    return strncmp(url.c_str(), "/local_actions", 14) == 0;
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    std::string json;
    json.reserve(256);
    json = "[";
    bool first = true;
    for (auto &a : local_action_registry()) {
      if (!first) json += ",";
      first = false;
      json += "{\"key\":\"" + json_escape(a.key) + "\",\"label\":\"" + json_escape(a.label) + "\"}";
    }
    json += "]";
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
  }

 private:
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
};

inline void register_local_action_endpoint() {
  static bool registered = false;
  if (registered) return;
  auto *server = esphome::web_server_idf::global_async_web_server();
  if (!server) {
    ESP_LOGW("espcontrol", "register_local_action_endpoint: server not ready");
    return;
  }
  server->addHandler(new LocalActionHandler());
  registered = true;
  ESP_LOGI("espcontrol", "Local action endpoint registered");
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
  return card_runtime_internal_push_mode(p.sensor);
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
