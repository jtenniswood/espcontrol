#pragma once

#include <climits>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace espcontrol::pages::alarm {
enum class Action : uint8_t { NONE, AWAY, HOME, NIGHT, VACATION, DISARM };
inline Action action(std::string_view mode) {
  return mode == "away"       ? Action::AWAY
         : mode == "home"     ? Action::HOME
         : mode == "night"    ? Action::NIGHT
         : mode == "vacation" ? Action::VACATION
         : mode == "disarm"   ? Action::DISARM
                              : Action::NONE;
}
inline const char* achieved(Action mode) {
  switch (mode) {
    case Action::AWAY:
      return "armed_away";
    case Action::HOME:
      return "armed_home";
    case Action::NIGHT:
      return "armed_night";
    case Action::VACATION:
      return "armed_vacation";
    case Action::DISARM:
      return "disarmed";
    default:
      return "";
  }
}
inline std::string_view normalized_arm_mode(std::string_view mode) {
  const auto known = action(mode);
  if (known != Action::NONE && known != Action::DISARM) return achieved(known);
  return mode == "armed_away" || mode == "armed_home" || mode == "armed_night" || mode == "armed_vacation" ||
                 mode == "armed_custom_bypass"
             ? mode
             : std::string_view{};
}
inline std::string_view effective(std::string_view state, std::string_view arm_mode) {
  if (state != "arming") return state;
  const auto normalized = normalized_arm_mode(arm_mode);
  return normalized.empty() ? state : normalized;
}
inline bool armed(std::string_view state) {
  return state.substr(0, 5) == "armed";
}
inline bool delay(std::string_view state) {
  return state == "arming" || state == "pending";
}
inline bool active(std::string_view state) {
  return armed(state) || delay(state);
}
inline bool available(std::string_view state) {
  return !state.empty() && state != "unavailable" && state != "unknown";
}
inline bool matches(Action mode, std::string_view state, std::string_view arm_mode) {
  return mode != Action::NONE && effective(state, arm_mode) == achieved(mode);
}
inline bool progressed(Action mode, std::string_view state, std::string_view arm_mode) {
  return matches(mode, state, arm_mode) || (mode != Action::DISARM && delay(state));
}
inline bool releases_label(std::string_view state) {
  return state == "disarmed" || armed(state);
}
inline const char* state_label(std::string_view state) {
  if (state.empty() || state == "unavailable") return "Unavailable";
  if (state == "disarmed") return "Disarmed";
  if (state == "armed_away") return "Armed Away";
  if (state == "armed_home") return "Armed Home";
  if (state == "armed_night") return "Armed Night";
  if (state == "armed_vacation") return "Armed Vacation";
  if (state == "armed_custom_bypass") return "Armed Custom";
  if (state == "arming") return "Arming";
  if (state == "pending") return "Pending";
  if (state == "triggered") return "Triggered";
  if (state == "unknown") return "Unknown";
  return nullptr;
}
inline const char* state_icon(std::string_view state, std::string_view mode) {
  const auto value = effective(state, mode);
  return value == "armed_home"                                     ? "Shield Home"
         : value == "armed_away" || value == "armed_custom_bypass" ? "Shield Lock"
         : value == "armed_night"                                  ? "Weather Night"
         : value == "armed_vacation"                               ? "Airplane"
         : value == "disarmed"                                     ? "Shield Off"
         : value == "triggered"                                    ? "Alarm Light"
                                                                   : "Alarm";
}
// Input is the same bounded HA short-state field as the legacy callback. Preserve
// its permissive digit scanning, but reject colon arithmetic that cannot fit an int.
inline int parse_delay(std::string_view value) {
  value = value.substr(0, 32);
  if (value.find(':') != std::string_view::npos) {
    int total = 0, part = 0;
    bool digit = false, in_part = false;
    for (char ch : value) {
      if (ch >= '0' && ch <= '9') {
        if (part > (INT_MAX - (ch - '0')) / 10) return -1;
        part = part * 10 + ch - '0';
        digit = in_part = true;
      } else if (ch == ':') {
        if (!in_part || total > (INT_MAX - part) / 60) return -1;
        total = total * 60 + part;
        part = 0;
        in_part = false;
      } else if (in_part)
        break;
    }
    return !digit || !in_part || total > (INT_MAX - part) / 60 ? -1 : total * 60 + part;
  }
  int seconds = 0;
  bool digit = false;
  for (char ch : value) {
    if (ch >= '0' && ch <= '9') {
      digit = true;
      if (seconds < 86400) seconds = seconds * 10 + ch - '0';
    } else if (digit)
      break;
  }
  return digit ? seconds : -1;
}
inline int remaining(int seconds, uint32_t started, uint32_t now) {
  if (seconds < 0) return -1;
  if (seconds == 0) return 0;
  const int elapsed = static_cast<int>((now - started) / 1000);
  return seconds > elapsed ? seconds - elapsed : 0;
}
inline int progress(int remaining, int total, int seconds) {
  if (remaining < 0) return -1;
  const int limit = total > 0 ? total : seconds;
  if (limit <= 0) return -1;
  if (remaining > limit) remaining = limit;
  return static_cast<int>((static_cast<int64_t>(remaining) * 100 + limit / 2) / limit);
}
}  // namespace espcontrol::pages::alarm
