#pragma once

#include "esphome/components/api/api_server.h"
#include "lvgl.h"
#include "../espcontrol/icons.h"
#include "../espcontrol/temperature_unit.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace espcontrol {

constexpr int EPAPER_DASHBOARD_PAGE_SLOTS = 12;
constexpr int EPAPER_DASHBOARD_PAGES = 1;
constexpr int EPAPER_DASHBOARD_COLS = 4;
constexpr int EPAPER_DASHBOARD_TOTAL_SLOTS =
    EPAPER_DASHBOARD_PAGE_SLOTS * EPAPER_DASHBOARD_PAGES;

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
  std::string action_state_entity;
  bool state_subscribed = false;
  bool sensor_subscribed = false;
  bool state_unavailable = false;
  bool sensor_unavailable = false;
};

inline std::array<EpaperDashboardTile, EPAPER_DASHBOARD_TOTAL_SLOTS> &epaper_dashboard_tiles() {
  static std::array<EpaperDashboardTile, EPAPER_DASHBOARD_TOTAL_SLOTS> tiles;
  return tiles;
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

inline int epaper_dashboard_hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
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

inline bool epaper_dashboard_state_active(const std::string &value) {
  std::string s;
  s.reserve(value.size());
  for (char ch : value) s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  return s == "on" || s == "open" || s == "unlocked" || s == "detected" ||
         s == "home" || s == "playing" || s == "heating" || s == "cooling" ||
         s == "armed_home" || s == "armed_away" || s == "armed_night" ||
         s == "triggered";
}

inline bool epaper_dashboard_state_unavailable(const std::string &value) {
  return value == "unavailable" || value == "unknown";
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

inline std::string epaper_dashboard_pretty_state(const std::string &value) {
  std::string text = value;
  for (char &ch : text) {
    if (ch == '_') ch = ' ';
  }
  bool cap = true;
  for (char &ch : text) {
    unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isspace(uch)) {
      cap = true;
      continue;
    }
    ch = cap ? static_cast<char>(std::toupper(uch)) : static_cast<char>(std::tolower(uch));
    cap = false;
  }
  return text;
}

inline bool epaper_dashboard_api_available() {
  return esphome::api::global_api_server != nullptr;
}

inline std::string epaper_dashboard_display_value(const EpaperDashboardTile &tile);

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
  lv_obj_t *label = nullptr;
  lv_obj_t *value = nullptr;
  lv_obj_t *unit = nullptr;
};

inline std::array<EpaperDashboardLvglSlot, EPAPER_DASHBOARD_PAGE_SLOTS> &epaper_dashboard_lvgl_slots() {
  static std::array<EpaperDashboardLvglSlot, EPAPER_DASHBOARD_PAGE_SLOTS> slots;
  return slots;
}

inline void epaper_dashboard_bind_lvgl_slot(int slot, lv_obj_t *tile, lv_obj_t *icon,
                                           lv_obj_t *sensor_container, lv_obj_t *label,
                                           lv_obj_t *value, lv_obj_t *unit = nullptr) {
  if (slot < 0 || slot >= EPAPER_DASHBOARD_PAGE_SLOTS) return;
  epaper_dashboard_lvgl_slots()[slot] = {tile, icon, sensor_container, label, value, unit};
  if (tile) {
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
  }
}

inline bool epaper_dashboard_command_only_type(const EpaperDashboardTile &tile) {
  return (tile.type == "action" && tile.entity.empty() && tile.action_state_entity.empty()) ||
         tile.type == "push" || tile.type == "webhook" || tile.type == "internal";
}

inline bool epaper_dashboard_tile_configured(const EpaperDashboardTile &tile) {
  if (tile.config.empty()) return false;
  if (!tile.entity.empty() || !tile.sensor.empty() || !tile.action_state_entity.empty()) return true;
  return tile.type == "clock" || tile.type == "timezone";
}

inline bool epaper_dashboard_sensor_card_type(const EpaperDashboardTile &tile) {
  return tile.type == "sensor" || tile.type == "weather" || tile.type == "weather_forecast" ||
         tile.type == "calendar" || tile.type == "clock" || tile.type == "timezone" ||
         tile.type == "door_window" || tile.type == "presence" ||
         !tile.sensor.empty() || !tile.action_state_entity.empty() ||
         (!tile.entity.empty() && !epaper_dashboard_command_only_type(tile));
}

inline bool epaper_dashboard_value_replaces_icon(const EpaperDashboardTile &tile) {
  return tile.type == "sensor" || tile.type == "weather" || tile.type == "weather_forecast" ||
         tile.type == "calendar" || tile.type == "clock" || tile.type == "timezone";
}

inline const char *epaper_dashboard_icon(const EpaperDashboardTile &tile, bool active) {
  std::string icon = active && !tile.icon_on.empty() && tile.icon_on != "Auto" ? tile.icon_on : tile.icon;
  if (!icon.empty() && icon != "Auto") return find_icon(icon.c_str());
  if (tile.type == "action") return find_icon("Flash");
  if (tile.type == "alarm" || tile.type == "alarm_action") return find_icon("Security");
  if (tile.type == "climate") return find_icon("Thermostat");
  if (tile.type == "cover") {
    if (tile.sensor == "open") return find_icon("Blinds Open");
    return find_icon("Blinds Horizontal");
  }
  if (tile.type == "door_window") return find_icon(active ? "Door Open" : "Door");
  if (tile.type == "fan_speed" || tile.type == "fan_switch" ||
      tile.type == "fan_oscillate" || tile.type == "fan_direction" ||
      tile.type == "fan_preset") return find_icon("Fan");
  if (tile.type == "garage") return find_icon(active || tile.sensor == "open" ? "Garage Open" : "Garage");
  if (tile.type == "light_brightness" || tile.type == "light_switch" ||
      tile.type == "light_temperature") return find_icon("Lightbulb");
  if (tile.type == "lock") return find_icon(active || tile.sensor == "unlock" ? "Lock Open" : "Lock");
  if (tile.type == "media") {
    if (tile.sensor == "previous") return find_icon("Skip Previous");
    if (tile.sensor == "next") return find_icon("Skip Next");
    if (tile.sensor == "volume") return find_icon("Volume High");
    if (tile.sensor == "position") return find_icon("Progress Clock");
    if (tile.sensor == "now_playing") return find_icon("Music");
    return find_icon("Play Pause");
  }
  if (tile.type == "presence") return find_icon("Account");
  if (tile.type == "push" || tile.type == "webhook") return find_icon("Gesture Tap");
  if (tile.type == "slider") return find_icon("Gauge");
  size_t dot = tile.entity.find('.');
  if (dot != std::string::npos) return domain_default_icon(tile.entity.substr(0, dot));
  return find_icon("Auto");
}

inline void epaper_dashboard_style_lvgl_tile(lv_obj_t *tile, lv_obj_t *icon, lv_obj_t *label,
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
  lv_obj_set_style_radius(tile, 6, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(tile, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(tile, 8, LV_PART_MAIN);
  if (icon) lv_obj_set_style_text_color(icon, lv_color_hex(fg), LV_PART_MAIN);
  if (label) lv_obj_set_style_text_color(label, lv_color_hex(fg), LV_PART_MAIN);
  if (value) lv_obj_set_style_text_color(value, lv_color_hex(fg), LV_PART_MAIN);
  if (unit) lv_obj_set_style_text_color(unit, lv_color_hex(fg), LV_PART_MAIN);
}

inline void epaper_dashboard_update_lvgl_page(int page) {
  page = epaper_dashboard_wrap_page(page);
  auto &tiles = epaper_dashboard_tiles();
  auto &slots = epaper_dashboard_lvgl_slots();
  int start = page * EPAPER_DASHBOARD_PAGE_SLOTS;

  for (int i = 0; i < EPAPER_DASHBOARD_PAGE_SLOTS; i++) {
    const auto &tile = tiles[start + i];
    auto &slot = slots[i];
    if (!slot.tile) continue;
    int col = i % EPAPER_DASHBOARD_COLS;
    int row = i / EPAPER_DASHBOARD_COLS;
    bool configured = epaper_dashboard_tile_configured(tile);
    lv_obj_set_grid_cell(slot.tile, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    if (!configured) {
      if (slot.icon) lv_label_set_text(slot.icon, "");
      if (slot.label) lv_label_set_text(slot.label, "");
      if (slot.value) lv_label_set_text(slot.value, "");
      if (slot.unit) lv_label_set_text(slot.unit, "");
      lv_obj_add_flag(slot.tile, LV_OBJ_FLAG_HIDDEN);
      lv_obj_invalidate(slot.tile);
      lv_obj_t *parent = lv_obj_get_parent(slot.tile);
      if (parent) lv_obj_invalidate(parent);
      continue;
    }
    lv_obj_clear_flag(slot.tile, LV_OBJ_FLAG_HIDDEN);
    const std::string &active_value = !tile.state.empty() ? tile.state : tile.sensor_value;
    bool active = configured && epaper_dashboard_state_active(active_value);
    bool show_value = configured && !epaper_dashboard_command_only_type(tile) &&
        epaper_dashboard_sensor_card_type(tile);
    bool value_replaces_icon = show_value && epaper_dashboard_value_replaces_icon(tile);
    epaper_dashboard_style_lvgl_tile(slot.tile, slot.icon, slot.label, slot.value, slot.unit, configured, active);
    if (slot.icon) {
      lv_label_set_text(slot.icon, epaper_dashboard_icon(tile, active));
      if (value_replaces_icon) lv_obj_add_flag(slot.icon, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_clear_flag(slot.icon, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.sensor_container) {
      if (show_value) {
        lv_obj_clear_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(slot.sensor_container, value_replaces_icon ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT, 0, 0);
      } else {
        lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (slot.label) {
      lv_label_set_text(slot.label, tile.label.c_str());
      lv_obj_clear_flag(slot.label, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.value) {
      std::string value = epaper_dashboard_display_value(tile);
      lv_label_set_text(slot.value, value.c_str());
      if (show_value) lv_obj_clear_flag(slot.value, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(slot.value, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.unit) {
      lv_label_set_text(slot.unit, tile.unit.c_str());
      if (show_value && !tile.unit.empty()) lv_obj_clear_flag(slot.unit, LV_OBJ_FLAG_HIDDEN);
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
  if (!tile.entity.empty() && !tile.state_subscribed) {
    tile.state_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        tile.entity, {}, [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.state = std::string(state.c_str(), state.size());
          tile.state_unavailable = epaper_dashboard_state_unavailable(tile.state);
          epaper_dashboard_mark_dirty();
        });
  }
  std::string sensor_source = !tile.action_state_entity.empty() ? tile.action_state_entity : tile.sensor;
  if (!sensor_source.empty() && !tile.sensor_subscribed) {
    tile.sensor_subscribed = true;
    esphome::api::global_api_server->subscribe_home_assistant_state(
        sensor_source, {}, [index](esphome::StringRef state) {
          auto &tile = epaper_dashboard_tiles()[index];
          tile.sensor_value = std::string(state.c_str(), state.size());
          tile.sensor_unavailable = epaper_dashboard_state_unavailable(tile.sensor_value);
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
  if (fields.size() > 2) tile.icon = fields[2];
  if (fields.size() > 3) tile.icon_on = fields[3];
  if (fields.size() > 4) tile.sensor = fields[4];
  if (fields.size() > 5) tile.unit = fields[5];
  if (fields.size() > 6) tile.type = fields[6];
  if (fields.size() > 7) tile.precision = fields[7];
  if (fields.size() > 8) tile.options = fields[8];
  if (tile.type == "action") {
    tile.action_state_entity = epaper_dashboard_option_value(tile.options, "state_entity");
    std::string action_unit = epaper_dashboard_option_value(tile.options, "state_unit");
    if (!action_unit.empty()) tile.unit = action_unit;
  }
  if (!epaper_dashboard_tile_configured(tile)) {
    tile = EpaperDashboardTile{};
    epaper_dashboard_mark_dirty();
    return;
  }
  if (tile.label.empty()) tile.label = epaper_dashboard_title_from_entity(!tile.sensor.empty() ? tile.sensor : tile.entity);
  epaper_dashboard_subscribe(index);
  epaper_dashboard_mark_dirty();
}

inline std::string epaper_dashboard_display_value(const EpaperDashboardTile &tile) {
  if (tile.config.empty()) return "";
  bool use_sensor_value = tile.type == "sensor" || tile.type == "weather" ||
      tile.type == "weather_forecast" || tile.type == "calendar" ||
      tile.type == "clock" || tile.type == "timezone" ||
      tile.type == "door_window" || tile.type == "presence" ||
      !tile.sensor.empty() || !tile.action_state_entity.empty();
  if (use_sensor_value) {
    if (tile.sensor_unavailable) return "--";
    if (!tile.sensor_value.empty()) {
      if (tile.precision == "text" || tile.type == "door_window" || tile.type == "presence") {
        return epaper_dashboard_pretty_state(tile.sensor_value);
      }
      return tile.sensor_value;
    }
    if (!tile.sensor.empty() || !tile.action_state_entity.empty()) return "...";
  }
  if (tile.state_unavailable) return "--";
  if (!tile.state.empty()) return epaper_dashboard_pretty_state(tile.state);
  if (!tile.entity.empty()) return "...";
  return "";
}

}  // namespace espcontrol
