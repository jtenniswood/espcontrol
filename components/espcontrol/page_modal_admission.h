#pragma once

#include <string_view>

#include "button_grid_contract_generated.h"

namespace espcontrol::pages::admission {
// Input-time classification borrows immutable configuration text. Owned strings
// and decoded panel metadata are created later by the maintenance loop.
template <size_t N>
inline bool contains(std::string_view value, const char* const (&values)[N]) {
  for (const auto* candidate : values)
    if (value == candidate) return true;
  return false;
}

inline std::string_view media_mode(std::string_view value) {
  return contains(value, CARD_CONTRACT_MEDIA_MODES) ? value : CARD_CONTRACT_MEDIA_DEFAULT_MODE;
}

inline bool option_select_action(std::string_view value) {
  return contains(value, CARD_CONTRACT_OPTION_SELECT_ACTIONS);
}

inline bool forecast_precision(std::string_view value) {
  return contains(value, CARD_CONTRACT_WEATHER_FORECAST_PRECISIONS);
}

inline bool option_token(std::string_view options, std::string_view name) {
  if (name.empty()) return false;
  while (!options.empty()) {
    const auto end = options.find(',');
    if (options.substr(0, end) == name) return true;
    if (end == std::string_view::npos) break;
    options.remove_prefix(end + 1);
  }
  return false;
}

inline std::string_view encoded_option(std::string_view options, std::string_view name) {
  if (name.empty()) return {};
  while (!options.empty()) {
    const auto end = options.find(',');
    const auto token = options.substr(0, end);
    if (token.size() > name.size() && token.substr(0, name.size()) == name && token[name.size()] == '=')
      return token.substr(name.size() + 1);
    if (end == std::string_view::npos) break;
    options.remove_prefix(end + 1);
  }
  return {};
}

inline int hex_digit(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  return -1;
}

// The saved-option decoder preserves malformed percent runs and '+' literally.
// Exactly one decoded ASCII byte can only be a literal byte or one %HH escape.
inline bool option_equals_ascii(std::string_view options, std::string_view name, char expected) {
  if (static_cast<unsigned char>(expected) >= 128) return false;
  const auto value = encoded_option(options, name);
  if (value.size() == 1) return value[0] == expected;
  if (value.size() != 3 || value[0] != '%') return false;
  const auto high = hex_digit(value[1]), low = hex_digit(value[2]);
  return high >= 0 && low >= 0 && high * 16 + low == static_cast<unsigned char>(expected);
}

inline bool option_equals(std::string_view options, std::string_view name, std::string_view expected) {
  const auto value = encoded_option(options, name);
  size_t offset = 0;
  for (const auto wanted : expected) {
    if (offset == value.size()) return false;
    auto actual = static_cast<unsigned char>(value[offset++]);
    if (actual == '%' && value.size() - offset >= 2) {
      const int high = hex_digit(value[offset]), low = hex_digit(value[offset + 1]);
      if (high >= 0 && low >= 0) {
        actual = static_cast<unsigned char>(high * 16 + low);
        offset += 2;
      }
    }
    if (actual != static_cast<unsigned char>(wanted)) return false;
  }
  return offset == value.size();
}

inline bool weather_forecast(std::string_view type, std::string_view precision, std::string_view options) {
  return type == "weather_forecast" || (type == "weather" &&
      (forecast_precision(precision) ||
       (precision == "rainfall" && option_equals(options, "rainfall_source", "forecast"))));
}
}  // namespace espcontrol::pages::admission
