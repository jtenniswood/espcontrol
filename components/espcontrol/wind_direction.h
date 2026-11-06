#pragma once

#include <cmath>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

namespace espcontrol::wind {

inline bool number(std::string_view text, double &value) {
  char buffer[128];
  if (text.empty() || text.size() >= sizeof(buffer)) return false;
  std::memcpy(buffer, text.data(), text.size());
  buffer[text.size()] = '\0';
  char *end = nullptr;
  value = std::strtod(buffer, &end);
  if (end == buffer || !std::isfinite(value)) return false;
  while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
  return end == buffer + text.size();
}

// Home Assistant accepts degrees clockwise from north, or 16-point abbreviations.
// The card groups these into the nearest eight compass points.
inline int direction(std::string_view text) {
  double bearing;
  if (number(text, bearing)) {
    if (bearing < 0 || bearing > 360) return -1;
    return static_cast<int>(std::floor((bearing + 22.5) / 45.0)) % 8;
  }
  char normalized[16];
  size_t size = 0;
  for (char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch)) && ch != '-' && ch != '_') {
      if (size == sizeof(normalized)) return -1;
      normalized[size++] = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
  }
  const std::string_view compass(normalized, size);
  static const char *names[] = {"NORTH", "NORTHEAST", "EAST", "SOUTHEAST",
                                 "SOUTH", "SOUTHWEST", "WEST", "NORTHWEST"};
  for (int i = 0; i < 8; ++i) if (compass == names[i]) return i;
  static const char *points[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                  "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  for (int i = 0; i < 16; ++i) if (compass == points[i]) return ((i + 1) / 2) % 8;
  return -1;
}

inline const char *label(int direction) {
  static const char *labels[] = {"North", "North East", "East", "South East",
                                 "South", "South West", "West", "North West"};
  return direction >= 0 && direction < 8 ? labels[direction] : "--";
}

inline const char *icon(int direction) {
  static const char *icons[] = {"Arrow Up", "Wind arrow-top-right", "Wind arrow-right",
    "Wind arrow-bottom-right", "Arrow Down", "Wind arrow-bottom-left",
    "Wind arrow-left", "Wind arrow-top-left"};
  return direction >= 0 && direction < 8 ? icons[direction] : "Weather Windy";
}

inline void speed(std::string_view text, char *output, size_t capacity) {
  if (!output || !capacity) return;
  double value;
  if (!number(text, value) || value < 0 || value > 10000) {
    std::snprintf(output, capacity, "--");
    return;
  }
  std::snprintf(output, capacity, "%.1f", value);
  const size_t size = std::strlen(output);
  if (size >= 2 && output[size - 2] == '.' && output[size - 1] == '0') output[size - 2] = '\0';
}
inline std::string speed(std::string_view text) {
  char buffer[24];
  speed(text, buffer, sizeof(buffer));
  return buffer;
}

}  // namespace espcontrol::wind
