#pragma once
// Include after the existing HA request and weather-template helpers.
#include <cassert>
#include <cstring>

#include "page_forecast_runtime.h"
namespace espcontrol::pages {
class PageHaForecastRequests final : public PageForecastRequests {
 public:
  ~PageHaForecastRequests() override {
    assert(pending_.call_id == 0);
  }
  PageHaForecastRequests() = default;
  PageHaForecastRequests(const PageHaForecastRequests&) = delete;
  PageHaForecastRequests& operator=(const PageHaForecastRequests&) = delete;
  bool connected() const override {
    return ha_api_state_connected();
  }
  uint32_t connection_generation() const override {
    return ha_read_coordinator().connection_generation();
  }
  uint32_t now_ms() const override {
    return esphome::millis();
  }
  bool start(void* owner, PageForecastToken token, std::string_view entity, Receive receive) override {
#if defined(ESPCONTROL_DISABLE_WEATHER_FORECAST) && ESPCONTROL_DISABLE_WEATHER_FORECAST
    (void)owner;
    (void)token;
    (void)entity;
    (void)receive;
    return false;
#else
    if (pending_.call_id || !owner || !receive || !token.request || !connected() ||
        !weather::entity_id_safe(entity) ||
        !ha_internal_heap_available("forecast request", HA_ACTION_INTERNAL_FREE_MIN_BYTES,
                                    HA_ACTION_INTERNAL_LARGEST_MIN_BYTES))
      return false;
    const auto call_id = next_weather_forecast_call_id();
    if (call_id == 0) return false;
    esphome::api::HomeassistantActionRequest request;
    if (!ha_action_begin(request, "weather.get_forecasts", false, 2, call_id)) return false;
    request.wants_response = true;
    const std::string name(entity), response_template = weather_daily_response_template(name);
    request.response_template = decltype(request.response_template)(response_template);
    ha_action_add_entity(request, name);
    ha_action_add_data(request, "type", "daily");
    pending_ = {owner, token, receive, call_id};
    if (!ha_register_action_response_callback(
            call_id,
            [this, call_id](const esphome::api::ActionResponse& response) { complete(call_id, response); })) {
      pending_ = {};
      return false;
    }
    if (!ha_action_send(request)) {
      if (pending_.call_id == call_id) pending_ = {};
      ha_cancel_action_response_callback(call_id, "send failed");
      return false;
    }
    return true;
#endif
  }
  void cancel(void* owner, PageForecastToken token) override {
    if (!pending_.call_id || pending_.owner != owner || pending_.token.configuration != token.configuration ||
        pending_.token.connection != token.connection || pending_.token.request != token.request ||
        pending_.token.source != token.source)
      return;
    const auto call_id = pending_.call_id;
    pending_ = {};
    ha_cancel_action_response_callback(call_id, "forecast owner retired");
  }

 private:
  struct Pending {
    void* owner{nullptr};
    PageForecastToken token;
    Receive receive{nullptr};
    uint32_t call_id{0};
  };
#if !defined(ESPCONTROL_DISABLE_WEATHER_FORECAST) || !ESPCONTROL_DISABLE_WEATHER_FORECAST
  void complete(uint32_t call_id, const esphome::api::ActionResponse& response) {
    if (pending_.call_id != call_id) return;
    const auto pending = pending_;
    pending_ = {};
    if (!response.is_success()) {
      pending.receive(pending.owner, pending.token, {}, false);
      return;
    }
    auto json = response.get_json();
    const char* payload = json["response"].as<const char*>();
    if (!payload) {
      pending.receive(pending.owner, pending.token, {}, false);
      return;
    }
    const auto size = strnlen(payload, weather::MAX_PAYLOAD_BYTES + 1);
    pending.receive(pending.owner, pending.token, {payload, size}, true);
  }
#endif
  Pending pending_;
};
}  // namespace espcontrol::pages
