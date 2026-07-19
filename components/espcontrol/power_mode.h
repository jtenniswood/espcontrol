#pragma once

#include <string>

namespace espcontrol {

enum class PowerMode {
  NORMAL,
  BATTERY_SAVER,
};

enum class NetworkPowerAction {
  NONE,
  REQUEST_HIGH_PERFORMANCE,
  RELEASE_HIGH_PERFORMANCE,
};

inline PowerMode parse_power_mode(const std::string &option) {
  return option == "Battery Saver" ? PowerMode::BATTERY_SAVER : PowerMode::NORMAL;
}

inline bool power_mode_is_battery_saver(const std::string &option) {
  return parse_power_mode(option) == PowerMode::BATTERY_SAVER;
}

inline NetworkPowerAction power_mode_network_action(PowerMode mode,
                                                     bool normal_request_active) {
  if (mode == PowerMode::NORMAL && !normal_request_active) {
    return NetworkPowerAction::REQUEST_HIGH_PERFORMANCE;
  }
  if (mode == PowerMode::BATTERY_SAVER && normal_request_active) {
    return NetworkPowerAction::RELEASE_HIGH_PERFORMANCE;
  }
  return NetworkPowerAction::NONE;
}

inline bool power_mode_defer_display_work(PowerMode mode, bool display_off) {
  return mode == PowerMode::BATTERY_SAVER && display_off;
}

inline bool power_mode_flush_deferred_work(PowerMode mode, bool work_deferred) {
  return mode == PowerMode::NORMAL && work_deferred;
}

}  // namespace espcontrol
