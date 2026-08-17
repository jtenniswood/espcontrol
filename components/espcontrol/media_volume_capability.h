#pragma once

#include <cstdint>

namespace espcontrol::media {

constexpr int SUPPORT_VOLUME_SET = 4;
constexpr int SUPPORT_VOLUME_STEP = 1024;

enum class VolumeControlMode : uint8_t {
  ABSOLUTE = 0,
  STEP = 1,
  READ_ONLY = 2,
};

enum class VolumeCommandKind : uint8_t {
  NONE = 0,
  SET_ABSOLUTE = 1,
  STEP_DOWN = 2,
  STEP_UP = 3,
};

struct VolumeCommand {
  VolumeCommandKind kind = VolumeCommandKind::NONE;
  int value = 0;
};

inline VolumeControlMode volume_control_mode(bool supported_features_known,
                                             int supported_features) {
  if (!supported_features_known ||
      (supported_features & SUPPORT_VOLUME_SET) != 0) {
    return VolumeControlMode::ABSOLUTE;
  }
  if ((supported_features & SUPPORT_VOLUME_STEP) != 0) {
    return VolumeControlMode::STEP;
  }
  return VolumeControlMode::READ_ONLY;
}

inline VolumeCommand volume_command(VolumeControlMode mode,
                                    int current_value,
                                    int requested_value,
                                    int maximum_value,
                                    bool current_value_known = true) {
  if (maximum_value < 1) maximum_value = 1;
  if (maximum_value > 100) maximum_value = 100;
  if (current_value < 0) current_value = 0;
  if (current_value > 100) current_value = 100;

  if (mode == VolumeControlMode::ABSOLUTE) {
    if (requested_value < 0) requested_value = 0;
    if (requested_value > maximum_value) requested_value = maximum_value;
    if (requested_value == current_value) return {};
    return {VolumeCommandKind::SET_ABSOLUTE, requested_value};
  }

  if (mode == VolumeControlMode::STEP) {
    if (requested_value > current_value &&
        (!current_value_known || current_value < maximum_value)) {
      return {VolumeCommandKind::STEP_UP, current_value};
    }
    if (requested_value < current_value &&
        (!current_value_known || current_value > 0)) {
      return {VolumeCommandKind::STEP_DOWN, current_value};
    }
  }

  return {};
}

inline bool volume_arc_interactive(VolumeControlMode mode) {
  return mode == VolumeControlMode::ABSOLUTE;
}

inline int volume_display_value(VolumeControlMode mode,
                                int reported_value,
                                int maximum_value) {
  if (reported_value < 0) reported_value = 0;
  if (reported_value > 100) reported_value = 100;
  if (mode != VolumeControlMode::ABSOLUTE) return reported_value;

  if (maximum_value < 1) maximum_value = 1;
  if (maximum_value > 100) maximum_value = 100;
  return reported_value > maximum_value ? maximum_value : reported_value;
}

inline bool volume_decrease_enabled(VolumeControlMode mode,
                                    int current_value,
                                    bool current_value_known = true) {
  if (mode == VolumeControlMode::READ_ONLY) return false;
  return mode == VolumeControlMode::STEP && !current_value_known
    ? true : current_value > 0;
}

inline bool volume_increase_enabled(VolumeControlMode mode,
                                    int current_value,
                                    int maximum_value) {
  return mode != VolumeControlMode::READ_ONLY && current_value < maximum_value;
}

}  // namespace espcontrol::media
