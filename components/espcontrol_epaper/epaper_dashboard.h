#pragma once

#include "esphome/components/api/api_server.h"
#include "esphome/components/api/homeassistant_service.h"
#include "lvgl.h"
#include "../espcontrol/button_grid_ha.h"
#include "../espcontrol/icons.h"
#include "../espcontrol/sun_calc.h"
#include "../espcontrol/temperature_unit.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace espcontrol {

constexpr int EPAPER_DASHBOARD_PAGE_SLOTS = 12;
constexpr int EPAPER_DASHBOARD_PAGES = 1;
constexpr int EPAPER_DASHBOARD_COLS = 4;
constexpr int EPAPER_DASHBOARD_TOTAL_SLOTS =
    EPAPER_DASHBOARD_PAGE_SLOTS * EPAPER_DASHBOARD_PAGES;
constexpr size_t EPAPER_DASHBOARD_FRIENDLY_NAME_MAX_LEN = 64;
constexpr size_t EPAPER_DASHBOARD_SHORT_STATE_MAX_LEN = 32;
constexpr size_t EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN = 96;
constexpr size_t EPAPER_DASHBOARD_TEXT_SENSOR_STATE_MAX_LEN = 256;
constexpr int EPAPER_DASHBOARD_FORECAST_PENDING_MAX = 8;
constexpr uint32_t EPAPER_DASHBOARD_FORECAST_REQUEST_TIMEOUT_MS = 60000;
constexpr uint32_t EPAPER_DASHBOARD_FORECAST_RETRY_DELAY_MS = 300000;

struct EpaperDashboardTile {
  std::string config;
  std::string entity;
  std::string sensor;
  std::string label;
  std::string icon;
  std::string icon_on;
  std::string unit;
  std::string type;
  std::string precision;
  std::string options;
  std::string state;
  std::string sensor_value;
  std::string secondary_value;
  std::string media_position_value;
  std::string media_duration_value;
  std::string media_position_updated_at_value;
  std::string climate_current_value;
  std::string climate_target_value;
  std::string climate_target_low_value;
  std::string climate_target_high_value;
  std::string climate_hvac_action;
  std::vector<std::string> fan_preset_modes;
  std::string action_state_entity;
  std::string friendly_name;
  std::string forecast_unit;
  std::string forecast_status_label;
  uint32_t media_position_received_ms = 0;
  int forecast_high = 0;
  int forecast_low = 0;
  bool label_configured = false;
  bool state_subscribed = false;
  bool sensor_subscribed = false;
  bool secondary_subscribed = false;
  bool friendly_name_subscribed = false;
  bool media_position_subscribed = false;
  bool media_duration_subscribed = false;
  bool media_position_updated_at_subscribed = false;
  bool climate_current_subscribed = false;
  bool climate_target_subscribed = false;
  bool climate_target_low_subscribed = false;
  bool climate_target_high_subscribed = false;
  bool climate_hvac_action_subscribed = false;
  bool fan_preset_modes_subscribed = false;
  bool state_unavailable = false;
  bool sensor_unavailable = false;
  bool secondary_unavailable = false;
  bool media_position_unavailable = false;
  bool media_duration_unavailable = false;
  bool media_position_updated_at_unavailable = false;
  bool climate_current_unavailable = false;
  bool climate_target_unavailable = false;
  bool climate_target_low_unavailable = false;
  bool climate_target_high_unavailable = false;
  bool climate_hvac_action_unavailable = false;
  bool forecast_valid = false;
};

struct EpaperDashboardTimeState {
  bool valid = false;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  time_t epoch = 0;
  bool use_12h = false;
  std::string active_timezone = "UTC (GMT+0)";
};

struct EpaperDashboardForecastPayload {
  bool today_valid = false;
  int today_high = 32767;
  int today_low = 32767;
  bool tomorrow_valid = false;
  int tomorrow_high = 32767;
  int tomorrow_low = 32767;
  std::string unit;
};

struct EpaperDashboardForecastPendingRequest {
  uint32_t call_id = 0;
  uint32_t started_ms = 0;
  std::string entity_id;
};

struct EpaperDashboardForecastQueuedRequest {
  std::string entity_id;
};

struct EpaperDashboardForecastRetryRequest {
  std::string entity_id;
  uint32_t due_ms = 0;
};

struct EpaperDashboardOrder {
  std::array<int, EPAPER_DASHBOARD_PAGE_SLOTS> positions{};
  std::array<int, EPAPER_DASHBOARD_TOTAL_SLOTS> row_span{};
  std::array<int, EPAPER_DASHBOARD_TOTAL_SLOTS> col_span{};
};

inline std::array<EpaperDashboardTile, EPAPER_DASHBOARD_TOTAL_SLOTS> &epaper_dashboard_tiles() {
  static std::array<EpaperDashboardTile, EPAPER_DASHBOARD_TOTAL_SLOTS> tiles;
  return tiles;
}

inline EpaperDashboardOrder &epaper_dashboard_order() {
  static EpaperDashboardOrder order;
  return order;
}

inline std::array<std::string, 12> &epaper_dashboard_custom_month_names() {
  static std::array<std::string, 12> names;
  return names;
}

inline EpaperDashboardTimeState &epaper_dashboard_time_state() {
  static EpaperDashboardTimeState state;
  return state;
}

inline bool &epaper_dashboard_dirty_flag() {
  static bool dirty = true;
  return dirty;
}

inline bool &epaper_dashboard_dark_theme_flag() {
  static bool dark = false;
  return dark;
}

inline bool epaper_dashboard_dark_theme() {
  return epaper_dashboard_dark_theme_flag();
}

inline void epaper_dashboard_mark_dirty() {
  epaper_dashboard_dirty_flag() = true;
}

inline void epaper_dashboard_set_dark_theme(bool dark) {
  if (epaper_dashboard_dark_theme_flag() != dark) epaper_dashboard_mark_dirty();
  epaper_dashboard_dark_theme_flag() = dark;
}

inline bool epaper_dashboard_is_dirty() {
  return epaper_dashboard_dirty_flag();
}

inline void epaper_dashboard_clear_dirty() {
  epaper_dashboard_dirty_flag() = false;
}

inline void epaper_dashboard_set_time(bool valid, int year, int month, int day,
                                      int hour, int minute, time_t epoch,
                                      const std::string &active_timezone,
                                      bool use_12h) {
  auto &state = epaper_dashboard_time_state();
  if (state.valid != valid || state.year != year || state.month != month ||
      state.day != day || state.hour != hour || state.minute != minute ||
      state.epoch != epoch || state.active_timezone != active_timezone ||
      state.use_12h != use_12h) {
    epaper_dashboard_mark_dirty();
  }
  state.valid = valid;
  state.year = year;
  state.month = month;
  state.day = day;
  state.hour = hour;
  state.minute = minute;
  state.epoch = epoch;
  state.active_timezone = active_timezone.empty() ? std::string("UTC (GMT+0)") : active_timezone;
  state.use_12h = use_12h;
}

inline std::string epaper_dashboard_string_ref_limited(esphome::StringRef value, size_t max_len) {
  size_t len = value.size();
  if (len > max_len) len = max_len;
  return std::string(value.c_str(), len);
}

inline int epaper_dashboard_page_count() {
  return EPAPER_DASHBOARD_PAGES;
}

inline int epaper_dashboard_wrap_page(int page) {
  if (page < 0) return EPAPER_DASHBOARD_PAGES - 1;
  if (page >= EPAPER_DASHBOARD_PAGES) return 0;
  return page;
}

inline std::vector<std::string> epaper_dashboard_split(const std::string &value, char delim) {
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

inline void epaper_dashboard_grid_token_spans(char suffix, int &row_span, int &col_span) {
  row_span = 1;
  col_span = 1;
  if (suffix == 'd') row_span = 2;
  else if (suffix == 'w') col_span = 2;
  else if (suffix == 'b') {
    row_span = 2;
    col_span = 2;
  } else if (suffix == 't') {
    row_span = 3;
  } else if (suffix == 'x') {
    col_span = 3;
  }
}

inline bool epaper_dashboard_grid_token_has_span_suffix(char suffix) {
  return suffix == 'd' || suffix == 'w' || suffix == 'b' ||
         suffix == 't' || suffix == 'x';
}

inline EpaperDashboardOrder epaper_dashboard_parse_order(const std::string &order_str) {
  EpaperDashboardOrder order;
  for (int i = 0; i < EPAPER_DASHBOARD_TOTAL_SLOTS; i++) {
    order.row_span[i] = 1;
    order.col_span[i] = 1;
  }

  if (order_str.empty()) {
    for (int i = 0; i < EPAPER_DASHBOARD_PAGE_SLOTS; i++) order.positions[i] = i + 1;
    return order;
  }

  size_t grid_pos = 0;
  size_t start = 0;
  while (start <= order_str.length() && grid_pos < EPAPER_DASHBOARD_PAGE_SLOTS) {
    size_t comma = order_str.find(',', start);
    if (comma == std::string::npos) comma = order_str.length();
    if (comma > start) {
      std::string token = order_str.substr(start, comma - start);
      int row_span = 1;
      int col_span = 1;
      if (!token.empty() && epaper_dashboard_grid_token_has_span_suffix(token.back())) {
        epaper_dashboard_grid_token_spans(token.back(), row_span, col_span);
        token.pop_back();
      }
      int slot = std::atoi(token.c_str());
      if (slot >= 1 && slot <= EPAPER_DASHBOARD_TOTAL_SLOTS) {
        order.positions[grid_pos] = slot;
        order.row_span[slot - 1] = row_span;
        order.col_span[slot - 1] = col_span;
      }
    }
    grid_pos++;
    start = comma + 1;
  }

  for (int pos = 0; pos < EPAPER_DASHBOARD_PAGE_SLOTS; pos++) {
    int slot = order.positions[pos];
    if (slot <= 0) continue;
    int idx = slot - 1;
    int row_span = order.row_span[idx] > 0 ? order.row_span[idx] : 1;
    int col_span = order.col_span[idx] > 0 ? order.col_span[idx] : 1;
    int col = pos % EPAPER_DASHBOARD_COLS;
    for (int r = 0; r < row_span; r++) {
      for (int c = 0; c < col_span; c++) {
        if (r == 0 && c == 0) continue;
        if (col + c >= EPAPER_DASHBOARD_COLS) continue;
        int covered = pos + r * EPAPER_DASHBOARD_COLS + c;
        if (covered < EPAPER_DASHBOARD_PAGE_SLOTS) order.positions[covered] = 0;
      }
    }
  }

  return order;
}

inline void epaper_dashboard_set_order(const std::string &order_str) {
  EpaperDashboardOrder next = epaper_dashboard_parse_order(order_str);
  auto &current = epaper_dashboard_order();
  if (current.positions != next.positions ||
      current.row_span != next.row_span ||
      current.col_span != next.col_span) {
    epaper_dashboard_mark_dirty();
  }
  current = next;
}

inline std::string epaper_dashboard_trim(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    start++;
  }
  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end--;
  }
  return value.substr(start, end - start);
}

inline void epaper_dashboard_set_month_names(const std::string &value) {
  auto &names = epaper_dashboard_custom_month_names();
  for (auto &name : names) name.clear();
  std::vector<std::string> parts = epaper_dashboard_split(value, ',');
  for (int i = 0; i < 12 && i < static_cast<int>(parts.size()); i++) {
    names[i] = epaper_dashboard_trim(parts[i]);
  }
  epaper_dashboard_mark_dirty();
}

inline int epaper_dashboard_hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

inline char epaper_dashboard_compact_hex_char(uint8_t value) {
  return value < 10 ? static_cast<char>('0' + value)
                    : static_cast<char>('A' + value - 10);
}

inline std::string epaper_dashboard_encode_field(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (unsigned char ch : value) {
    if (ch == '%' || ch == ',' || ch == ';' || ch == '|' || ch == ':') {
      out.push_back('%');
      out.push_back(epaper_dashboard_compact_hex_char((ch >> 4) & 0x0F));
      out.push_back(epaper_dashboard_compact_hex_char(ch & 0x0F));
    } else {
      out.push_back(static_cast<char>(ch));
    }
  }
  return out;
}

inline std::string epaper_dashboard_decode_field(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (size_t i = 0; i < value.size(); i++) {
    if (value[i] == '%' && i + 2 < value.size()) {
      int hi = epaper_dashboard_hex_digit(value[i + 1]);
      int lo = epaper_dashboard_hex_digit(value[i + 2]);
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

inline std::vector<std::string> epaper_dashboard_config_fields(const std::string &config) {
  if (!config.empty() && config[0] == '~') {
    std::vector<std::string> decoded;
    for (const auto &field : epaper_dashboard_split(config.substr(1), ',')) {
      decoded.push_back(epaper_dashboard_decode_field(field));
    }
    return decoded;
  }
  return epaper_dashboard_split(config, ';');
}

inline std::string epaper_dashboard_title_from_entity(const std::string &entity) {
  size_t dot = entity.find('.');
  std::string text = dot == std::string::npos ? entity : entity.substr(dot + 1);
  for (char &ch : text) {
    if (ch == '_') ch = ' ';
  }
  bool cap = true;
  for (char &ch : text) {
    if (std::isspace(static_cast<unsigned char>(ch))) {
      cap = true;
      continue;
    }
    if (cap) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    cap = false;
  }
  return text;
}

inline std::string epaper_dashboard_sentence_cap_text(const std::string &state) {
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

inline std::string epaper_dashboard_normalized_state_text(const std::string &value);
inline std::string epaper_dashboard_climate_number_mode(const EpaperDashboardTile &tile);

inline bool epaper_dashboard_state_active(const std::string &value) {
  std::string state = epaper_dashboard_normalized_state_text(value);
  return state == "on" || state == "true" || state == "1" ||
         state == "open" || state == "opened" || state == "opening" || state == "closing" ||
         state == "unlocked" || state == "unlocking" || state == "jammed" ||
         state == "home" || state == "playing";
}

inline bool epaper_dashboard_state_unavailable(const std::string &value) {
  std::string state = epaper_dashboard_normalized_state_text(value);
  return state.empty() || state == "unavailable" || state == "unknown";
}

inline bool epaper_dashboard_entity_accepts_unknown_state(const std::string &entity_id) {
  return (entity_id.size() > 7 && entity_id.compare(0, 7, "button.") == 0) ||
         (entity_id.size() > 13 && entity_id.compare(0, 13, "input_button.") == 0);
}

inline bool epaper_dashboard_entity_state_unavailable(const std::string &entity_id,
                                                      const std::string &value) {
  std::string state = epaper_dashboard_normalized_state_text(value);
  if (state.empty() || state == "unavailable") return true;
  if (state == "unknown") return !epaper_dashboard_entity_accepts_unknown_state(entity_id);
  return false;
}

inline std::string epaper_dashboard_option_value(const std::string &options, const char *name) {
  if (!name || !*name || options.empty()) return "";
  std::string prefix = std::string(name) + "=";
  size_t start = 0;
  while (start <= options.length()) {
    size_t end = options.find(',', start);
    if (end == std::string::npos) end = options.length();
    if (options.compare(start, prefix.length(), prefix) == 0) {
      return epaper_dashboard_decode_field(options.substr(start + prefix.length(), end - start - prefix.length()));
    }
    start = end + 1;
  }
  return "";
}

inline bool epaper_dashboard_option_present(const std::string &options, const char *name) {
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

inline bool epaper_dashboard_todo_card_show_count(const EpaperDashboardTile &tile) {
  return tile.type == "todo" &&
         epaper_dashboard_option_value(tile.options, "count_display") != "icon";
}

inline bool epaper_dashboard_alarm_action_mode_valid(const std::string &mode);

inline std::string epaper_dashboard_normalize_todo_options(const std::string &options) {
  bool show_count = epaper_dashboard_option_value(options, "count_display") != "icon";
  std::string out = show_count ? "" : "count_display=icon";
  if (show_count && epaper_dashboard_option_present(options, "large_numbers")) {
    out = "large_numbers";
  }
  return out;
}

inline std::string epaper_dashboard_normalize_switch_options(const std::string &options,
                                                             const std::string &sensor,
                                                             const std::string &precision) {
  std::string out;
  if (!sensor.empty() && precision != "text" &&
      epaper_dashboard_option_present(options, "large_numbers")) {
    out = "large_numbers";
  }
  bool confirm_off = epaper_dashboard_option_present(options, "confirm_off");
  bool confirm_on = epaper_dashboard_option_present(options, "confirm_on");
  if (!confirm_off && !confirm_on) return out;
  if (confirm_off) {
    if (!out.empty()) out += ",";
    out += "confirm_off";
  }
  if (confirm_on) {
    if (!out.empty()) out += ",";
    out += "confirm_on";
  }
  std::string mode = confirm_off && confirm_on ? "both" : (confirm_on ? "on" : "off");
  std::string message = epaper_dashboard_option_value(options, "confirm_message");
  std::string default_message = mode == "both" ? "Toggle this device?"
    : (mode == "on" ? "Turn on this device?" : "Turn off this device?");
  if (!message.empty() && message != default_message) {
    out += ",confirm_message=" + epaper_dashboard_encode_field(message);
  }
  std::string yes = epaper_dashboard_option_value(options, "confirm_yes");
  if (!yes.empty() && yes != "Yes") out += ",confirm_yes=" + epaper_dashboard_encode_field(yes);
  std::string no = epaper_dashboard_option_value(options, "confirm_no");
  if (!no.empty() && no != "No") out += ",confirm_no=" + epaper_dashboard_encode_field(no);
  return out;
}

inline std::string epaper_dashboard_normalize_sensor_options(const std::string &options,
                                                             const std::string &precision) {
  std::string out;
  if (precision != "icon" && precision != "text" &&
      epaper_dashboard_option_present(options, "large_numbers")) {
    out = "large_numbers";
  }
  if (precision == "text" && epaper_dashboard_option_present(options, "state_labels")) {
    if (!out.empty()) out += ",";
    out += "state_labels";
    std::string input = epaper_dashboard_option_value(options, "state_input");
    std::string output = epaper_dashboard_option_value(options, "state_output");
    if (input.empty() && !epaper_dashboard_option_value(options, "state_high_label").empty()) {
      input = "high";
      output = epaper_dashboard_option_value(options, "state_high_label");
    } else if (input.empty() && !epaper_dashboard_option_value(options, "state_low_label").empty()) {
      input = "low";
      output = epaper_dashboard_option_value(options, "state_low_label");
    }
    if (!input.empty()) {
      out += ",state_input=" + epaper_dashboard_encode_field(input);
    }
    if (!output.empty()) {
      out += ",state_output=" + epaper_dashboard_encode_field(output);
    }
    std::string input_2 = epaper_dashboard_option_value(options, "state_input_2");
    std::string output_2 = epaper_dashboard_option_value(options, "state_output_2");
    if (!input_2.empty()) {
      out += ",state_input_2=" + epaper_dashboard_encode_field(input_2);
    }
    if (!output_2.empty()) {
      out += ",state_output_2=" + epaper_dashboard_encode_field(output_2);
    }
  }
  return out;
}

inline std::string epaper_dashboard_normalize_weather_options(const std::string &options,
                                                              const std::string &precision) {
  return (precision == "today" || precision == "tomorrow") &&
         epaper_dashboard_option_present(options, "large_numbers")
           ? std::string("large_numbers")
           : std::string();
}

inline std::string epaper_dashboard_normalize_weather_precision(const std::string &precision) {
  return precision == "today" || precision == "tomorrow" ? precision : "";
}

inline std::string epaper_dashboard_normalize_date_time_options(const std::string &options) {
  return epaper_dashboard_option_present(options, "large_numbers")
           ? std::string("large_numbers")
           : std::string();
}

inline int epaper_dashboard_normalize_light_temperature_bound(int value) {
  int rounded = ((value + 50) / 100) * 100;
  if (rounded < 1000) rounded = 1000;
  if (rounded > 10000) rounded = 10000;
  return rounded;
}

inline std::string epaper_dashboard_normalize_light_temperature_range(const std::string &value) {
  size_t dash = value.find('-');
  if (dash == std::string::npos || dash == 0) return "2000-6500";
  std::string min_text = value.substr(0, dash);
  std::string max_text = value.substr(dash + 1);
  char *end_min = nullptr;
  char *end_max = nullptr;
  long min_k = std::strtol(min_text.c_str(), &end_min, 10);
  long max_k = std::strtol(max_text.c_str(), &end_max, 10);
  if (end_min == min_text.c_str() || end_max == max_text.c_str()) return "2000-6500";
  int min_bound = epaper_dashboard_normalize_light_temperature_bound(static_cast<int>(min_k));
  int max_bound = epaper_dashboard_normalize_light_temperature_bound(static_cast<int>(max_k));
  if (max_bound <= min_bound) return "2000-6500";
  return std::to_string(min_bound) + "-" + std::to_string(max_bound);
}

inline std::string epaper_dashboard_normalize_climate_options(const std::string &options) {
  std::string label_display = epaper_dashboard_option_value(options, "label_display");
  if (label_display != "status" && label_display != "actual" && label_display != "target") {
    label_display = "label";
  }
  std::string number_display = epaper_dashboard_option_value(options, "number_display");
  if (number_display != "icon" && number_display != "actual") {
    number_display = "target";
  }
  std::string out;
  if (label_display != "label") out = "label_display=" + label_display;
  if (number_display != "target") {
    if (!out.empty()) out += ",";
    out += "number_display=" + number_display;
  }
  if (number_display != "icon" && epaper_dashboard_option_present(options, "large_numbers")) {
    if (!out.empty()) out += ",";
    out += "large_numbers";
  }
  return out;
}

inline std::string epaper_dashboard_normalize_garage_options(const std::string &options,
                                                             const std::string &mode) {
  if (mode == "open" || mode == "close") return "";
  return epaper_dashboard_option_value(options, "label_display") == "status"
    ? "label_display=status"
    : "";
}

inline std::string epaper_dashboard_normalize_action_options(const std::string &options) {
  std::string state_entity = epaper_dashboard_option_value(options, "state_entity");
  if (state_entity.empty()) return "";
  std::string state_precision = epaper_dashboard_option_value(options, "state_precision");
  if (state_precision != "icon" && state_precision != "text" &&
      state_precision != "0" && state_precision != "1" && state_precision != "2") {
    state_precision.clear();
  }
  std::string state_unit = (state_precision == "icon" || state_precision == "text")
    ? std::string()
    : epaper_dashboard_option_value(options, "state_unit");
  std::string out = "state_entity=" + epaper_dashboard_encode_field(state_entity);
  if (!state_unit.empty()) out += ",state_unit=" + epaper_dashboard_encode_field(state_unit);
  if (!state_precision.empty()) out += ",state_precision=" + state_precision;
  if ((state_precision == "0" || state_precision == "1" ||
       state_precision == "2" || !state_unit.empty()) &&
      epaper_dashboard_option_present(options, "large_numbers")) {
    out += ",large_numbers";
  }
  return out;
}

inline std::string epaper_dashboard_normalize_alarm_options(const std::string &options) {
  std::string out;
  if (epaper_dashboard_option_value(options, "pin_arm") == "0") out = "pin_arm=0";
  if (epaper_dashboard_option_value(options, "pin_disarm") == "0") {
    if (!out.empty()) out += ",";
    out += "pin_disarm=0";
  }
  std::string actions = epaper_dashboard_option_value(options, "actions");
  if (!actions.empty()) {
    std::string filtered;
    bool saw_valid = false;
    size_t start = 0;
    while (start <= actions.length()) {
      size_t end = actions.find('|', start);
      if (end == std::string::npos) end = actions.length();
      std::string action = actions.substr(start, end - start);
      if (epaper_dashboard_alarm_action_mode_valid(action)) {
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
  std::string icon_display = epaper_dashboard_option_value(options, "icon_display");
  if (icon_display != "status" && icon_display != "static") icon_display = "status";
  if (icon_display != "status") {
    if (!out.empty()) out += ",";
    out += "icon_display=" + icon_display;
  }
  std::string label_display = epaper_dashboard_option_value(options, "label_display");
  if (label_display != "status" && label_display != "name") label_display = "status";
  if (label_display != "status") {
    if (!out.empty()) out += ",";
    out += "label_display=" + label_display;
  }
  return out;
}

inline std::string epaper_dashboard_normalize_cover_mode(const std::string &value) {
  return value == "tilt" || value == "toggle" || value == "open" ||
         value == "close" || value == "stop" || value == "set_position"
    ? value
    : "";
}

inline std::string epaper_dashboard_normalize_lock_mode(const std::string &value) {
  return value == "lock" || value == "unlock" ? value : "";
}

inline std::string epaper_dashboard_normalize_cover_position(const std::string &value) {
  char *end = nullptr;
  long pos = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) pos = 50;
  if (pos < 0) pos = 0;
  if (pos > 100) pos = 100;
  return std::to_string(static_cast<int>(pos));
}

inline std::string epaper_dashboard_pretty_state(const std::string &value) {
  std::string text;
  text.reserve(value.size());
  bool cap = true;
  bool last_space = false;
  for (char ch : value) {
    unsigned char uch = static_cast<unsigned char>(ch);
    if (ch == '_' || ch == '-' || std::isspace(uch)) {
      if (!text.empty() && !last_space) {
        text.push_back(' ');
        last_space = true;
      }
      cap = true;
      continue;
    }
    if (std::isalpha(uch)) {
      text.push_back(cap ? static_cast<char>(std::toupper(uch)) : static_cast<char>(std::tolower(uch)));
      cap = false;
    } else {
      text.push_back(ch);
    }
    last_space = false;
  }
  if (!text.empty() && text.back() == ' ') text.pop_back();
  return text;
}

inline std::string epaper_dashboard_format_seconds(const std::string &value) {
  char *end = nullptr;
  float parsed = std::strtof(value.c_str(), &end);
  if (end == value.c_str() || std::isnan(parsed) || parsed < 0) return "0:00";
  int total = static_cast<int>(parsed + 0.5f);
  int minutes = total / 60;
  int seconds = total % 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d:%02d", minutes, seconds);
  return buf;
}

inline std::string epaper_dashboard_format_seconds_value(float value) {
  if (!std::isfinite(value) || value < 0) return "--";
  int total = static_cast<int>(value + 0.5f);
  int minutes = total / 60;
  int seconds = total % 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d:%02d", minutes, seconds);
  return buf;
}

inline std::string epaper_dashboard_format_percent(const std::string &value) {
  char *end = nullptr;
  float parsed = std::strtof(value.c_str(), &end);
  if (end == value.c_str() || std::isnan(parsed)) return epaper_dashboard_pretty_state(value);
  if (parsed >= 0.0f && parsed <= 1.0f) parsed *= 100.0f;
  char buf[16];
  snprintf(buf, sizeof(buf), "%.0f", parsed);
  return buf;
}

inline int epaper_dashboard_parse_precision(const std::string &value) {
  if (value.empty()) return 0;
  int precision = atoi(value.c_str());
  if (precision < 0) return 0;
  if (precision > 3) return 3;
  return precision;
}

inline std::string epaper_dashboard_format_number(const std::string &value, int precision) {
  char *end = nullptr;
  float parsed = std::strtof(value.c_str(), &end);
  if (end == value.c_str() || std::isnan(parsed)) return "";
  if (precision < 0) precision = 0;
  if (precision > 3) precision = 3;
  char buf[24];
  if (precision == 1) snprintf(buf, sizeof(buf), "%.1f", parsed);
  else if (precision == 2) snprintf(buf, sizeof(buf), "%.2f", parsed);
  else if (precision == 3) snprintf(buf, sizeof(buf), "%.3f", parsed);
  else snprintf(buf, sizeof(buf), "%.0f", parsed);
  return buf;
}

inline std::string epaper_dashboard_climate_option_label(const std::string &raw) {
  std::string value = epaper_dashboard_normalized_state_text(raw);
  if (value == "off") return "Off";
  if (value == "heat") return "Heat";
  if (value == "cool") return "Cool";
  if (value == "heat_cool") return "Heat/Cool";
  if (value == "auto") return "Auto";
  if (value == "dry") return "Dry";
  if (value == "fan_only") return "Fan";
  return epaper_dashboard_pretty_state(value);
}

inline std::string epaper_dashboard_climate_action_label(const EpaperDashboardTile &tile) {
  std::string mode = epaper_dashboard_normalized_state_text(tile.state);
  std::string action = epaper_dashboard_normalized_state_text(tile.climate_hvac_action);
  if (tile.state_unavailable || mode == "unknown" ||
      mode == "unavailable") return "Unavailable";
  if (mode.empty() || mode == "off") return "Off";
  if (action.empty() || action == "unknown" || action == "unavailable") {
    return epaper_dashboard_climate_option_label(mode);
  }
  if (action == "heating") return "Heating";
  if (action == "cooling") return "Cooling";
  if (action == "drying") return "Drying";
  if (action == "fan") return "Fan";
  if (action == "idle") return "Idle";
  if (action == "off") return "Off";
  return "Idle";
}

inline bool epaper_dashboard_climate_active(const EpaperDashboardTile &tile) {
  std::string mode = epaper_dashboard_normalized_state_text(tile.state);
  std::string action = epaper_dashboard_normalized_state_text(tile.climate_hvac_action);
  if (tile.state_unavailable || mode.empty() || mode == "unknown" ||
      mode == "unavailable" || mode == "off") return false;
  if (action.empty() || action == "unknown" || action == "unavailable") return true;
  return action != "idle" && action != "off";
}

inline bool epaper_dashboard_climate_controls_enabled(const EpaperDashboardTile &tile) {
  std::string mode = epaper_dashboard_normalized_state_text(tile.state);
  return !tile.state_unavailable && !mode.empty() &&
         mode != "unknown" && mode != "unavailable" && mode != "off";
}

inline std::string epaper_dashboard_climate_format_temperature(
    const std::string &value, bool unavailable, const EpaperDashboardTile &tile) {
  if (unavailable) return "";
  if (value.empty()) return "";
  return epaper_dashboard_format_number(value, epaper_dashboard_parse_precision(tile.precision));
}

inline std::string epaper_dashboard_climate_actual_value(const EpaperDashboardTile &tile) {
  std::string value = epaper_dashboard_climate_format_temperature(
      tile.climate_current_value, tile.climate_current_unavailable, tile);
  return value.empty() ? "--" : value;
}

inline std::string epaper_dashboard_climate_target_value(const EpaperDashboardTile &tile) {
  std::string low = epaper_dashboard_climate_format_temperature(
      tile.climate_target_low_value, tile.climate_target_low_unavailable, tile);
  std::string high = epaper_dashboard_climate_format_temperature(
      tile.climate_target_high_value, tile.climate_target_high_unavailable, tile);
  if (!low.empty() && !high.empty()) return low + "-" + high;

  std::string target = epaper_dashboard_climate_format_temperature(
      tile.climate_target_value, tile.climate_target_unavailable, tile);
  if (!target.empty()) return target;
  if (!low.empty()) return low;
  if (!high.empty()) return high;
  return "--";
}

inline std::string epaper_dashboard_climate_card_value(const EpaperDashboardTile &tile) {
  return epaper_dashboard_climate_number_mode(tile) == "actual"
    ? epaper_dashboard_climate_actual_value(tile)
    : epaper_dashboard_climate_target_value(tile);
}

inline const char *epaper_dashboard_month_name(int month) {
  static const char *months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
  if (month < 1 || month > 12) return "Date";
  const std::string &custom = epaper_dashboard_custom_month_names()[month - 1];
  if (!custom.empty()) return custom.c_str();
  return months[month - 1];
}

inline std::string epaper_dashboard_format_time(int hour, int minute, bool use_12h) {
  char buf[8];
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return "--:--";
  if (use_12h) {
    int hour12 = hour % 12;
    if (hour12 == 0) hour12 = 12;
    snprintf(buf, sizeof(buf), "%d:%02d", hour12, minute);
  } else {
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
  }
  return buf;
}

inline bool epaper_dashboard_timezone_localtime(const std::string &timezone,
                                                time_t epoch, struct tm &out) {
  int offset_minutes = 0;
  if (!::timezone_offset_minutes_at_utc(timezone, epoch, offset_minutes)) return false;
  time_t local_epoch = epoch + static_cast<time_t>(offset_minutes) * 60;
  return gmtime_r(&local_epoch, &out) != nullptr;
}

inline std::string epaper_dashboard_timezone_city_label(const std::string &timezone) {
  std::string tz_id = ::timezone_id_from_option(timezone);
  if (tz_id.empty()) return "World Clock";
  if (tz_id == "UTC") return "UTC";
  size_t slash = tz_id.rfind('/');
  std::string city = slash == std::string::npos ? tz_id : tz_id.substr(slash + 1);
  for (char &ch : city) {
    if (ch == '_') ch = ' ';
  }
  return city.empty() ? std::string("World Clock") : city;
}

inline bool epaper_dashboard_parse_calendar_date(const std::string &value,
                                                 int &day, int &month) {
  if (value.length() >= 10 &&
      std::isdigit(static_cast<unsigned char>(value[0])) &&
      std::isdigit(static_cast<unsigned char>(value[1])) &&
      std::isdigit(static_cast<unsigned char>(value[2])) &&
      std::isdigit(static_cast<unsigned char>(value[3])) &&
      value[4] == '-' &&
      std::isdigit(static_cast<unsigned char>(value[5])) &&
      std::isdigit(static_cast<unsigned char>(value[6])) &&
      value[7] == '-' &&
      std::isdigit(static_cast<unsigned char>(value[8])) &&
      std::isdigit(static_cast<unsigned char>(value[9]))) {
    month = (value[5] - '0') * 10 + (value[6] - '0');
    day = (value[8] - '0') * 10 + (value[9] - '0');
    return day >= 1 && day <= 31 && month >= 1 && month <= 12;
  }
  if (value.length() >= 10 &&
      std::isdigit(static_cast<unsigned char>(value[0])) &&
      std::isdigit(static_cast<unsigned char>(value[1])) &&
      (value[2] == '/' || value[2] == '-') &&
      std::isdigit(static_cast<unsigned char>(value[3])) &&
      std::isdigit(static_cast<unsigned char>(value[4])) &&
      (value[5] == '/' || value[5] == '-') &&
      std::isdigit(static_cast<unsigned char>(value[6])) &&
      std::isdigit(static_cast<unsigned char>(value[7])) &&
      std::isdigit(static_cast<unsigned char>(value[8])) &&
      std::isdigit(static_cast<unsigned char>(value[9]))) {
    day = (value[0] - '0') * 10 + (value[1] - '0');
    month = (value[3] - '0') * 10 + (value[4] - '0');
    return day >= 1 && day <= 31 && month >= 1 && month <= 12;
  }
  return false;
}

inline bool epaper_dashboard_calendar_date(const EpaperDashboardTile &tile,
                                           int &day, int &month) {
  if (!tile.state_unavailable && epaper_dashboard_parse_calendar_date(tile.state, day, month)) {
    return true;
  }
  if (!tile.entity.empty()) return false;
  const auto &time = epaper_dashboard_time_state();
  if (!time.valid || time.day < 1 || time.day > 31 ||
      time.month < 1 || time.month > 12) return false;
  day = time.day;
  month = time.month;
  return true;
}

inline std::string epaper_dashboard_calendar_value(const EpaperDashboardTile &tile) {
  const auto &time = epaper_dashboard_time_state();
  if (tile.precision == "datetime") {
    if (!time.valid || time.day < 1 || time.day > 31 ||
        time.month < 1 || time.month > 12) return "--";
    return epaper_dashboard_format_time(time.hour, time.minute, time.use_12h);
  }
  int day = 0;
  int month = 0;
  if (!epaper_dashboard_calendar_date(tile, day, month)) return "--";
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", day);
  return buf;
}

inline std::string epaper_dashboard_calendar_label(const EpaperDashboardTile &tile) {
  const auto &time = epaper_dashboard_time_state();
  if (tile.precision == "datetime") {
    if (!time.valid || time.day < 1 || time.day > 31 ||
        time.month < 1 || time.month > 12) return "Date";
    char buf[32];
    snprintf(buf, sizeof(buf), "%d %s", time.day, epaper_dashboard_month_name(time.month));
    return buf;
  }
  int day = 0;
  int month = 0;
  if (!epaper_dashboard_calendar_date(tile, day, month)) return "Date";
  return epaper_dashboard_month_name(month);
}

inline std::string epaper_dashboard_clock_value() {
  const auto &time = epaper_dashboard_time_state();
  if (!time.valid) return "--:--";
  return epaper_dashboard_format_time(time.hour, time.minute, time.use_12h);
}

inline std::string epaper_dashboard_timezone_value(const EpaperDashboardTile &tile) {
  const auto &time = epaper_dashboard_time_state();
  if (!time.valid) return "--:--";
  std::string timezone = tile.entity.empty() ? time.active_timezone : tile.entity;
  struct tm local_tm;
  if (!epaper_dashboard_timezone_localtime(timezone, time.epoch, local_tm)) return "--:--";
  return epaper_dashboard_format_time(local_tm.tm_hour, local_tm.tm_min, time.use_12h);
}

inline int epaper_dashboard_media_volume_max(const EpaperDashboardTile &tile) {
  std::string max_text = epaper_dashboard_option_value(tile.options, "volume_max");
  int max_value = max_text.empty() ? 100 : atoi(max_text.c_str());
  if (max_value < 1) return 100;
  if (max_value > 100) return 100;
  return max_value;
}

inline int epaper_dashboard_normalize_media_volume_max(const std::string &value) {
  if (value.empty()) return 100;
  char *end = nullptr;
  long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) return 100;
  if (parsed < 1) return 1;
  if (parsed > 100) return 100;
  return static_cast<int>(parsed);
}

inline std::string epaper_dashboard_media_mode(const std::string &mode) {
  if (mode == "previous" || mode == "next" || mode == "volume" ||
      mode == "position" || mode == "now_playing") {
    return mode;
  }
  return "play_pause";
}

inline bool epaper_dashboard_brightness_slider_type(const std::string &type) {
  return type == "slider" || type == "light_brightness" || type == "fan_speed";
}

inline bool epaper_dashboard_fan_card_type(const std::string &type) {
  return type == "fan_switch" || type == "fan_speed" ||
         type == "fan_oscillate" || type == "fan_direction" ||
         type == "fan_preset";
}

inline bool epaper_dashboard_media_state_display_mode(const std::string &mode) {
  return mode == "play_pause" || mode == "position";
}

inline bool epaper_dashboard_media_now_playing_control(const std::string &precision) {
  return precision.empty() || precision == "progress" || precision == "play_pause";
}

inline std::string epaper_dashboard_normalize_media_options(const std::string &options,
                                                            const std::string &mode) {
  if (mode != "volume" && mode != "position") return "";
  std::string out;
  int max_pct = epaper_dashboard_normalize_media_volume_max(
    epaper_dashboard_option_value(options, "volume_max"));
  if (mode == "volume" && max_pct < 100) {
    out = "volume_max=" + std::to_string(max_pct);
  }
  if (epaper_dashboard_option_present(options, "large_numbers")) {
    if (!out.empty()) out += ",";
    out += "large_numbers";
  }
  return out;
}

inline std::string epaper_dashboard_webhook_method(const std::string &value) {
  std::string method;
  method.reserve(value.size());
  for (char ch : value) {
    method.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  if (method == "POST" || method == "PUT" || method == "PATCH" ||
      method == "DELETE") {
    return method;
  }
  return "GET";
}

inline std::string epaper_dashboard_normalize_webhook_options(const std::string &options) {
  std::string headers = epaper_dashboard_option_value(options, "webhook_headers");
  return headers.empty() ? std::string() : "webhook_headers=" + epaper_dashboard_encode_field(headers);
}

inline bool epaper_dashboard_alarm_action_mode_valid(const std::string &mode) {
  return mode == "away" || mode == "home" || mode == "disarm";
}

inline bool epaper_dashboard_alarm_action_legacy_icon(const std::string &mode,
                                                      const std::string &icon) {
  return (mode == "away" && icon == "Security") ||
         (mode == "home" && icon == "Home") ||
         (mode == "disarm" && icon == "Lock Open");
}

inline std::string epaper_dashboard_format_media_volume(const EpaperDashboardTile &tile) {
  char *end = nullptr;
  float parsed = std::strtof(tile.sensor_value.c_str(), &end);
  if (end == tile.sensor_value.c_str() || std::isnan(parsed)) {
    return "--";
  }
  if (parsed >= 0.0f && parsed <= 1.0f) parsed *= 100.0f;
  char buf[16];
  snprintf(buf, sizeof(buf), "%.0f", parsed);
  return buf;
}

inline std::string epaper_dashboard_normalized_state_text(const std::string &value) {
  std::string text = value;
  size_t start = 0;
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) start++;
  size_t end = text.size();
  while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) end--;
  text = text.substr(start, end - start);
  for (char &ch : text) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return text;
}

inline bool epaper_dashboard_bool_value(const std::string &value, bool &out) {
  std::string text = epaper_dashboard_normalized_state_text(value);
  if (text == "true" || text == "on" || text == "yes" || text == "1") {
    out = true;
    return true;
  }
  if (text == "false" || text == "off" || text == "no" || text == "0") {
    out = false;
    return true;
  }
  return false;
}

inline std::string epaper_dashboard_text_sensor_display_text(
    const std::string &value,
    size_t max_len = EPAPER_DASHBOARD_TEXT_SENSOR_STATE_MAX_LEN) {
  std::string out;
  size_t len = value.size() > max_len ? max_len : value.size();
  out.reserve(len);
  bool cap_next = true;
  bool last_space = false;
  for (size_t i = 0; i < len; i++) {
    char ch = value[i];
    unsigned char c = static_cast<unsigned char>(ch);
    if (ch == '\r' || ch == '\n') {
      if (ch == '\r' && i + 1 < len && value[i + 1] == '\n') continue;
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

inline std::string epaper_dashboard_sensor_state_display_text(const EpaperDashboardTile &tile) {
  if (epaper_dashboard_option_present(tile.options, "state_labels")) {
    std::string state = epaper_dashboard_normalized_state_text(tile.sensor_value);
    std::string input = epaper_dashboard_option_value(tile.options, "state_input");
    std::string output = epaper_dashboard_option_value(tile.options, "state_output");
    if (input.empty() && !epaper_dashboard_option_value(tile.options, "state_high_label").empty()) {
      input = "high";
      output = epaper_dashboard_option_value(tile.options, "state_high_label");
    } else if (input.empty() && !epaper_dashboard_option_value(tile.options, "state_low_label").empty()) {
      input = "low";
      output = epaper_dashboard_option_value(tile.options, "state_low_label");
    }
    if (!input.empty() && state == epaper_dashboard_normalized_state_text(input)) return output;
    input = epaper_dashboard_option_value(tile.options, "state_input_2");
    output = epaper_dashboard_option_value(tile.options, "state_output_2");
    if (!input.empty() && state == epaper_dashboard_normalized_state_text(input)) return output;
  }
  return epaper_dashboard_text_sensor_display_text(tile.sensor_value);
}

inline const char *epaper_dashboard_weather_icon_for_state(const std::string &state) {
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

inline std::string epaper_dashboard_weather_label_for_state(const std::string &state) {
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
  return epaper_dashboard_pretty_state(state);
}

inline bool epaper_dashboard_weather_forecast_card(const EpaperDashboardTile &tile) {
  return tile.type == "weather_forecast" ||
         (tile.type == "weather" && (tile.precision == "today" || tile.precision == "tomorrow"));
}

inline std::string epaper_dashboard_weather_forecast_day(const EpaperDashboardTile &tile) {
  return tile.precision == "today" ? "today" : "tomorrow";
}

inline bool epaper_dashboard_forecast_entity_id_safe(const std::string &entity_id) {
  if (entity_id.compare(0, 8, "weather.") != 0) return false;
  for (char ch : entity_id) {
    if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.')) return false;
  }
  return true;
}

inline bool epaper_dashboard_parse_forecast_temp(const std::string &value, int &out) {
  if (value.empty()) return false;
  char *end = nullptr;
  float parsed = std::strtof(value.c_str(), &end);
  if (end == value.c_str()) return false;
  out = static_cast<int>(parsed >= 0 ? parsed + 0.5f : parsed - 0.5f);
  return true;
}

inline bool epaper_dashboard_parse_forecast_payload(const std::string &payload,
                                                    EpaperDashboardForecastPayload &out) {
  size_t p1 = payload.find('|');
  if (p1 == std::string::npos) return false;
  size_t p2 = payload.find('|', p1 + 1);
  if (p2 == std::string::npos) return false;
  size_t p3 = payload.find('|', p2 + 1);
  if (p3 == std::string::npos) return false;
  size_t p4 = payload.find('|', p3 + 1);
  if (p4 == std::string::npos) return false;

  bool today_has_high = epaper_dashboard_parse_forecast_temp(payload.substr(0, p1), out.today_high);
  bool today_has_low = epaper_dashboard_parse_forecast_temp(payload.substr(p1 + 1, p2 - p1 - 1), out.today_low);
  bool tomorrow_has_high = epaper_dashboard_parse_forecast_temp(payload.substr(p2 + 1, p3 - p2 - 1), out.tomorrow_high);
  bool tomorrow_has_low = epaper_dashboard_parse_forecast_temp(payload.substr(p3 + 1, p4 - p3 - 1), out.tomorrow_low);
  out.today_valid = today_has_high || today_has_low;
  out.tomorrow_valid = tomorrow_has_high || tomorrow_has_low;
  out.unit = payload.substr(p4 + 1);
  return out.today_valid || out.tomorrow_valid;
}

inline std::string epaper_dashboard_forecast_response_template(const std::string &entity_id) {
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set entity_response = response[entity] if entity in response else none %}"
    "{% set forecasts = entity_response['forecast'] if entity_response is not none and 'forecast' in entity_response else [] %}"
    "{% set today = forecasts[0] if forecasts|length > 0 else none %}"
    "{% set tomorrow = forecasts[1] if forecasts|length > 1 else none %}"
    "{% set today_high = today['temperature'] if today is not none and 'temperature' in today else (today['temperature_high'] if today is not none and 'temperature_high' in today else (today['high_temperature'] if today is not none and 'high_temperature' in today else (today['high'] if today is not none and 'high' in today else ''))) %}"
    "{% set today_low = today['templow'] if today is not none and 'templow' in today else (today['temperature_low'] if today is not none and 'temperature_low' in today else (today['low_temperature'] if today is not none and 'low_temperature' in today else (today['low'] if today is not none and 'low' in today else ''))) %}"
    "{% set tomorrow_high = tomorrow['temperature'] if tomorrow is not none and 'temperature' in tomorrow else (tomorrow['temperature_high'] if tomorrow is not none and 'temperature_high' in tomorrow else (tomorrow['high_temperature'] if tomorrow is not none and 'high_temperature' in tomorrow else (tomorrow['high'] if tomorrow is not none and 'high' in tomorrow else ''))) %}"
    "{% set tomorrow_low = tomorrow['templow'] if tomorrow is not none and 'templow' in tomorrow else (tomorrow['temperature_low'] if tomorrow is not none and 'temperature_low' in tomorrow else (tomorrow['low_temperature'] if tomorrow is not none and 'low_temperature' in tomorrow else (tomorrow['low'] if tomorrow is not none and 'low' in tomorrow else ''))) %}"
    "{{ today_high }}|{{ today_low }}|{{ tomorrow_high }}|{{ tomorrow_low }}|"
    "{{ state_attr(entity, 'temperature_unit') or '' }}";
}

inline void epaper_dashboard_apply_forecast_to_entity(const std::string &entity_id,
                                                      const EpaperDashboardForecastPayload &forecast) {
  for (auto &tile : epaper_dashboard_tiles()) {
    if (!epaper_dashboard_weather_forecast_card(tile) || tile.entity != entity_id) continue;
    bool today = epaper_dashboard_weather_forecast_day(tile) == "today";
    tile.forecast_valid = today ? forecast.today_valid : forecast.tomorrow_valid;
    tile.forecast_high = today ? forecast.today_high : forecast.tomorrow_high;
    tile.forecast_low = today ? forecast.today_low : forecast.tomorrow_low;
    tile.forecast_unit = forecast.unit;
    tile.forecast_status_label.clear();
  }
  epaper_dashboard_mark_dirty();
}

inline void epaper_dashboard_apply_forecast_unavailable(const std::string &entity_id,
                                                        const std::string &status_label = "") {
  for (auto &tile : epaper_dashboard_tiles()) {
    if (!epaper_dashboard_weather_forecast_card(tile) || tile.entity != entity_id) continue;
    tile.forecast_valid = false;
    tile.forecast_high = 0;
    tile.forecast_low = 0;
    tile.forecast_unit.clear();
    tile.forecast_status_label = status_label;
  }
  epaper_dashboard_mark_dirty();
}

inline uint32_t epaper_dashboard_next_forecast_call_id() {
  static uint32_t call_id = 10000;
  return call_id++;
}

inline EpaperDashboardForecastPendingRequest *epaper_dashboard_forecast_pending_requests() {
  static EpaperDashboardForecastPendingRequest requests[EPAPER_DASHBOARD_FORECAST_PENDING_MAX];
  return requests;
}

inline EpaperDashboardForecastQueuedRequest *epaper_dashboard_forecast_queued_requests() {
  static EpaperDashboardForecastQueuedRequest requests[EPAPER_DASHBOARD_FORECAST_PENDING_MAX];
  return requests;
}

inline EpaperDashboardForecastRetryRequest *epaper_dashboard_forecast_retry_requests() {
  static EpaperDashboardForecastRetryRequest requests[EPAPER_DASHBOARD_FORECAST_PENDING_MAX];
  return requests;
}

inline uint32_t &epaper_dashboard_forecast_action_ready_ms() {
  static uint32_t due_ms = 0;
  return due_ms;
}

inline bool epaper_dashboard_forecast_actions_ready() {
  if (!ha_api_state_connected()) {
    epaper_dashboard_forecast_action_ready_ms() = 0;
    return false;
  }
  uint32_t &due_ms = epaper_dashboard_forecast_action_ready_ms();
  uint32_t now = esphome::millis();
  if (due_ms == 0) {
    due_ms = now + 10000;
    ESP_LOGI("epaper_forecast",
      "Waiting 10 seconds for Home Assistant action subscription before requesting forecasts");
    return false;
  }
  return (int32_t) (now - due_ms) >= 0;
}

inline bool epaper_dashboard_forecast_pending_key(const std::string &entity_id) {
  EpaperDashboardForecastPendingRequest *requests = epaper_dashboard_forecast_pending_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id != 0 && requests[i].entity_id == entity_id) return true;
  }
  return false;
}

inline bool epaper_dashboard_forecast_queue_key(const std::string &entity_id) {
  EpaperDashboardForecastQueuedRequest *requests = epaper_dashboard_forecast_queued_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id == entity_id) return true;
  }
  return false;
}

inline bool epaper_dashboard_forecast_retry_key(const std::string &entity_id) {
  EpaperDashboardForecastRetryRequest *requests = epaper_dashboard_forecast_retry_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id == entity_id) return true;
  }
  return false;
}

inline bool epaper_dashboard_forecast_any_pending() {
  EpaperDashboardForecastPendingRequest *requests = epaper_dashboard_forecast_pending_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id != 0) return true;
  }
  return false;
}

inline void epaper_dashboard_forecast_clear_pending(uint32_t call_id) {
  if (call_id == 0) return;
  EpaperDashboardForecastPendingRequest *requests = epaper_dashboard_forecast_pending_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id == call_id) requests[i] = EpaperDashboardForecastPendingRequest();
  }
}

inline bool epaper_dashboard_forecast_track_pending(uint32_t call_id,
                                                    const std::string &entity_id) {
  if (call_id == 0) return false;
  EpaperDashboardForecastPendingRequest *requests = epaper_dashboard_forecast_pending_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id == call_id) return true;
  }
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].call_id == 0) {
      requests[i].call_id = call_id;
      requests[i].started_ms = esphome::millis();
      requests[i].entity_id = entity_id;
      return true;
    }
  }
  return false;
}

inline void epaper_dashboard_forecast_clear_queue() {
  EpaperDashboardForecastQueuedRequest *requests = epaper_dashboard_forecast_queued_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    requests[i] = EpaperDashboardForecastQueuedRequest();
  }
}

inline void epaper_dashboard_forecast_clear_retry(const std::string &entity_id) {
  EpaperDashboardForecastRetryRequest *requests = epaper_dashboard_forecast_retry_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id == entity_id) requests[i] = EpaperDashboardForecastRetryRequest();
  }
}

inline void epaper_dashboard_forecast_clear_retries() {
  EpaperDashboardForecastRetryRequest *requests = epaper_dashboard_forecast_retry_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    requests[i] = EpaperDashboardForecastRetryRequest();
  }
}

inline bool epaper_dashboard_forecast_schedule_retry(const std::string &entity_id,
                                                     const char *reason) {
  if (!epaper_dashboard_forecast_entity_id_safe(entity_id)) return false;
  if (epaper_dashboard_forecast_pending_key(entity_id) ||
      epaper_dashboard_forecast_queue_key(entity_id) ||
      epaper_dashboard_forecast_retry_key(entity_id)) {
    return true;
  }
  EpaperDashboardForecastRetryRequest *requests = epaper_dashboard_forecast_retry_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) {
      requests[i].entity_id = entity_id;
      requests[i].due_ms = esphome::millis() + EPAPER_DASHBOARD_FORECAST_RETRY_DELAY_MS;
      ESP_LOGW("epaper_forecast", "Retrying forecast request for %s in %u seconds: %s",
        entity_id.c_str(), (unsigned) (EPAPER_DASHBOARD_FORECAST_RETRY_DELAY_MS / 1000),
        reason ? reason : "failed");
      return true;
    }
  }
  ESP_LOGW("epaper_forecast", "Too many delayed forecast retries; skipping %s",
    entity_id.c_str());
  return false;
}

inline bool epaper_dashboard_forecast_enqueue(const std::string &entity_id) {
  if (!epaper_dashboard_forecast_entity_id_safe(entity_id)) return false;
  epaper_dashboard_forecast_clear_retry(entity_id);
  if (epaper_dashboard_forecast_pending_key(entity_id) ||
      epaper_dashboard_forecast_queue_key(entity_id)) {
    return true;
  }
  EpaperDashboardForecastQueuedRequest *requests = epaper_dashboard_forecast_queued_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) {
      requests[i].entity_id = entity_id;
      return true;
    }
  }
  ESP_LOGW("epaper_forecast", "Too many queued forecast requests; skipping %s",
    entity_id.c_str());
  return false;
}

inline bool epaper_dashboard_forecast_dequeue(std::string &entity_id) {
  EpaperDashboardForecastQueuedRequest *requests = epaper_dashboard_forecast_queued_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) continue;
    entity_id = requests[i].entity_id;
    requests[i] = EpaperDashboardForecastQueuedRequest();
    return true;
  }
  return false;
}

inline bool epaper_dashboard_forecast_enqueue_due_retries() {
  if (!ha_api_state_connected()) return false;
  EpaperDashboardForecastRetryRequest *requests = epaper_dashboard_forecast_retry_requests();
  uint32_t now = esphome::millis();
  bool queued = false;
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    if (requests[i].entity_id.empty()) continue;
    if ((int32_t) (now - requests[i].due_ms) < 0) continue;
    std::string entity_id = requests[i].entity_id;
    requests[i] = EpaperDashboardForecastRetryRequest();
    queued = epaper_dashboard_forecast_enqueue(entity_id) || queued;
  }
  return queued;
}

inline void epaper_dashboard_forecast_send_next_queued();

inline void epaper_dashboard_forecast_cancel_pending_requests() {
  epaper_dashboard_forecast_action_ready_ms() = 0;
  EpaperDashboardForecastPendingRequest *requests = epaper_dashboard_forecast_pending_requests();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    uint32_t call_id = requests[i].call_id;
    if (call_id == 0) continue;
    std::string entity_id = requests[i].entity_id;
    requests[i] = EpaperDashboardForecastPendingRequest();
    ha_cancel_action_response_callback(call_id, "api disconnected");
    epaper_dashboard_forecast_schedule_retry(entity_id, "api disconnected");
  }
}

inline bool epaper_dashboard_forecast_cancel_stale_requests() {
  EpaperDashboardForecastPendingRequest *requests = epaper_dashboard_forecast_pending_requests();
  uint32_t now = esphome::millis();
  for (int i = 0; i < EPAPER_DASHBOARD_FORECAST_PENDING_MAX; i++) {
    uint32_t call_id = requests[i].call_id;
    if (call_id == 0) continue;
    if (now - requests[i].started_ms < EPAPER_DASHBOARD_FORECAST_REQUEST_TIMEOUT_MS) continue;
    std::string entity_id = requests[i].entity_id;
    requests[i] = EpaperDashboardForecastPendingRequest();
    ESP_LOGW("epaper_forecast", "Cancelling forecast request %u for %s: timeout",
      (unsigned) call_id, entity_id.c_str());
    ha_cancel_action_response_callback(call_id, "timeout");
    return true;
  }
  return false;
}

inline void epaper_dashboard_request_forecast_entity(const std::string &entity_id) {
  if (!epaper_dashboard_forecast_entity_id_safe(entity_id) ||
      !ha_api_state_connected() ||
      !epaper_dashboard_forecast_actions_ready()) {
    epaper_dashboard_apply_forecast_unavailable(entity_id);
    return;
  }
  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = epaper_dashboard_next_forecast_call_id();
  if (!ha_action_begin(req, "weather.get_forecasts", false, 2, call_id)) {
    epaper_dashboard_apply_forecast_unavailable(entity_id);
    epaper_dashboard_forecast_schedule_retry(entity_id, "request setup failed");
    return;
  }
  req.wants_response = true;
  std::string response_template = epaper_dashboard_forecast_response_template(entity_id);
  req.response_template = decltype(req.response_template)(response_template);
  ha_action_add_entity(req, entity_id);
  ha_action_add_data(req, "type", "daily");
  if (!ha_register_action_response_callback(
      req.call_id,
      [entity_id, call_id = req.call_id](const esphome::api::ActionResponse &response) {
        epaper_dashboard_forecast_clear_pending(call_id);
        if (!response.is_success()) {
          std::string error = response.get_error_message();
          epaper_dashboard_apply_forecast_unavailable(
            entity_id, error == "timeout" ? "HA Actions" : "");
          epaper_dashboard_forecast_schedule_retry(entity_id, error.c_str());
          epaper_dashboard_forecast_send_next_queued();
          return;
        }
        const char *payload = response.get_json()["response"].as<const char *>();
        EpaperDashboardForecastPayload forecast;
        if (payload == nullptr || !epaper_dashboard_parse_forecast_payload(payload, forecast)) {
          epaper_dashboard_apply_forecast_unavailable(entity_id);
          epaper_dashboard_forecast_schedule_retry(entity_id, payload == nullptr ? "empty response" : "invalid response");
          epaper_dashboard_forecast_send_next_queued();
          return;
        }
        epaper_dashboard_apply_forecast_to_entity(entity_id, forecast);
        epaper_dashboard_forecast_send_next_queued();
      })) {
    epaper_dashboard_apply_forecast_unavailable(entity_id);
    epaper_dashboard_forecast_schedule_retry(entity_id, "callback setup failed");
    return;
  }
  if (!epaper_dashboard_forecast_track_pending(req.call_id, entity_id)) {
    ha_cancel_action_response_callback(req.call_id, "too many pending forecasts");
    epaper_dashboard_apply_forecast_unavailable(entity_id);
    return;
  }
  ESP_LOGI("epaper_forecast", "Requesting daily forecast for %s", entity_id.c_str());
  if (!ha_action_send(req)) {
    epaper_dashboard_forecast_clear_pending(req.call_id);
    ha_cancel_action_response_callback(req.call_id, "send failed");
    epaper_dashboard_apply_forecast_unavailable(entity_id);
    epaper_dashboard_forecast_schedule_retry(entity_id, "send failed");
    epaper_dashboard_forecast_send_next_queued();
  }
}

inline void epaper_dashboard_forecast_send_next_queued() {
  if (!epaper_dashboard_forecast_actions_ready() ||
      epaper_dashboard_forecast_any_pending()) {
    return;
  }
  epaper_dashboard_forecast_enqueue_due_retries();
  std::string entity_id;
  if (!epaper_dashboard_forecast_dequeue(entity_id)) return;
  epaper_dashboard_request_forecast_entity(entity_id);
}

inline void epaper_dashboard_refresh_weather_forecasts() {
  std::vector<std::string> requested;
  for (const auto &tile : epaper_dashboard_tiles()) {
    if (!epaper_dashboard_weather_forecast_card(tile) || tile.entity.empty()) continue;
    bool already_requested = false;
    for (const auto &existing : requested) {
      if (existing == tile.entity) {
        already_requested = true;
        break;
      }
    }
    if (already_requested) continue;
    requested.push_back(tile.entity);
    epaper_dashboard_forecast_enqueue(tile.entity);
  }
  epaper_dashboard_forecast_send_next_queued();
}

inline void epaper_dashboard_weather_forecast_tick() {
  if (!ha_api_state_connected()) {
    epaper_dashboard_forecast_cancel_pending_requests();
    return;
  }
  bool cancelled = epaper_dashboard_forecast_cancel_stale_requests();
  if (cancelled) epaper_dashboard_forecast_send_next_queued();
  else epaper_dashboard_forecast_send_next_queued();
}

inline std::string epaper_dashboard_weather_forecast_value(const EpaperDashboardTile &tile) {
  if (!tile.forecast_valid) return "--/--";
  char buf[24];
  char high_buf[12];
  char low_buf[12];
  if (tile.forecast_high == 32767) snprintf(high_buf, sizeof(high_buf), "--");
  else snprintf(high_buf, sizeof(high_buf), "%d", tile.forecast_high);
  if (tile.forecast_low == 32767) snprintf(low_buf, sizeof(low_buf), "--");
  else snprintf(low_buf, sizeof(low_buf), "%d", tile.forecast_low);
  snprintf(buf, sizeof(buf), "%s/%s", high_buf, low_buf);
  return buf;
}

inline std::string epaper_dashboard_alarm_label_for_state(const std::string &state) {
  if (state.empty()) return "Unavailable";
  if (state == "disarmed") return "Disarmed";
  if (state == "armed_away") return "Armed Away";
  if (state == "armed_home") return "Armed Home";
  if (state == "armed_night") return "Armed Night";
  if (state == "armed_custom_bypass") return "Armed Custom";
  if (state == "arming") return "Arming";
  if (state == "pending") return "Pending";
  if (state == "triggered") return "Triggered";
  if (state == "unavailable") return "Unavailable";
  if (state == "unknown") return "Unknown";
  return epaper_dashboard_pretty_state(state);
}

inline std::string epaper_dashboard_alarm_normalized_arm_mode(const std::string &arm_mode) {
  if (arm_mode == "away") return "armed_away";
  if (arm_mode == "home") return "armed_home";
  if (arm_mode == "night") return "armed_night";
  if (arm_mode == "armed_away" || arm_mode == "armed_home" ||
      arm_mode == "armed_night" || arm_mode == "armed_custom_bypass") {
    return arm_mode;
  }
  return "";
}

inline std::string epaper_dashboard_alarm_effective_state(const EpaperDashboardTile &tile) {
  if (tile.state != "arming") return tile.state;
  std::string normalized = epaper_dashboard_alarm_normalized_arm_mode(tile.secondary_value);
  return normalized.empty() ? tile.state : normalized;
}

inline bool epaper_dashboard_alarm_state_active(const std::string &state) {
  return state == "arming" || state == "pending" || state == "triggered" ||
         state.compare(0, 5, "armed") == 0;
}

inline std::string epaper_dashboard_alarm_action_achieved_state(const std::string &mode) {
  if (mode == "away") return "armed_away";
  if (mode == "home") return "armed_home";
  if (mode == "disarm") return "disarmed";
  return "";
}

inline bool epaper_dashboard_alarm_action_matches(const EpaperDashboardTile &tile) {
  std::string achieved = epaper_dashboard_alarm_action_achieved_state(tile.sensor);
  return !achieved.empty() && epaper_dashboard_alarm_effective_state(tile) == achieved;
}

inline bool epaper_dashboard_garage_state_active(const std::string &state) {
  return state == "open" || state == "opening" || state == "closing";
}

inline bool epaper_dashboard_garage_state_uses_open_icon(const std::string &state) {
  return state == "open" || state == "opening";
}

inline bool epaper_dashboard_cover_toggle_mode(const std::string &mode) {
  return mode == "toggle";
}

inline bool epaper_dashboard_cover_toggle_state_active(const std::string &state) {
  return state == "closed" || state == "closing";
}

inline bool epaper_dashboard_lock_state_active(const std::string &state) {
  return state == "unlocked" || state == "unlocking" ||
         state == "open" || state == "opening" ||
         state == "jammed";
}

inline const char *epaper_dashboard_alarm_action_icon(const std::string &mode) {
  if (mode == "home") return find_icon("Shield Home");
  if (mode == "disarm") return find_icon("Shield Off");
  return find_icon("Shield Lock");
}

inline std::string epaper_dashboard_alarm_action_label(const std::string &mode) {
  if (mode == "home") return "Arm Home";
  if (mode == "disarm") return "Disarm";
  return "Arm Away";
}

inline bool epaper_dashboard_fan_non_speed_card(const EpaperDashboardTile &tile) {
  return tile.type == "fan_switch" || tile.type == "fan_oscillate" ||
         tile.type == "fan_direction" || tile.type == "fan_preset";
}

inline bool epaper_dashboard_fan_preset_active(const std::string &value) {
  std::string state = epaper_dashboard_normalized_state_text(value);
  return !state.empty() && state != "none" && state != "off" &&
         !epaper_dashboard_state_unavailable(state);
}

inline std::string epaper_dashboard_fan_option_label(const std::string &value) {
  if (value.empty()) return "None";
  return epaper_dashboard_text_sensor_display_text(value);
}

inline bool epaper_dashboard_fan_token_is_header(const std::string &value) {
  std::string state = epaper_dashboard_normalized_state_text(value);
  return state == "presetmode" || state == "presetmodes" ||
         state == "direction" || state == "oscillating";
}

inline std::vector<std::string> epaper_dashboard_fan_parse_options(esphome::StringRef value) {
  std::string raw = epaper_dashboard_string_ref_limited(value, EPAPER_DASHBOARD_FRIENDLY_NAME_MAX_LEN * 4);
  std::vector<std::string> out;
  std::string cur;
  bool quoted = false;
  char quote_char = 0;
  for (char ch : raw) {
    if (ch == '\'' || ch == '"') {
      if (quoted && ch == quote_char) {
        quoted = false;
        quote_char = 0;
      } else if (!quoted) {
        quoted = true;
        quote_char = ch;
      } else {
        cur.push_back(ch);
      }
      continue;
    }
    if (!quoted && (ch == '[' || ch == ']')) continue;
    if (!quoted && ch == ',') {
      std::string item = epaper_dashboard_trim(cur);
      if (!item.empty() && !epaper_dashboard_fan_token_is_header(item)) {
        std::string normalized = epaper_dashboard_normalized_state_text(item);
        bool duplicate = false;
        for (const auto &existing : out) {
          if (epaper_dashboard_normalized_state_text(existing) == normalized) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate) out.push_back(item);
      }
      cur.clear();
      continue;
    }
    cur.push_back(ch);
  }
  std::string item = epaper_dashboard_trim(cur);
  if (!item.empty() && !epaper_dashboard_fan_token_is_header(item)) {
    std::string normalized = epaper_dashboard_normalized_state_text(item);
    bool duplicate = false;
    for (const auto &existing : out) {
      if (epaper_dashboard_normalized_state_text(existing) == normalized) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) out.push_back(item);
  }
  return out;
}

inline std::string epaper_dashboard_fan_default_label(const EpaperDashboardTile &tile) {
  if (tile.type == "fan_oscillate") return "Oscillation";
  if (tile.type == "fan_direction") return "Direction";
  if (tile.type == "fan_preset") return "Preset";
  return "Fan";
}

inline const char *epaper_dashboard_fan_default_icon_name(const EpaperDashboardTile &tile) {
  if (tile.type == "fan_switch") return "Fan Off";
  if (tile.type == "fan_speed") return "Fan Speed 2";
  if (tile.type == "fan_direction") return "Swap Horizontal";
  if (tile.type == "fan_preset") return "Fan Auto";
  return "Fan";
}

inline bool epaper_dashboard_fan_attribute_known(const EpaperDashboardTile &tile) {
  return !tile.sensor_value.empty() && !tile.sensor_unavailable;
}

inline std::string epaper_dashboard_fan_status_text(const EpaperDashboardTile &tile) {
  if (tile.state_unavailable) return "Unavailable";
  if (tile.type == "fan_switch") {
    if (!tile.state.empty()) return epaper_dashboard_state_active(tile.state) ? "On" : "Off";
    return "...";
  }
  if (tile.type == "fan_oscillate") {
    bool oscillating = false;
    if (!epaper_dashboard_bool_value(tile.sensor_value, oscillating)) return "Unsupported";
    return oscillating ? "Oscillating" : "Still";
  }
  if (tile.type == "fan_direction") {
    std::string direction = epaper_dashboard_normalized_state_text(tile.sensor_value);
    if (direction != "forward" && direction != "reverse") return "Unsupported";
    return epaper_dashboard_fan_option_label(direction);
  }
  if (tile.type == "fan_preset") {
    if (tile.fan_preset_modes.empty()) return "Unsupported";
    return epaper_dashboard_fan_preset_active(tile.sensor_value)
      ? epaper_dashboard_fan_option_label(tile.sensor_value)
      : std::string("Preset");
  }
  return "Fan";
}

inline bool epaper_dashboard_window_card(const EpaperDashboardTile &tile) {
  return tile.type == "door_window" && tile.precision == "window";
}

inline const char *epaper_dashboard_alarm_icon_for_state(const std::string &state) {
  if (state == "armed_home") return find_icon("Shield Home");
  if (state == "armed_away" || state == "armed_custom_bypass") return find_icon("Shield Lock");
  if (state == "armed_night") return find_icon("Weather Night");
  if (state == "disarmed") return find_icon("Shield Off");
  if (state == "triggered") return find_icon("Alarm Light");
  return find_icon("Alarm");
}

inline bool epaper_dashboard_api_available() {
  return esphome::api::global_api_server != nullptr;
}

inline std::string epaper_dashboard_display_value(const EpaperDashboardTile &tile);
inline std::string epaper_dashboard_display_unit(const EpaperDashboardTile &tile);

inline bool epaper_dashboard_parse_float(esphome::StringRef state, float &out) {
  std::string text(state.c_str(), state.size());
  char *end = nullptr;
  float value = std::strtof(text.c_str(), &end);
  if (end == text.c_str()) return false;
  out = value;
  return true;
}

inline std::string &epaper_dashboard_indoor_temperature_entity() {
  static std::string entity;
  return entity;
}

inline std::string &epaper_dashboard_outdoor_temperature_entity() {
  static std::string entity;
  return entity;
}

inline void epaper_dashboard_format_temperature(char *buf, size_t size, float value) {
  if (!buf || size == 0) return;
  if (std::isnan(value)) {
    snprintf(buf, size, "-");
    return;
  }
  snprintf(buf, size, "%.0f", value);
}

inline void epaper_dashboard_set_temperature_label(lv_obj_t *label,
                                                   bool indoor_on, bool outdoor_on,
                                                   float indoor, float outdoor) {
  if (!label) return;
  if (!indoor_on && !outdoor_on) {
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);

  char indoor_text[16];
  char outdoor_text[16];
  char text[40];
  const char *suffix = display_clock_bar_temperature_suffix();
  epaper_dashboard_format_temperature(indoor_text, sizeof(indoor_text), indoor);
  epaper_dashboard_format_temperature(outdoor_text, sizeof(outdoor_text), outdoor);
  if (indoor_on && outdoor_on) {
    snprintf(text, sizeof(text), "%s%s / %s%s", outdoor_text, suffix, indoor_text, suffix);
  } else if (outdoor_on) {
    snprintf(text, sizeof(text), "%s%s", outdoor_text, suffix);
  } else {
    snprintf(text, sizeof(text), "%s%s", indoor_text, suffix);
  }
  lv_label_set_text(label, text);
}

inline void epaper_dashboard_refresh_temperature_label(lv_obj_t *label, lv_obj_t *main_page_obj,
                                                       bool indoor_on, bool outdoor_on,
                                                       float indoor, float outdoor) {
  if (!label || !main_page_obj) return;
  epaper_dashboard_set_temperature_label(label, indoor_on, outdoor_on, indoor, outdoor);
  if (lv_scr_act() != main_page_obj) lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
  epaper_dashboard_mark_dirty();
}

inline void epaper_dashboard_configure_temperatures(bool indoor_on, bool outdoor_on,
                                                    const std::string &indoor_entity,
                                                    const std::string &outdoor_entity,
                                                    const std::string &temperature_unit,
                                                    const std::string &timezone,
                                                    bool degree_symbol,
                                                    lv_obj_t *label,
                                                    lv_obj_t *main_page_obj,
                                                    float *indoor_temp,
                                                    float *outdoor_temp) {
  set_display_temperature_unit(temperature_unit, timezone);
  set_display_temperature_degree_symbol(degree_symbol);
  float indoor = indoor_temp ? *indoor_temp : NAN;
  float outdoor = outdoor_temp ? *outdoor_temp : NAN;
  epaper_dashboard_refresh_temperature_label(label, main_page_obj, indoor_on, outdoor_on, indoor, outdoor);
  if (!epaper_dashboard_api_available()) return;

  if (indoor_on && !indoor_entity.empty() &&
      epaper_dashboard_indoor_temperature_entity() != indoor_entity) {
    epaper_dashboard_indoor_temperature_entity() = indoor_entity;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        indoor_entity, {}, [label, main_page_obj, indoor_on, outdoor_on, indoor_temp, outdoor_temp](esphome::StringRef state) {
          if (!indoor_temp) return;
          float parsed = NAN;
          if (!epaper_dashboard_parse_float(state, parsed)) return;
          *indoor_temp = parsed;
          float outdoor = outdoor_temp ? *outdoor_temp : NAN;
          epaper_dashboard_refresh_temperature_label(label, main_page_obj, indoor_on, outdoor_on, *indoor_temp, outdoor);
        });
  }

  if (outdoor_on && !outdoor_entity.empty() &&
      epaper_dashboard_outdoor_temperature_entity() != outdoor_entity) {
    epaper_dashboard_outdoor_temperature_entity() = outdoor_entity;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        outdoor_entity, {}, [label, main_page_obj, indoor_on, outdoor_on, indoor_temp, outdoor_temp](esphome::StringRef state) {
          if (!outdoor_temp) return;
          float parsed = NAN;
          if (!epaper_dashboard_parse_float(state, parsed)) return;
          *outdoor_temp = parsed;
          float indoor = indoor_temp ? *indoor_temp : NAN;
          epaper_dashboard_refresh_temperature_label(label, main_page_obj, indoor_on, outdoor_on, indoor, *outdoor_temp);
        });
  }
}

struct EpaperDashboardLvglSlot {
  lv_obj_t *tile = nullptr;
  lv_obj_t *icon = nullptr;
  lv_obj_t *sensor_container = nullptr;
  lv_obj_t *track = nullptr;
  lv_obj_t *track_fill = nullptr;
  lv_obj_t *label = nullptr;
  lv_obj_t *badge = nullptr;
  lv_obj_t *value = nullptr;
  lv_obj_t *unit = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *value_font = nullptr;
};

inline std::array<EpaperDashboardLvglSlot, EPAPER_DASHBOARD_PAGE_SLOTS> &epaper_dashboard_lvgl_slots() {
  static std::array<EpaperDashboardLvglSlot, EPAPER_DASHBOARD_PAGE_SLOTS> slots;
  return slots;
}

inline void epaper_dashboard_bind_lvgl_slot(int slot, lv_obj_t *tile, lv_obj_t *icon,
                                           lv_obj_t *sensor_container, lv_obj_t *label,
                                           lv_obj_t *value, lv_obj_t *unit = nullptr,
                                           lv_obj_t *badge = nullptr,
                                           lv_obj_t *track = nullptr,
                                           lv_obj_t *track_fill = nullptr) {
  if (slot < 0 || slot >= EPAPER_DASHBOARD_PAGE_SLOTS) return;
  epaper_dashboard_lvgl_slots()[slot] = {
    tile, icon, sensor_container, track, track_fill, label, badge, value, unit,
    label ? lv_obj_get_style_text_font(label, LV_PART_MAIN) : nullptr,
    value ? lv_obj_get_style_text_font(value, LV_PART_MAIN) : nullptr
  };
  if (tile) {
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
  }
}

inline bool epaper_dashboard_tile_configured(const EpaperDashboardTile &tile) {
  if (tile.config.empty()) return false;
  if (!tile.entity.empty() || !tile.sensor.empty() || !tile.action_state_entity.empty()) return true;
  return !tile.type.empty() && tile.type != "subpage";
}

inline bool epaper_dashboard_sensor_card_type(const EpaperDashboardTile &tile) {
  return tile.type == "sensor" || tile.type == "weather" || tile.type == "weather_forecast" ||
         tile.type == "calendar" || tile.type == "clock" || tile.type == "timezone";
}

inline bool epaper_dashboard_sensor_field_is_entity(const EpaperDashboardTile &tile) {
  if (tile.sensor.empty()) return false;
  if (tile.type == "media" || tile.type == "garage" || tile.type == "lock" ||
      tile.type == "cover" || tile.type == "climate" || tile.type == "alarm" ||
      tile.type == "alarm_action" || tile.type == "light_temperature" ||
      tile.type == "light_brightness" || tile.type == "slider" ||
      tile.type == "fan_speed" || tile.type == "fan_switch" ||
      tile.type == "fan_oscillate" || tile.type == "fan_direction" ||
      tile.type == "fan_preset" || tile.type == "option_select" ||
      tile.type == "todo" || tile.type == "internal") {
    return false;
  }
  return true;
}

inline std::string epaper_dashboard_sensor_source(const EpaperDashboardTile &tile) {
  if (!tile.action_state_entity.empty()) return tile.action_state_entity;
  return epaper_dashboard_sensor_field_is_entity(tile) ? tile.sensor : "";
}

inline bool epaper_dashboard_cover_command_mode(const std::string &mode) {
  return mode == "open" || mode == "close" || mode == "stop" || mode == "set_position";
}

inline bool epaper_dashboard_garage_command_mode(const std::string &mode) {
  return mode == "open" || mode == "close";
}

inline bool epaper_dashboard_lock_command_mode(const std::string &mode) {
  return mode == "lock" || mode == "unlock";
}

inline std::string epaper_dashboard_climate_label_mode(const EpaperDashboardTile &tile) {
  std::string mode = epaper_dashboard_option_value(tile.options, "label_display");
  if (mode == "status" || mode == "actual" || mode == "target") return mode;
  return "label";
}

inline std::string epaper_dashboard_climate_number_mode(const EpaperDashboardTile &tile) {
  std::string mode = epaper_dashboard_option_value(tile.options, "number_display");
  if (mode == "icon" || mode == "actual") return mode;
  return "target";
}

inline std::string epaper_dashboard_attribute_source(const EpaperDashboardTile &tile,
                                                     std::string &attribute) {
  attribute.clear();
  if (tile.entity.empty()) return "";
  if (tile.type == "light_brightness" || tile.type == "slider") {
    attribute = "brightness";
    return tile.entity;
  }
  if (tile.type == "fan_speed") {
    attribute = "percentage";
    return tile.entity;
  }
  if (tile.type == "fan_oscillate") {
    attribute = "oscillating";
    return tile.entity;
  }
  if (tile.type == "fan_direction") {
    attribute = "direction";
    return tile.entity;
  }
  if (tile.type == "fan_preset") {
    attribute = "preset_mode";
    return tile.entity;
  }
  if (tile.type == "cover" && epaper_dashboard_cover_command_mode(tile.sensor)) {
    return "";
  }
  if (tile.type == "cover" && tile.sensor != "toggle") {
    attribute = tile.sensor == "tilt" ? "current_tilt_position" : "current_position";
    return tile.entity;
  }
  if (tile.type == "light_temperature") {
    attribute = "color_temp_kelvin";
    return tile.entity;
  }
  if (tile.type == "media") {
    if (tile.sensor == "volume") {
      attribute = "volume_level";
      return tile.entity;
    }
    if (tile.sensor == "position") {
      attribute = "media_position";
      return tile.entity;
    }
    if (tile.sensor == "now_playing") {
      attribute = "media_title";
      return tile.entity;
    }
  }
  if (tile.type == "climate") {
    std::string mode = epaper_dashboard_climate_number_mode(tile);
    if (mode != "actual" && mode != "target") {
      mode = epaper_dashboard_climate_label_mode(tile);
    }
    if (mode == "actual") {
      attribute = "current_temperature";
      return tile.entity;
    }
    if (mode == "target") {
      attribute = "temperature";
      return tile.entity;
    }
  }
  return "";
}

inline std::string epaper_dashboard_secondary_attribute_source(const EpaperDashboardTile &tile,
                                                              std::string &attribute) {
  attribute.clear();
  if (tile.entity.empty()) return "";
  if (tile.type == "media" && tile.sensor == "now_playing") {
    attribute = "media_artist";
    return tile.entity;
  }
  if (tile.type == "media" && tile.sensor == "position") {
    attribute = "media_duration";
    return tile.entity;
  }
  if (tile.type == "alarm" || tile.type == "alarm_action") {
    attribute = "arm_mode";
    return tile.entity;
  }
  return "";
}

inline bool epaper_dashboard_has_sensor_value(const EpaperDashboardTile &tile) {
  return !epaper_dashboard_sensor_source(tile).empty() ||
         tile.type == "clock" || tile.type == "timezone" ||
         epaper_dashboard_todo_card_show_count(tile);
}

inline bool epaper_dashboard_card_large_numbers(const EpaperDashboardTile &tile) {
  return epaper_dashboard_option_present(tile.options, "large_numbers");
}

inline bool epaper_dashboard_card_large_numbers_supported(const EpaperDashboardTile &tile) {
  if (tile.type == "sensor") return tile.precision != "icon" && tile.precision != "text";
  if (epaper_dashboard_weather_forecast_card(tile)) return true;
  return tile.type.empty() || tile.type == "action" || tile.type == "calendar" ||
         tile.type == "clock" || tile.type == "climate" || tile.type == "media" ||
         tile.type == "timezone";
}

inline bool epaper_dashboard_text_sensor_card(const EpaperDashboardTile &tile) {
  return (tile.type == "sensor" && tile.precision == "text") || tile.type == "text_sensor";
}

inline bool epaper_dashboard_toggle_text_sensor_card(const EpaperDashboardTile &tile) {
  return tile.type.empty() && tile.precision == "text" && !tile.sensor.empty();
}

inline bool epaper_dashboard_toggle_numeric_sensor_card(const EpaperDashboardTile &tile) {
  return tile.type.empty() && tile.precision != "text" && !tile.sensor.empty();
}

inline bool epaper_dashboard_action_option_select(const EpaperDashboardTile &tile) {
  return tile.type == "action" &&
         (tile.sensor == "input_select.select_option" || tile.sensor == "select.select_option");
}

inline bool epaper_dashboard_option_select_card(const EpaperDashboardTile &tile) {
  return tile.type == "option_select" || epaper_dashboard_action_option_select(tile);
}

inline std::string epaper_dashboard_state_source(const EpaperDashboardTile &tile) {
  if (tile.entity.empty()) return "";
  if (tile.type == "action") {
    return epaper_dashboard_option_select_card(tile) ? tile.entity : "";
  }
  if (tile.type == "push" || tile.type == "webhook") return "";
  if (tile.type == "internal") return "";
  return tile.entity;
}

inline size_t epaper_dashboard_state_text_max_len(const EpaperDashboardTile &tile) {
  return epaper_dashboard_option_select_card(tile)
           ? EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN
           : EPAPER_DASHBOARD_SHORT_STATE_MAX_LEN;
}

inline std::string epaper_dashboard_action_state_precision(const EpaperDashboardTile &tile) {
  return tile.type == "action" ? epaper_dashboard_option_value(tile.options, "state_precision") : "";
}

inline bool epaper_dashboard_action_state_display_enabled(const EpaperDashboardTile &tile) {
  return !tile.action_state_entity.empty();
}

inline bool epaper_dashboard_action_state_icon_card(const EpaperDashboardTile &tile) {
  return epaper_dashboard_action_state_display_enabled(tile) &&
         epaper_dashboard_action_state_precision(tile) == "icon";
}

inline bool epaper_dashboard_action_state_text_card(const EpaperDashboardTile &tile) {
  if (!epaper_dashboard_action_state_display_enabled(tile)) return false;
  std::string precision = epaper_dashboard_action_state_precision(tile);
  return precision == "text" || (precision.empty() && tile.unit.empty());
}

inline bool epaper_dashboard_action_state_numeric_card(const EpaperDashboardTile &tile) {
  if (!epaper_dashboard_action_state_display_enabled(tile)) return false;
  std::string precision = epaper_dashboard_action_state_precision(tile);
  return precision == "0" || precision == "1" || precision == "2" || !tile.unit.empty();
}

inline bool epaper_dashboard_active_color_enabled(const EpaperDashboardTile &tile) {
  return epaper_dashboard_option_present(tile.options, "active_color");
}

inline const std::string &epaper_dashboard_binary_card_state(const EpaperDashboardTile &tile) {
  return !tile.state.empty() ? tile.state : tile.sensor_value;
}

inline bool epaper_dashboard_binary_card_unavailable(const EpaperDashboardTile &tile) {
  return !tile.state.empty() ? tile.state_unavailable : tile.sensor_unavailable;
}

inline bool epaper_dashboard_internal_push_mode(const EpaperDashboardTile &tile) {
  return tile.type == "internal" && tile.sensor == "push";
}

inline bool epaper_dashboard_value_replaces_icon(const EpaperDashboardTile &tile) {
  if (epaper_dashboard_text_sensor_card(tile)) return false;
  if (epaper_dashboard_option_select_card(tile)) return true;
  if (epaper_dashboard_toggle_numeric_sensor_card(tile)) return true;
  if (tile.type == "sensor") return tile.precision != "icon";
  if (epaper_dashboard_weather_forecast_card(tile) ||
      tile.type == "calendar" || tile.type == "clock" || tile.type == "timezone") return true;
  if (tile.type == "action") {
    std::string mode = epaper_dashboard_option_value(tile.options, "state_precision");
    return epaper_dashboard_action_state_display_enabled(tile) &&
           mode != "icon" && mode != "text";
  }
  if (tile.type == "media") return tile.sensor == "volume" || tile.sensor == "position" ||
                                    tile.sensor == "now_playing";
  if (tile.type == "climate") {
    std::string mode = epaper_dashboard_climate_number_mode(tile);
    return mode == "actual" || mode == "target";
  }
  if (epaper_dashboard_todo_card_show_count(tile)) return true;
  return epaper_dashboard_card_large_numbers(tile) && epaper_dashboard_has_sensor_value(tile);
}

inline bool epaper_dashboard_tile_active(const EpaperDashboardTile &tile) {
  if (tile.type == "alarm") {
    return epaper_dashboard_alarm_state_active(epaper_dashboard_alarm_effective_state(tile));
  }
  if (tile.type == "alarm_action") {
    return !tile.state_unavailable && epaper_dashboard_alarm_action_matches(tile);
  }
  if (tile.type == "climate") {
    return epaper_dashboard_climate_active(tile);
  }
  if (tile.type == "fan_oscillate") {
    bool oscillating = false;
    return !tile.state_unavailable &&
           epaper_dashboard_bool_value(tile.sensor_value, oscillating) && oscillating;
  }
  if (tile.type == "fan_direction") {
    return !tile.state_unavailable &&
           epaper_dashboard_normalized_state_text(tile.sensor_value) == "reverse";
  }
  if (tile.type == "fan_preset") {
    return !tile.state_unavailable &&
           epaper_dashboard_fan_preset_active(tile.sensor_value);
  }
  if (tile.type == "garage" && !epaper_dashboard_garage_command_mode(tile.sensor)) {
    return !tile.state_unavailable && epaper_dashboard_garage_state_active(tile.state);
  }
  if (tile.type == "garage" && epaper_dashboard_garage_command_mode(tile.sensor)) {
    return false;
  }
  if (tile.type == "lock" && !epaper_dashboard_lock_command_mode(tile.sensor)) {
    return !tile.state_unavailable && epaper_dashboard_lock_state_active(tile.state);
  }
  if (tile.type == "lock" && epaper_dashboard_lock_command_mode(tile.sensor)) {
    return false;
  }
  if (tile.type == "cover" && epaper_dashboard_cover_command_mode(tile.sensor)) {
    return false;
  }
  if (tile.type == "cover" && epaper_dashboard_cover_toggle_mode(tile.sensor)) {
    return !tile.state_unavailable &&
           epaper_dashboard_cover_toggle_state_active(tile.state);
  }
  if (tile.type == "door_window") {
    return !epaper_dashboard_binary_card_unavailable(tile) &&
           epaper_dashboard_active_color_enabled(tile) &&
           epaper_dashboard_state_active(epaper_dashboard_binary_card_state(tile));
  }
  if (tile.type == "presence") {
    const std::string &state = epaper_dashboard_binary_card_state(tile);
    return !epaper_dashboard_binary_card_unavailable(tile) &&
           epaper_dashboard_active_color_enabled(tile) &&
           (epaper_dashboard_normalized_state_text(state) == "detected" ||
            epaper_dashboard_state_active(state));
  }
  if (tile.type == "action" && !tile.action_state_entity.empty()) {
    return !tile.sensor_unavailable && epaper_dashboard_state_active(tile.sensor_value);
  }
  if (tile.type == "sensor") {
    if (tile.sensor_unavailable) return false;
    return (tile.precision == "icon" || epaper_dashboard_active_color_enabled(tile)) &&
           epaper_dashboard_state_active(tile.sensor_value);
  }
  const std::string &active_value = !tile.state.empty() ? tile.state : tile.sensor_value;
  return epaper_dashboard_state_active(active_value);
}

inline bool epaper_dashboard_slider_visual_card(const EpaperDashboardTile &tile) {
  if (tile.type == "light_brightness" || tile.type == "slider" ||
      tile.type == "light_temperature" || tile.type == "fan_speed") return true;
  if (tile.type == "cover") {
    return tile.sensor != "toggle" && !epaper_dashboard_cover_command_mode(tile.sensor);
  }
  if (tile.type == "media") {
    return tile.sensor == "position" ||
           (tile.sensor == "now_playing" && (tile.precision == "progress" ||
                                             tile.precision == "play_pause"));
  }
  return false;
}

inline bool epaper_dashboard_wide_large_clock_card(const EpaperDashboardTile &tile,
                                                   int row_span, int col_span) {
  return tile.type == "clock" && epaper_dashboard_card_large_numbers(tile) &&
         row_span == 1 && col_span == 2;
}

inline bool epaper_dashboard_media_now_playing_progress_card(const EpaperDashboardTile &tile) {
  return tile.type == "media" && tile.sensor == "now_playing" &&
         tile.precision == "progress";
}

inline bool epaper_dashboard_media_now_playing_play_pause_card(const EpaperDashboardTile &tile) {
  return tile.type == "media" && tile.sensor == "now_playing" &&
         tile.precision == "play_pause";
}

inline int epaper_dashboard_clamp_percent(int pct) {
  if (pct < 0) return 0;
  if (pct > 100) return 100;
  return pct;
}

inline bool epaper_dashboard_parse_float_value(const std::string &value, float &out) {
  if (value.empty() || epaper_dashboard_state_unavailable(value)) return false;
  char *end = nullptr;
  out = std::strtof(value.c_str(), &end);
  return end != value.c_str() && std::isfinite(out);
}

inline bool epaper_dashboard_parse_fixed_int(const char *text, size_t len,
                                             size_t pos, size_t digits,
                                             int &out) {
  if (!text || pos + digits > len) return false;
  int value = 0;
  for (size_t i = 0; i < digits; i++) {
    char c = text[pos + i];
    if (c < '0' || c > '9') return false;
    value = value * 10 + (c - '0');
  }
  out = value;
  return true;
}

inline int64_t epaper_dashboard_days_from_civil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

inline bool epaper_dashboard_parse_ha_timestamp(const std::string &value, time_t &epoch) {
  const char *s = value.c_str();
  size_t len = value.size();
  if (len < 19 || s[4] != '-' || s[7] != '-' ||
      (s[10] != 'T' && s[10] != ' ')) return false;
  int year, month, day, hour, minute, second;
  if (!epaper_dashboard_parse_fixed_int(s, len, 0, 4, year) ||
      !epaper_dashboard_parse_fixed_int(s, len, 5, 2, month) ||
      !epaper_dashboard_parse_fixed_int(s, len, 8, 2, day) ||
      !epaper_dashboard_parse_fixed_int(s, len, 11, 2, hour) ||
      !epaper_dashboard_parse_fixed_int(s, len, 14, 2, minute) ||
      !epaper_dashboard_parse_fixed_int(s, len, 17, 2, second)) {
    return false;
  }
  if (month < 1 || month > 12 || day < 1 || day > 31 ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
      second < 0 || second > 60) {
    return false;
  }

  size_t tz_pos = 19;
  while (tz_pos < len && s[tz_pos] != 'Z' && s[tz_pos] != '+' && s[tz_pos] != '-') tz_pos++;
  int offset_seconds = 0;
  if (tz_pos < len && (s[tz_pos] == '+' || s[tz_pos] == '-')) {
    int offset_hour, offset_minute;
    if (!epaper_dashboard_parse_fixed_int(s, len, tz_pos + 1, 2, offset_hour) ||
        tz_pos + 3 >= len || s[tz_pos + 3] != ':' ||
        !epaper_dashboard_parse_fixed_int(s, len, tz_pos + 4, 2, offset_minute) ||
        offset_hour > 23 || offset_minute > 59) {
      return false;
    }
    offset_seconds = (offset_hour * 60 + offset_minute) * 60;
    if (s[tz_pos] == '-') offset_seconds = -offset_seconds;
  }

  int64_t days = epaper_dashboard_days_from_civil(
      year, static_cast<unsigned>(month), static_cast<unsigned>(day));
  int64_t seconds_since_epoch = days * 86400 + hour * 3600 + minute * 60 + second;
  seconds_since_epoch -= offset_seconds;
  if (seconds_since_epoch < 0) return false;
  epoch = static_cast<time_t>(seconds_since_epoch);
  return true;
}

inline bool epaper_dashboard_media_adjusted_position_seconds(
    const EpaperDashboardTile &tile, const std::string &position_value,
    float duration, float &out) {
  float position = 0.0f;
  if (!epaper_dashboard_parse_float_value(position_value, position)) return false;
  if (position < 0.0f) position = 0.0f;
  if (tile.state == "playing") {
    bool adjusted = false;
    if (!tile.media_position_updated_at_unavailable &&
        !tile.media_position_updated_at_value.empty()) {
      time_t updated_epoch = 0;
      time_t now_epoch = std::time(nullptr);
      if (now_epoch > 0 &&
          epaper_dashboard_parse_ha_timestamp(tile.media_position_updated_at_value, updated_epoch) &&
          updated_epoch > 0 && updated_epoch <= now_epoch) {
        position += static_cast<float>(now_epoch - updated_epoch);
        adjusted = true;
      }
    }
    if (!adjusted && tile.media_position_received_ms > 0) {
      position += (esphome::millis() - tile.media_position_received_ms) / 1000.0f;
    }
  }
  if (duration > 0.0f && position > duration) position = duration;
  out = position;
  return true;
}

inline int epaper_dashboard_light_brightness_percent(const std::string &value) {
  float brightness = 0.0f;
  if (!epaper_dashboard_parse_float_value(value, brightness)) return 0;
  if (brightness <= 0.0f) return 0;
  int pct = epaper_dashboard_clamp_percent(static_cast<int>((brightness * 100.0f + 127.0f) / 255.0f));
  return pct < 1 ? 1 : pct;
}

inline int epaper_dashboard_percent_value(const std::string &value) {
  float pct = 0.0f;
  if (!epaper_dashboard_parse_float_value(value, pct)) return 0;
  return epaper_dashboard_clamp_percent(static_cast<int>(pct + 0.5f));
}

inline void epaper_dashboard_parse_kelvin_range(const std::string &unit,
                                                int &min_k, int &max_k) {
  min_k = 2000;
  max_k = 6500;
  if (unit.empty()) return;
  size_t dash = unit.find('-');
  if (dash == std::string::npos || dash == 0) return;
  int parsed_min = std::atoi(unit.substr(0, dash).c_str());
  int parsed_max = std::atoi(unit.substr(dash + 1).c_str());
  if (parsed_min >= 1000 && parsed_max > parsed_min) {
    min_k = parsed_min;
    max_k = parsed_max;
  }
}

inline int epaper_dashboard_light_temperature_percent(const EpaperDashboardTile &tile) {
  float kelvin = 0.0f;
  if (!epaper_dashboard_parse_float_value(tile.sensor_value, kelvin)) return 0;
  int min_k = 2000;
  int max_k = 6500;
  epaper_dashboard_parse_kelvin_range(tile.unit, min_k, max_k);
  if (kelvin < min_k) kelvin = min_k;
  if (kelvin > max_k) kelvin = max_k;
  int range = max_k - min_k;
  if (range <= 0) return 50;
  return epaper_dashboard_clamp_percent(static_cast<int>(((kelvin - min_k) * 100.0f / range) + 0.5f));
}

inline int epaper_dashboard_track_fill_percent(const EpaperDashboardTile &tile) {
  if (tile.state_unavailable) return 0;
  if ((tile.type == "light_brightness" || tile.type == "slider" ||
       tile.type == "fan_speed") &&
      !tile.state.empty() && !epaper_dashboard_state_active(tile.state)) return 0;
  if (tile.type == "light_temperature" &&
      !epaper_dashboard_state_active(tile.state)) return 0;
  if ((tile.type == "light_brightness" || tile.type == "slider") &&
      !tile.sensor_value.empty()) return epaper_dashboard_light_brightness_percent(tile.sensor_value);
  if (tile.type == "cover" && !tile.sensor_value.empty()) {
    return 100 - epaper_dashboard_percent_value(tile.sensor_value);
  }
  if (tile.type == "fan_speed" &&
      !tile.sensor_value.empty()) return epaper_dashboard_percent_value(tile.sensor_value);
  if (tile.type == "light_temperature" && !tile.sensor_value.empty()) {
    return epaper_dashboard_light_temperature_percent(tile);
  }
  if (tile.type == "media" && tile.sensor == "position" && !tile.sensor_value.empty()) {
    float duration = 0.0f;
    float position = 0.0f;
    if (epaper_dashboard_parse_float_value(tile.secondary_value, duration) &&
        duration > 0.0f &&
        epaper_dashboard_media_adjusted_position_seconds(tile, tile.sensor_value, duration, position)) {
      return epaper_dashboard_clamp_percent(static_cast<int>((position * 100.0f / duration) + 0.5f));
    }
    return 0;
  }
  if (epaper_dashboard_media_now_playing_progress_card(tile)) {
    if (tile.media_position_unavailable || tile.media_duration_unavailable) return 0;
    float duration = 0.0f;
    float position = 0.0f;
    if (epaper_dashboard_parse_float_value(tile.media_duration_value, duration) &&
        duration > 0.0f &&
        epaper_dashboard_media_adjusted_position_seconds(tile, tile.media_position_value, duration, position)) {
      return epaper_dashboard_clamp_percent(static_cast<int>((position * 100.0f / duration) + 0.5f));
    }
    return 0;
  }
  if (epaper_dashboard_media_now_playing_play_pause_card(tile)) return 0;
  if (tile.type == "cover" && tile.sensor == "tilt") return 35;
  if (tile.type == "light_temperature") return 0;
  if (tile.type == "fan_speed") return 40;
  return 50;
}

inline const char *epaper_dashboard_icon(const EpaperDashboardTile &tile, bool active) {
  std::string icon = active && !tile.icon_on.empty() && tile.icon_on != "Auto" ? tile.icon_on : tile.icon;
  if (tile.type == "cover") {
    if (epaper_dashboard_cover_command_mode(tile.sensor)) {
      if (!tile.icon.empty() && tile.icon != "Auto") return find_icon(tile.icon.c_str());
      return find_icon("Blinds");
    }
    float cover_position = 0.0f;
    bool open_icon = epaper_dashboard_cover_toggle_mode(tile.sensor)
      ? epaper_dashboard_garage_state_uses_open_icon(tile.state)
      : (epaper_dashboard_parse_float_value(tile.sensor_value, cover_position)
           ? epaper_dashboard_clamp_percent(static_cast<int>(cover_position + 0.5f)) > 0
           : (!tile.state_unavailable && epaper_dashboard_state_active(tile.state)));
    std::string cover_icon = open_icon && !tile.icon_on.empty() && tile.icon_on != "Auto"
      ? tile.icon_on
      : tile.icon;
    if (!cover_icon.empty() && cover_icon != "Auto") return find_icon(cover_icon.c_str());
    return find_icon(open_icon ? "Blinds Open" : "Blinds");
  }
  if (tile.type == "garage") {
    if (epaper_dashboard_garage_command_mode(tile.sensor)) {
      if (!tile.icon.empty() && tile.icon != "Auto") return find_icon(tile.icon.c_str());
      return find_icon(tile.sensor == "open" ? "Garage Open" : "Garage");
    }
    bool open_icon = epaper_dashboard_garage_state_uses_open_icon(tile.state);
    std::string garage_icon = open_icon && !tile.icon_on.empty() && tile.icon_on != "Auto"
      ? tile.icon_on
      : tile.icon;
    if (!garage_icon.empty() && garage_icon != "Auto") return find_icon(garage_icon.c_str());
    return find_icon(open_icon ? "Garage Open" : "Garage");
  }
  if (tile.type == "weather" && !epaper_dashboard_weather_forecast_card(tile)) {
    return epaper_dashboard_weather_icon_for_state(tile.state);
  }
  if (tile.type == "alarm" &&
      epaper_dashboard_option_value(tile.options, "icon_display") != "static") {
    return epaper_dashboard_alarm_icon_for_state(epaper_dashboard_alarm_effective_state(tile));
  }
  if (!icon.empty() && icon != "Auto") return find_icon(icon.c_str());
  if (tile.type == "action") return find_icon("Flash");
  if (tile.type == "alarm") return find_icon("Security");
  if (tile.type == "alarm_action") return epaper_dashboard_alarm_action_icon(tile.sensor);
  if (tile.type == "climate") return find_icon("Thermostat");
  if (tile.type == "door_window") {
    if (epaper_dashboard_window_card(tile)) return find_icon(active ? "Window Open" : "Window Closed");
    return find_icon(active ? "Door Open" : "Door");
  }
  if (tile.type == "fan_speed" || tile.type == "fan_switch" ||
      tile.type == "fan_oscillate" || tile.type == "fan_direction" ||
      tile.type == "fan_preset") return find_icon(epaper_dashboard_fan_default_icon_name(tile));
  if (tile.type == "light_brightness" || tile.type == "light_switch") {
    return find_icon(active ? "Lightbulb" : "Lightbulb Outline");
  }
  if (tile.type == "light_temperature") return find_icon("Lightbulb");
  if (tile.type == "lock") return find_icon(active || tile.sensor == "unlock" ? "Lock Open" : "Lock");
  if (tile.type == "media") {
    if (tile.sensor == "previous") return find_icon("Skip Previous");
    if (tile.sensor == "next") return find_icon("Skip Next");
    if (tile.sensor == "volume") return find_icon("Volume High");
    if (tile.sensor == "position") return find_icon("Progress Clock");
    if (tile.sensor == "now_playing") return find_icon("Music");
    return find_icon("Play Pause");
  }
  if (tile.type == "presence") return find_icon(active ? "Motion Sensor" : "Motion Sensor Off");
  if (tile.type == "push") return find_icon("Gesture Tap");
  if (tile.type == "webhook") return find_icon("Flash");
  if (tile.type == "todo") return find_icon("Check");
  if (tile.type == "internal") {
    return find_icon(epaper_dashboard_internal_push_mode(tile) ? "Gesture Tap" : "Lightbulb Outline");
  }
  if (tile.type == "slider") {
    size_t dot = tile.entity.find('.');
    if (dot != std::string::npos) return domain_default_icon(tile.entity.substr(0, dot));
    return find_icon("Auto");
  }
  size_t dot = tile.entity.find('.');
  if (dot != std::string::npos) return domain_default_icon(tile.entity.substr(0, dot));
  return find_icon("Auto");
}

inline const char *epaper_dashboard_badge_icon(const EpaperDashboardTile &tile) {
  if (tile.type.empty()) {
    if (!tile.sensor.empty()) {
      return tile.precision == "text" ? find_icon("Format Text") : find_icon("Gauge");
    }
    return find_icon("Toggle Switch Variant Off");
  }
  if (tile.type == "sensor") {
    if (tile.precision == "icon") return find_icon("Toggle Switch");
    if (tile.precision == "text") return find_icon("Format Text");
    return find_icon("Gauge");
  }
  if (tile.type == "door_window") return find_icon(tile.precision == "window" ? "Window Closed" : "Door");
  if (tile.type == "presence") return find_icon("Motion Sensor");
  if (tile.type == "weather") {
    if (tile.precision == "forecast" || tile.precision == "today" ||
        tile.precision == "tomorrow") return find_icon("Weather Partly Cloudy");
    return find_icon("Weather Cloudy");
  }
  if (tile.type == "weather_forecast") return find_icon("Weather Partly Cloudy");
  if (tile.type == "calendar") return find_icon("Calendar Month");
  if (tile.type == "clock") return nullptr;
  if (tile.type == "timezone") return find_icon("Map Clock");
  if (tile.type == "media") return find_icon("Speaker");
  if (tile.type == "climate") return find_icon("Thermostat");
  if (tile.type == "garage") return find_icon("Garage");
  if (tile.type == "lock") return find_icon("Lock");
  if (tile.type == "alarm" || tile.type == "alarm_action") return nullptr;
  if (epaper_dashboard_option_select_card(tile)) {
    return find_icon("Chevron Down");
  }
  if (tile.type == "action") {
    return find_icon("Flash");
  }
  if (tile.type == "push") return find_icon("Gesture Tap");
  if (tile.type == "webhook") return find_icon("Webhook");
  if (tile.type == "todo") return find_icon("Check");
  if (tile.type == "internal") {
    return find_icon(epaper_dashboard_internal_push_mode(tile) ? "Gesture Tap" : "Power Plug");
  }
  if (tile.type == "light_brightness" || tile.type == "slider") return find_icon("Tune Vertical Variant");
  if (tile.type == "light_switch" || tile.type == "light_temperature") return find_icon("Lightbulb");
  if (tile.type == "fan_speed") return find_icon("Fan Speed 2");
  if (tile.type == "fan_switch") return find_icon("Fan");
  if (tile.type == "fan_oscillate") return find_icon("Sync");
  if (tile.type == "fan_direction") return find_icon("Swap Horizontal");
  if (tile.type == "fan_preset") return find_icon("Fan Auto");
  if (tile.type == "cover") return find_icon("Blinds Horizontal");
  return nullptr;
}

inline std::string epaper_dashboard_media_mode_label(const std::string &mode) {
  if (mode == "previous") return "Previous";
  if (mode == "next") return "Next";
  if (mode == "volume") return "Volume";
  if (mode == "position") return "Position";
  if (mode == "now_playing") return "Now Playing";
  return "Play/Pause";
}

inline std::string epaper_dashboard_media_status_text(const std::string &state) {
  if (state == "playing") return "Playing";
  if (state == "paused") return "Paused";
  if (state == "idle") return "Idle";
  if (state == "off") return "Off";
  if (state == "unavailable") return "Unavailable";
  if (state == "unknown" || state.empty()) return "Unknown";
  return epaper_dashboard_pretty_state(state);
}

inline std::string epaper_dashboard_default_label_source(const EpaperDashboardTile &tile) {
  if (epaper_dashboard_option_select_card(tile)) return "";
  if (epaper_dashboard_weather_forecast_card(tile)) return "";
  if (tile.type == "calendar" || tile.type == "clock" || tile.type == "timezone") return "";
  if (tile.type == "weather") return "";
  if (tile.type == "action") return "";
  if (tile.type == "media") return "";
  if (tile.type == "todo") return "";
  if (tile.type == "climate") return "";
  if (tile.type == "alarm") return "";
  if (tile.type == "internal") return "";
  if (tile.type == "garage" && !epaper_dashboard_garage_command_mode(tile.sensor)) return "";
  if (tile.type == "lock" && !epaper_dashboard_lock_command_mode(tile.sensor)) return "";
  if (tile.type.empty() && !tile.entity.empty()) return tile.entity;
  if (tile.type == "cover" &&
      (tile.sensor == "toggle" || epaper_dashboard_cover_command_mode(tile.sensor))) {
    return tile.entity;
  }
  if ((tile.type == "light_brightness" || tile.type == "light_temperature" ||
       tile.type == "slider" || tile.type == "fan_speed" || tile.type == "cover") &&
      !tile.entity.empty()) {
    return tile.entity;
  }
  if (!tile.sensor.empty()) return tile.sensor;
  return tile.entity;
}

inline std::string epaper_dashboard_friendly_label_source(const EpaperDashboardTile &tile) {
  if (tile.label_configured) return "";
  if (tile.type == "calendar" || tile.type == "clock" || tile.type == "timezone" ||
      tile.type == "weather" || tile.type == "weather_forecast" ||
      tile.type == "push" || tile.type == "webhook" || tile.type == "internal" ||
      tile.type == "alarm_action" || tile.type == "media") {
    return "";
  }
  if (tile.type == "todo" && !tile.entity.empty()) return tile.entity;
  if (tile.type == "action" && !epaper_dashboard_option_select_card(tile)) return "";
  if (tile.type == "garage" && epaper_dashboard_garage_command_mode(tile.sensor)) return "";
  if (tile.type == "garage" &&
      epaper_dashboard_option_value(tile.options, "label_display") == "status") {
    return "";
  }
  if (tile.type == "lock" && epaper_dashboard_lock_command_mode(tile.sensor)) return "";
  if (tile.type == "cover" &&
      (tile.sensor == "toggle" || epaper_dashboard_cover_command_mode(tile.sensor))) {
    return tile.entity;
  }
  if (tile.type == "climate") return "";
  if (tile.type == "alarm" &&
      epaper_dashboard_option_value(tile.options, "label_display") != "name") {
    return "";
  }
  if (tile.type == "alarm" && !tile.entity.empty()) return tile.entity;
  if (tile.type == "fan_direction" || tile.type == "fan_preset") return "";
  if (epaper_dashboard_option_select_card(tile) && !tile.entity.empty()) return tile.entity;
  if (tile.type.empty() && !tile.entity.empty()) return tile.entity;
  std::string sensor_source = epaper_dashboard_sensor_source(tile);
  if (!sensor_source.empty()) return sensor_source;
  return tile.entity;
}

inline std::string epaper_dashboard_tile_label(const EpaperDashboardTile &tile) {
  if (epaper_dashboard_text_sensor_card(tile)) return epaper_dashboard_display_value(tile);
  if (epaper_dashboard_toggle_text_sensor_card(tile) &&
      epaper_dashboard_state_active(tile.state) && !tile.sensor_value.empty()) {
    return epaper_dashboard_sensor_state_display_text(tile);
  }
  if (tile.type == "calendar") return epaper_dashboard_calendar_label(tile);
  if (tile.type == "clock") return "";
  if (tile.type == "timezone") {
    if (!tile.label.empty()) return tile.label;
    std::string timezone = tile.entity.empty()
      ? epaper_dashboard_time_state().active_timezone
      : tile.entity;
    return epaper_dashboard_timezone_city_label(timezone);
  }
  if (tile.type == "weather") {
    if (epaper_dashboard_weather_forecast_card(tile)) {
      if (!tile.forecast_status_label.empty()) return tile.forecast_status_label;
      if (tile.label_configured && !tile.label.empty()) return tile.label;
      return tile.precision == "today" ? "Today" : "Tomorrow";
    }
    if (!tile.state.empty()) return epaper_dashboard_weather_label_for_state(tile.state);
    return "Weather";
  }
  if (tile.type == "weather_forecast") {
    if (!tile.forecast_status_label.empty()) return tile.forecast_status_label;
    if (tile.label_configured && !tile.label.empty()) return tile.label;
    return "Temperatures Tomorrow";
  }
  if (tile.type == "media" && tile.sensor == "now_playing") {
    if (tile.secondary_unavailable) return "--";
    if (!tile.secondary_value.empty()) return tile.secondary_value;
    return "--";
  }
  if (epaper_dashboard_action_state_text_card(tile)) {
    if (!tile.sensor_value.empty()) {
      return epaper_dashboard_text_sensor_display_text(
          tile.sensor_value, EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN);
    }
    if (tile.sensor_unavailable) return "";
  }
  if (epaper_dashboard_fan_non_speed_card(tile)) {
    std::string entity_label = tile.entity.empty() ? "" : epaper_dashboard_title_from_entity(tile.entity);
    if (tile.type == "fan_direction" || tile.type == "fan_preset") {
      if (!tile.state.empty() && !tile.state_unavailable) return epaper_dashboard_fan_status_text(tile);
      if (epaper_dashboard_fan_attribute_known(tile)) return epaper_dashboard_fan_status_text(tile);
      if (tile.type == "fan_preset" && !tile.fan_preset_modes.empty()) return epaper_dashboard_fan_status_text(tile);
    }
    if ((tile.type == "fan_switch" || tile.type == "fan_oscillate") &&
        !tile.label_configured && !tile.friendly_name.empty()) {
      return tile.friendly_name;
    }
    if (tile.label.empty() || tile.label == entity_label) {
      return epaper_dashboard_fan_default_label(tile);
    }
  }
  if (tile.type == "door_window" && !tile.label_configured) {
    if (!tile.friendly_name.empty()) return tile.friendly_name;
    return epaper_dashboard_window_card(tile) ? "Window" : "Door";
  }
  if (tile.type == "presence" && !tile.label_configured) {
    if (!tile.friendly_name.empty()) return tile.friendly_name;
    return "Presence";
  }
  if (tile.type == "cover" &&
      (tile.sensor == "toggle" || epaper_dashboard_cover_command_mode(tile.sensor)) &&
      !tile.label_configured) {
    if (!tile.friendly_name.empty()) return tile.friendly_name;
    return !tile.label.empty() ? tile.label : "Cover";
  }
  if (tile.type == "garage" && epaper_dashboard_garage_command_mode(tile.sensor) &&
      !tile.label_configured) {
    return tile.sensor == "open" ? "Open" : "Close";
  }
  if (tile.type == "lock" && epaper_dashboard_lock_command_mode(tile.sensor) &&
      !tile.label_configured) {
    return tile.sensor == "unlock" ? "Unlock" : "Lock";
  }
  if (tile.type == "media" &&
      (tile.sensor == "play_pause" || tile.sensor == "position") &&
      tile.precision == "state") {
    if (tile.state_unavailable) return "Unavailable";
    if (tile.state.empty()) return "Paused";
    return epaper_dashboard_media_status_text(tile.state);
  }
  if (tile.type == "media" && tile.label.empty()) return epaper_dashboard_media_mode_label(tile.sensor);
  if (!tile.label_configured && !tile.friendly_name.empty() &&
      !epaper_dashboard_friendly_label_source(tile).empty()) {
    return tile.friendly_name;
  }
  if (tile.type == "garage" &&
      epaper_dashboard_option_value(tile.options, "label_display") == "status") {
    if (tile.state.empty()) return "--";
    return epaper_dashboard_pretty_state(tile.state);
  }
  if (tile.type == "garage" && !epaper_dashboard_garage_command_mode(tile.sensor) &&
      !tile.label_configured && tile.label.empty()) {
    return "Garage Door";
  }
  if (tile.type == "lock" && !epaper_dashboard_lock_command_mode(tile.sensor) &&
      !tile.label_configured && tile.label.empty()) {
    return "Lock";
  }
  if (epaper_dashboard_option_select_card(tile) && tile.label.empty()) {
    if (!tile.entity.empty()) return tile.entity;
    return "Option";
  }
  if (tile.type == "action" && tile.label.empty()) {
    if (!tile.entity.empty()) return tile.entity;
    return "Action";
  }
  if (tile.type == "push" && tile.label.empty()) return "Trigger";
  if (tile.type == "webhook" && tile.label.empty()) {
    if (!tile.entity.empty()) return tile.entity;
    return "Webhook";
  }
  if (tile.type == "todo" && tile.label.empty()) {
    if (!tile.friendly_name.empty()) return tile.friendly_name;
    if (!tile.entity.empty()) return tile.entity;
    return "Todo";
  }
  if (tile.type == "internal" && tile.label.empty()) {
    if (!tile.entity.empty()) return epaper_dashboard_sentence_cap_text(tile.entity);
    return "Relay";
  }
  if (tile.type == "climate") {
    std::string label_mode = epaper_dashboard_climate_label_mode(tile);
    if (label_mode == "status") return epaper_dashboard_climate_action_label(tile);
    if (label_mode == "actual" || label_mode == "target") {
      std::string value = label_mode == "actual"
        ? epaper_dashboard_climate_actual_value(tile)
        : epaper_dashboard_climate_target_value(tile);
      if (value.empty() || value == "--") return "--";
      return value + epaper_dashboard_display_unit(tile);
    }
    if (tile.label.empty()) return "Climate";
  }
  if (tile.type == "alarm" &&
      epaper_dashboard_option_value(tile.options, "label_display") != "name") {
    return epaper_dashboard_alarm_label_for_state(epaper_dashboard_alarm_effective_state(tile));
  }
  if (tile.type == "alarm" && tile.label.empty()) {
    if (!tile.entity.empty()) return tile.entity;
    return "Alarm";
  }
  if (tile.type == "alarm_action" && !tile.label_configured) {
    return epaper_dashboard_alarm_action_label(tile.sensor);
  }
  return tile.label;
}

inline std::string epaper_dashboard_display_unit(const EpaperDashboardTile &tile) {
  if (epaper_dashboard_weather_forecast_card(tile) && tile.forecast_valid) {
    return display_temperature_unit_symbol();
  }
  if (epaper_dashboard_action_state_numeric_card(tile) && tile.sensor_unavailable) return "";
  if ((tile.type == "sensor" || epaper_dashboard_toggle_numeric_sensor_card(tile)) &&
      tile.precision != "text" && tile.sensor_unavailable) {
    return "";
  }
  if (tile.type == "climate" && epaper_dashboard_value_replaces_icon(tile) && tile.unit.empty()) {
    return display_temperature_unit_symbol();
  }
  return epaper_dashboard_trim(tile.unit);
}

inline bool epaper_dashboard_value_uses_numeric_font(const std::string &value) {
  for (char c : value) {
    if (std::isdigit(static_cast<unsigned char>(c))) continue;
    if (c == ' ' || c == '!' || c == '"' || c == '%' || c == '(' || c == ')' ||
        c == '+' || c == ',' || c == '-' || c == '.' || c == '/' || c == ':') {
      continue;
    }
    return false;
  }
  return true;
}

inline void epaper_dashboard_style_lvgl_tile(lv_obj_t *tile, lv_obj_t *icon, lv_obj_t *label,
                                            lv_obj_t *badge, lv_obj_t *track,
                                            lv_obj_t *track_fill,
                                            lv_obj_t *value, lv_obj_t *unit,
                                            bool configured, bool active) {
  if (!tile) return;
  bool dark = epaper_dashboard_dark_theme();
  uint32_t bg = dark ? 0x000000 : 0xFFFFFF;
  uint32_t fg = dark ? 0xFFFFFF : 0x000000;
  if (active) {
    bg = dark ? 0xFFFFFF : 0x000000;
    fg = dark ? 0x000000 : 0xFFFFFF;
  }
  uint32_t border = dark ? 0xFFFFFF : 0x000000;
  lv_obj_set_style_bg_color(tile, lv_color_hex(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(tile, lv_color_hex(border), LV_PART_MAIN);
  lv_obj_set_style_border_width(tile, configured ? 2 : 1, LV_PART_MAIN);
  lv_obj_set_style_radius(tile, 18, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(tile, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(tile, 8, LV_PART_MAIN);
  if (icon) lv_obj_set_style_text_color(icon, lv_color_hex(fg), LV_PART_MAIN);
  if (label) lv_obj_set_style_text_color(label, lv_color_hex(fg), LV_PART_MAIN);
  if (badge) lv_obj_set_style_text_color(badge, lv_color_hex(fg), LV_PART_MAIN);
  if (track) {
    lv_obj_set_style_bg_color(track, lv_color_hex(fg), LV_PART_MAIN);
    lv_obj_set_style_border_color(track, lv_color_hex(fg), LV_PART_MAIN);
  }
  if (track_fill) lv_obj_set_style_bg_color(track_fill, lv_color_hex(fg), LV_PART_MAIN);
  if (value) lv_obj_set_style_text_color(value, lv_color_hex(fg), LV_PART_MAIN);
  if (unit) lv_obj_set_style_text_color(unit, lv_color_hex(fg), LV_PART_MAIN);
}

inline void epaper_dashboard_update_lvgl_page(int page) {
  page = epaper_dashboard_wrap_page(page);
  auto &tiles = epaper_dashboard_tiles();
  auto &slots = epaper_dashboard_lvgl_slots();
  auto &order = epaper_dashboard_order();
  int start = page * EPAPER_DASHBOARD_PAGE_SLOTS;

  for (int i = 0; i < EPAPER_DASHBOARD_PAGE_SLOTS; i++) {
    auto &slot = slots[i];
    if (!slot.tile) continue;
    lv_obj_add_flag(slot.tile, LV_OBJ_FLAG_HIDDEN);
  }

  for (int pos = 0; pos < EPAPER_DASHBOARD_PAGE_SLOTS; pos++) {
    int slot_index = order.positions[pos];
    if (slot_index < 1 || slot_index > EPAPER_DASHBOARD_PAGE_SLOTS) continue;
    const auto &tile = tiles[start + slot_index - 1];
    auto &slot = slots[slot_index - 1];
    if (!slot.tile) continue;
    int col = pos % EPAPER_DASHBOARD_COLS;
    int row = pos / EPAPER_DASHBOARD_COLS;
    int row_span = order.row_span[start + slot_index - 1] > 0
      ? order.row_span[start + slot_index - 1]
      : 1;
    int col_span = order.col_span[start + slot_index - 1] > 0
      ? order.col_span[start + slot_index - 1]
      : 1;
    bool configured = epaper_dashboard_tile_configured(tile);
    lv_obj_set_grid_cell(
        slot.tile,
        LV_GRID_ALIGN_STRETCH, col, col_span,
        LV_GRID_ALIGN_STRETCH, row, row_span);
    if (!configured) {
      if (slot.icon) lv_label_set_text(slot.icon, "");
      if (slot.label) lv_label_set_text(slot.label, "");
      if (slot.badge) lv_label_set_text(slot.badge, "");
      if (slot.track) lv_obj_add_flag(slot.track, LV_OBJ_FLAG_HIDDEN);
      if (slot.value) lv_label_set_text(slot.value, "");
      if (slot.unit) lv_label_set_text(slot.unit, "");
      lv_obj_add_flag(slot.tile, LV_OBJ_FLAG_HIDDEN);
      lv_obj_invalidate(slot.tile);
      lv_obj_t *parent = lv_obj_get_parent(slot.tile);
      if (parent) lv_obj_invalidate(parent);
      continue;
    }
    lv_obj_clear_flag(slot.tile, LV_OBJ_FLAG_HIDDEN);
    bool active = configured && epaper_dashboard_tile_active(tile);
    bool icon_active = active;
    if (tile.type == "door_window") {
      icon_active = !epaper_dashboard_binary_card_unavailable(tile) &&
                    epaper_dashboard_state_active(epaper_dashboard_binary_card_state(tile));
    }
    if (tile.type == "presence") {
      const std::string &state = epaper_dashboard_binary_card_state(tile);
      icon_active = !epaper_dashboard_binary_card_unavailable(tile) &&
                    (epaper_dashboard_normalized_state_text(state) == "detected" ||
                     epaper_dashboard_state_active(state));
    }
    if (tile.type == "climate") {
      icon_active = epaper_dashboard_climate_controls_enabled(tile);
    }
    bool has_sensor_value = epaper_dashboard_has_sensor_value(tile);
    bool show_track = configured && epaper_dashboard_slider_visual_card(tile);
    bool show_value = configured && !epaper_dashboard_text_sensor_card(tile) &&
        (epaper_dashboard_sensor_card_type(tile) || has_sensor_value ||
         epaper_dashboard_value_replaces_icon(tile));
    if (tile.type == "sensor" && tile.precision == "icon") show_value = false;
    if (epaper_dashboard_toggle_text_sensor_card(tile)) show_value = false;
    if (epaper_dashboard_toggle_numeric_sensor_card(tile) && !active) show_value = false;
    if (epaper_dashboard_action_state_icon_card(tile) ||
        epaper_dashboard_action_state_text_card(tile)) show_value = false;
    if (epaper_dashboard_slider_visual_card(tile) && tile.type != "media") show_value = false;
    if (tile.type == "weather" && !epaper_dashboard_weather_forecast_card(tile)) show_value = false;
    if (tile.type == "door_window" || tile.type == "presence") show_value = false;
    if (tile.type == "todo" && !epaper_dashboard_todo_card_show_count(tile)) show_value = false;
    bool value_replaces_icon = show_value && epaper_dashboard_value_replaces_icon(tile);
    bool style_active = active && !show_track;
    epaper_dashboard_style_lvgl_tile(slot.tile, slot.icon, slot.label, slot.badge,
                                     slot.track, slot.track_fill,
                                     slot.value, slot.unit, configured, style_active);
    if (slot.icon) {
      lv_label_set_text(slot.icon, epaper_dashboard_icon(tile, icon_active));
      if (value_replaces_icon) lv_obj_add_flag(slot.icon, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_clear_flag(slot.icon, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.sensor_container) {
      if (show_value) {
        lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
        if (tile.type == "media" && tile.sensor == "now_playing") {
          lv_obj_set_width(slot.sensor_container, lv_pct(100));
        } else {
          lv_obj_set_width(slot.sensor_container, LV_SIZE_CONTENT);
        }
        lv_align_t value_align = epaper_dashboard_wide_large_clock_card(tile, row_span, col_span)
          ? LV_ALIGN_LEFT_MID
          : (value_replaces_icon ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT);
        lv_obj_align(slot.sensor_container, value_align, 0, 0);
      } else {
        lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (slot.label) {
      std::string label = epaper_dashboard_tile_label(tile);
      lv_label_set_long_mode(
          slot.label, row_span > 1 ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_DOT);
      const char *badge = epaper_dashboard_badge_icon(tile);
      lv_obj_set_width(slot.label, badge ? lv_pct(80) : lv_pct(100));
      lv_label_set_text(slot.label, label.c_str());
      lv_obj_clear_flag(slot.label, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.badge) {
      const char *badge = epaper_dashboard_badge_icon(tile);
      if (badge) {
        lv_label_set_text(slot.badge, badge);
        lv_obj_clear_flag(slot.badge, LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_label_set_text(slot.badge, "");
        lv_obj_add_flag(slot.badge, LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (slot.track) {
      if (show_track) {
        lv_obj_clear_flag(slot.track, LV_OBJ_FLAG_HIDDEN);
        if (slot.track_fill) {
          if (epaper_dashboard_media_now_playing_play_pause_card(tile)) {
            lv_obj_add_flag(slot.track_fill, LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_clear_flag(slot.track_fill, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_width(slot.track_fill, lv_pct(epaper_dashboard_track_fill_percent(tile)));
          }
        }
      } else {
        lv_obj_add_flag(slot.track, LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (slot.value) {
      std::string value = epaper_dashboard_display_value(tile);
      bool media_title = tile.type == "media" && tile.sensor == "now_playing";
      if (slot.value_font) {
        lv_obj_set_style_text_font(
            slot.value,
            ((media_title || !epaper_dashboard_value_uses_numeric_font(value)) && slot.label_font)
              ? slot.label_font : slot.value_font,
            LV_PART_MAIN);
      }
      lv_obj_set_width(slot.value, media_title ? lv_pct(100) : LV_SIZE_CONTENT);
      lv_label_set_text(slot.value, value.c_str());
      if (show_value) lv_obj_clear_flag(slot.value, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(slot.value, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.unit) {
      std::string unit = epaper_dashboard_display_unit(tile);
      lv_label_set_text(slot.unit, unit.c_str());
      if (show_value && !unit.empty()) lv_obj_clear_flag(slot.unit, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(slot.unit, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_invalidate(slot.tile);
  }
  epaper_dashboard_clear_dirty();
}

inline void epaper_dashboard_subscribe(int index) {
  auto &tiles = epaper_dashboard_tiles();
  if (index < 0 || index >= EPAPER_DASHBOARD_TOTAL_SLOTS) return;
  auto &tile = tiles[index];
  if (!epaper_dashboard_api_available()) return;
  std::string state_source = epaper_dashboard_state_source(tile);
  if (!state_source.empty() && !tile.state_subscribed) {
    tile.state_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        state_source, {}, [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.state = epaper_dashboard_string_ref_limited(
              state, epaper_dashboard_state_text_max_len(tile));
          tile.state_unavailable = epaper_dashboard_state_unavailable(tile.state);
          epaper_dashboard_mark_dirty();
        });
  }
  std::string friendly_source = epaper_dashboard_friendly_label_source(tile);
  if (!friendly_source.empty() &&
      !tile.label_configured && !tile.friendly_name_subscribed) {
    tile.friendly_name_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        friendly_source, "friendly_name", [index](esphome::StringRef name) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.friendly_name = epaper_dashboard_string_ref_limited(
              name, EPAPER_DASHBOARD_FRIENDLY_NAME_MAX_LEN);
          epaper_dashboard_mark_dirty();
        });
  }
  std::string sensor_source = epaper_dashboard_sensor_source(tile);
  std::string attribute;
  std::string attribute_entity = epaper_dashboard_attribute_source(tile, attribute);
  std::string secondary_attribute;
  std::string secondary_attribute_entity =
      epaper_dashboard_secondary_attribute_source(tile, secondary_attribute);
  if (!sensor_source.empty() && !tile.sensor_subscribed) {
    tile.sensor_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        sensor_source, {}, [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.sensor_value = std::string(state.c_str(), state.size());
          tile.sensor_unavailable = !tile.action_state_entity.empty()
              ? epaper_dashboard_entity_state_unavailable(
                    tile.action_state_entity, tile.sensor_value)
              : epaper_dashboard_state_unavailable(tile.sensor_value);
          epaper_dashboard_mark_dirty();
        });
  } else if (!attribute_entity.empty() && !attribute.empty() && !tile.sensor_subscribed) {
    tile.sensor_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        attribute_entity, attribute, [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.sensor_value = tile.type == "media" && tile.sensor == "now_playing"
            ? epaper_dashboard_string_ref_limited(state, EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN)
            : std::string(state.c_str(), state.size());
          tile.sensor_unavailable = epaper_dashboard_state_unavailable(tile.sensor_value);
          epaper_dashboard_mark_dirty();
        });
  }
  if (tile.type == "fan_preset" && !tile.entity.empty() &&
      !tile.fan_preset_modes_subscribed) {
    tile.fan_preset_modes_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        tile.entity, "preset_modes", [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.fan_preset_modes = epaper_dashboard_fan_parse_options(state);
          epaper_dashboard_mark_dirty();
        });
  }
  if (!secondary_attribute_entity.empty() && !secondary_attribute.empty() &&
      !tile.secondary_subscribed) {
    tile.secondary_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        secondary_attribute_entity, secondary_attribute, [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.secondary_value = tile.type == "media" && tile.sensor == "now_playing"
            ? epaper_dashboard_string_ref_limited(state, EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN)
            : std::string(state.c_str(), state.size());
          tile.secondary_unavailable = epaper_dashboard_state_unavailable(tile.secondary_value);
          epaper_dashboard_mark_dirty();
        });
  }
  if (tile.type == "climate" && !tile.entity.empty()) {
    auto subscribe_climate_attribute =
        [index](const std::string &entity, const char *attribute,
                std::string EpaperDashboardTile::*value_field,
                bool EpaperDashboardTile::*unavailable_field,
                bool EpaperDashboardTile::*subscribed_field) {
          auto &tile = epaper_dashboard_tiles()[index];
          if (tile.*subscribed_field) return;
          tile.*subscribed_field = true;
          esphome::api::global_api_server->subscribe_home_assistant_state(
              entity, attribute, [index, value_field, unavailable_field](esphome::StringRef state) {
                auto &tile = epaper_dashboard_tiles()[index];
                tile.*value_field = std::string(state.c_str(), state.size());
                tile.*unavailable_field =
                    epaper_dashboard_state_unavailable(tile.*value_field);
                epaper_dashboard_mark_dirty();
              });
        };
    subscribe_climate_attribute(
        tile.entity, "current_temperature",
        &EpaperDashboardTile::climate_current_value,
        &EpaperDashboardTile::climate_current_unavailable,
        &EpaperDashboardTile::climate_current_subscribed);
    subscribe_climate_attribute(
        tile.entity, "temperature",
        &EpaperDashboardTile::climate_target_value,
        &EpaperDashboardTile::climate_target_unavailable,
        &EpaperDashboardTile::climate_target_subscribed);
    subscribe_climate_attribute(
        tile.entity, "target_temp_low",
        &EpaperDashboardTile::climate_target_low_value,
        &EpaperDashboardTile::climate_target_low_unavailable,
        &EpaperDashboardTile::climate_target_low_subscribed);
    subscribe_climate_attribute(
        tile.entity, "target_temp_high",
        &EpaperDashboardTile::climate_target_high_value,
        &EpaperDashboardTile::climate_target_high_unavailable,
        &EpaperDashboardTile::climate_target_high_subscribed);
    subscribe_climate_attribute(
        tile.entity, "hvac_action",
        &EpaperDashboardTile::climate_hvac_action,
        &EpaperDashboardTile::climate_hvac_action_unavailable,
        &EpaperDashboardTile::climate_hvac_action_subscribed);
  }
  if (epaper_dashboard_media_now_playing_progress_card(tile)) {
    if (!tile.media_position_subscribed) {
      tile.media_position_subscribed = true;
      esphome::api::global_api_server->subscribe_home_assistant_state(
          tile.entity, "media_position", [index](esphome::StringRef state) {
            auto &tile = epaper_dashboard_tiles()[index];
            tile.media_position_value = std::string(state.c_str(), state.size());
            tile.media_position_received_ms = esphome::millis();
            tile.media_position_unavailable =
                epaper_dashboard_state_unavailable(tile.media_position_value);
            epaper_dashboard_mark_dirty();
          });
    }
    if (!tile.media_duration_subscribed) {
      tile.media_duration_subscribed = true;
      esphome::api::global_api_server->subscribe_home_assistant_state(
          tile.entity, "media_duration", [index](esphome::StringRef state) {
            auto &tile = epaper_dashboard_tiles()[index];
            tile.media_duration_value = std::string(state.c_str(), state.size());
            tile.media_duration_unavailable =
                epaper_dashboard_state_unavailable(tile.media_duration_value);
            epaper_dashboard_mark_dirty();
          });
    }
  }
  if (tile.type == "media" &&
      (tile.sensor == "position" || epaper_dashboard_media_now_playing_progress_card(tile)) &&
      !tile.entity.empty() && !tile.media_position_updated_at_subscribed) {
    tile.media_position_updated_at_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        tile.entity, "media_position_updated_at", [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.media_position_updated_at_value = std::string(state.c_str(), state.size());
          tile.media_position_updated_at_unavailable =
              epaper_dashboard_state_unavailable(tile.media_position_updated_at_value);
          epaper_dashboard_mark_dirty();
        });
  }
}

inline void epaper_dashboard_set_config(int index, const std::string &config) {
  if (index < 0 || index >= EPAPER_DASHBOARD_TOTAL_SLOTS) return;
  auto &tile = epaper_dashboard_tiles()[index];
  if (tile.config == config) {
    epaper_dashboard_subscribe(index);
    return;
  }
  tile = EpaperDashboardTile{};
  tile.config = config;
  auto fields = epaper_dashboard_config_fields(config);
  if (fields.size() > 0) tile.entity = fields[0];
  if (fields.size() > 1) tile.label = fields[1];
  tile.label_configured = !tile.label.empty();
  if (fields.size() > 2) tile.icon = fields[2];
  if (fields.size() > 3) tile.icon_on = fields[3];
  if (fields.size() > 4) tile.sensor = fields[4];
  if (fields.size() > 5) tile.unit = fields[5];
  if (fields.size() > 6) tile.type = fields[6];
  if (fields.size() > 7) tile.precision = fields[7];
  if (fields.size() > 8) tile.options = fields[8];
  if (tile.type == "subpage") {
    tile = EpaperDashboardTile{};
    epaper_dashboard_mark_dirty();
    return;
  }
  if (tile.type.empty()) {
    if (tile.sensor.empty()) {
      tile.unit.clear();
      tile.precision.clear();
    } else if (tile.precision == "text") {
      tile.unit.clear();
    }
    tile.options = epaper_dashboard_normalize_switch_options(tile.options, tile.sensor, tile.precision);
  }
  if (epaper_dashboard_brightness_slider_type(tile.type) && !tile.sensor.empty()) {
    tile.sensor.clear();
  }
  if (epaper_dashboard_brightness_slider_type(tile.type)) {
    tile.options.clear();
  }
  if (epaper_dashboard_fan_card_type(tile.type)) {
    tile.sensor.clear();
    tile.unit.clear();
    tile.precision.clear();
    tile.options.clear();
    if (tile.icon.empty() || tile.icon == "Auto") {
      tile.icon = epaper_dashboard_fan_default_icon_name(tile);
    }
    if (tile.type == "fan_switch") {
      if (tile.icon_on.empty() || tile.icon_on == "Auto") tile.icon_on = "Fan";
    } else {
      tile.icon_on.clear();
    }
  }
  if (tile.type == "weather_forecast") {
    tile.type = "weather";
    tile.precision = "tomorrow";
    if (tile.label == "Weather") {
      tile.label.clear();
      tile.label_configured = false;
    }
  }
  if (tile.type == "text_sensor") {
    tile.type = "sensor";
    tile.precision = "text";
    tile.entity.clear();
    tile.label.clear();
    tile.label_configured = false;
    tile.unit.clear();
    tile.icon_on = "Auto";
    if (tile.icon.empty()) tile.icon = "Auto";
  }
  if (tile.type == "sensor") {
    if (tile.precision == "text") {
      tile.label.clear();
      tile.label_configured = false;
      tile.unit.clear();
      tile.icon_on.clear();
    } else if (tile.precision != "icon") {
      tile.icon_on.clear();
    }
    tile.options = epaper_dashboard_normalize_sensor_options(tile.options, tile.precision);
  }
  if (tile.type == "weather") {
    tile.precision = epaper_dashboard_normalize_weather_precision(tile.precision);
    if (tile.precision.empty()) {
      tile.label.clear();
      tile.label_configured = false;
    }
    tile.options = epaper_dashboard_normalize_weather_options(tile.options, tile.precision);
  }
  if (tile.type == "media") {
    bool legacy_controls = tile.sensor == "controls";
    tile.sensor = epaper_dashboard_media_mode(tile.sensor);
    tile.unit.clear();
    if (legacy_controls && (tile.icon.empty() || tile.icon == "Speaker")) {
      tile.icon = "Auto";
    }
    tile.icon_on.clear();
    if (tile.sensor == "previous" && tile.label == "Skip Previous") tile.label = "Previous";
    if (tile.sensor == "next" && tile.label == "Skip Next") tile.label = "Next";
    if (tile.sensor == "play_pause") tile.icon = "Auto";
    if (tile.sensor == "volume") {
      if (tile.label.empty() || tile.label == "Media") {
        tile.label = "Volume";
        tile.label_configured = true;
      }
      tile.icon = "Auto";
    }
    if (tile.sensor == "position" && (tile.label.empty() || tile.label == "Track")) {
      tile.label = "Position";
      tile.label_configured = true;
    }
    if (tile.sensor == "now_playing") {
      if (!epaper_dashboard_media_now_playing_control(tile.precision)) tile.precision.clear();
    } else if (epaper_dashboard_media_state_display_mode(tile.sensor) && tile.precision == "state") {
      tile.precision = "state";
    } else {
      tile.precision.clear();
    }
    tile.options = epaper_dashboard_normalize_media_options(tile.options, tile.sensor);
  }
  if (tile.type == "climate") {
    tile.sensor.clear();
    tile.unit.clear();
    if (tile.icon.empty()) tile.icon = "Thermostat";
    if (tile.icon_on.empty()) tile.icon_on = "Auto";
    tile.options = epaper_dashboard_normalize_climate_options(tile.options);
  }
  if (tile.type == "cover") {
    tile.sensor = epaper_dashboard_normalize_cover_mode(tile.sensor);
    tile.precision.clear();
    tile.options.clear();
    if (tile.sensor == "set_position") {
      tile.unit = epaper_dashboard_normalize_cover_position(tile.unit);
    } else {
      tile.unit.clear();
    }
    if (epaper_dashboard_cover_command_mode(tile.sensor)) tile.icon_on.clear();
  }
  if (tile.type == "garage") {
    if (!epaper_dashboard_garage_command_mode(tile.sensor)) tile.sensor.clear();
    tile.unit.clear();
    tile.precision.clear();
    if (!tile.sensor.empty()) tile.icon_on.clear();
    tile.options = epaper_dashboard_normalize_garage_options(tile.options, tile.sensor);
  }
  if (tile.type == "alarm") {
    tile.sensor.clear();
    tile.unit.clear();
    tile.precision.clear();
    tile.icon_on.clear();
    if (tile.icon.empty() || tile.icon == "Auto") tile.icon = "Security";
    tile.options = epaper_dashboard_normalize_alarm_options(tile.options);
  }
  if (tile.type == "alarm_action") {
    if (!epaper_dashboard_alarm_action_mode_valid(tile.sensor)) tile.sensor = "away";
    tile.unit.clear();
    tile.precision.clear();
    tile.icon_on.clear();
    if (tile.icon.empty() || tile.icon == "Auto" ||
        epaper_dashboard_alarm_action_legacy_icon(tile.sensor, tile.icon)) {
      if (tile.sensor == "home") tile.icon = "Shield Home";
      else if (tile.sensor == "disarm") tile.icon = "Shield Off";
      else tile.icon = "Shield Lock";
    }
    tile.options = epaper_dashboard_normalize_alarm_options(tile.options);
  }
  if (tile.type == "webhook") {
    tile.sensor = epaper_dashboard_webhook_method(tile.sensor);
    if (tile.sensor == "GET" || tile.sensor == "DELETE") tile.unit.clear();
    tile.precision.clear();
    tile.icon_on.clear();
    if (tile.icon.empty()) tile.icon = "Auto";
    tile.options = epaper_dashboard_normalize_webhook_options(tile.options);
  }
  if (tile.type == "push") {
    tile.entity.clear();
    tile.sensor.clear();
    tile.unit.clear();
    tile.precision.clear();
    tile.icon_on = "Auto";
    tile.options.clear();
    if (tile.icon.empty() || tile.icon == "Auto") tile.icon = "Gesture Tap";
  }
  if (tile.type == "internal") {
    bool push_mode = epaper_dashboard_internal_push_mode(tile);
    tile.sensor = push_mode ? "push" : "";
    tile.unit.clear();
    tile.precision.clear();
    tile.options.clear();
    if (tile.icon.empty() || tile.icon == "Auto" || tile.icon == "Power" ||
        (!push_mode && tile.icon == "Power Plug")) {
      tile.icon = push_mode ? "Gesture Tap" : "Lightbulb Outline";
    }
    if (push_mode) {
      tile.icon_on = "Auto";
    } else if (tile.icon_on.empty() || tile.icon_on == "Auto" || tile.icon_on == "Power" ||
               tile.icon_on == "Flash") {
      tile.icon_on = "Lightbulb";
    }
  }
  if (tile.type == "todo") {
    tile.sensor.clear();
    tile.unit.clear();
    tile.precision.clear();
    tile.icon_on = "Auto";
    if (tile.icon.empty() || tile.icon == "Auto") tile.icon = "Check";
    tile.options = epaper_dashboard_normalize_todo_options(tile.options);
  }
  if (tile.type == "light_switch") {
    tile.sensor.clear();
    tile.unit.clear();
    tile.precision.clear();
    tile.options.clear();
    if (tile.icon.empty() || tile.icon == "Auto") tile.icon = "Lightbulb Outline";
    if (tile.icon_on.empty() || tile.icon_on == "Auto") tile.icon_on = "Lightbulb";
  }
  if (tile.type == "lock") {
    tile.sensor = epaper_dashboard_normalize_lock_mode(tile.sensor);
    tile.unit.clear();
    tile.precision.clear();
    tile.options.clear();
    if (tile.icon.empty() || tile.icon == "Auto") tile.icon = "Lock";
    if (tile.icon_on.empty() || tile.icon_on == "Auto") tile.icon_on = "Lock Open";
  }
  if (tile.type == "light_temperature") {
    if (tile.sensor == "kelvin") tile.sensor.clear();
    tile.unit = epaper_dashboard_normalize_light_temperature_range(tile.unit);
    tile.options.clear();
  }
  if (tile.type == "calendar" || tile.type == "clock" || tile.type == "timezone") {
    tile.label.clear();
    tile.label_configured = false;
    tile.sensor.clear();
    tile.unit.clear();
    if (tile.type == "clock") tile.entity.clear();
    if (tile.type == "calendar") {
      if (tile.precision != "datetime") tile.precision.clear();
    } else {
      tile.precision.clear();
    }
    if (tile.icon.empty()) tile.icon = "Auto";
    if (tile.icon_on.empty()) tile.icon_on = "Auto";
    tile.options = epaper_dashboard_normalize_date_time_options(tile.options);
  }
  if (tile.type == "door_window") {
    tile.entity.clear();
    tile.unit.clear();
    tile.precision = tile.precision == "window" ? "window" : "door";
    if (tile.icon.empty() || tile.icon == "Auto") {
      tile.icon = tile.precision == "window" ? "Window Closed" : "Door";
    }
    if (tile.icon_on.empty() || tile.icon_on == "Auto") {
      tile.icon_on = tile.precision == "window" ? "Window Open" : "Door Open";
    }
    tile.options = epaper_dashboard_option_present(tile.options, "active_color")
      ? "active_color"
      : "";
  }
  if (tile.type == "presence") {
    tile.entity.clear();
    tile.unit.clear();
    tile.precision.clear();
    if (tile.icon.empty() || tile.icon == "Auto") tile.icon = "Motion Sensor Off";
    if (tile.icon_on.empty() || tile.icon_on == "Auto") tile.icon_on = "Motion Sensor";
    tile.options = epaper_dashboard_option_present(tile.options, "active_color")
      ? "active_color"
      : "";
  }
  if (tile.type == "option_select") {
    tile.type = "action";
    tile.sensor = "input_select.select_option";
    tile.unit.clear();
    tile.precision.clear();
    tile.options.clear();
    tile.icon_on.clear();
    if (tile.icon.empty() || tile.icon == "Auto" || tile.icon == "Chevron Down") {
      tile.icon = "Flash";
    }
  }
  if (epaper_dashboard_action_option_select(tile)) {
    tile.sensor = "input_select.select_option";
    tile.unit.clear();
    tile.precision.clear();
    tile.options.clear();
    tile.icon_on.clear();
    if (tile.icon.empty() || tile.icon == "Auto" || tile.icon == "Chevron Down") {
      tile.icon = "Flash";
    }
  }
  if (tile.type == "action") {
    tile.options = epaper_dashboard_normalize_action_options(tile.options);
    if (epaper_dashboard_option_value(tile.options, "state_precision") != "icon") {
      tile.icon_on.clear();
    }
    tile.action_state_entity = epaper_dashboard_option_value(tile.options, "state_entity");
    std::string action_unit = epaper_dashboard_option_value(tile.options, "state_unit");
    if (!action_unit.empty()) tile.unit = action_unit;
  }
  if (!tile.type.empty() && tile.type != "action" && tile.type != "alarm" &&
      tile.type != "alarm_action" && tile.type != "climate" &&
      tile.type != "garage" && tile.type != "webhook" && tile.type != "todo" &&
      tile.type != "sensor" && tile.type != "door_window" &&
      tile.type != "presence" && tile.type != "media" &&
      !epaper_dashboard_fan_card_type(tile.type) &&
      !epaper_dashboard_card_large_numbers_supported(tile)) {
    tile.options.clear();
  }
  if (!epaper_dashboard_tile_configured(tile)) {
    tile = EpaperDashboardTile{};
    epaper_dashboard_mark_dirty();
    return;
  }
  if (tile.label.empty()) {
    std::string label_source = epaper_dashboard_default_label_source(tile);
    if (!label_source.empty()) tile.label = epaper_dashboard_title_from_entity(label_source);
  }
  epaper_dashboard_subscribe(index);
  if (epaper_dashboard_weather_forecast_card(tile)) {
    epaper_dashboard_forecast_enqueue(tile.entity);
    epaper_dashboard_forecast_send_next_queued();
  }
  epaper_dashboard_mark_dirty();
}

inline std::string epaper_dashboard_display_value(const EpaperDashboardTile &tile) {
  if (tile.config.empty()) return "";
  if (tile.type == "calendar") return epaper_dashboard_calendar_value(tile);
  if (tile.type == "clock") return epaper_dashboard_clock_value();
  if (tile.type == "timezone") return epaper_dashboard_timezone_value(tile);
  if (epaper_dashboard_weather_forecast_card(tile)) return epaper_dashboard_weather_forecast_value(tile);
  if (epaper_dashboard_option_select_card(tile)) {
    if (tile.state_unavailable) return "--";
    if (!tile.state.empty()) return tile.state;
    return "--";
  }
  if (tile.type == "todo") {
    if (tile.state_unavailable) return "--";
    if (!tile.state.empty()) return tile.state;
    return "--";
  }
  if (tile.type == "climate") {
    return epaper_dashboard_climate_card_value(tile);
  }
  bool use_sensor_value = tile.type == "sensor" || tile.type == "weather" ||
      tile.type == "weather_forecast" || tile.type == "calendar" ||
      tile.type == "clock" || tile.type == "timezone" ||
      !epaper_dashboard_sensor_source(tile).empty() || !tile.action_state_entity.empty() ||
      tile.type == "media";
  if (use_sensor_value) {
    if (epaper_dashboard_text_sensor_card(tile) && tile.sensor_unavailable) return "Unavailable";
    if (epaper_dashboard_action_state_numeric_card(tile) && tile.sensor_unavailable) return "";
    if ((tile.type == "sensor" || epaper_dashboard_toggle_numeric_sensor_card(tile)) &&
        tile.precision != "text" && tile.sensor_unavailable) {
      return "";
    }
    if (tile.sensor_unavailable) return "--";
    if (!tile.sensor_value.empty()) {
      if (tile.type == "media" && tile.sensor == "volume") return epaper_dashboard_format_media_volume(tile);
      if (tile.type == "media" && tile.sensor == "position") {
        float duration = 0.0f;
        float position = 0.0f;
        if (epaper_dashboard_parse_float_value(tile.secondary_value, duration) &&
            epaper_dashboard_media_adjusted_position_seconds(tile, tile.sensor_value, duration, position)) {
          return epaper_dashboard_format_seconds_value(position);
        }
        return epaper_dashboard_format_seconds(tile.sensor_value);
      }
      if (tile.precision == "text") {
        return epaper_dashboard_sensor_state_display_text(tile);
      }
      if (tile.type == "sensor" || tile.type.empty() || tile.type == "action" ||
          tile.type == "climate") {
        std::string precision = tile.type == "action"
          ? epaper_dashboard_option_value(tile.options, "state_precision")
          : tile.precision;
        if (precision != "icon" && precision != "text") {
          return epaper_dashboard_format_number(tile.sensor_value,
                                                epaper_dashboard_parse_precision(precision));
        }
      }
      return tile.sensor_value;
    }
    if (epaper_dashboard_action_state_numeric_card(tile)) return "--";
    if (tile.type == "media") {
      return tile.sensor == "position" ? "0:00" : "--";
    }
    if (tile.type == "sensor" || epaper_dashboard_toggle_numeric_sensor_card(tile) ||
        !epaper_dashboard_sensor_source(tile).empty()) {
      return "--";
    }
    if (!tile.action_state_entity.empty() || tile.type == "climate") return "--";
  }
  if (tile.state_unavailable) return "--";
  if (!tile.state.empty()) return epaper_dashboard_pretty_state(tile.state);
  if (!tile.entity.empty()) return "...";
  return "";
}

}  // namespace espcontrol
