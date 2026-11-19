#include <cassert>
#include <cmath>
#include <string>

#include "flights_data.h"

using namespace espcontrol::flights;

static void parses_template_output() {
  const auto flights = parse_list(
    "BA123|British Airways|Airbus A320neo|London|Madrid|12.3|10500|440|1200|0;"
    "N123AB|None|C172|None|None|3.2|0|0|0|1");
  assert(flights.size() == 2);
  assert(flights[0].flight == "BA123");
  assert(flights[0].airline == "British Airways");
  assert(flights[0].aircraft == "Airbus A320neo");
  assert(flights[0].origin == "London");
  assert(flights[0].destination == "Madrid");
  assert(std::fabs(flights[0].distance_km - 12.3f) < 0.001f);
  assert(flights[0].altitude_ft == 10500.0f);
  assert(flights[0].speed_kt == 440.0f);
  assert(flights[0].vertical_fpm == 1200.0f);
  assert(!flights[0].on_ground);
  assert(vertical_state(flights[0]) == Vertical::CLIMBING);

  assert(flights[1].airline.empty());
  assert(flights[1].origin.empty());
  assert(flights[1].on_ground);
  assert(vertical_state(flights[1]) == Vertical::ON_GROUND);
}

static void tolerates_missing_and_bad_values() {
  assert(parse_list("").empty());
  assert(parse_list(";;").empty());
  // A row without a flight number or aircraft type is not shown.
  assert(parse_list("|Airline||A|B|1|2|3|4|0").empty());

  const auto flights = parse_list("X1|||||abc|None||-50");
  assert(flights.size() == 1);
  assert(std::isnan(flights[0].distance_km));
  assert(std::isnan(flights[0].altitude_ft));
  assert(std::isnan(flights[0].speed_kt));
  assert(vertical_state(flights[0]) == Vertical::LEVEL);
  assert(format_distance(flights[0].distance_km, false).empty());
  assert(route(flights[0]).empty());
  assert(tile_summary(flights[0]) == "X1");

  const auto unknown = parse_list("X2");
  assert(unknown.size() == 1);
  assert(vertical_state(unknown[0]) == Vertical::UNKNOWN);
}

static void caps_list_and_field_lengths() {
  std::string text;
  for (int i = 0; i < 25; ++i) text += "F" + std::to_string(i) + "|A|B|C|D|1|2|3|4|0;";
  const auto flights = parse_list(text);
  assert(flights.size() == MAX_FLIGHTS);
  assert(flights.back().flight == "F9");

  // Truncation keeps whole UTF-8 characters.
  std::string long_city(MAX_FIELD_LEN - 1, 'a');
  long_city += "\xC3\xA3o";  // "ão" straddles the limit
  const auto truncated = parse_list("X|A|B|" + long_city);
  assert(truncated.size() == 1);
  assert(truncated[0].origin.size() == MAX_FIELD_LEN - 1);
}

static void formats_units() {
  assert(format_distance(12.34f, false) == "12.3 km");
  assert(format_distance(12.34f, true) == "7.7 mi");
  assert(format_distance(250.0f, false) == "250 km");
  assert(format_altitude(10500.0f, false) == "3,200 m");
  assert(format_altitude(10500.0f, true) == "10,500 ft");
  assert(format_altitude(800.0f, true) == "800 ft");
  assert(format_speed(440.0f, false) == "815 km/h");
  assert(format_speed(440.0f, true) == "506 mph");
}

static void builds_summaries() {
  const auto flights = parse_list(
    "BA123|British Airways|A320|London|Madrid;|Air|C172|||;LH9|Lufthansa|A350||Rome");
  assert(flights.size() == 3);
  assert(route(flights[0]) == "London \xE2\x80\xBA Madrid");
  assert(tile_summary(flights[0]) == "BA123 \xE2\x80\xBA Madrid");
  assert(flight_title(flights[1]) == "C172");
  assert(tile_summary(flights[1]) == "C172");
  assert(route(flights[2]) == "? \xE2\x80\xBA Rome");
  assert(join({"a", "", "b"}, " - ") == "a - b");
}

static void detects_changes() {
  const auto a = parse_list("BA1|A|B|C|D|1|2|3|4|0");
  auto b = parse_list("BA1|A|B|C|D|1|2|3|4|0");
  assert(a == b);
  b = parse_list("BA1|A|B|C|D|1.5|2|3|4|0");
  assert(!(a == b));
  assert(parse_list("X") == parse_list("X"));
}

int main() {
  parses_template_output();
  tolerates_missing_and_bad_values();
  caps_list_and_field_lengths();
  formats_units();
  builds_summaries();
  detects_changes();
  return 0;
}
