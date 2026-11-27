#pragma once

#include "flights_data.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace espcontrol::flights {

struct MapPoint {
  float x = 0;
  float y = 0;
};

struct MapBounds {
  float north = 0;
  float south = 0;
  float west = 0;
  float east = 0;

  explicit operator bool() const { return north > south && east > west; }
  bool operator==(const MapBounds &other) const {
    return north == other.north && south == other.south &&
           west == other.west && east == other.east;
  }
};

inline MapBounds parse_map_bounds(const std::string &text) {
  std::array<float, 4> values{};
  size_t begin = text.compare(0, 4, "map:") == 0 ? 4 : 0;
  for (size_t i = 0; i < values.size(); ++i) {
    const size_t end = text.find(',', begin);
    if ((end == std::string::npos) != (i == values.size() - 1)) return {};
    values[i] = parse_number(trim_field(text, begin, end == std::string::npos ? text.size() : end));
    if (!std::isfinite(values[i])) return {};
    begin = end + 1;
  }
  auto [north, south, west, east] = values;
  if (std::abs(west) > 360 || std::abs(east) > 360 ||
      north > 90 || south < -90 || north - south < 0.001f) return {};
  if (east < west) east += 360;
  if (east - west < 0.001f || east - west > 360) return {};
  return {north, south, west, east};
}

constexpr float MAP_RAD = 0.01745329252f;
constexpr uint32_t MAP_COAST_MS = 30000;
constexpr uint32_t MAP_CORRECTION_MS = 2500;

struct MapProjection {
  float lat = 0;
  float lon = 0;
  float x_scale = 0;
  float y_scale = 0;
  int width = 0;
  int height = 0;

  MapProjection() = default;
  MapProjection(MapBounds bounds, int w, int h)
      : lat((bounds.north + bounds.south) / 2), lon((bounds.east + bounds.west) / 2),
        width(w), height(h) {
    if (!bounds || w <= 0 || h <= 0) return;
    const float cos_lat = std::max(0.01f, std::cos(lat * MAP_RAD));
    y_scale = std::min(h * 0.90f / (bounds.north - bounds.south),
                       w * 0.90f / ((bounds.east - bounds.west) * cos_lat));
    x_scale = y_scale * cos_lat;
  }

  MapPoint project(float latitude, float longitude) const {
    return {width * 0.5f + std::remainder(longitude - lon, 360.0f) * x_scale,
            height * 0.5f - (latitude - lat) * y_scale};
  }
};

struct MapTrack {
  std::string id;
  float latitude = 0;
  float longitude = 0;
  float heading = MISSING;
  float speed = MISSING;
  uint32_t received = 0;
  MapPoint correction{};
  bool on_ground = false;

  MapPoint position(const MapProjection &map, uint32_t now) const {
    auto point = map.project(latitude, longitude);
    const uint32_t age = now - received;
    if (!on_ground && std::isfinite(speed) && speed > 0 && speed <= 1200 &&
        std::isfinite(heading)) {
      const float distance_deg = speed * (1.852f / 111.195f / 3600000.0f) *
                                 std::min(age, MAP_COAST_MS);
      const float angle = heading * MAP_RAD;
      const float cos_lat = std::max(0.01f, std::cos(latitude * MAP_RAD));
      point.x += distance_deg * std::sin(angle) * map.x_scale / cos_lat;
      point.y -= distance_deg * std::cos(angle) * map.y_scale;
    }
    const float t = std::min(1.0f, static_cast<float>(age) / MAP_CORRECTION_MS);
    const float ease = (1 - t) * (1 - t) * (1 + 2 * t);
    return {point.x + correction.x * ease, point.y + correction.y * ease};
  }

  void update(const Flight &flight, const MapProjection &map, uint32_t now) {
    const bool same_flight = id == flight.map_id;
    const auto previous = same_flight ? position(map, now) : MapPoint{};
    id = flight.map_id;
    latitude = flight.latitude;
    longitude = flight.longitude;
    heading = flight.heading;
    speed = flight.speed_kt;
    on_ground = flight.on_ground;
    received = now;
    correction = {};
    if (same_flight) {
      const auto next = position(map, now);
      correction = {previous.x - next.x, previous.y - next.y};
    }
  }
};

}  // namespace espcontrol::flights
