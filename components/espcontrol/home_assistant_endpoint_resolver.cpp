#include "home_assistant_endpoint_resolver.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#if defined(USE_ESP32) && defined(USE_MDNS)
#include <mdns.h>
#endif

namespace espcontrol {
using namespace home_assistant_endpoint;
static const char *const TAG = "ha_endpoint";
constexpr uint32_t QUERY_TIMEOUT_MS = 2000;
constexpr size_t QUERY_MAX_RESULTS = 8;
constexpr uint32_t RETRY_DELAYS_MS[] = {5000, 30000, 300000};

std::string HomeAssistantEndpointResolver::fallback() const {
  const std::string &host = mode_ == Mode::MANUAL && !manual_host_.empty()
      ? manual_host_ : client_address_;
  return parse_origin(build_origin(protocol_, host, port_));
}
void HomeAssistantEndpointResolver::setup() { setup_complete_ = true; }

void HomeAssistantEndpointResolver::configure(const std::string &mode,
    const std::string &protocol, uint16_t port, const std::string &client,
    const std::string &manual_host) {
  const auto next_mode = mode == "Manual" ? Mode::MANUAL : Mode::AUTOMATIC;
  const auto next_protocol = normalize_protocol(protocol);
  const auto next_client = normalize_address(client);
  const auto parsed_manual = parse_origin(build_origin("http", manual_host, 80));
  const std::string next_manual_host = parsed_manual.empty()
      ? std::string() : parsed_manual.substr(7, parsed_manual.size() - 10);
  const uint16_t next_port = port ? port : 8123;
  if (next_mode == mode_ && next_protocol == protocol_ && next_port == port_ &&
      next_client == client_address_ && next_manual_host == manual_host_) return;
  const bool same_client = next_client == client_address_;
  mode_ = next_mode;
  protocol_ = next_protocol;
  port_ = next_port;
  client_address_ = next_client;
  manual_host_ = next_manual_host;
  cancel_query();
  ++selection_generation_;
  ++origin_generation_;
  selecting_ = probe_pending_ = healthy_ = recovery_ = recovery_requested_ = false;
  next_recovery_ms_ = esphome::millis();
  transport_failures_ = retry_stage_ = 0;
  records_.clear();
  candidates_.clear();
  if (mode_ == Mode::MANUAL) {
    health_ = "Manual connection";
    publish(fallback(), Source::MANUAL);
  } else {
    health_ = "Checking connection";
    // An old host must never remain a token-bearing destination after reconnect.
    publish(same_client ? origin_ : std::string(), Source::DISCOVERING);
    next_query_ms_ = esphome::millis();
  }
}

void HomeAssistantEndpointResolver::request_discovery() {
  if (mode_ != Mode::AUTOMATIC) return;
  cancel_query();
  ++selection_generation_;
  selecting_ = probe_pending_ = false;
  next_query_ms_ = esphome::millis();
  retry_stage_ = 0;
}

void HomeAssistantEndpointResolver::invalidate_connection() {
  // A replacement API session can have the same peer address. Its downloads
  // and probes must still belong to a different generation.
  ++origin_generation_;
  healthy_ = recovery_ = recovery_requested_ = false;
  transport_failures_ = 0;
  records_.clear();
  request_discovery();
}

void HomeAssistantEndpointResolver::publish(std::string origin, Source source) {
  const bool changed = origin_ != origin;
  if (changed) {
    ++origin_generation_;
    transport_failures_ = 0;
  }
  const bool source_changed = source_ != source;
  origin_ = std::move(origin);
  source_ = source;
  const char *prefix = source == Source::MANUAL ? "Manual" : source == Source::AUTOMATIC ? "Automatic"
                       : source == Source::FALLBACK ? "Fallback" : "Discovering";
  status_ = origin_.empty() ? "Discovering" : std::string(prefix) + " — " + origin_;
  if (changed || source_changed) ESP_LOGI(TAG, "%s", status_.c_str());
  // Diagnostic-only changes must never cancel/restart artwork downloads.
  if (changed && change_callback_) change_callback_();
}

void HomeAssistantEndpointResolver::accept_discovery(const std::vector<ServiceRecord> &records) {
  if (mode_ != Mode::AUTOMATIC || client_address_.empty()) return;
  records_ = records;
  const bool ambiguous = discover(records_, client_address_, protocol_, port_).ambiguous;
  if (healthy_ && !ambiguous) {
    next_query_ms_ = esphome::millis() + RETRY_DELAYS_MS[2];
    return;
  }
  if (ambiguous) healthy_ = false;
  begin_selection(recovery_);
}

void HomeAssistantEndpointResolver::begin_selection(bool recovery) {
  const auto discovered = discover(records_, client_address_, protocol_, port_);
  if (discovered.ambiguous) ESP_LOGW(TAG, "Ambiguous Home Assistant discovery; using configured fallback");
  if (discovered.invalid_internal_url) ESP_LOGW(TAG, "Rejected malformed Home Assistant internal URL");
  candidates_ = discovered.candidates;
  if (recovery && !origin_.empty() && !discovered.ambiguous) {
    candidates_.erase(std::remove_if(candidates_.begin(), candidates_.end(), [&](const Candidate &c) {
      return c.origin == origin_;
    }), candidates_.end());
    candidates_.insert(candidates_.begin(), {origin_, source_, "rechecking selected endpoint"});
  }
  ++selection_generation_;
  recovery_ = recovery;
  recovery_requested_ = false;
  selecting_ = true;
  probe_pending_ = false;
  candidate_index_ = 0;
  next_probe_ms_ = esphome::millis();
  health_ = "Checking connection";
}

void HomeAssistantEndpointResolver::finish_selection(uint32_t now) {
  selecting_ = probe_pending_ = false;
  recovery_requested_ = false;
  next_recovery_ms_ = now + RETRY_DELAYS_MS[std::min<unsigned>(retry_stage_, 2)];
  healthy_ = false;
  if (health_ != "Access denied") health_ = "Connection failed";
  publish(fallback(), Source::FALLBACK);
  schedule_retry(now);
}

void HomeAssistantEndpointResolver::process_probe(uint32_t now) {
  EndpointProbeResult result;
  if (probe_->take(result) && probe_pending_ && result.generation == selection_generation_) {
    probe_pending_ = false;
    const auto candidate = candidates_[candidate_index_];
    ESP_LOGI(TAG, "Endpoint check (%s): %s status=%d error=%d result=%u",
             candidate.reason, candidate.origin.c_str(), result.status, result.error,
             static_cast<unsigned>(result.outcome));
    if (result.outcome == EndpointProbeOutcome::READY) {
      selecting_ = false;
      healthy_ = true;
      transport_failures_ = 0;
      if (recovery_) {
        next_recovery_ms_ = now + RETRY_DELAYS_MS[std::min<unsigned>(retry_stage_, 2)];
        if (retry_stage_ < 2) ++retry_stage_;
      }
      health_ = "Connection checked";
      publish(candidate.origin, candidate.source);
      next_query_ms_ = now + RETRY_DELAYS_MS[2];
      return;
    }
    if (result.outcome == EndpointProbeOutcome::ACCESS_DENIED) {
      selecting_ = false;
      health_ = "Access denied";
      // HTTP exists here. Changing protocol cannot repair an access policy.
      publish(candidate.origin, candidate.source);
      schedule_retry(now);
      return;
    }
    if (result.outcome == EndpointProbeOutcome::RESOURCE) {
      health_ = "Connection failed";
      if (origin_.empty()) publish(fallback(), Source::FALLBACK);
      next_probe_ms_ = now + RETRY_DELAYS_MS[std::min<unsigned>(retry_stage_, 2)];
      if (retry_stage_ < 2) ++retry_stage_;
      return;
    }
    ++candidate_index_;
  }
  if (!selecting_ || probe_pending_ || static_cast<int32_t>(now - next_probe_ms_) < 0) return;
  if (candidate_index_ >= candidates_.size()) { finish_selection(now); return; }
  if (probe_->start(candidates_[candidate_index_].origin, selection_generation_,
                    candidates_[candidate_index_].require_local_destination)) {
    probe_pending_ = true;
    health_ = "Checking connection";
  } else {
    // A timed-out old worker or allocation pressure cannot create more tasks.
    next_probe_ms_ = now + RETRY_DELAYS_MS[std::min<unsigned>(retry_stage_, 2)];
    if (retry_stage_ < 2) ++retry_stage_;
    health_ = "Connection failed";
    if (origin_.empty()) publish(fallback(), Source::FALLBACK);
  }
}

void HomeAssistantEndpointResolver::report_download(const std::string &origin,
    uint32_t generation, int status, bool success, bool transport_failure) {
  if (mode_ != Mode::AUTOMATIC || origin != origin_ || generation != origin_generation_) return;
  if (success) {
    const bool recovered = !healthy_ || selecting_;
    healthy_ = true;
    health_ = "Images received";
    transport_failures_ = retry_stage_ = 0;
    recovery_requested_ = false;
    next_recovery_ms_ = esphome::millis();
    ++selection_generation_;
    selecting_ = probe_pending_ = false;
    if (recovered) next_query_ms_ = esphome::millis() + RETRY_DELAYS_MS[2];
  } else if (status == 401 || status == 403) {
    health_ = "Access denied";
    recovery_requested_ = false;
    transport_failures_ = 0;
    ++selection_generation_;
    selecting_ = probe_pending_ = false;
    next_query_ms_ = esphome::millis() + RETRY_DELAYS_MS[2];
  } else if (transport_failure) {
    if (transport_failures_ < 2) ++transport_failures_;
    if (transport_failures_ == 2 && !selecting_) {
      if (!recovery_requested_)
        ESP_LOGW(TAG, "Rechecking endpoint after two image transport failures (status=%d)", status);
      healthy_ = false;
      recovery_requested_ = true;
    }
  } else {
    // Consecutive *transport* failures only, not an HTTP/decoder error between.
    transport_failures_ = 0;
  }
}

void HomeAssistantEndpointResolver::loop() {
  if (!setup_complete_) return;
  const uint32_t now = esphome::millis();
  if (mode_ == Mode::AUTOMATIC && recovery_requested_ && !selecting_ &&
      static_cast<int32_t>(now - next_recovery_ms_) >= 0) {
    begin_selection(true);
    // Refresh service metadata as well: HA may have moved its HTTP listener.
    // A newer discovery result invalidates any result from this selection pass.
    if (!query_) start_query(now);
  }
  process_probe(now);
#if defined(USE_ESP32) && defined(USE_MDNS)
  if (query_ != nullptr) {
    mdns_result_t *results = nullptr;
    if (!mdns_query_async_get_results(query_, 0, &results, nullptr)) return;
    std::vector<home_assistant_endpoint::ServiceRecord> records;
    for (mdns_result_t *result = results; result != nullptr; result = result->next) {
      home_assistant_endpoint::ServiceRecord record;
      record.port = result->port;
      if (result->instance_name) record.identity = result->instance_name;
      if (result->hostname) record.identity += std::string("@") + result->hostname;
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
        if (std::strcmp(key, "uuid") == 0 && value != nullptr && value[0] != '\0')
          record.identity = value;
        if (std::strcmp(key, "internal_url") == 0 && value != nullptr)
          record.internal_url = home_assistant_endpoint::trim_copy(value);
        if (std::strcmp(key, "landingpage") == 0 && value != nullptr) {
          std::string landing_page = home_assistant_endpoint::trim_copy(value);
          std::transform(landing_page.begin(), landing_page.end(), landing_page.begin(),
                         [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
          record.landing_page = landing_page == "true" || landing_page == "1";
        }
      }
      records.push_back(std::move(record));
    }
    if (results) mdns_query_results_free(results);
    mdns_query_async_delete(query_);
    query_ = nullptr;
    if (rediscover_after_query_) {
      rediscover_after_query_ = false;
      next_query_ms_ = now;
    } else accept_discovery(records);
    return;
  }
#endif
  if (mode_ == Mode::AUTOMATIC && !client_address_.empty() && !selecting_ && !recovery_requested_ &&
      static_cast<int32_t>(now - next_query_ms_) >= 0) start_query(now);
}

void HomeAssistantEndpointResolver::start_query(uint32_t now) {
#if defined(USE_ESP32) && defined(USE_MDNS)
  query_ = mdns_query_async_new(nullptr, "_home-assistant", "_tcp", MDNS_TYPE_PTR,
                              QUERY_TIMEOUT_MS, QUERY_MAX_RESULTS, nullptr);
  if (query_) return;
#endif
  if (!healthy_) accept_discovery({});
  else next_query_ms_ = now + RETRY_DELAYS_MS[2];
}
void HomeAssistantEndpointResolver::schedule_retry(uint32_t now) {
  next_query_ms_ = now + RETRY_DELAYS_MS[std::min<unsigned>(retry_stage_, 2)];
  if (retry_stage_ < 2) ++retry_stage_;
}
void HomeAssistantEndpointResolver::cancel_query() {
#if defined(USE_ESP32) && defined(USE_MDNS)
  if (query_) {
    mdns_result_t *results = nullptr;
    if (mdns_query_async_get_results(query_, 0, &results, nullptr)) {
      if (results) mdns_query_results_free(results);
      mdns_query_async_delete(query_);
      query_ = nullptr;
      rediscover_after_query_ = false;
    } else rediscover_after_query_ = true;
  }
#endif
}
void HomeAssistantEndpointResolver::shutdown() {
  setup_complete_ = false;
  ++selection_generation_;
  ++origin_generation_;
  selecting_ = probe_pending_ = false;
  probe_->shutdown();
  cancel_query();
#if defined(USE_ESP32) && defined(USE_MDNS)
  if (query_) {
    mdns_result_t *results = nullptr;
    if (mdns_query_async_get_results(query_, QUERY_TIMEOUT_MS, &results, nullptr)) {
      if (results) mdns_query_results_free(results);
      mdns_query_async_delete(query_);
      query_ = nullptr;
    }
  }
#endif
}
}  // namespace espcontrol
