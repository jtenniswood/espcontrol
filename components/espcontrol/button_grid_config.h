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

#include "button_grid_config_parser.h"

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
