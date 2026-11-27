#pragma once

// Flights card data: parses the compact flight list that the documented Home
// Assistant template sensor builds from the Flightradar24 integration.
//
// The integration's own `flights` attribute is far larger than one ESPHome API
// message can carry, so the template reduces it to the nearest flights and a
// few fields. Flights are separated by ';' and fields by '|', in this order:
//   0 flight number   1 airline        2 aircraft model  3 origin
//   4 destination     5 distance (km)  6 altitude (ft)   7 ground speed (kt)
//   8 vertical speed (ft/min)          9 on ground (1/0)
// Missing values are empty fields. Pure C++ so it can be unit tested on host.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace espcontrol::flights {

constexpr size_t MAX_FLIGHTS = 10;
constexpr size_t MAX_FIELD_LEN = 48;
constexpr size_t MAX_LIST_TEXT_LEN = MAX_FLIGHTS * 10 * (MAX_FIELD_LEN + 1);
constexpr float MISSING = NAN;

inline bool uses_compact_flight_list(std::string_view entity_id) {
  // Built-in Flightradar24 sensors send full tracks in this attribute. They
  // can exceed the native API receive limit before the card can parse them.
  constexpr std::string_view integration_prefix = "sensor.flightradar24_";
  return entity_id.substr(0, integration_prefix.size()) != integration_prefix;
}

enum class Vertical { UNKNOWN, ON_GROUND, CLIMBING, DESCENDING, LEVEL };

struct Flight {
  std::string flight;
  std::string airline;
  std::string aircraft;
  std::string origin;
  std::string destination;
  float distance_km = MISSING;
  float altitude_ft = MISSING;
  float speed_kt = MISSING;
  float vertical_fpm = MISSING;
  bool on_ground = false;

  bool operator==(const Flight &other) const {
    auto same = [](float a, float b) {
      return (std::isnan(a) && std::isnan(b)) || a == b;
    };
    return flight == other.flight && airline == other.airline &&
           aircraft == other.aircraft && origin == other.origin &&
           destination == other.destination &&
           same(distance_km, other.distance_km) &&
           same(altitude_ft, other.altitude_ft) &&
           same(speed_kt, other.speed_kt) &&
           same(vertical_fpm, other.vertical_fpm) &&
           on_ground == other.on_ground;
  }
};

inline std::string trim_field(const std::string &text, size_t start, size_t end) {
  while (start < end && (text[start] == ' ' || text[start] == '\t' ||
                         text[start] == '\r' || text[start] == '\n')) ++start;
  while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' ||
                         text[end - 1] == '\r' || text[end - 1] == '\n')) --end;
  std::string value = text.substr(start, end - start);
  if (value == "None" || value == "none" || value == "null" ||
      value == "unknown" || value == "unavailable") return "";
  if (value.size() > MAX_FIELD_LEN) {
    // Cut on a UTF-8 character boundary so labels never show a broken glyph.
    size_t cut = MAX_FIELD_LEN;
    while (cut > 0 && (static_cast<unsigned char>(value[cut]) & 0xC0) == 0x80) --cut;
    value.resize(cut);
  }
  return value;
}

inline float parse_number(const std::string &value) {
  if (value.empty()) return MISSING;
  char *end = nullptr;
  const float parsed = std::strtof(value.c_str(), &end);
  if (end == value.c_str() || *end != '\0' || !std::isfinite(parsed)) return MISSING;
  return parsed;
}

inline bool parse_flag(const std::string &value) {
  return value == "1" || value == "True" || value == "true" || value == "on";
}

inline std::vector<Flight> parse_list(const std::string &text) {
  std::vector<Flight> flights;
  size_t row_start = 0;
  while (row_start <= text.size() && flights.size() < MAX_FLIGHTS) {
    size_t row_end = text.find(';', row_start);
    if (row_end == std::string::npos) row_end = text.size();
    std::vector<std::string> fields;
    size_t field_start = row_start;
    while (field_start <= row_end && fields.size() < 10) {
      size_t field_end = text.find('|', field_start);
      if (field_end == std::string::npos || field_end > row_end) field_end = row_end;
      fields.push_back(trim_field(text, field_start, field_end));
      field_start = field_end + 1;
    }
    fields.resize(10);
    if (!fields[0].empty() || !fields[2].empty()) {
      Flight flight;
      flight.flight = fields[0];
      flight.airline = fields[1];
      flight.aircraft = fields[2];
      flight.origin = fields[3];
      flight.destination = fields[4];
      flight.distance_km = parse_number(fields[5]);
      flight.altitude_ft = parse_number(fields[6]);
      flight.speed_kt = parse_number(fields[7]);
      flight.vertical_fpm = parse_number(fields[8]);
      flight.on_ground = parse_flag(fields[9]);
      flights.push_back(std::move(flight));
    }
    if (row_end == text.size()) break;
    row_start = row_end + 1;
  }
  return flights;
}

inline Vertical vertical_state(const Flight &flight) {
  if (flight.on_ground) return Vertical::ON_GROUND;
  if (std::isnan(flight.vertical_fpm)) return Vertical::UNKNOWN;
  // Matches Flightradar24's own climb/descent threshold for small fluctuations.
  if (flight.vertical_fpm > 200.0f) return Vertical::CLIMBING;
  if (flight.vertical_fpm < -200.0f) return Vertical::DESCENDING;
  return Vertical::LEVEL;
}

inline std::string group_thousands(long value) {
  const bool negative = value < 0;
  std::string digits = std::to_string(negative ? -value : value);
  for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) digits.insert(i, ",");
  return negative ? "-" + digits : digits;
}

inline std::string format_distance(float km, bool imperial) {
  if (std::isnan(km)) return "";
  const float value = imperial ? km * 0.621371f : km;
  char text[24];
  std::snprintf(text, sizeof(text), value < 100.0f ? "%.1f %s" : "%.0f %s",
                value, imperial ? "mi" : "km");
  return text;
}

inline std::string format_altitude(float feet, bool imperial) {
  if (std::isnan(feet)) return "";
  const long value = std::lround(imperial ? feet : feet * 0.3048f);
  return group_thousands(value) + (imperial ? " ft" : " m");
}

inline std::string format_speed(float knots, bool imperial) {
  if (std::isnan(knots)) return "";
  const long value = std::lround(knots * (imperial ? 1.15078f : 1.852f));
  return std::to_string(value) + (imperial ? " mph" : " km/h");
}

inline std::string route(const Flight &flight) {
  if (flight.origin.empty() && flight.destination.empty()) return "";
  const std::string from = flight.origin.empty() ? "?" : flight.origin;
  const std::string to = flight.destination.empty() ? "?" : flight.destination;
  return from + " \xE2\x80\xBA " + to;  // U+203A, included in the text fonts
}

inline std::string join(const std::vector<std::string> &parts, const char *separator) {
  std::string out;
  for (const auto &part : parts) {
    if (part.empty()) continue;
    if (!out.empty()) out += separator;
    out += part;
  }
  return out;
}

}  // namespace espcontrol::flights
