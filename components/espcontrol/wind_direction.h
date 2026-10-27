#pragma once

#include <cmath>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <string>

namespace espcontrol::wind {

inline bool number(const std::string &text, double &value) {
  char *end = nullptr;
  value = std::strtod(text.c_str(), &end);
  if (end == text.c_str() || !std::isfinite(value)) return false;
  while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
  return *end == '\0';
}

// Home Assistant accepts degrees clockwise from north, or 16-point abbreviations.
// The card groups these into the nearest eight compass points.
inline int direction(const std::string &text) {
  double bearing;
  if (number(text, bearing)) {
    if (bearing < 0 || bearing > 360) return -1;
    return static_cast<int>(std::floor((bearing + 22.5) / 45.0)) % 8;
  }
  std::string compass;
  for (char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch)))
      compass += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
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

inline std::string speed(const std::string &text) {
  double value;
  if (!number(text, value) || value < 0 || value > 10000) return "--";
  char buffer[24];
  std::snprintf(buffer, sizeof(buffer), "%.1f", value);
  std::string formatted(buffer);
  if (formatted.size() >= 2 && formatted.substr(formatted.size() - 2) == ".0")
    formatted.resize(formatted.size() - 2);
  return formatted;
}

}  // namespace espcontrol::wind
