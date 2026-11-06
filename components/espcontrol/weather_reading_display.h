#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>
#include <string_view>
#include <cstring>

namespace espcontrol::weather_reading {
inline double value(std::string_view text) {
  char buffer[128];
  if (text.empty() || text.size() >= sizeof(buffer)) return NAN;
  std::memcpy(buffer, text.data(), text.size());
  buffer[text.size()] = '\0';
  char *end = nullptr;
  double result = std::strtod(buffer, &end);
  if (end == buffer) return NAN;
  while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
  return end == buffer + text.size() ? result : NAN;
}
inline void format(double value, char *output, size_t capacity) {
  if (!output || !capacity) return;
  if (!std::isfinite(value) || value < 0 || value > 1e12) {
    std::snprintf(output, capacity, "--");
    return;
  }
  std::snprintf(output, capacity, "%.3f", value);
  size_t size = std::strlen(output);
  while (size && output[size - 1] == '0') output[--size] = '\0';
  if (size && output[size - 1] == '.') output[--size] = '\0';
  if (std::strcmp(output, "-0") == 0) std::snprintf(output, capacity, "0");
}
inline std::string amount(double value, const std::string &unit) {
  char buffer[40];
  format(value, buffer, sizeof(buffer));
  if (std::strcmp(buffer,"--") == 0) return buffer;
  std::string number(buffer);
  const bool has_unit = !unit.empty() && unit != "unknown" && unit != "unavailable";
  return number + (has_unit ? " " + unit : "");
}
}  // namespace espcontrol::weather_reading
