#pragma once

#include <cstdint>
#include <string>

namespace espcontrol::media {

constexpr int SUPPORT_TURN_ON = 128;
constexpr int SUPPORT_TURN_OFF = 256;

enum class PowerCommand : uint8_t {
  NONE = 0,
  TURN_ON = 1,
  TURN_OFF = 2,
};

inline bool power_toggle_supported(bool supported_features_known,
                                   int supported_features) {
  if (!supported_features_known) return false;
  const int required = SUPPORT_TURN_ON | SUPPORT_TURN_OFF;
  return (supported_features & required) == required;
}

constexpr int media_control_tab_count(bool progress_supported,
                                      bool power_supported) {
  return 2 + (progress_supported ? 1 : 0) + (power_supported ? 1 : 0);
}

inline PowerCommand power_command(bool supported_features_known,
                                  int supported_features,
                                  bool state_known,
                                  bool available,
                                  const std::string &state) {
  if (!power_toggle_supported(supported_features_known, supported_features) ||
      !state_known || !available || state.empty() || state == "unknown" ||
      state == "unavailable") {
    return PowerCommand::NONE;
  }
  return state == "off" ? PowerCommand::TURN_ON : PowerCommand::TURN_OFF;
}

}  // namespace espcontrol::media
