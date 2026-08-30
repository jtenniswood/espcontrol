#include "home_assistant_endpoint_resolver.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#if defined(USE_ESP32) && defined(USE_MDNS)
#include <mdns.h>
#endif

namespace espcontrol {

static const char *const TAG = "ha_endpoint";
constexpr uint32_t QUERY_TIMEOUT_MS = 2000;
constexpr size_t QUERY_MAX_RESULTS = 8;
constexpr uint32_t RETRY_DELAYS_MS[] = {5000, 30000, 300000};

void HomeAssistantEndpointResolver::setup() {
  setup_complete_ = true;
  recompute_fallback();
}

void HomeAssistantEndpointResolver::configure(
    const std::string &mode, const std::string &protocol, uint16_t port,
    const std::string &client_address) {
  const auto next_mode = mode == "Manual"
                             ? home_assistant_endpoint::Mode::MANUAL
                             : home_assistant_endpoint::Mode::AUTOMATIC;
  const std::string next_protocol =
      home_assistant_endpoint::normalize_protocol(protocol);
  const uint16_t next_port = port == 0 ? 8123 : port;
  const std::string next_client =
      home_assistant_endpoint::normalize_address(client_address);
  const bool protocol_changed = next_protocol != protocol_;
  const bool client_changed = next_client != client_address_;
  const bool discovery_input_changed = next_mode != mode_ ||
      protocol_changed || client_changed;
  const bool fallback_changed =
      next_port != port_ || protocol_changed || client_changed;
  mode_ = next_mode;
  protocol_ = next_protocol;
  port_ = next_port;
  client_address_ = next_client;
  if (mode_ == home_assistant_endpoint::Mode::MANUAL) {
    cancel_query();
    retry_stage_ = 0;
    recompute_fallback();
    return;
  }
  if (discovery_input_changed) {
    cancel_query();
    retry_stage_ = 0;
    // Keep relative artwork usable through the discovery window using the
    // current verified origin when it still belongs to this client, otherwise
    // use the configured fallback on the newly connected HA host. Discovery
    // may replace it only after an unambiguous address match.
    const std::string pending_origin =
        !origin_.empty() && !protocol_changed && !client_changed
            ? origin_
            : home_assistant_endpoint::build_origin(protocol_, client_address_, port_);
    publish(pending_origin, home_assistant_endpoint::Source::DISCOVERING);
    next_query_ms_ = esphome::millis();
  } else if (fallback_changed && source_ == home_assistant_endpoint::Source::FALLBACK) {
    recompute_fallback();
  }
}

void HomeAssistantEndpointResolver::request_discovery() {
  if (mode_ != home_assistant_endpoint::Mode::AUTOMATIC) return;
  cancel_query();
  retry_stage_ = 0;
  const std::string pending_origin = origin_.empty()
      ? home_assistant_endpoint::build_origin(protocol_, client_address_, port_)
      : origin_;
  publish(pending_origin, home_assistant_endpoint::Source::DISCOVERING);
  next_query_ms_ = esphome::millis();
}

void HomeAssistantEndpointResolver::recompute_fallback() {
  const std::string fallback = home_assistant_endpoint::build_origin(
      protocol_, client_address_, port_);
  publish(fallback, mode_ == home_assistant_endpoint::Mode::MANUAL
                        ? home_assistant_endpoint::Source::MANUAL
                        : home_assistant_endpoint::Source::FALLBACK);
}

void HomeAssistantEndpointResolver::publish(
    std::string origin, home_assistant_endpoint::Source source) {
  std::string status;
  switch (source) {
    case home_assistant_endpoint::Source::DISCOVERING:
      status = "Discovering";
      break;
    case home_assistant_endpoint::Source::AUTOMATIC:
      status = "Automatic — " + origin;
      break;
    case home_assistant_endpoint::Source::FALLBACK:
      status = "Fallback — " + origin;
      break;
    case home_assistant_endpoint::Source::MANUAL:
      status = "Manual — " + origin;
      break;
  }
  if (origin_ == origin && source_ == source && status_ == status) return;
  origin_ = std::move(origin);
  source_ = source;
  status_ = std::move(status);
  ESP_LOGI(TAG, "%s", status_.c_str());
  if (change_callback_) change_callback_();
}

void HomeAssistantEndpointResolver::loop() {
  if (!setup_complete_) return;
  const uint32_t now = esphome::millis();
#if defined(USE_ESP32) && defined(USE_MDNS)
  if (query_ != nullptr) {
    mdns_result_t *results = nullptr;
    if (!mdns_query_async_get_results(query_, 0, &results, nullptr)) return;
    std::vector<home_assistant_endpoint::ServiceRecord> records;
    for (mdns_result_t *result = results; result != nullptr; result = result->next) {
      home_assistant_endpoint::ServiceRecord record;
      record.port = result->port;
      for (mdns_ip_addr_t *address = result->addr; address != nullptr;
           address = address->next) {
        char buffer[64]{};
        if (address->addr.type == ESP_IPADDR_TYPE_V6) {
          std::snprintf(buffer, sizeof(buffer), IPV6STR,
                        IPV62STR(address->addr.u_addr.ip6));
        } else {
          std::snprintf(buffer, sizeof(buffer), IPSTR,
                        IP2STR(&address->addr.u_addr.ip4));
        }
        record.addresses.emplace_back(buffer);
      }
      for (size_t index = 0; index < result->txt_count; ++index) {
        const char *key = result->txt[index].key;
        const char *value = result->txt[index].value;
        if (key == nullptr) continue;
        if (std::strcmp(key, "internal_url") == 0 && value != nullptr)
          record.internal_url = value;
        if (std::strcmp(key, "landingpage") == 0 && value != nullptr &&
            (std::strcmp(value, "True") == 0 || std::strcmp(value, "true") == 0))
          record.landing_page = true;
      }
      records.push_back(std::move(record));
    }
    const std::string discovered = mode_ == home_assistant_endpoint::Mode::AUTOMATIC
        ? home_assistant_endpoint::select_discovered_origin(
              records, client_address_, protocol_)
        : std::string();
    if (results != nullptr) mdns_query_results_free(results);
    mdns_query_async_delete(query_);
    query_ = nullptr;
    if (mode_ != home_assistant_endpoint::Mode::AUTOMATIC ||
        client_address_.empty()) {
      rediscover_after_query_ = false;
      recompute_fallback();
      return;
    }
    if (rediscover_after_query_) {
      rediscover_after_query_ = false;
      next_query_ms_ = now;
      return;
    }
    if (!discovered.empty()) {
      retry_stage_ = 0;
      publish(discovered, home_assistant_endpoint::Source::AUTOMATIC);
      next_query_ms_ = now + RETRY_DELAYS_MS[2];
      return;
    }
    recompute_fallback();
    schedule_retry(now);
    return;
  }
#endif
  if (mode_ != home_assistant_endpoint::Mode::AUTOMATIC ||
      client_address_.empty()) return;
  if (static_cast<int32_t>(now - next_query_ms_) >= 0) start_query(now);
}

void HomeAssistantEndpointResolver::start_query(uint32_t now) {
#if defined(USE_ESP32) && defined(USE_MDNS)
  query_ = mdns_query_async_new(nullptr, "_home-assistant", "_tcp",
                                MDNS_TYPE_PTR, QUERY_TIMEOUT_MS,
                                QUERY_MAX_RESULTS, nullptr);
  if (query_ != nullptr) {
    ESP_LOGD(TAG, "Discovering the connected Home Assistant HTTP endpoint");
    return;
  }
#endif
  recompute_fallback();
  schedule_retry(now);
}

void HomeAssistantEndpointResolver::schedule_retry(uint32_t now) {
  const size_t index = std::min<size_t>(retry_stage_, 2);
  next_query_ms_ = now + RETRY_DELAYS_MS[index];
  if (retry_stage_ < 2) ++retry_stage_;
}

void HomeAssistantEndpointResolver::cancel_query() {
#if defined(USE_ESP32) && defined(USE_MDNS)
  if (query_ != nullptr) {
    mdns_result_t *results = nullptr;
    if (mdns_query_async_get_results(query_, 0, &results, nullptr)) {
      if (results != nullptr) mdns_query_results_free(results);
      mdns_query_async_delete(query_);
      query_ = nullptr;
    } else {
      // Espressif only permits deleting a completed asynchronous query. Mark
      // the request stale and start a fresh one immediately after it finishes.
      rediscover_after_query_ = true;
    }
  }
#endif
}

void HomeAssistantEndpointResolver::shutdown() {
  cancel_query();
#if defined(USE_ESP32) && defined(USE_MDNS)
  if (query_ != nullptr) {
    mdns_result_t *results = nullptr;
    // Shutdown is outside the display loop, so it is safe to wait for the
    // query's bounded two-second lifetime before releasing its resources.
    if (mdns_query_async_get_results(query_, QUERY_TIMEOUT_MS, &results, nullptr)) {
      if (results != nullptr) mdns_query_results_free(results);
      mdns_query_async_delete(query_);
      query_ = nullptr;
    }
  }
#endif
  setup_complete_ = false;
}

}  // namespace espcontrol
