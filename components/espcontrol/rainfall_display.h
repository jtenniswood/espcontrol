#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>

namespace espcontrol::rainfall {
inline double value(const std::string &text) {
  char *end = nullptr;
  double result = std::strtod(text.c_str(), &end);
  if (end == text.c_str()) return NAN;
  while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
  return *end == '\0' ? result : NAN;
}
inline std::string amount(double value, const std::string &unit) {
  if (!std::isfinite(value) || value < 0 || value > 1e12) return "--";
  char buffer[40];
  std::snprintf(buffer, sizeof(buffer), "%.3f", value);
  std::string number(buffer);
  while (!number.empty() && number.back() == '0') number.pop_back();
  if (!number.empty() && number.back() == '.') number.pop_back();
  if (number == "-0") number = "0";
  const bool has_unit = !unit.empty() && unit != "unknown" && unit != "unavailable";
  return number + (has_unit ? " " + unit : "");
}
}  // namespace espcontrol::rainfall
