#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "page_access_state_runtime.h"
#include "page_alarm_ack_runtime.h"
#include "page_cover_toggle_runtime.h"
#include "page_fan_status_runtime.h"
#include "page_forecast_runtime.h"
#include "page_light_temperature_runtime.h"
#include "page_local_relay_runtime.h"
#include "page_local_sensor_runtime.h"
#include "page_media_state_runtime.h"
#include "page_number_slider_runtime.h"
#include "page_percent_slider_runtime.h"
#include "page_timer_runtime.h"
#include "panel_config_document.h"

static bool online = true, busy = false;
static uint32_t tick = 100, connection = 1;
namespace esphome {
uint32_t millis() { return tick; }
}  // namespace esphome
struct Coordinator {
  using State = std::string;
  struct Entry {
    std::string entity, attribute;
    std::function<void(State)> receive;
    void* owner;
    uint32_t scope;
  };
  std::vector<Entry> entries;
  bool subscribe(std::string entity, std::string attribute, std::function<void(State)> receive,
                 uint32_t scope, void* owner, bool replay, bool, bool) {
    assert(!replay && scope == 1u << 4);
    entries.push_back({std::move(entity), std::move(attribute), std::move(receive), owner, scope});
    return true;
  }
  void release_owner(void* owner) {
    entries.erase(
        std::remove_if(entries.begin(), entries.end(), [owner](const auto& e) { return e.owner == owner; }),
        entries.end());
  }
  bool dispatching() const { return busy; }
  uint32_t connection_generation() const { return connection; }
  void report(const std::string& entity, const std::string& attr, const std::string& value) {
    for (const auto& e : entries)
      if (e.entity == entity && e.attribute == attr) e.receive(value);
  }
} coordinator;
Coordinator& ha_read_coordinator() { return coordinator; }
bool ha_api_state_connected() { return online; }
void ha_reannounce_state_subscriptions() {}
#include "page_ha_media_transport.h"

// Native producer/forecast sources are doubled; the production state owner,
// graph, all semantic models and their actual event facades are compiled here.
namespace espcontrol::pages {
inline bool (*native_dispatch_safe)() = nullptr;
inline bool page_native_command_ready() { return !busy && (!native_dispatch_safe || native_dispatch_safe()); }
inline size_t native_forecast_start_calls = 0;
class PageHaForecastRequests : public PageForecastRequests {
 public:
  bool connected() const override { return online; }
  uint32_t connection_generation() const override { return connection; }
  uint32_t now_ms() const override { return tick; }
  bool start(void*, PageForecastToken, std::string_view, Receive) override { ++native_forecast_start_calls; return false; }
  void cancel(void*, PageForecastToken) override {}
};
constexpr size_t LOCAL_NUMERIC_PRODUCERS = 1, LOCAL_TEXT_PRODUCERS = 0;
template <size_t, size_t>
class PageESPHomeLocalSensorProvider : public PageLocalSensorProvider {
 public:
  size_t capacity() const override { return 1; }
  uint32_t resolve(std::string_view key, bool text) const override {
    return !text && key == "local_value" ? 0 : NO_LOCAL_SENSOR;
  }
  bool watch(uint32_t source) override { return source == 0; }
  PageLocalSensorSample read(uint32_t) const override { return {1, 23.5f, {}}; }
};
class PageInternalRelayProvider : public PageLocalRelayProvider {
 public:
  PageLocalRelaySnapshot read(std::string_view) const override { return {false, "Relay"}; }
};
}  // namespace espcontrol::pages
struct CalendarBase {
  bool date_valid;
  int day, month;
  bool time_valid;
  int hour, minute;
  bool use_12h;
} calendar_base{};
CalendarBase& calendar_date_state() { return calendar_base; }
size_t calendar_count = 0;
size_t calendar_card_count() { return calendar_count; }
struct TimezoneBase {
  bool valid;
  time_t epoch;
  std::string active_timezone;
  bool use_12h;
} timezone_base{};
TimezoneBase& timezone_card_state() { return timezone_base; }
size_t timezone_card_count() { return 0; }
std::string_view timezone_id_view(std::string_view option) { return option; }
const char* timezone_posix_view(std::string_view, time_t) { return "UTC0"; }
#include "page_native_scene_state.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
