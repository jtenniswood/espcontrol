#pragma once

#include "esphome/components/api/api_server.h"
#include "lvgl.h"

#include <array>
#include <cctype>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace espcontrol {

constexpr int EPAPER_DASHBOARD_PAGE_SLOTS = 20;
constexpr int EPAPER_DASHBOARD_PAGES = 4;
constexpr int EPAPER_DASHBOARD_TOTAL_SLOTS =
    EPAPER_DASHBOARD_PAGE_SLOTS * EPAPER_DASHBOARD_PAGES;

struct EpaperDashboardTile {
  std::string config;
  std::string entity;
  std::string sensor;
  std::string label;
  std::string unit;
  std::string type;
  std::string value;
  bool subscribed = false;
  bool unavailable = false;
};

inline std::array<EpaperDashboardTile, EPAPER_DASHBOARD_TOTAL_SLOTS> &epaper_dashboard_tiles() {
  static std::array<EpaperDashboardTile, EPAPER_DASHBOARD_TOTAL_SLOTS> tiles;
  return tiles;
}

inline bool &epaper_dashboard_dirty_flag() {
  static bool dirty = true;
  return dirty;
}

inline void epaper_dashboard_mark_dirty() {
  epaper_dashboard_dirty_flag() = true;
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
         s == "home" || s == "playing" || s == "heating" || s == "cooling";
}

inline bool epaper_dashboard_state_unavailable(const std::string &value) {
  return value == "unavailable" || value == "unknown";
}

inline bool epaper_dashboard_api_available() {
  return esphome::api::global_api_server != nullptr;
}

inline std::string epaper_dashboard_display_value(const EpaperDashboardTile &tile);

struct EpaperDashboardLvglSlot {
  lv_obj_t *tile = nullptr;
  lv_obj_t *label = nullptr;
  lv_obj_t *value = nullptr;
  lv_obj_t *unit = nullptr;
};

inline std::array<EpaperDashboardLvglSlot, EPAPER_DASHBOARD_PAGE_SLOTS> &epaper_dashboard_lvgl_slots() {
  static std::array<EpaperDashboardLvglSlot, EPAPER_DASHBOARD_PAGE_SLOTS> slots;
  return slots;
}

inline lv_obj_t *&epaper_dashboard_lvgl_page_label() {
  static lv_obj_t *label = nullptr;
  return label;
}

inline void epaper_dashboard_bind_lvgl_page_label(lv_obj_t *label) {
  epaper_dashboard_lvgl_page_label() = label;
}

inline void epaper_dashboard_bind_lvgl_slot(int slot, lv_obj_t *tile, lv_obj_t *label, lv_obj_t *value,
                                           lv_obj_t *unit = nullptr) {
  if (slot < 0 || slot >= EPAPER_DASHBOARD_PAGE_SLOTS) return;
  epaper_dashboard_lvgl_slots()[slot] = {tile, label, value, unit};
  if (tile) {
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
  }
}

inline void epaper_dashboard_style_lvgl_tile(lv_obj_t *tile, lv_obj_t *label, lv_obj_t *value,
                                            bool configured, bool active) {
  if (!tile) return;
  uint32_t bg = active ? 0x000000 : 0xFFFFFF;
  uint32_t fg = active ? 0xFFFFFF : 0x000000;
  lv_obj_set_style_bg_color(tile, lv_color_hex(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(tile, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_border_width(tile, configured ? 2 : 1, LV_PART_MAIN);
  lv_obj_set_style_radius(tile, 6, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(tile, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(tile, 8, LV_PART_MAIN);
  if (label) lv_obj_set_style_text_color(label, lv_color_hex(fg), LV_PART_MAIN);
  if (value) lv_obj_set_style_text_color(value, lv_color_hex(fg), LV_PART_MAIN);
}

inline void epaper_dashboard_update_lvgl_page(int page) {
  page = epaper_dashboard_wrap_page(page);
  auto &tiles = epaper_dashboard_tiles();
  auto &slots = epaper_dashboard_lvgl_slots();
  int start = page * EPAPER_DASHBOARD_PAGE_SLOTS;

  if (auto *page_label = epaper_dashboard_lvgl_page_label()) {
    char page_text[20];
    std::snprintf(page_text, sizeof(page_text), "Page %d/%d", page + 1, EPAPER_DASHBOARD_PAGES);
    lv_label_set_text(page_label, page_text);
  }

  for (int i = 0; i < EPAPER_DASHBOARD_PAGE_SLOTS; i++) {
    const auto &tile = tiles[start + i];
    auto &slot = slots[i];
    if (!slot.tile) continue;
    int col = i % 5;
    int row = i / 5;
    bool configured = !tile.config.empty();
    bool active = configured && epaper_dashboard_state_active(tile.value);
    epaper_dashboard_style_lvgl_tile(slot.tile, slot.label, slot.value, configured, active);
    lv_obj_set_grid_cell(slot.tile, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    if (slot.label) {
      lv_label_set_text(slot.label, configured ? tile.label.c_str() : "Configure");
      lv_obj_clear_flag(slot.label, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.value) {
      std::string value = epaper_dashboard_display_value(tile);
      lv_label_set_text(slot.value, configured ? value.c_str() : "");
      if (configured) lv_obj_clear_flag(slot.value, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(slot.value, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot.unit) {
      lv_label_set_text(slot.unit, configured ? tile.unit.c_str() : "");
      if (configured && !tile.unit.empty()) lv_obj_clear_flag(slot.unit, LV_OBJ_FLAG_HIDDEN);
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
  if (tile.subscribed) return;
  std::string source = !tile.sensor.empty() ? tile.sensor : tile.entity;
  if (!epaper_dashboard_api_available() || source.empty()) return;
  tile.subscribed = true;
  esphome::api::global_api_server->subscribe_home_assistant_state(
      source, {}, [index](esphome::StringRef state) {
        auto &tile = epaper_dashboard_tiles()[index];
        tile.value = std::string(state.c_str(), state.size());
        tile.unavailable = epaper_dashboard_state_unavailable(tile.value);
        epaper_dashboard_mark_dirty();
      });
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
  if (fields.size() > 4) tile.sensor = fields[4];
  if (fields.size() > 5) tile.unit = fields[5];
  if (fields.size() > 6) tile.type = fields[6];
  if (tile.label.empty()) tile.label = epaper_dashboard_title_from_entity(!tile.sensor.empty() ? tile.sensor : tile.entity);
  epaper_dashboard_subscribe(index);
  epaper_dashboard_mark_dirty();
}

inline std::string epaper_dashboard_display_value(const EpaperDashboardTile &tile) {
  if (tile.config.empty()) return "";
  if (tile.unavailable) return "--";
  if (!tile.value.empty()) return tile.value;
  if (!tile.entity.empty() || !tile.sensor.empty()) return "...";
  return "";
}

}  // namespace espcontrol
