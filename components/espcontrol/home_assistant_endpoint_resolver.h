#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "esphome/core/defines.h"
#include "home_assistant_endpoint_policy.h"

struct mdns_search_once_s;
typedef struct mdns_search_once_s mdns_search_once_t;

namespace espcontrol {

class HomeAssistantEndpointResolver {
 public:
  using ChangeCallback = std::function<void()>;

  void setup();
  void loop();
  void shutdown();

  void configure(const std::string &mode, const std::string &protocol,
                 uint16_t port, const std::string &client_address);
  void request_discovery();
  void set_change_callback(ChangeCallback callback) {
    change_callback_ = std::move(callback);
  }

  const std::string &origin() const { return origin_; }
  const std::string &status() const { return status_; }
  home_assistant_endpoint::Source source() const { return source_; }

 private:
  void recompute_fallback();
  void publish(std::string origin, home_assistant_endpoint::Source source);
  void start_query(uint32_t now);
  void schedule_retry(uint32_t now);
  void cancel_query();

  home_assistant_endpoint::Mode mode_{home_assistant_endpoint::Mode::AUTOMATIC};
  std::string protocol_{"http"};
  uint16_t port_{8123};
  std::string client_address_;
  std::string origin_;
  std::string status_{"Discovering"};
  home_assistant_endpoint::Source source_{home_assistant_endpoint::Source::DISCOVERING};
  ChangeCallback change_callback_;
  uint32_t next_query_ms_{0};
  uint8_t retry_stage_{0};
  bool setup_complete_{false};
  bool rediscover_after_query_{false};
  mdns_search_once_t *query_{nullptr};
};

}  // namespace espcontrol
