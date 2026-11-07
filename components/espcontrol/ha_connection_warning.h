#pragma once
#include <cstdint>

namespace espcontrol {
// A subscribed HA client is required; a diagnostic API connection is not enough.
class HaConnectionWarning {
 public:
  bool update(uint32_t now, bool network_connected, bool configured,
              bool ha_subscribed) {
    if (!network_connected || !configured || ha_subscribed) {
      waiting_ = false;
      return false;
    }
    if (!waiting_) {
      waiting_ = true;
      disconnected_at_ = now;
    }
    return static_cast<uint32_t>(now - disconnected_at_) >= 60000;
  }
 private:
  bool waiting_{false};
  uint32_t disconnected_at_{0};
};
}  // namespace espcontrol
