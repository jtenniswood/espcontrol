#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace espcontrol::camera {

enum class RefreshMode { OFF, PERIODIC, ACTIVITY };

inline RefreshMode refresh_mode(const std::string &value) {
  if (value == "periodic") return RefreshMode::PERIODIC;
  if (value == "activity") return RefreshMode::ACTIVITY;
  return RefreshMode::OFF;
}

inline bool valid_trigger(const std::string &value) {
  const size_t prefix = value.rfind("binary_sensor.", 0) == 0 ? 14 :
                        value.rfind("event.", 0) == 0 ? 6 : 0;
  if (!prefix || value.size() <= prefix) return false;
  for (size_t i = prefix; i < value.size(); ++i) {
    const char c = value[i];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
  }
  return true;
}

inline uint32_t refresh_interval_ms(const std::string &value) {
  return value == "5" ? 5000 : value == "30" ? 30000 : 10000;
}

inline bool valid_state(const std::string &value) {
  return !value.empty() && value != "unknown" && value != "unavailable" && value != "None";
}

// Connection baselines are not activity. A sustained on state is one activation.
struct ActivityTrigger {
  std::string previous;
  uint32_t connection = 0;
  bool baselined = false;

  bool observe(const std::string &value, bool binary, uint32_t epoch) {
    if (connection != epoch) {
      connection = epoch;
      baselined = false;
    }
    if (!valid_state(value) || (binary && value != "on" && value != "off")) {
      baselined = false;
      previous.clear();
      return false;
    }
    const bool activated = baselined && (binary ? previous == "off" && value == "on"
                                             : previous != value);
    previous = value;
    baselined = true;
    return activated;
  }
  bool on(uint32_t epoch) const { return baselined && connection == epoch && previous == "on"; }
};

// All deadlines use wrap-safe millis arithmetic; only the expanded view owns a schedule.
struct RefreshSchedule {
  RefreshMode mode = RefreshMode::OFF;
  uint32_t interval_ms = 10000;
  uint32_t window_end = 0;
  uint32_t next_due = 0;
  uint32_t last_completed = 0;
  uint8_t failures = 0;
  bool open = false;
  bool window = false;
  bool in_flight = false;
  bool completed = false;

  uint32_t interval() const { return mode == RefreshMode::ACTIVITY ? 5000 : interval_ms; }
  void close() {
    open = window = in_flight = completed = false;
    failures = 0;
  }
  void begin(uint32_t now, bool sensor_on) {
    close();
    open = true;
    next_due = now;
    if (sensor_on) activate(now);
  }
  void activate(uint32_t now) {
    if (!open || mode != RefreshMode::ACTIVITY) return;
    const bool running = window && static_cast<int32_t>(window_end - now) > 0;
    window = true;
    window_end = now + 30000;
    if (!running && failures == 0) {
      next_due = completed && static_cast<uint32_t>(now - last_completed) < 5000
          ? last_completed + 5000 : now;
    }
  }
  bool enabled(uint32_t now) const {
    return open && (mode == RefreshMode::PERIODIC ||
        (mode == RefreshMode::ACTIVITY && window && static_cast<int32_t>(window_end - now) > 0));
  }
  bool due(uint32_t now, bool connected) const {
    return connected && !in_flight && enabled(now) && static_cast<int32_t>(now - next_due) >= 0;
  }
  void started() { in_flight = true; }
  void finished(uint32_t now, bool success) {
    in_flight = false;
    completed = true;
    last_completed = now;
    failures = success ? 0 : std::min<unsigned>(failures + 1, 4);
    const uint32_t backoff = failures ? std::min<uint32_t>(5000U << (failures - 1), 30000) : 0;
    next_due = now + std::max(interval(), backoff);
  }
};

// A URL/token is not an image revision. Keep separate acknowledgements for tile and modal.
struct ImageRevision {
  std::string timestamp;
  uint32_t latest = 0;
  uint32_t tile_requested = 0;
  uint32_t tile_applied = 0;
  uint32_t modal_requested = 0;
  uint32_t modal_applied = 0;
  bool observe(const std::string &value) {
    if (!valid_state(value) || value == timestamp) return false;
    timestamp = value;
    ++latest;
    return true;
  }
  bool tile_dirty() const { return latest != tile_applied; }
  bool modal_dirty() const { return latest != modal_applied; }
};

}  // namespace espcontrol::camera
