#pragma once

#include <cstdint>
#include <string>

// Only a real HA switch may control the guest network. Never target the panel Wi-Fi.
inline bool guest_wifi_valid_entity(const std::string &entity) {
  if (entity.size() <= 7 || entity.compare(0, 7, "switch.") != 0) return false;
  for (size_t i = 7; i < entity.size(); ++i) {
    const char c = entity[i];
    if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '_') return false;
  }
  return true;
}

struct GuestWifiState {
  bool known = false;
  bool on = false;
  bool pending = false;
  bool target_on = false;
  bool failed = false;
  uint32_t requested_at = 0;

  void receive(const std::string &state) {
    known = state == "on" || state == "off";
    if (!known) { pending = false; failed = false; return; }
    const bool next_on = state == "on";
    if (next_on != on) failed = false;
    on = next_on;
    if (pending && on == target_on) { pending = false; failed = false; }
  }
  bool begin(uint32_t now) {
    if (!known || pending) return false;
    target_on = !on;
    pending = true;
    failed = false;
    requested_at = now;
    return true;
  }
  void send_failed() { pending = false; failed = true; }
  void tick(uint32_t now) {
    if (pending && static_cast<uint32_t>(now - requested_at) >= 10000) send_failed();
  }
  void disconnect() { known = false; pending = false; failed = false; }
  const char *status_key() const {
    if (!known) return "unavailable";
    if (pending) return "loading";
    if (failed) return "wifi_did_not_change";
    return on ? "on" : "off";
  }
};
