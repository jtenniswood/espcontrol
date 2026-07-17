#pragma once

#include <cstdint>
#include <string_view>

namespace espcontrol::climate {

constexpr int SUPPORT_TARGET_TEMPERATURE = 1;
constexpr int SUPPORT_TARGET_TEMPERATURE_RANGE = 2;

enum class TargetKind {
  NONE = 0,
  SINGLE = 1,
  RANGE = 2,
};

constexpr TargetKind target_kind(bool supported_features_known,
                                 int supported_features,
                                 bool has_target,
                                 bool has_low,
                                 bool has_high) {
  if (supported_features_known) {
    const bool single_supported =
      (supported_features & SUPPORT_TARGET_TEMPERATURE) != 0;
    const bool range_supported =
      (supported_features & SUPPORT_TARGET_TEMPERATURE_RANGE) != 0;
    if (single_supported && has_target) return TargetKind::SINGLE;
    if (range_supported) return TargetKind::RANGE;
    if (single_supported) return TargetKind::SINGLE;
    return TargetKind::NONE;
  }
  if (has_target) return TargetKind::SINGLE;
  if (has_low || has_high) return TargetKind::RANGE;
  return TargetKind::NONE;
}

constexpr bool target_values_complete(TargetKind kind,
                                      bool has_target,
                                      bool has_low,
                                      bool has_high) {
  if (kind == TargetKind::SINGLE) return has_target;
  if (kind == TargetKind::RANGE) return has_low && has_high;
  return false;
}

enum class CommandKind {
  NONE = 0,
  SINGLE = 1,
  RANGE = 2,
};

constexpr CommandKind command_kind(TargetKind kind, bool values_complete) {
  if (!values_complete) return CommandKind::NONE;
  if (kind == TargetKind::SINGLE) return CommandKind::SINGLE;
  if (kind == TargetKind::RANGE) return CommandKind::RANGE;
  return CommandKind::NONE;
}

constexpr bool capability_change_invalidates_pending(TargetKind previous_kind,
                                                      TargetKind next_kind,
                                                      bool next_values_complete) {
  return previous_kind != next_kind || !next_values_complete;
}

enum class TargetSelection {
  RETAIN = 0,
  LOW = 1,
  HIGH = 2,
};

constexpr TargetSelection target_selection_for_mode(std::string_view mode) {
  if (mode == "cool" || mode == "dry") return TargetSelection::HIGH;
  if (mode == "heat") return TargetSelection::LOW;
  return TargetSelection::RETAIN;
}

constexpr TargetSelection handle_selection_at_point(int point_x, int point_y,
                                                    int low_x, int low_y,
                                                    int high_x, int high_y,
                                                    int hit_radius) {
  if (hit_radius < 0) return TargetSelection::RETAIN;
  const int64_t low_dx = static_cast<int64_t>(point_x) - low_x;
  const int64_t low_dy = static_cast<int64_t>(point_y) - low_y;
  const int64_t high_dx = static_cast<int64_t>(point_x) - high_x;
  const int64_t high_dy = static_cast<int64_t>(point_y) - high_y;
  const int64_t low_distance = low_dx * low_dx + low_dy * low_dy;
  const int64_t high_distance = high_dx * high_dx + high_dy * high_dy;
  const int64_t max_distance = static_cast<int64_t>(hit_radius) * hit_radius;
  if (low_distance > max_distance && high_distance > max_distance)
    return TargetSelection::RETAIN;
  return high_distance < low_distance
    ? TargetSelection::HIGH : TargetSelection::LOW;
}

constexpr int target_from_arc_angle(int angle_degrees, int minimum, int maximum) {
  if (maximum <= minimum) return minimum;
  int angle = angle_degrees % 360;
  if (angle < 0) angle += 360;
  int progress = 0;
  if (angle <= 45) progress = angle + 225;
  else if (angle < 135) progress = angle < 90 ? 270 : 0;
  else progress = angle - 135;
  const int64_t span = static_cast<int64_t>(maximum) - minimum;
  return minimum + static_cast<int>((span * progress + 135) / 270);
}

constexpr int clamp(int value, int minimum, int maximum) {
  return value < minimum ? minimum : (value > maximum ? maximum : value);
}

constexpr int constrain_range_target(int value,
                                     bool edit_high,
                                     int low,
                                     int high,
                                     int minimum,
                                     int maximum,
                                     int step) {
  value = clamp(value, minimum, maximum);
  if (edit_high && value < low + step) value = low + step;
  if (!edit_high && value > high - step) value = high - step;
  return clamp(value, minimum, maximum);
}

}  // namespace espcontrol::climate
