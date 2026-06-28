#ifndef ESPCONTROL_TEMPERATURE_UNIT_H
#define ESPCONTROL_TEMPERATURE_UNIT_H

#pragma once

#include <string>
#include <cctype>
#include "sun_calc.h"

inline std::string temperature_unit_trim(std::string value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
  return value;
}

inline std::string temperature_unit_lower(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (char ch : value) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  return out;
}

inline std::string normalize_temperature_unit_option(const std::string &option) {
  std::string lower = temperature_unit_lower(temperature_unit_trim(option));
  if (lower == "f" || lower == "\u00B0f" || lower == "fahrenheit") return "\u00B0F";
  if (lower == "c" || lower == "\u00B0c" || lower == "celsius" || lower == "centigrade") return "\u00B0C";
  return "Auto";
}

inline std::string temperature_timezone_id_from_option(const std::string &option) {
  size_t pos = option.find(" (");
  return pos == std::string::npos ? option : option.substr(0, pos);
}

inline bool timezone_prefers_fahrenheit(const std::string &timezone_option) {
  std::string tz = temperature_timezone_id_from_option(timezone_option);
  static const char *fahrenheit_zones[] = {
    "America/Adak",
    "America/Anchorage",
    "America/Boise",
    "America/Chicago",
    "America/Denver",
    "America/Detroit",
    "America/Juneau",
    "America/Los_Angeles",
    "America/New_York",
    "America/Phoenix",
    "America/Puerto_Rico",
    "Pacific/Guam",
    "Pacific/Honolulu",
    "Pacific/Pago_Pago",
  };
  for (const char *zone : fahrenheit_zones) {
    if (tz == zone) return true;
  }
  return false;
}

inline std::string &display_temperature_unit_option() {
  static std::string option = "Auto";
  return option;
}

inline std::string &display_temperature_timezone_option() {
  static std::string timezone = "UTC (GMT+0)";
  return timezone;
}

inline bool &display_temperature_degree_symbol_enabled() {
  static bool enabled = true;
  return enabled;
}

inline void set_display_temperature_unit(const std::string &unit_option,
                                         const std::string &timezone_option) {
  display_temperature_unit_option() = normalize_temperature_unit_option(unit_option);
  std::string effective = effective_timezone_option(timezone_option);
  display_temperature_timezone_option() = effective.empty() ? std::string("UTC (GMT+0)") : effective;
}

inline void set_display_temperature_degree_symbol(bool enabled) {
  display_temperature_degree_symbol_enabled() = enabled;
}

inline bool display_temperature_uses_fahrenheit() {
  std::string option = normalize_temperature_unit_option(display_temperature_unit_option());
  if (option == "\u00B0F") return true;
  if (option == "\u00B0C") return false;
  return timezone_prefers_fahrenheit(display_temperature_timezone_option());
}

inline const char *display_temperature_unit_symbol() {
  return display_temperature_uses_fahrenheit() ? "\u00B0F" : "\u00B0C";
}

inline float convert_temperature_value_for_display_float(float value,
                                                         const std::string &source_unit) {
  std::string source = normalize_temperature_unit_option(source_unit);
  if (source != "\u00B0F" && source != "\u00B0C") return value;
  bool target_fahrenheit = display_temperature_uses_fahrenheit();
  if ((source == "\u00B0F") == target_fahrenheit) return value;
  return target_fahrenheit
    ? (value * 9.0f / 5.0f) + 32.0f
    : (static_cast<float>(value) - 32.0f) * 5.0f / 9.0f;
}

inline int convert_temperature_value_for_display(int value,
                                                 const std::string &source_unit) {
  float converted = convert_temperature_value_for_display_float(
    static_cast<float>(value), source_unit);
  return static_cast<int>(converted >= 0.0f ? converted + 0.5f : converted - 0.5f);
}

inline const char *display_clock_bar_temperature_suffix() {
  if (display_temperature_degree_symbol_enabled()) return "\u00B0";
  return "";
}

#endif  // ESPCONTROL_TEMPERATURE_UNIT_H
