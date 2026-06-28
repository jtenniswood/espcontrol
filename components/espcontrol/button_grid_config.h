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
static_assert(correct_display_color(0xF0F0F0, 200, 200, 200) == 0xFFFFFF,
              "colour correction must clamp channels at 255");

#ifndef ESPCONTROL_MAX_GRID_SLOTS
#define ESPCONTROL_MAX_GRID_SLOTS 25
#endif

constexpr int MAX_GRID_SLOTS = ESPCONTROL_MAX_GRID_SLOTS;
static_assert(MAX_GRID_SLOTS > 0, "ESPCONTROL_MAX_GRID_SLOTS must be positive");
constexpr int MAX_SUBPAGE_ITEMS = MAX_GRID_SLOTS * MAX_GRID_SLOTS;
#include "button_grid_contract_generated.h"
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
constexpr const char *LIGHT_CONTROL_TABS_OPTION = card_runtime_option_name_light_tabs();
constexpr const char *LIGHT_CONTROL_DEFAULT_TABS_VALUE = "power|brightness|temperature|color";
constexpr const char *COVER_CONTROL_TABS_OPTION = card_runtime_option_name_cover_tabs();
constexpr const char *COVER_CONTROL_DEFAULT_TABS_VALUE = "position|controls|tilt";
constexpr const char *FAN_CONTROL_TABS_OPTION = "fan_tabs";
constexpr const char *FAN_CONTROL_DEFAULT_TABS_VALUE = "power|speed|preset|oscillation|direction";
constexpr const char *LABEL_DISPLAY_OPTION = card_runtime_option_name_label_display();
constexpr const char *NUMBER_DISPLAY_OPTION = card_runtime_option_name_number_display();
constexpr const char *TEMPERATURE_STEP_OPTION = card_runtime_option_name_temperature_step();
constexpr const char *VOLUME_MAX_OPTION = card_runtime_option_name_volume_max();

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

struct ScreenLockCardRef {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  const char *locked_icon = nullptr;
  const char *unlocked_icon = nullptr;
};

inline bool &screen_lock_enabled() {
  static bool locked = false;
  return locked;
}

inline std::vector<lv_obj_t *> &screen_lock_controlled_buttons() {
  static std::vector<lv_obj_t *> buttons;
  return buttons;
}

inline std::vector<ScreenLockCardRef> &screen_lock_card_refs() {
  static std::vector<ScreenLockCardRef> refs;
  return refs;
}

inline std::vector<lv_obj_t *> &screen_lock_clickable_objects() {
  static std::vector<lv_obj_t *> objects;
  return objects;
}

inline void screen_lock_reset_registry() {
  screen_lock_controlled_buttons().clear();
  screen_lock_card_refs().clear();
  screen_lock_clickable_objects().clear();
}

inline bool screen_lock_button_is_lock_card(lv_obj_t *btn) {
  for (const auto &ref : screen_lock_card_refs()) {
    if (ref.btn == btn) return true;
  }
  return false;
}

inline void screen_lock_register_controlled_button(lv_obj_t *btn) {
  if (!btn) return;
  auto &buttons = screen_lock_controlled_buttons();
  if (std::find(buttons.begin(), buttons.end(), btn) == buttons.end()) {
    buttons.push_back(btn);
  }
}

inline void screen_lock_register_card(const BtnSlot &s, const ParsedCfg &p);

inline void screen_lock_clear_clickable_tree(lv_obj_t *obj) {
  if (!obj) return;
  auto &clickable = screen_lock_clickable_objects();
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    if (std::find(clickable.begin(), clickable.end(), obj) == clickable.end()) {
      clickable.push_back(obj);
    }
  }
  int32_t child_count = static_cast<int32_t>(lv_obj_get_child_cnt(obj));
  for (int32_t i = 0; i < child_count; i++) {
    screen_lock_clear_clickable_tree(lv_obj_get_child(obj, i));
  }
}

inline void screen_lock_apply() {
  bool locked = screen_lock_enabled();
  if (screen_lock_card_refs().empty()) {
    locked = false;
    screen_lock_enabled() = false;
  }

  auto &clickable = screen_lock_clickable_objects();
  for (lv_obj_t *btn : screen_lock_controlled_buttons()) {
    if (!btn || screen_lock_button_is_lock_card(btn)) continue;
    if (locked) {
      screen_lock_clear_clickable_tree(btn);
    }
  }
  if (!locked) {
    for (lv_obj_t *obj : clickable) {
      if (obj) lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    clickable.clear();
  }

  for (const auto &ref : screen_lock_card_refs()) {
    if (!ref.btn) continue;
    set_card_checked_state(ref.btn, locked);
    lv_obj_add_flag(ref.btn, LV_OBJ_FLAG_CLICKABLE);
    if (ref.icon_lbl) {
      const char *icon = locked ? ref.locked_icon : ref.unlocked_icon;
      lv_label_set_text(ref.icon_lbl, icon ? icon : "");
    }
    if (ref.text_lbl) {
      lv_label_set_text(ref.text_lbl,
        locked ? espcontrol_i18n("Screen Locked") : espcontrol_i18n("Screen Unlocked"));
    }
  }
}

inline void screen_lock_set_enabled(bool locked) {
  screen_lock_enabled() = locked;
  screen_lock_apply();
}

inline void screen_lock_toggle() {
  screen_lock_set_enabled(!screen_lock_enabled());
}

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
    value == "cover" || value == "garage" ||
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

inline std::string climate_card_options_normalized(const std::string &options) {
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
  if (p.type == "climate") {
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
    p.precision = normalize_climate_precision_config(p.precision);
    p.options = climate_card_options_normalized(p.options);
  }
  if (p.type == "garage") {
    if (!card_runtime_garage_mode_valid(p.sensor)) p.sensor.clear();
    p.unit.clear();
    p.precision.clear();
    if (!p.sensor.empty()) p.icon_on = "Auto";
    p.options = garage_card_options_normalized(p.options, p.sensor);
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
  if (!p.type.empty() && p.type != "action" && p.type != "alarm" && p.type != "alarm_action" && p.type != "climate" && p.type != "cover" && p.type != "garage" && p.type != "webhook" && p.type != "screen_lock" && p.type != "todo" && p.type != "sensor" && p.type != "door_window" && p.type != "presence" && p.type != "media" && p.type != "subpage" && p.type != "image" && p.type != "light_control" && p.type != "vacuum" && p.type != "lawn_mower" && !fan_card_type(p.type) && !card_large_numbers_supported(p)) {
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
  return p.type == "media" && p.sensor == "volume"
    ? normalize_media_volume_max_percent(cfg_option_value(p.options, VOLUME_MAX_OPTION))
    : card_runtime_media_volume_max_default();
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

struct HaControlAvailabilityRef {
  lv_obj_t *visual_obj;
  lv_obj_t *input_obj;
  bool disable_interaction;
};

inline std::vector<HaControlAvailabilityRef> &ha_control_availability_refs() {
  static std::vector<HaControlAvailabilityRef> refs;
  return refs;
}

inline void reset_ha_control_availability_refs() {
  ha_control_availability_refs().clear();
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

inline void register_ha_control_availability(lv_obj_t *visual_obj, lv_obj_t *input_obj,
                                             bool disable_interaction = true) {
  if (!visual_obj && !input_obj) return;
  std::vector<HaControlAvailabilityRef> &refs = ha_control_availability_refs();
  for (const auto &ref : refs) {
    if (ref.visual_obj == visual_obj && ref.input_obj == input_obj &&
        ref.disable_interaction == disable_interaction) {
      return;
    }
  }
  refs.push_back({visual_obj, input_obj, disable_interaction});
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

inline void apply_registered_ha_control_availability(bool available) {
  const auto &refs = ha_control_availability_refs();
  for (const auto &ref : refs) {
    apply_control_availability(ref.visual_obj, ref.input_obj, available, ref.disable_interaction);
  }
  if (!refs.empty()) notify_dashboard_content_changed();
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
          ? (ref.day == "today" ? espcontrol_i18n(std::string("Today")) : espcontrol_i18n(std::string("Tomorrow")))
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
    apply_control_availability(refs[i].btn, refs[i].btn, refs[i].valid, false);
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
  apply_control_availability(weather_forecast_card_refs()[count - 1].btn,
                             weather_forecast_card_refs()[count - 1].btn,
                             false, false);
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
    "{% set response_data = response if response is defined and response is not none else {} %}"
    "{% set entity_response = response_data if 'forecast' in response_data else (response_data[entity] if entity in response_data else {}) %}"
    "{% set forecasts = entity_response['forecast'] if 'forecast' in entity_response else [] %}"
    "{% set today_date = now().date() %}{% set tomorrow_date = (now() + timedelta(days=1)).date() %}"
    "{% set ns = namespace(today=none, tomorrow=none) %}{% for item in forecasts %}"
    "{% set item_dt = as_datetime(item['datetime']) if 'datetime' in item else none %}{% set item_date = as_local(item_dt).date() if item_dt is not none else (as_datetime(item['date']).date() if 'date' in item else none) %}"
    "{% if item_date == today_date and ns.today is none %}{% set ns.today = item %}{% elif item_date == tomorrow_date and ns.tomorrow is none %}{% set ns.tomorrow = item %}{% endif %}"
    "{% endfor %}"
    "{% set today = ns.today if ns.today is not none else (forecasts[0] if forecasts|length > 0 else none) %}"
    "{% set tomorrow = ns.tomorrow if ns.tomorrow is not none else (forecasts[1] if forecasts|length > 1 else none) %}"
    "{% set high_keys = ['temperature','native_temperature','temperature_high','native_temperature_high','high_temperature','max_temperature','temperature_max','temp_high','max_temp','high'] %}"
    "{% set low_keys = ['templow','native_templow','temperature_low','native_temperature_low','low_temperature','min_temperature','temperature_min','temp_low','min_temp','low'] %}"
    "{% set unit_keys = ['temperature_unit','native_temperature_unit','unit_of_measurement','native_unit_of_measurement','unit'] %}"
    "{% set out = namespace(today_high='', today_low='', tomorrow_high='', tomorrow_low='', unit='') %}"
    "{% for key in high_keys %}{% if out.today_high == '' and today is not none and key in today %}{% set out.today_high = today[key] %}{% endif %}{% if out.tomorrow_high == '' and tomorrow is not none and key in tomorrow %}{% set out.tomorrow_high = tomorrow[key] %}{% endif %}{% endfor %}"
    "{% for key in low_keys %}{% if out.today_low == '' and today is not none and key in today %}{% set out.today_low = today[key] %}{% endif %}{% if out.tomorrow_low == '' and tomorrow is not none and key in tomorrow %}{% set out.tomorrow_low = tomorrow[key] %}{% endif %}{% endfor %}"
    "{% for key in unit_keys %}{% if out.unit == '' and key in entity_response %}{% set out.unit = entity_response[key] %}{% endif %}{% if out.unit == '' and today is not none and key in today %}{% set out.unit = today[key] %}{% endif %}{% if out.unit == '' and tomorrow is not none and key in tomorrow %}{% set out.unit = tomorrow[key] %}{% endif %}{% endfor %}"
    "{{ out.today_high }}|{{ out.today_low }}|{{ out.tomorrow_high }}|{{ out.tomorrow_low }}|"
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
  if (p.sensor == "open") return espcontrol_i18n("Open");
  if (p.sensor == "close") return espcontrol_i18n("Close");
  return espcontrol_i18n("Garage Door");
}

inline bool garage_card_show_status(const ParsedCfg &p) {
  return normalize_garage_label_display(cfg_option_value(p.options, "label_display")) == "status";
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
  if (p.sensor == "lock") return espcontrol_i18n("Lock");
  if (p.sensor == "unlock") return espcontrol_i18n("Unlock");
  return espcontrol_i18n("Lock");
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
inline std::string local_endpoint_json_escape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 4);
  for (char c : s) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else out += c;
  }
  return out;
}

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
      json += "{\"key\":\"" + local_endpoint_json_escape(a.key) +
              "\",\"label\":\"" + local_endpoint_json_escape(a.label) + "\"}";
    }
    json += "]";
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK) ESP_LOGE("espcontrol", "httpd_resp_send failed: %d", err);
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

class LocalSensorHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:

  static std::string build_json() {
    std::string json;
    json.reserve(512);
    json = "[";
    bool first = true;
    auto append = [&](const std::string &key, const std::string &name,
                      const std::string &unit, const char *type, bool internal) {
      if (!first) json += ",";
      first = false;
      json += "{\"key\":\"" + local_endpoint_json_escape(key) + "\",\"name\":\"" + local_endpoint_json_escape(name) +
              "\",\"unit\":\"" + local_endpoint_json_escape(unit) + "\",\"type\":\"" + type + "\"";
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
  return p.entity.empty() ? espcontrol_i18n(std::string("Relay")) : sentence_cap_text(p.entity);
}

inline const char *internal_relay_icon(const ParsedCfg &p, bool push_mode) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(push_mode ? "Gesture Tap" : "Power Plug");
}

inline void apply_internal_relay_state(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                       bool on, bool has_icon_on,
                                       const char *icon_off, const char *icon_on) {
  if (btn) {
    set_card_checked_state(btn, on);
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
    set_card_checked_state(w.parent_btn, true);
    if (w.parent_has_alt_icon && w.parent_icon)
      lv_label_set_text(w.parent_icon, w.parent_on_glyph);
  } else {
    set_card_checked_state(w.parent_btn, false);
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
  if (state == "open") return espcontrol_i18n_key("state_open");
  return espcontrol_i18n(sentence_cap_text(state));
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
