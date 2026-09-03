#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

namespace espcontrol::number_slider {

constexpr int MAX_SLIDER_POSITIONS = 10000;
constexpr int MAX_DECIMAL_PLACES = std::numeric_limits<double>::max_digits10;
constexpr int GENERAL_FORMAT_DECIMAL_PLACES = MAX_DECIMAL_PLACES + 1;
constexpr double MAX_STEP_COUNT_TOLERANCE = 1e-3;

struct Metadata {
  double minimum = 0.0;
  double maximum = 0.0;
  double step = 0.0;
};

inline bool has_domain(const std::string &entity_id, const char *domain) {
  const std::string prefix = std::string(domain) + ".";
  return entity_id.size() > prefix.size() &&
         entity_id.compare(0, prefix.size(), prefix) == 0;
}

inline bool is_number_entity(const std::string &entity_id) {
  return has_domain(entity_id, "number");
}

inline bool is_input_number_entity(const std::string &entity_id) {
  return has_domain(entity_id, "input_number");
}

inline bool is_numeric_entity(const std::string &entity_id) {
  return is_number_entity(entity_id) || is_input_number_entity(entity_id);
}

inline const char *service_for_entity(const std::string &entity_id) {
  if (is_number_entity(entity_id)) return "number.set_value";
  if (is_input_number_entity(entity_id)) return "input_number.set_value";
  return nullptr;
}

inline bool metadata_valid(const Metadata &metadata) {
  return std::isfinite(metadata.minimum) && std::isfinite(metadata.maximum) &&
         std::isfinite(metadata.step) && metadata.maximum > metadata.minimum &&
         metadata.step > 0.0;
}

inline long long legal_step_count(const Metadata &metadata) {
  if (!metadata_valid(metadata)) return 0;
  const double raw = (metadata.maximum - metadata.minimum) / metadata.step;
  if (!std::isfinite(raw) || raw <= 0.0) return 0;
  const double nearest = std::round(raw);
  const bool float_precision =
    static_cast<double>(static_cast<float>(metadata.minimum)) == metadata.minimum &&
    static_cast<double>(static_cast<float>(metadata.maximum)) == metadata.maximum &&
    static_cast<double>(static_cast<float>(metadata.step)) == metadata.step;
  const double source_epsilon = float_precision
    ? static_cast<double>(std::numeric_limits<float>::epsilon())
    : std::numeric_limits<double>::epsilon();
  const double range = metadata.maximum - metadata.minimum;
  const double cancellation_scale = std::max(
    1.0,
    std::max(std::fabs(metadata.minimum), std::fabs(metadata.maximum)) /
      std::fabs(range));
  const double integer_tolerance = std::min(
    MAX_STEP_COUNT_TOLERANCE,
    source_epsilon * std::max(1.0, std::fabs(raw)) *
      2.0 * (1.0 + cancellation_scale));
  const double rounded = std::fabs(raw - nearest) <= integer_tolerance
    ? nearest
    : std::ceil(raw);
  if (rounded >= static_cast<double>(std::numeric_limits<long long>::max()))
    return std::numeric_limits<long long>::max();
  return std::max(1LL, static_cast<long long>(rounded));
}

inline int slider_position_count(const Metadata &metadata) {
  return static_cast<int>(
    std::min(legal_step_count(metadata), static_cast<long long>(MAX_SLIDER_POSITIONS)));
}

inline int clamp_position(const Metadata &metadata, int position) {
  return std::max(0, std::min(slider_position_count(metadata), position));
}

inline long long nearest_step_index(const Metadata &metadata, double value) {
  if (!metadata_valid(metadata) || !std::isfinite(value)) return 0;
  const long long count = legal_step_count(metadata);
  if (value >= metadata.maximum) return count;
  if (value <= metadata.minimum) return 0;
  const double raw_index = (value - metadata.minimum) / metadata.step;
  if (!std::isfinite(raw_index)) return value <= metadata.minimum ? 0 : count;
  if (raw_index >= static_cast<double>(count))
    return count;
  if (raw_index <= 0.0) return 0;
  return std::llround(raw_index);
}

inline double value_for_step_index(const Metadata &metadata, long long index) {
  if (!metadata_valid(metadata)) return metadata.minimum;
  const long long count = legal_step_count(metadata);
  index = std::max(0LL, std::min(count, index));
  const double value = metadata.minimum + static_cast<double>(index) * metadata.step;
  return std::max(metadata.minimum, std::min(metadata.maximum, value));
}

inline double snap_value(const Metadata &metadata, double value) {
  return value_for_step_index(metadata, nearest_step_index(metadata, value));
}

inline double value_for_position(const Metadata &metadata, int position) {
  const int positions = slider_position_count(metadata);
  const long long steps = legal_step_count(metadata);
  if (positions <= 0 || steps <= 0) return metadata.minimum;
  position = clamp_position(metadata, position);
  const long long step_index = positions == steps
    ? position
    : std::llround(static_cast<double>(position) * steps / positions);
  return value_for_step_index(metadata, step_index);
}

inline int position_for_value(const Metadata &metadata, double value) {
  const int positions = slider_position_count(metadata);
  const long long steps = legal_step_count(metadata);
  if (positions <= 0 || steps <= 0) return 0;
  const long long step_index = nearest_step_index(metadata, value);
  if (positions == steps) return static_cast<int>(step_index);
  return clamp_position(
    metadata,
    static_cast<int>(std::llround(static_cast<double>(step_index) * positions / steps)));
}

inline int decimal_places(double step) {
  if (!std::isfinite(step) || step <= 0.0) return 0;
  const bool float_precision =
    static_cast<double>(static_cast<float>(step)) == step;
  const double source_epsilon = float_precision
    ? static_cast<double>(std::numeric_limits<float>::epsilon())
    : std::numeric_limits<double>::epsilon();
  double scaled = step;
  for (int places = 0; places <= MAX_DECIMAL_PLACES; places++) {
    const double rounded = std::round(scaled);
    const double tolerance = source_epsilon *
                             std::max(1.0, std::fabs(scaled)) * 2.0;
    if (rounded >= 1.0 && std::fabs(scaled - rounded) <= tolerance)
      return places;
    scaled *= 10.0;
  }
  return GENERAL_FORMAT_DECIMAL_PLACES;
}

inline int decimal_places(const Metadata &metadata) {
  if (!metadata_valid(metadata)) return 0;
  return std::max({decimal_places(metadata.step),
                   decimal_places(std::fabs(metadata.minimum)),
                   decimal_places(std::fabs(metadata.maximum))});
}

inline std::string format_value(double value, const Metadata &metadata) {
  char buffer[48];
  const int places = decimal_places(metadata);
  const double magnitude = std::fabs(value);
  const int integer_places = magnitude >= 1.0
    ? static_cast<int>(std::floor(std::log10(magnitude))) + 1
    : 1;
  const bool fixed_format_fits = places <= MAX_DECIMAL_PLACES &&
                                 integer_places + places + 3 <=
                                   static_cast<int>(sizeof(buffer));
  if (fixed_format_fits) {
    if (magnitude < 0.5 * std::pow(10.0, -places)) value = 0.0;
    std::snprintf(buffer, sizeof(buffer), "%.*f", places, value);
  } else {
    if (value == 0.0) value = std::fabs(value);
    std::snprintf(buffer, sizeof(buffer), "%.*g", MAX_DECIMAL_PLACES, value);
  }
  return buffer;
}

}  // namespace espcontrol::number_slider
