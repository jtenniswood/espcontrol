#pragma once
#include <string>

namespace espcontrol::badge {
enum class CardContent { AUTO, TEXT, LABEL, VALUE, SLIDER, TRACK, DATE, DATETIME };

inline bool card_uses_slider(const std::string &type, const std::string &mode, const std::string &precision) {
  return type == "slider" || type == "light_brightness" || type == "light_temperature" || type == "fan_speed" ||
    (type == "cover" && (mode.empty() || mode == "tilt")) ||
    (type == "media" && (mode == "position" || (mode == "now_playing" && precision == "progress")));
}

// Checked against the same acceptance cases as the web compact presentation.
inline CardContent card_content(const std::string &type, const std::string &mode,
                                const std::string &precision, const std::string &display) {
  if (display == "label") return CardContent::LABEL;
  if (type == "calendar") return precision == "datetime" ? CardContent::DATETIME : CardContent::DATE;
  if (display == "value") {
    if (type == "media" && (mode == "now_playing" || mode == "cover_art")) return CardContent::TRACK;
    return card_uses_slider(type, mode, precision) ? CardContent::SLIDER : CardContent::VALUE;
  }
  if (type == "media") {
    if (mode == "now_playing" || mode == "cover_art") return CardContent::TRACK;
    return mode == "volume" ? CardContent::VALUE : CardContent::TEXT;
  }
  if (card_uses_slider(type, mode, precision)) return CardContent::SLIDER;
  if (type == "sensor" || type == "local_sensor" || type == "timer" || type == "calendar" || type == "clock" || type == "timezone" || type == "weather_forecast") return CardContent::VALUE;
  if (type == "weather") return precision == "today" || precision == "tomorrow" ? CardContent::VALUE : CardContent::TEXT;
  if (type == "climate" || type == "climate_control" || type == "light_control" || type == "light_switch" || type == "fan_control" || type == "fan_switch" || type == "fan_direction" || type == "fan_oscillate" || type == "fan_preset" ||
      type == "alarm" || type == "alarm_action" || type == "door_window" || type == "presence" || type == "garage" || type == "gate" || type == "lock" || type == "vacuum" || type == "lawn_mower" || type == "image" || type == "wifi_qr" || type == "wifi_qr_card" || type == "screen_lock" || type == "push" || type == "webhook") return CardContent::TEXT;
  return CardContent::AUTO;
}
}  // namespace espcontrol::badge
