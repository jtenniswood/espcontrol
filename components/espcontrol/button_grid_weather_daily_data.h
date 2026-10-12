#pragma once
#include <memory>
#include <vector>
#include "weather_daily_forecast.h"

// Only configured multi-day cards allocate a source. Cards for the same entity
// share one compact week; the legacy per-slot registry is not enlarged.
struct WeatherDailySource {
  std::string entity_id;
  espcontrol::weather::Forecast forecast;
  unsigned users = 0;
  bool loading = true;
  bool stale = false;
};
inline std::vector<std::unique_ptr<WeatherDailySource>> &weather_daily_sources() {
  static std::vector<std::unique_ptr<WeatherDailySource>> sources;
  return sources;
}
inline WeatherDailySource *weather_daily_find_source(const std::string &entity) {
  for (const auto &source : weather_daily_sources())
    if (source->entity_id == entity) return source.get();
  return nullptr;
}
inline WeatherDailySource *weather_daily_acquire_source(const std::string &entity) {
  if (auto *source = weather_daily_find_source(entity)) { ++source->users; return source; }
  auto &sources = weather_daily_sources();
  if (sources.size() >= espcontrol::weather::MAX_SOURCES) return nullptr;
  std::unique_ptr<WeatherDailySource> source(new (std::nothrow) WeatherDailySource);
  if (!source) return nullptr;
  source->entity_id = entity; source->users = 1;
  auto *result = source.get(); sources.push_back(std::move(source)); return result;
}
inline void weather_daily_release_source(WeatherDailySource *source) {
  if (!source || --source->users != 0) return;
  auto &sources = weather_daily_sources();
  sources.erase(std::remove_if(sources.begin(), sources.end(),
    [source](const auto &entry) { return entry.get() == source; }), sources.end());
  if (sources.empty()) std::vector<std::unique_ptr<WeatherDailySource>>().swap(sources);
}
inline void weather_daily_refresh_visuals();
inline void weather_daily_set_loading(const std::string &entity) {
  if (auto *source = weather_daily_find_source(entity)) source->loading = true;
}
inline void weather_daily_set_unavailable(const std::string &entity) {
  if (auto *source = weather_daily_find_source(entity)) {
    source->loading = false;
    source->stale = source->forecast.count > 0;
  }
}
inline std::string weather_daily_response_template(const std::string &entity_id) {
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set r = response if response is defined and response is not none else {} %}"
    "{% set r = r if 'forecast' in r else r.get(entity, {}) %}"
    "{% set forecasts = r.get('forecast', []) or [] %}"
    "{% set today = now().date() %}{% set tomorrow = (now() + timedelta(days=1)).date() %}"
    "{{ 'WF1|' ~ today ~ '|' ~ tomorrow ~ '|' ~ (state_attr(entity, 'temperature_unit') or '') ~ '|' ~ (state_attr(entity, 'precipitation_unit') or '') ~ '|' ~ (state_attr(entity, 'wind_speed_unit') or '') ~ '\\n' }}"
    "{% set high_keys = ['temperature','native_temperature','temperature_high','native_temperature_high','high_temperature','max_temperature','temperature_max','temp_high','max_temp','high'] %}"
    "{% set low_keys = ['templow','native_templow','temperature_low','native_temperature_low','low_temperature','min_temperature','temperature_min','temp_low','min_temp','low'] %}"
    "{% set ns = namespace(count=0, last='') %}"
    "{% for item in forecasts | sort(attribute='datetime') %}"
    "{% set dt = as_datetime(item.get('datetime')) %}"
    "{% if dt is not none %}{% set date = as_local(dt).date() | string %}"
    "{% if date >= today|string and date != ns.last and ns.count < 7 %}"
    "{% set ns.last = date %}{% set ns.count = ns.count + 1 %}"
    "{% set values = namespace(high='', low='') %}"
    "{% for key in high_keys %}{% if values.high == '' and item.get(key) is number %}{% set values.high = item[key] %}{% endif %}{% endfor %}"
    "{% for key in low_keys %}{% if values.low == '' and item.get(key) is number %}{% set values.low = item[key] %}{% endif %}{% endfor %}"
    "{{ date ~ '|' ~ (item.get('condition', '') or '')[:32] | replace('|', '-') | replace('\\n', ' ') }}"
    "{{ '|' ~ values.high ~ '|' ~ values.low }}"
    "{% for key in ['precipitation_probability', 'precipitation', 'wind_speed', 'humidity'] %}"
    "{% set value = item.get(key) %}{{ '|' ~ (value if value is number else '') }}{% endfor %}{{ '\\n' }}"
    "{% endif %}{% endif %}{% endfor %}";
}
