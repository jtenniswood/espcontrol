#include <cassert>
#include <string>
#include <vector>
#include "button_grid_weather_daily_data.h"
#include "weather_daily_forecast.h"
using namespace espcontrol::weather;
int main() {
  Forecast forecast;
  const std::string header = "WF1|2026-10-11|2026-10-12|°C|mm|km/h\n";
  assert(parse(header + "2026-10-12|rainy|-2|-8|0|0|12.5|75\n2026-10-14|cloudy|4|||||\n", forecast));
  assert(forecast.count == 2 && std::string(forecast.days[0].date) == "2026-10-12");
  assert(forecast.days[0].high == -2 && forecast.days[0].low == -8);
  assert(forecast.days[0].rain_probability == 0 && forecast.days[0].precipitation == 0);
  assert(std::isnan(forecast.days[1].low) && std::isnan(forecast.days[1].rain_probability));
  assert(weekday("2026-10-11") == 0);
  assert(valid_date("2028-02-29") && !valid_date("2026-02-29"));
  assert(!valid_date("2026-00-12") && !valid_date("2026-10-33"));
  assert(!parse(header + "2026-10-12|rainy|2|1|5|0|1|90\n2026-10-12|sunny|3|1|5|0|1|90\n", forecast));
  assert(!parse(header + "2026-10-12|rainy|2|1\n", forecast));
  assert(!parse(std::string(MAX_PAYLOAD_BYTES + 1, 'x'), forecast));
  assert(parse(header + "2026-10-12|rainy|nan|1junk|200|-2|-3|120\n", forecast));
  const auto &bad = forecast.days[0];
  assert(std::isnan(bad.high) && std::isnan(bad.low) && std::isnan(bad.rain_probability));
  assert(std::isnan(bad.precipitation) && std::isnan(bad.wind) && std::isnan(bad.humidity));
  std::string week = header;
  for (int i = 12; i < 19; ++i) week += "2026-10-" + std::to_string(i) + "|sunny|20|10||||\n";
  assert(parse(week, forecast) && forecast.count == MAX_DAYS);
  assert(!parse(week + "2026-10-19|sunny|20|10||||\n", forecast));
  assert(visible_days(120, 90) == 1);
  assert(visible_days(300, 90) == 3);
  assert(visible_days(630, 90) == 7);
  assert(visible_days(300, 90, 2) == 2 && visible_days(300, 90, 0) == 0);
  assert(visible_days(0, 90) == 1);
  auto *first = weather_daily_acquire_source("weather.home");
  auto *second = weather_daily_acquire_source("weather.home");
  assert(first == second && first->users == 2 && weather_daily_sources().size() == 1);
  first->forecast.count = 1;
  assert(second->forecast.count == 1);
  weather_daily_release_source(first);
  assert(second->users == 1 && weather_daily_sources().size() == 1);
  weather_daily_release_source(second);
  assert(weather_daily_sources().empty());
  for (size_t i = 0; i < MAX_SOURCES; ++i)
    assert(weather_daily_acquire_source("weather.entity_" + std::to_string(i)));
  assert(!weather_daily_acquire_source("weather.overflow"));
  while (!weather_daily_sources().empty()) weather_daily_release_source(weather_daily_sources().back().get());
  assert(weather_daily_sources().capacity() == 0);
  static_assert(sizeof(WeatherDailySource) < 800, "Bound one source including identity and lifecycle");
}
