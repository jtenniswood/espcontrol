#pragma once

#include <string>

// Shared pure discovery policy for card binding and staged dependency planning.
constexpr const char *DEFAULT_MEDIA_SPEAKER_DISCOVERY_ENTITY = "sensor.speaker_group";

inline std::string media_group_trim(std::string value) {
  size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  size_t last = value.find_last_not_of(" \t\r\n");
  value = value.substr(first, last - first + 1);
  if (value.size() >= 2 &&
      ((value.front() == '\'' && value.back() == '\'') ||
       (value.front() == '"' && value.back() == '"'))) {
    value = value.substr(1, value.size() - 2);
  }
  return value;
}

inline std::string media_group_discovery_entity(const std::string &configured) {
  std::string entity_id = media_group_trim(configured);
  return entity_id.empty() ? DEFAULT_MEDIA_SPEAKER_DISCOVERY_ENTITY : entity_id;
}

inline const char *media_group_discovery_attribute(const std::string &entity_id) {
  return entity_id.rfind("sensor.", 0) == 0 ? "data" : "entity_id";
}
