#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "esphome/core/defines.h"
#include "home_assistant_endpoint_policy.h"
#include "home_assistant_endpoint_probe.h"

struct mdns_search_once_s;
typedef struct mdns_search_once_s mdns_search_once_t;

namespace espcontrol {
class HomeAssistantEndpointResolver {
 public:
  using ChangeCallback = std::function<void()>;
  explicit HomeAssistantEndpointResolver(EndpointProbe *probe = nullptr)
      : probe_(probe ? probe : &probe_service_) {}
  void setup();
  void loop();
  void shutdown();
  void configure(const std::string &mode, const std::string &protocol,
                 uint16_t port, const std::string &client_address,
                 const std::string &manual_host = "");
  void request_discovery();
  void invalidate_connection();
  void set_change_callback(ChangeCallback callback) { change_callback_ = std::move(callback); }
  const std::string &origin() const { return origin_; }
  const std::string &status() const { return status_; }
  const std::string &health() const { return health_; }
  uint32_t generation() const { return origin_generation_; }
  home_assistant_endpoint::Source source() const { return source_; }

  // Called on the main loop. Exposed separately from mDNS for host-side tests.
  void accept_discovery(const std::vector<home_assistant_endpoint::ServiceRecord> &records);
  void report_download(const std::string &origin, uint32_t generation,
                       int status, bool success, bool transport_failure);
 private:
  void publish(std::string origin, home_assistant_endpoint::Source source);
  void start_query(uint32_t now);
  void schedule_retry(uint32_t now);
  void cancel_query();
  void begin_selection(bool recovery);
  void process_probe(uint32_t now);
  void finish_selection(uint32_t now);
  std::string fallback() const;

  home_assistant_endpoint::Mode mode_{home_assistant_endpoint::Mode::AUTOMATIC};
  std::string protocol_{"http"};
  uint16_t port_{8123};
  std::string client_address_, manual_host_, origin_;
  std::string status_{"Discovering"}, health_{"Checking connection"};
  home_assistant_endpoint::Source source_{home_assistant_endpoint::Source::DISCOVERING};
  ChangeCallback change_callback_;
  uint32_t next_query_ms_{0}, next_probe_ms_{0}, next_recovery_ms_{0};
  uint32_t selection_generation_{1}, origin_generation_{1};
  uint8_t retry_stage_{0}, transport_failures_{0};
  bool setup_complete_{false}, rediscover_after_query_{false};
  bool selecting_{false}, probe_pending_{false}, healthy_{false};
  bool recovery_{false}, recovery_requested_{false};
  size_t candidate_index_{0};
  std::vector<home_assistant_endpoint::ServiceRecord> records_;
  std::vector<home_assistant_endpoint::Candidate> candidates_;
  mdns_search_once_t *query_{nullptr};
  EndpointProbeService probe_service_;
  EndpointProbe *probe_;
};
}  // namespace espcontrol
