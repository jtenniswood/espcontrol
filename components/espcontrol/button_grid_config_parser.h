#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include "button_grid_card_runtime.h"
#include "button_grid_string.h"
#include "button_grid_saved_config_action_generated.h"
#include "button_grid_saved_config_access_generated.h"
#include "button_grid_saved_config_security_generated.h"
#include "button_grid_saved_config_weather_generated.h"
#include "button_grid_saved_config_image_generated.h"
#include "button_grid_saved_config_climate_generated.h"
#include "button_grid_saved_config_light_control_generated.h"
#include "button_grid_saved_config_webhook_generated.h"
#include "button_grid_saved_config_subpage_generated.h"
#include "button_grid_saved_config_switch_generated.h"
#include "button_grid_saved_config_date_time_generated.h"
#include "button_grid_saved_config_fan_generated.h"
#include "button_grid_saved_config_media_generated.h"
#include "button_grid_saved_config_mower_generated.h"
#include "button_grid_saved_config_occupancy_generated.h"
#include "button_grid_saved_config_sensor_generated.h"
#include "button_grid_saved_config_static_generated.h"
#include "button_grid_saved_config_vacuum_generated.h"

constexpr const char *SENSOR_STATE_LABELS_OPTION = card_runtime_option_name_state_labels();
constexpr const char *SENSOR_STATE_INPUT_OPTION = card_runtime_option_name_state_input();
constexpr const char *SENSOR_STATE_OUTPUT_OPTION = card_runtime_option_name_state_output();
constexpr const char *SENSOR_STATE_INPUT_2_OPTION = card_runtime_option_name_state_input_2();
constexpr const char *SENSOR_STATE_OUTPUT_2_OPTION = card_runtime_option_name_state_output_2();
constexpr const char *SENSOR_STATE_LOW_LABEL_OPTION = card_runtime_option_name_state_low_label();
constexpr const char *SENSOR_STATE_HIGH_LABEL_OPTION = card_runtime_option_name_state_high_label();
constexpr const char *SENSOR_TIME_UNIT_OPTION = card_runtime_option_name_time_unit();
constexpr const char *IMAGE_LABEL_OPTION = card_runtime_option_name_image_label();
constexpr const char *IMAGE_ICON_OPTION = card_runtime_option_name_image_icon();
constexpr const char *IMAGE_MODAL_MODE_OPTION = card_runtime_option_name_image_modal_mode();
constexpr const char *MEDIA_COVER_ART_OPTION = card_runtime_option_name_media_cover_art();
constexpr const char *MEDIA_COVER_ART_ACTION_OPTION = card_runtime_option_name_cover_art_action();
constexpr const char *MEDIA_COVER_ART_DETAILS_OPTION = card_runtime_option_name_cover_art_details();
constexpr const char *MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION = card_runtime_option_name_cover_art_secondary_entity();
constexpr const char *LIGHT_CONTROL_TABS_OPTION = card_runtime_option_name_light_tabs();
constexpr const char *LIGHT_CONTROL_DEFAULT_TABS_VALUE = "power|brightness|temperature|color";
constexpr const char *COVER_CONTROL_TABS_OPTION = card_runtime_option_name_cover_tabs();
constexpr const char *CLIMATE_CONTROL_TABS_OPTION = "climate_tabs";
constexpr const char *CLIMATE_CONTROL_DEFAULT_TABS_VALUE = "temperature|mode|preset|fan|swing";
constexpr const char *FAN_CONTROL_TABS_OPTION = "fan_tabs";
constexpr const char *FAN_CONTROL_DEFAULT_TABS_VALUE = "power|speed|preset|oscillation|direction";
constexpr const char *LABEL_DISPLAY_OPTION = card_runtime_option_name_label_display();
constexpr const char *NUMBER_DISPLAY_OPTION = card_runtime_option_name_number_display();
constexpr const char *TEMPERATURE_STEP_OPTION = card_runtime_option_name_temperature_step();
constexpr const char *VOLUME_MAX_OPTION = card_runtime_option_name_volume_max();
constexpr const char *MEDIA_SPEAKER_GROUP_ENTITY_OPTION = card_runtime_option_name_speaker_group_entity();
constexpr const char *MEDIA_PLAYLIST_CONTENT_ID_OPTION = card_runtime_option_name_playlist_content_id();
constexpr const char *MEDIA_PLAYLIST_CONTENT_TYPE_OPTION = card_runtime_option_name_playlist_content_type();
constexpr const char *MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION = card_runtime_option_name_playlist_player_source();
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

inline bool valid_utf8_bytes(const std::string &value) {
  size_t index = 0;
  while (index < value.size()) {
    const unsigned char first = static_cast<unsigned char>(value[index]);
    if (first <= 0x7F) { ++index; continue; }
    size_t count = 0;
    if (first >= 0xC2 && first <= 0xDF) count = 1;
    else if (first >= 0xE0 && first <= 0xEF) count = 2;
    else if (first >= 0xF0 && first <= 0xF4) count = 3;
    else return false;
    if (index + count >= value.size()) return false;
    const unsigned char second = static_cast<unsigned char>(value[index + 1]);
    if ((second & 0xC0) != 0x80) return false;
    if (first == 0xE0 && second < 0xA0) return false;
    if (first == 0xED && second > 0x9F) return false;
    if (first == 0xF0 && second < 0x90) return false;
    if (first == 0xF4 && second > 0x8F) return false;
    for (size_t offset = 2; offset <= count; ++offset) {
      if ((static_cast<unsigned char>(value[index + offset]) & 0xC0) != 0x80) return false;
    }
    index += count + 1;
  }
  return true;
}

inline std::string decode_compact_field(const std::string &value, size_t start, size_t len) {
  if (start > value.size()) return "";
  size_t end = start + len;
  if (end < start || end > value.size()) end = value.size();
  std::string out;
  out.reserve(end - start);
  for (size_t i = start; i < end;) {
    if (value[i] == '%' && i + 2 < end) {
      size_t run_end = i;
      std::string decoded;
      while (run_end + 2 < end && value[run_end] == '%') {
        int hi = hex_digit(value[run_end + 1]);
        int lo = hex_digit(value[run_end + 2]);
        if (hi < 0 || lo < 0) break;
        decoded.push_back(static_cast<char>((hi << 4) | lo));
        run_end += 3;
      }
      if (run_end > i) {
        if (valid_utf8_bytes(decoded)) out += decoded;
        else out.append(value, i, run_end - i);
        i = run_end;
        continue;
      }
    }
    out.push_back(value[i]);
    ++i;
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

inline void append_confirm_options(std::string &out, const std::string &options,
                                   const std::string &default_off_message,
                                   const std::string &default_on_message,
                                   const std::string &default_both_message,
                                   const std::string &requested_mode = "") {
  std::string stored_mode;
  if (cfg_option_token_present(options, "confirm_off") &&
      cfg_option_token_present(options, "confirm_on")) {
    stored_mode = "both";
  } else if (cfg_option_token_present(options, "confirm_on")) {
    stored_mode = "on";
  } else if (cfg_option_token_present(options, "confirm_off")) {
    stored_mode = "off";
  }
  if (stored_mode.empty()) return;
  std::string mode = requested_mode == "on" || requested_mode == "off"
    ? requested_mode
    : stored_mode;
  if (mode == "off" || mode == "both") {
    if (!out.empty()) out += ",";
    out += "confirm_off";
  }
  if (mode == "on" || mode == "both") {
    if (!out.empty()) out += ",";
    out += "confirm_on";
  }
  std::string message = cfg_option_value(options, "confirm_message");
  std::string yes = cfg_option_value(options, "confirm_yes");
  std::string no = cfg_option_value(options, "confirm_no");
  std::string default_message = mode == "on" ? default_on_message
    : mode == "both" ? default_both_message
    : default_off_message;
  std::string stored_default_message = stored_mode == "on" ? default_on_message
    : stored_mode == "both" ? default_both_message
    : default_off_message;
  if (!message.empty() && message != default_message && message != stored_default_message) {
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

inline int normalize_media_volume_max_percent(const std::string &value) {
  if (value.empty()) return card_runtime_media_volume_max_default();
  char *end = nullptr;
  long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) return card_runtime_media_volume_max_default();
  if (parsed < card_runtime_media_volume_max_min()) return card_runtime_media_volume_max_min();
  if (parsed > card_runtime_media_volume_max_max()) return card_runtime_media_volume_max_max();
  return static_cast<int>(parsed);
}

inline std::string trim_saved_option_value(const std::string &value) {
  const size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

#include "button_grid_media_config.h"

inline std::string media_card_options_normalized(const std::string &options,
                                                 const std::string &mode) {
  if (mode == "control_modal") {
    std::string out;
    std::string speaker_group_entity = trim_saved_option_value(
      cfg_option_value(options, MEDIA_SPEAKER_GROUP_ENTITY_OPTION));
    if (!speaker_group_entity.empty()) {
      out = std::string(MEDIA_SPEAKER_GROUP_ENTITY_OPTION) + "=" +
        encode_compact_field(speaker_group_entity);
    }
    if (cfg_option_value(options, "label_display") == "label") {
      if (!out.empty()) out += ",";
      out += "label_display=label";
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
  if (mode == "speaker_group") {
    std::string out;
    std::string speaker_group_entity = trim_saved_option_value(
      cfg_option_value(options, MEDIA_SPEAKER_GROUP_ENTITY_OPTION));
    if (!speaker_group_entity.empty()) {
      out = std::string(MEDIA_SPEAKER_GROUP_ENTITY_OPTION) + "=" +
        encode_compact_field(speaker_group_entity);
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
    std::string content_id = trim_saved_option_value(cfg_option_value(options, MEDIA_PLAYLIST_CONTENT_ID_OPTION));
    if (!content_id.empty()) {
      out = std::string(MEDIA_PLAYLIST_CONTENT_ID_OPTION) + "=" + encode_compact_field(content_id);
    }
    std::string content_type = trim_saved_option_value(cfg_option_value(options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION));
    if (content_type.empty()) content_type = "playlist";
    if (content_type != "playlist") {
      if (!out.empty()) out += ",";
      out += std::string(MEDIA_PLAYLIST_CONTENT_TYPE_OPTION) + "=" + encode_compact_field(content_type);
    }
    std::string player_source = trim_saved_option_value(cfg_option_value(options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION));
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
  if (mode == "cover_art") {
    std::string out;
    if (cfg_option_value(options, MEDIA_COVER_ART_ACTION_OPTION) == "control_modal") {
      out = std::string(MEDIA_COVER_ART_ACTION_OPTION) + "=control_modal";
    }
    if (cfg_option_token_present(options, MEDIA_COVER_ART_DETAILS_OPTION)) {
      if (!out.empty()) out += ",";
      out += MEDIA_COVER_ART_DETAILS_OPTION;
    }
    std::string secondary_entity = trim_saved_option_value(
      cfg_option_value(options, MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION));
    if (!secondary_entity.empty()) {
      if (!out.empty()) out += ",";
      out += std::string(MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION) + "=" +
             encode_compact_field(secondary_entity);
    }
    return out;
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

inline void normalize_saved_config_media_fields(ParsedCfg &p) {
  const std::string raw_mode = p.sensor;
  if (raw_mode == "controls" && (p.icon.empty() || p.icon == "Speaker")) p.icon = "Auto";
  p.sensor = card_runtime_media_mode(p.sensor);
  if (p.sensor == "now_playing" && cfg_option_token_present(p.options, MEDIA_COVER_ART_OPTION)) {
    p.sensor = "cover_art";
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
  } else if (p.sensor == "cover_art") {
    p.precision.clear();
  } else if (card_runtime_media_state_display_mode(p.sensor) && p.precision == "state") {
    p.precision = "state";
  } else {
    p.precision.clear();
  }
}

inline std::string weather_card_options_normalized(const std::string &options,
                                                   const ParsedCfg &p) {
  if (!card_runtime_weather_forecast_precision(p.precision)) return "";
  std::string out;
  append_large_numbers_option(out, options);
  return out;
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
  return espcontrol::media::decode_config_v1(p).mode == espcontrol::media::Mode::COVER_ART;
}

inline std::string media_cover_art_press_action(const ParsedCfg &p) {
  return espcontrol::media::decode_config_v1(p).cover_art_action ==
             espcontrol::media::CoverArtAction::CONTROL_MODAL
    ? "control_modal"
    : "play_pause";
}

inline bool media_cover_art_details_enabled(const ParsedCfg &p) {
  return espcontrol::media::decode_config_v1(p).show_track_details;
}

inline std::string media_cover_art_secondary_entity(const ParsedCfg &p) {
  return espcontrol::media::decode_config_v1(p).secondary_entity;
}

inline std::string sensor_card_options_normalized(const std::string &options,
                                                  const std::string &precision) {
  std::string out;
  if (precision != "icon" && precision != "text" && precision != "time" &&
      (cfg_option_token_present(options, "large_numbers") ||
       large_numbers_explicitly_disabled(options))) {
    append_large_numbers_option(out, options);
  }
  if (precision != "time" && cfg_option_token_present(options, "active_color")) {
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
      if (output.empty()) output = cfg_option_value(options, SENSOR_STATE_HIGH_LABEL_OPTION);
    } else if (input.empty() && !cfg_option_value(options, SENSOR_STATE_LOW_LABEL_OPTION).empty()) {
      input = "low";
      if (output.empty()) output = cfg_option_value(options, SENSOR_STATE_LOW_LABEL_OPTION);
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
  if (precision == "time") {
    std::string time_unit = cfg_option_value(options, SENSOR_TIME_UNIT_OPTION);
    if (time_unit == "seconds" || time_unit == "minutes" ||
        time_unit == "hours" || time_unit == "days") {
      if (!out.empty()) out += ",";
      out += std::string(SENSOR_TIME_UNIT_OPTION) + "=" + time_unit;
    }
  }
  return out;
}

inline void normalize_saved_config_sensor_fields(ParsedCfg &p,
                                                 bool was_legacy_text_sensor) {
  if (was_legacy_text_sensor && p.icon.empty()) p.icon = "Auto";
  if (!sensor_card_local_sensor(p) && p.precision == "time") {
    p.unit.clear();
    p.icon = "Auto";
    p.icon_on = "Auto";
  }
  if (!sensor_card_local_sensor(p)) return;
  p.icon_on = "Auto";
  p.options.clear();
  if (p.precision != "text" && p.precision != "1" && p.precision != "2") p.precision.clear();
  if (p.precision != "text" && (p.icon.empty() || p.icon == "Auto")) p.icon = "Auto";
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
  std::string out = normalize_garage_label_display(cfg_option_value(options, LABEL_DISPLAY_OPTION)) == "status"
    ? std::string(LABEL_DISPLAY_OPTION) + "=status"
    : "";
  std::string confirmation_mode = sensor == "open" ? "on" : sensor == "close" ? "off" : "";
  append_confirm_options(out, options, "Close the garage door?", "Open the garage door?",
                         "Open or close the garage door?", confirmation_mode);
  return out;
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
  append_confirm_options(out, options, "Turn off this device?", "Turn on this device?", "Toggle this device?");
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
  std::string state_entity = trim_saved_option_value(cfg_option_value(options, "state_entity"));
  if (!state_entity.empty()) {
    append_config_token(out, "state_entity=" + encode_compact_field(state_entity));
    std::string state_precision = cfg_option_value(options, "state_precision");
    if (state_precision == "icon" || state_precision == "text") {
      append_config_token(out, "state_precision=" + state_precision);
    } else {
      std::string state_unit = trim_saved_option_value(cfg_option_value(options, "state_unit"));
      if (!state_unit.empty()) {
        append_config_token(out, "state_unit=" + encode_compact_field(state_unit));
      }
      if (state_precision == "0" || state_precision == "1" || state_precision == "2") {
        append_config_token(out, "state_precision=" + state_precision);
      }
      append_large_numbers_option(out, options);
    }
  }

  if (action == "script.turn_on") {
    std::string fields = trim_saved_option_value(cfg_option_value(options, "script_fields"));
    if (!fields.empty()) {
      append_config_token(out, "script_fields=" + encode_compact_field(fields));
    }
  }

  if (action == "script.turn_on" && cfg_option_token_present(options, "confirm_on")) {
    append_config_token(out, "confirm_on");
    std::string message = trim_saved_option_value(cfg_option_value(options, "confirm_message"));
    std::string yes = trim_saved_option_value(cfg_option_value(options, "confirm_yes"));
    std::string no = trim_saved_option_value(cfg_option_value(options, "confirm_no"));
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

inline void normalize_saved_config_action_fields(ParsedCfg &p) {
  if (action_card_option_select(p)) {
    p.sensor = card_runtime_option_select_canonical_action();
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto" || p.icon == "Chevron Down") p.icon = "Flash";
    return;
  }
  if (action_card_local_action(p)) {
    p.unit.clear();
    p.precision.clear();
    p.options.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto" || p.icon == "Flash") p.icon = "Gesture Tap";
    return;
  }
  p.precision.clear();
}

inline void normalize_saved_config_fan_fields(ParsedCfg &p) {
  if (p.icon.empty() || p.icon == "Auto") p.icon = fan_card_default_icon_name(p.type);
  if (p.type == "fan_switch") {
    if (p.icon_on.empty() || p.icon_on == "Auto") p.icon_on = "Fan";
  } else {
    p.icon_on = "Auto";
  }
}

inline void normalize_saved_config_date_time_fields(ParsedCfg &p) {
  if (!p.entity.empty()) return;
  if (p.type == "calendar") p.entity = "sensor.date";
  else if (p.type == "timezone") p.entity = "UTC (GMT+0)";
}

inline void normalize_saved_config_mower_fields(ParsedCfg &p) {
  p.sensor = card_runtime_lawn_mower_mode(p.sensor);
  if (p.icon.empty() || p.icon == "Auto") {
    p.icon = card_runtime_lawn_mower_default_icon_name(p.sensor);
  }
}

inline void normalize_saved_config_occupancy_fields(ParsedCfg &p) {
  if (p.type == "door_window") {
    p.precision = normalize_door_window_subtype(p.precision);
    if (p.icon.empty() || p.icon == "Auto") p.icon = door_window_closed_icon_name(p.precision);
    if (p.icon_on.empty() || p.icon_on == "Auto") p.icon_on = door_window_open_icon_name(p.precision);
  } else if (p.type == "presence") {
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Motion Sensor Off";
    if (p.icon_on.empty() || p.icon_on == "Auto") p.icon_on = "Motion Sensor";
  }
}

inline std::string normalize_saved_config_occupancy_options(
    const std::string &options, const ParsedCfg &p) {
  return p.type == "door_window" ? door_window_card_options_normalized(options)
                                  : presence_card_options_normalized(options);
}

inline void normalize_saved_config_access_fields(ParsedCfg &p) {
  if (p.type == "garage") {
    if (!card_runtime_garage_mode_valid(p.sensor)) p.sensor.clear();
    if (!p.sensor.empty()) p.icon_on = "Auto";
  } else if (p.type == "gate") {
    if (!card_runtime_gate_mode_valid(p.sensor)) p.sensor.clear();
    if (!p.sensor.empty()) p.icon_on = "Auto";
  } else if (p.type == "cover") {
    if (!card_runtime_cover_mode_valid(p.sensor)) p.sensor.clear();
    if (p.sensor != "set_position") p.unit.clear();
  } else if (p.type == "lock") {
    if (!card_runtime_lock_mode_valid(p.sensor)) p.sensor.clear();
    if (!p.sensor.empty()) {
      p.icon_on = "Auto";
    } else if (p.icon_on.empty() || p.icon_on == "Auto") {
      p.icon_on = "Lock Open";
    }
  }
}

inline std::string normalize_saved_config_access_options(
    const std::string &options, const ParsedCfg &p) {
  if (p.type == "garage") return garage_card_options_normalized(options, p.sensor);
  if (p.type == "gate") return gate_card_options_normalized(options, p.sensor);
  return cover_card_options_normalized(options, p.sensor);
}

inline void normalize_saved_config_security_fields(ParsedCfg &p) {
  if (p.type == "alarm") {
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Security";
    return;
  }
  if (!alarm_action_mode_valid(p.sensor)) p.sensor = "away";
  if (p.icon.empty() || p.icon == "Auto" || alarm_action_legacy_icon_name(p.sensor, p.icon)) {
    p.icon = alarm_action_icon_name(p.sensor);
  }
}

inline std::string normalize_saved_config_security_options(
    const std::string &options, const ParsedCfg &) {
  return alarm_card_options_normalized(options);
}

inline void normalize_saved_config_weather_fields(ParsedCfg &p, bool was_legacy_forecast) {
  if (was_legacy_forecast && p.label == "Weather") p.label.clear();
  if (!card_runtime_weather_forecast_precision(p.precision)) p.precision.clear();
}

inline std::string normalize_saved_config_weather_options(
    const std::string &options, const ParsedCfg &p) {
  return weather_card_options_normalized(options, p);
}

inline void normalize_saved_config_image_fields(ParsedCfg &p) {
  p.icon = image_card_icon_enabled(p)
    ? (p.icon.empty() || p.icon == "Auto" ? "Camera" : p.icon)
    : "Auto";
  if (!image_card_label_enabled(p)) p.label.clear();
}

inline std::string normalize_saved_config_image_options(
    const std::string &options, const ParsedCfg &) {
  return image_card_options_normalized(options);
}

inline void normalize_saved_config_climate_fields(ParsedCfg &p) {
  if (p.icon.empty()) p.icon = "Thermostat";
  if (p.icon_on.empty()) p.icon_on = "Auto";
  p.precision = normalize_climate_precision_config(p.precision);
}

inline std::string normalize_saved_config_climate_options(
    const std::string &options, const ParsedCfg &) {
  return climate_card_options_normalized(options, true);
}

inline std::string normalize_saved_config_light_control_options(
    const std::string &options, const ParsedCfg &) {
  return light_control_card_options_normalized(options);
}

inline void normalize_saved_config_webhook_fields(ParsedCfg &p) {
  p.sensor = normalize_webhook_method(p.sensor);
  if (p.sensor == "GET" || p.sensor == "DELETE") p.unit.clear();
  if (p.icon.empty()) p.icon = "Auto";
}

inline std::string normalize_saved_config_webhook_options(
    const std::string &options, const ParsedCfg &) {
  return webhook_card_options_normalized(options);
}

inline const char *saved_config_subpage_default_label(const std::string &kind) {
  if (kind == "switch") return "Switch";
  if (kind == "lights") return "Lighting";
  if (kind == "climate") return "Climate";
  if (kind == "presence") return "Presence";
  if (kind == "media") return "Media";
  if (kind == "alarm") return "Alarm";
  if (kind == "cover") return "Cover";
  if (kind == "garage") return "Garage";
  if (kind == "gate") return "Gate";
  if (kind == "lock") return "Lock";
  if (kind == "vacuum") return "Vacuum";
  if (kind == "lawn_mower") return "Lawn Mower";
  if (kind == "weather") return "Weather";
  if (kind == "sensor") return "Sensor";
  if (kind == "image") return "Camera";
  return "";
}

inline const char *saved_config_subpage_default_icon(const std::string &kind) {
  if (kind == "switch") return "Power Plug";
  if (kind == "lights") return "Lightbulb";
  if (kind == "climate") return "Thermostat";
  if (kind == "presence") return "Account";
  if (kind == "media") return "Speaker";
  if (kind == "alarm") return "Security";
  if (kind == "cover") return "Blinds";
  if (kind == "garage") return "Garage";
  if (kind == "gate") return "Gate";
  if (kind == "lock") return "Lock";
  if (kind == "vacuum") return "Robot Vacuum";
  if (kind == "lawn_mower") return "Robot Mower";
  if (kind == "weather") return "Weather Partly Cloudy";
  if (kind == "sensor") return "Gauge";
  if (kind == "image") return "Camera";
  return "";
}

inline void normalize_saved_config_subpage_fields(ParsedCfg &p) {
  const std::string kind = normalize_subpage_kind(cfg_option_value(p.options, "subpage_kind"));
  if (kind.empty()) return;
  if (p.label.empty()) p.label = saved_config_subpage_default_label(kind);
  if (p.icon.empty() || p.icon == "Auto") p.icon = saved_config_subpage_default_icon(kind);
  p.icon_on = "Auto";
  p.sensor = "indicator";
  p.unit.clear();
  p.precision.clear();
}

inline std::string normalize_saved_config_subpage_options(
    const std::string &options, const ParsedCfg &p) {
  return subpage_card_options_normalized(options, p.sensor, p.precision);
}

inline ParsedCfg normalize_parsed_cfg(ParsedCfg p) {
  migrate_saved_config_action_legacy(p);
  const bool was_legacy_text_sensor = p.type == "text_sensor";
  migrate_saved_config_sensor_legacy(p);
  const bool normalized_saved_fan = normalize_saved_config_fan(
      p, normalize_saved_config_fan_fields, fan_control_card_options_normalized);
  const bool was_legacy_weather_forecast = migrate_saved_config_weather_legacy(p);
  normalize_saved_config_weather(
      p, was_legacy_weather_forecast, normalize_saved_config_weather_fields,
      normalize_saved_config_weather_options);
  normalize_saved_config_media(p, normalize_saved_config_media_fields,
                               media_card_options_normalized);
  normalize_saved_config_climate(
      p, normalize_saved_config_climate_fields, normalize_saved_config_climate_options);
  const bool normalized_saved_access = normalize_saved_config_access(
      p, normalize_saved_config_access_fields, normalize_saved_config_access_options);
  normalize_saved_config_security(
      p, normalize_saved_config_security_fields, normalize_saved_config_security_options);
  normalize_saved_config_webhook(
      p, normalize_saved_config_webhook_fields, normalize_saved_config_webhook_options);
  normalize_saved_config_image(
      p, normalize_saved_config_image_fields, normalize_saved_config_image_options);
  const bool normalized_saved_static = normalize_saved_config_static(p);
  normalize_saved_config_date_time(
      p, normalize_saved_config_date_time_fields, date_time_card_options_normalized);
  if (p.type == "todo") {
    p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    p.icon_on = "Auto";
    if (p.icon.empty() || p.icon == "Auto") p.icon = "Check";
    p.options = todo_card_options_normalized(p.options);
  }
  normalize_saved_config_light_control(p, normalize_saved_config_light_control_options);
  normalize_saved_config_subpage(
      p, normalize_saved_config_subpage_fields, normalize_saved_config_subpage_options);
  normalize_saved_config_action(p, normalize_saved_config_action_fields,
                                action_card_options_normalized);
  if (migrate_saved_config_vacuum_legacy(p)) {
    if (p.icon.empty() || p.icon == "Auto") p.icon = card_runtime_vacuum_default_icon_name(p.sensor);
  }
  if (p.type == "vacuum") {
    p.sensor = normalize_saved_config_vacuum_sensor(p.sensor);
    if (p.sensor != "clean_area") p.unit.clear();
    p.precision = normalize_saved_config_vacuum_precision(p.precision);
    p.options = normalize_saved_config_vacuum_options(p.options);
    p.icon_on = normalize_saved_config_vacuum_icon_on(p.icon_on);
    if (p.icon.empty() || p.icon == "Auto") p.icon = card_runtime_vacuum_default_icon_name(p.sensor);
  }
  const bool normalized_saved_mower =
      normalize_saved_config_mower(p, normalize_saved_config_mower_fields);
  normalize_saved_config_switch(p, switch_card_options_normalized);
  const bool normalized_saved_occupancy = normalize_saved_config_occupancy(
      p, normalize_saved_config_occupancy_fields,
      normalize_saved_config_occupancy_options);
  if (!normalized_saved_static && !normalized_saved_fan && !normalized_saved_mower && !normalized_saved_occupancy && !normalized_saved_access && !p.type.empty() && p.type != "action" && p.type != "alarm" && p.type != "alarm_action" && !climate_card_type(p.type) && p.type != "webhook" && p.type != "todo" && p.type != "sensor" && p.type != "media" && p.type != "subpage" && p.type != "image" && p.type != "light_control" && p.type != "vacuum" && !card_large_numbers_supported(p)) {
    p.options.clear();
  }
  normalize_saved_config_sensor(p, was_legacy_text_sensor,
                                normalize_saved_config_sensor_fields,
                                sensor_card_options_normalized);
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
  return p.type == "media" && (p.sensor == "volume" || p.sensor == "control_modal" ||
                               p.sensor == "speaker_group")
    ? normalize_media_volume_max_percent(cfg_option_value(p.options, VOLUME_MAX_OPTION))
    : card_runtime_media_volume_max_default();
}

inline std::string media_speaker_group_entity(const ParsedCfg &p) {
  if (p.type != "media" || (p.sensor != "control_modal" && p.sensor != "speaker_group")) {
    return "";
  }
  return trim_saved_option_value(cfg_option_value(p.options, MEDIA_SPEAKER_GROUP_ENTITY_OPTION));
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

inline bool garage_confirmation_enabled(const ParsedCfg &p) {
  return p.type == "garage" &&
         (cfg_option_enabled(p.options, "confirm_off") ||
          cfg_option_enabled(p.options, "confirm_on"));
}

inline bool garage_confirmation_required(const ParsedCfg &p, bool pending_open) {
  if (p.type != "garage") return false;
  return pending_open
    ? cfg_option_enabled(p.options, "confirm_on")
    : cfg_option_enabled(p.options, "confirm_off");
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
  if (p.type == "garage") {
    if (confirm_off && confirm_on) return espcontrol_i18n(std::string("Open or close the garage door?"));
    if (confirm_on) return espcontrol_i18n(std::string("Open the garage door?"));
    return espcontrol_i18n(std::string("Close the garage door?"));
  }
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

inline bool duration_unit_seconds_multiplier(const std::string &raw_unit,
                                             double &multiplier) {
  const std::string unit = trim_display_unit(raw_unit);
  if (unit == "seconds" || unit == "s") multiplier = 1.0;
  else if (unit == "minutes" || unit == "min") multiplier = 60.0;
  else if (unit == "hours" || unit == "h") multiplier = 3600.0;
  else if (unit == "days" || unit == "d") multiplier = 86400.0;
  else if (unit == "ms") multiplier = 0.001;
  else if (unit == "µs") multiplier = 0.000001;
  else return false;
  return true;
}

inline bool format_duration_value(char *buffer, size_t buffer_size,
                                  double value, const std::string &input_unit) {
  if (!buffer || buffer_size == 0) return false;
  buffer[0] = '\0';
  double multiplier = 0.0;
  if (!std::isfinite(value) || value < 0.0 ||
      !duration_unit_seconds_multiplier(input_unit, multiplier)) return false;
  const double converted = value * multiplier;
  if (!std::isfinite(converted) || converted < 0.0 ||
      converted >= static_cast<double>(std::numeric_limits<uint64_t>::max())) return false;

  const uint64_t total_seconds = static_cast<uint64_t>(std::floor(converted));
  if (total_seconds == 0) {
    std::snprintf(buffer, buffer_size, "0s");
    return true;
  }

  const uint64_t values[] = {
    total_seconds / 86400ULL,
    (total_seconds % 86400ULL) / 3600ULL,
    (total_seconds % 3600ULL) / 60ULL,
    total_seconds % 60ULL,
  };
  const char *suffixes[] = {"d", "h", "m", "s"};
  size_t used = 0;
  int components = 0;
  for (int i = 0; i < 4 && components < 2; ++i) {
    if (values[i] == 0) continue;
    const int written = std::snprintf(
      buffer + used, buffer_size - used, components == 0 ? "%llu%s" : " %llu%s",
      static_cast<unsigned long long>(values[i]), suffixes[i]);
    if (written < 0 || static_cast<size_t>(written) >= buffer_size - used) {
      buffer[0] = '\0';
      return false;
    }
    used += static_cast<size_t>(written);
    ++components;
  }
  return components > 0;
}

inline bool format_duration_sensor_state(char *buffer, size_t buffer_size,
                                         const std::string &state, bool has_state,
                                         const std::string &auto_unit, bool has_auto_unit,
                                         const std::string &manual_unit = "") {
  if (!buffer || buffer_size == 0) return false;
  buffer[0] = '\0';
  if (!has_state || (manual_unit.empty() && !has_auto_unit)) return false;
  const std::string &input_unit = manual_unit.empty() ? auto_unit : manual_unit;
  char *end = nullptr;
  const char *begin = state.c_str();
  const double value = std::strtod(begin, &end);
  while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
  if (end == begin || !end || *end != '\0') return false;
  return format_duration_value(buffer, buffer_size, value, input_unit);
}

inline bool is_text_sensor_card(const std::string &type, const std::string &precision) {
  return (type == "sensor" && precision == "text") || type == "text_sensor";
}

inline bool is_text_sensor_card(const ParsedCfg &p) {
  if (sensor_card_local_sensor(p)) return false;
  return is_text_sensor_card(p.type, p.precision);
}

#ifndef ESPCONTROL_HA_SUBSCRIPTION_SCOPE_CONSTANTS_DEFINED
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_ALL = 0;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_DEFAULT = 1u << 0;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_COVER_ART = 1u << 1;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_PHASE3 = 1u << 2;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS = 1u << 3;
#define ESPCONTROL_HA_SUBSCRIPTION_SCOPE_CONSTANTS_DEFINED 1
#endif

constexpr size_t HA_STATE_TEXT_MAX_LEN = 96;
constexpr size_t HA_TEXT_SENSOR_STATE_MAX_LEN = 256;
constexpr size_t HA_SHORT_STATE_MAX_LEN = 32;
constexpr size_t HA_FRIENDLY_NAME_MAX_LEN = 64;

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

inline bool sensor_active_color_state_ref(esphome::StringRef state,
                                          bool numeric_mode) {
  return is_entity_on_ref(state) ||
         (numeric_mode && numeric_state_positive_ref(state));
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

#ifndef ESPCONTROL_HA_GENERATION_HELPERS_DEFINED
inline uint32_t &ha_subscription_generation() {
  static uint32_t generation = 1;
  return generation;
}

inline void bump_ha_subscription_generation() {
  uint32_t &generation = ha_subscription_generation();
  generation++;
  if (generation == 0) generation = 1;
  ha_reset_deferred_state_requests();
  ha_reset_subscription_callbacks(
      HA_SUBSCRIPTION_SCOPE_DEFAULT | HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS);
}
#endif

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
