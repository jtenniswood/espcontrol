#pragma once

#include <cstdint>

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
  SCREENSAVER,
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

inline Registration registration(Family family, bool known,
                                 bool allow_in_subpage) {
  return {
    REGISTRY_VERSION,
    family,
    known,
    allow_in_subpage,
  };
}

}  // namespace espcontrol::cards
