#pragma once

#include "number_slider_policy.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

namespace espcontrol::badge {

inline std::string card_options(const std::string &options) {
  std::string result;
  size_t start = 0;
  while (start < options.size()) {
    auto end = options.find(',', start);
    if (end == std::string::npos) end = options.size();
    const auto token = options.substr(start, end - start);
    if (!token.empty() && token.compare(0, 6, "badge_") != 0) {
      if (!result.empty()) result += ',';
      result += token;
    }
    start = end + 1;
  }
  return result;
}

inline std::string slider_value_text(double value) {
  if (!std::isfinite(value)) return "--";
  value = std::round(value);
  if (value == 0.0) value = 0.0;  // Avoid displaying negative zero.
  char buffer[std::numeric_limits<double>::max_exponent10 + 4];
  std::snprintf(buffer, sizeof(buffer), "%.0f", value);
  return buffer;
}

inline int icon_diameter(int line_height) {
  return line_height * 8 / 5;
}

inline int pill_height(int line_height) {
  return line_height * 23 / 10;
}

// Keep the card family's curve, scaled to a compact control. The quarter-height
// cap leaves room for the renderer's continuous-corner transition on both ends.
inline int corner_radius(int card_radius, int height) {
  return std::max(1, std::min(card_radius, height / 4));
}

inline int content_left_padding(int line_height) {
  return line_height / 4;
}

inline int content_gap(int line_height) {
  return line_height / 8;
}

inline int content_right_padding(int line_height) {
  return line_height * 9 / 10;
}

inline int width_for_text(int line_height, int text_width, bool slider) {
  // Slider pills follow their displayed content instead of reserving a wide track.
  const long long minimum = slider ? 0 : static_cast<long long>(line_height) * 9 / 2;
  const long long natural = static_cast<long long>(text_width) +
    content_left_padding(line_height) + content_right_padding(line_height) +
    content_gap(line_height) + icon_diameter(line_height);
  return static_cast<int>(std::min<long long>(INT_MAX, std::max(minimum, natural)));
}

inline std::string display_text(const std::string &title, const std::string &reading, bool label_only) {
  std::string text = label_only ? (title.empty() ? reading : title) : (reading.empty() ? title : reading);
  std::replace(text.begin(), text.end(), '\r', ' ');
  std::replace(text.begin(), text.end(), '\n', ' ');
  return text;
}

inline int progress_percent(int value, int minimum, int maximum) {
  if (maximum <= minimum) return 0;
  return static_cast<int>(std::clamp((static_cast<long long>(value) - minimum) * 100 / (static_cast<long long>(maximum) - minimum), 0LL, 100LL));
}

inline bool toggle_checked(bool available, bool is_on) {
  return available && is_on;
}

inline std::string action(const std::string &value) {
  return value == "subpage" || value == "modal" || value == "toggle" || value == "button" ||
         value == "slider" || value == "card" ? value : "display";
}

inline bool subpage_unsupported(const std::string &action, const std::string &card_type) {
  return action == "subpage" || card_type == "subpage";
}

inline bool button_supported(const std::string &entity) {
  return number_slider::has_domain(entity, "button") || number_slider::has_domain(entity, "input_button");
}

inline const char *button_action(const std::string &entity) {
  return number_slider::has_domain(entity, "input_button") ? "input_button.press" : "button.press";
}

inline bool toggle_supported(const std::string &entity) {
  return number_slider::has_domain(entity, "light") ||
         number_slider::has_domain(entity, "switch") ||
         number_slider::has_domain(entity, "input_boolean") ||
         number_slider::has_domain(entity, "fan");
}

inline bool slider_supported(const std::string &entity) {
  return number_slider::is_numeric_entity(entity) ||
         number_slider::has_domain(entity, "light") ||
         number_slider::has_domain(entity, "fan") ||
         number_slider::has_domain(entity, "media_player");
}

inline const char *level_attribute(const std::string &entity) {
  if (number_slider::has_domain(entity, "light")) return "brightness";
  if (number_slider::has_domain(entity, "fan")) return "percentage";
  if (number_slider::has_domain(entity, "media_player")) return "volume_level";
  return "";
}

inline double level_percent(const std::string &entity, double value) {
  if (number_slider::has_domain(entity, "light")) value = value * 100.0 / 255.0;
  else if (number_slider::has_domain(entity, "media_player")) value *= 100.0;
  return std::max(0.0, std::min(100.0, value));
}

}  // namespace espcontrol::badge
