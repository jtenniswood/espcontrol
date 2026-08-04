#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace espcontrol::number_slider {

constexpr int MAX_SLIDER_POSITIONS = 10000;
constexpr int MAX_DECIMAL_PLACES = 6;

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

inline int legal_step_count(const Metadata &metadata) {
  if (!metadata_valid(metadata)) return 0;
  const double raw = (metadata.maximum - metadata.minimum) / metadata.step;
  if (!std::isfinite(raw) || raw <= 0.0) return 0;
  const double nearest = std::round(raw);
  const double integer_tolerance = std::max(
    1e-6, std::min(1e-3, std::fabs(raw) * 1e-7));
  const double rounded = std::fabs(raw - nearest) < integer_tolerance
    ? nearest
    : std::floor(raw);
  if (rounded > static_cast<double>(2147483647)) return 2147483647;
  return std::max(1, static_cast<int>(rounded));
}

inline int slider_position_count(const Metadata &metadata) {
  return std::min(legal_step_count(metadata), MAX_SLIDER_POSITIONS);
}

inline int clamp_position(const Metadata &metadata, int position) {
  return std::max(0, std::min(slider_position_count(metadata), position));
}

inline long long nearest_step_index(const Metadata &metadata, double value) {
  if (!metadata_valid(metadata) || !std::isfinite(value)) return 0;
  const long long count = legal_step_count(metadata);
  long long index = std::llround((value - metadata.minimum) / metadata.step);
  return std::max(0LL, std::min(count, index));
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
  const int steps = legal_step_count(metadata);
  if (positions <= 0 || steps <= 0) return metadata.minimum;
  position = clamp_position(metadata, position);
  const long long step_index = positions == steps
    ? position
    : std::llround(static_cast<double>(position) * steps / positions);
  return value_for_step_index(metadata, step_index);
}

inline int position_for_value(const Metadata &metadata, double value) {
  const int positions = slider_position_count(metadata);
  const int steps = legal_step_count(metadata);
  if (positions <= 0 || steps <= 0) return 0;
  const long long step_index = nearest_step_index(metadata, value);
  if (positions == steps) return static_cast<int>(step_index);
  return clamp_position(
    metadata,
    static_cast<int>(std::llround(static_cast<double>(step_index) * positions / steps)));
}

inline int decimal_places(double step) {
  if (!std::isfinite(step) || step <= 0.0) return 0;
  double scaled = step;
  for (int places = 0; places <= MAX_DECIMAL_PLACES; places++) {
    const double rounded = std::round(scaled);
    if (rounded >= 1.0 && std::fabs(scaled - rounded) < 1e-6) return places;
    scaled *= 10.0;
  }
  return MAX_DECIMAL_PLACES;
}

inline std::string format_value(double value, double step) {
  char buffer[48];
  const int places = decimal_places(step);
  if (std::fabs(value) < 0.5 * std::pow(10.0, -places)) value = 0.0;
  std::snprintf(buffer, sizeof(buffer), "%.*f", places, value);
  return buffer;
}

}  // namespace espcontrol::number_slider
