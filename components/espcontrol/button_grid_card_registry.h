#pragma once

#include <cstdint>
#include <string>

namespace espcontrol::cards {

constexpr uint8_t REGISTRY_VERSION = 1;

enum class Family : uint8_t {
  TOGGLE,
  ACTION,
  ALARM,
  ALARM_ACTION,
  DATE_TIME,
  CLIMATE,
  COVER,
  OCCUPANCY,
  FAN,
  ACCESS,
  IMAGE,
  INTERNAL,
  MOWER,
  LIGHT_CONTROL,
  LIGHT_TEMPERATURE,
  LOCAL_SENSOR,
  MEDIA,
  OPTION_SELECT,
  PUSH,
  SCREEN_LOCK,
  SENSOR,
  SLIDER,
  SUBPAGE,
  VACUUM,
  WEATHER,
  WEBHOOK,
  TODO,
  UNKNOWN,
};

struct Registration {
  uint8_t version = REGISTRY_VERSION;
  Family family = Family::UNKNOWN;
  bool known = false;
  bool allow_in_subpage = false;
};

inline Family family_for_type(const std::string &type) {
  if (type.empty() || type == "light_switch") return Family::TOGGLE;
  if (type == "action" || type == "local") return Family::ACTION;
  if (type == "alarm") return Family::ALARM;
  if (type == "alarm_action") return Family::ALARM_ACTION;
  if (type == "calendar" || type == "clock" || type == "timezone") return Family::DATE_TIME;
  if (type == "climate" || type == "climate_control") return Family::CLIMATE;
  if (type == "cover") return Family::COVER;
  if (type == "door_window" || type == "presence") return Family::OCCUPANCY;
  if (type == "fan_control" || type == "fan_direction" ||
      type == "fan_oscillate" || type == "fan_preset" ||
      type == "fan_speed" || type == "fan_switch") return Family::FAN;
  if (type == "garage" || type == "gate" || type == "lock") return Family::ACCESS;
  if (type == "image") return Family::IMAGE;
  if (type == "internal") return Family::INTERNAL;
  if (type == "lawn_mower") return Family::MOWER;
  if (type == "light_control") return Family::LIGHT_CONTROL;
  if (type == "light_temperature") return Family::LIGHT_TEMPERATURE;
  if (type == "local_sensor") return Family::LOCAL_SENSOR;
  if (type == "media") return Family::MEDIA;
  if (type == "option_select") return Family::OPTION_SELECT;
  if (type == "push") return Family::PUSH;
  if (type == "screen_lock") return Family::SCREEN_LOCK;
  if (type == "sensor" || type == "text_sensor") return Family::SENSOR;
  if (type == "slider" || type == "light_brightness") return Family::SLIDER;
  if (type == "subpage") return Family::SUBPAGE;
  if (type == "vacuum") return Family::VACUUM;
  if (type == "weather" || type == "weather_forecast") return Family::WEATHER;
  if (type == "webhook") return Family::WEBHOOK;
  if (type == "todo") return Family::TODO;
  return Family::UNKNOWN;
}

inline Registration registration_for_type(const std::string &type,
                                           bool allow_in_subpage) {
  const Family family = family_for_type(type);
  return {
    REGISTRY_VERSION,
    family,
    family != Family::UNKNOWN,
    allow_in_subpage,
  };
}

inline bool is_family(const std::string &type, Family family) {
  return family_for_type(type) == family;
}

// Visual behavior can cut across entity families. Fan speed remains a fan for
// binding and commands, but it uses the same slider presentation as lights.
inline bool uses_slider_visual(const std::string &type) {
  const Family family = family_for_type(type);
  return family == Family::SLIDER || family == Family::COVER ||
         type == "fan_speed";
}

}  // namespace espcontrol::cards
