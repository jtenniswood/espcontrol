#pragma once

#include <string_view>

namespace espcontrol::climate {

enum class Status {
  UNAVAILABLE = 0,
  OFF,
  HEATING,
  COOLING,
  DRYING,
  FAN,
  IDLE,
  MODE_FALLBACK,
};

constexpr bool action_is_working(std::string_view action) {
  return action == "heating" || action == "cooling" ||
         action == "drying" || action == "fan";
}

constexpr bool unavailable_value(std::string_view value) {
  return value.empty() || value == "unknown" || value == "unavailable";
}

constexpr Status status(bool available, std::string_view mode,
                        std::string_view action) {
  if (!available) return Status::UNAVAILABLE;
  if (mode == "off") return Status::OFF;
  if (action == "heating") return Status::HEATING;
  if (action == "cooling") return Status::COOLING;
  if (action == "drying") return Status::DRYING;
  if (action == "fan") return Status::FAN;
  if (unavailable_value(action)) return Status::MODE_FALLBACK;
  if (action == "off") return Status::OFF;
  return Status::IDLE;
}

constexpr bool active(bool available, std::string_view mode,
                      std::string_view action) {
  if (!available || mode == "off") return false;
  if (action_is_working(action)) return true;
  if (unavailable_value(action)) return !unavailable_value(mode);
  return action != "idle" && action != "off";
}

constexpr bool parent_indicator_active(bool available, std::string_view mode,
                                       std::string_view action) {
  return available && mode != "off" && action_is_working(action);
}

constexpr bool icon_enabled(bool available, std::string_view mode) {
  return available && mode != "off";
}

}  // namespace espcontrol::climate
