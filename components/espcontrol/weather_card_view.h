#pragma once
#include "weather_daily_forecast.h"
namespace espcontrol::weather {
enum class View : uint8_t { DAILY, TODAY, TOMORROW };
inline size_t selected_day(const Forecast& forecast, View view) {
  if (view == View::DAILY) return 0;
  const char* date = view == View::TODAY ? forecast.today : forecast.tomorrow;
  size_t index = 0;
  while (index < forecast.count && index < MAX_DAYS && std::strcmp(forecast.days[index].date, date) != 0)
    ++index;
  return index;
}
}  // namespace espcontrol::weather
