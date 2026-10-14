#pragma once
#include <memory>
#include <vector>
#include "weather_daily_forecast.h"

// Only configured multi-day cards allocate a source. Cards for the same entity
// share one compact forecast; the legacy per-slot registry is not enlarged.
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
    "{% set high_keys = ['temperature','native_temperature','temperature_high','native_temperature_high','high_temperature','max_temperature','temperature_max','temp_high','max_temp','high'] %}"
    "{% set low_keys = ['templow','native_templow','temperature_low','native_temperature_low','low_temperature','min_temperature','temperature_min','temp_low','min_temp','low'] %}"
    "{% macro source_unit(key, item) %}"
    "{% set unit_keys = ['native_temperature_unit','native_unit_of_measurement','temperature_unit','unit_of_measurement','unit'] if key.startswith('native_') else ['temperature_unit','unit_of_measurement','unit','native_temperature_unit','native_unit_of_measurement'] %}"
    "{% set result = namespace(unit='') %}"
    "{% for key in unit_keys %}{% if result.unit == '' %}{% set result.unit = item.get(key) or r.get(key) or state_attr(entity, key) or '' %}{% endif %}{% endfor %}{{ result.unit }}"
    "{% endmacro %}"
    "{% set dates = namespace(items=[]) %}"
    "{% for item in forecasts %}"
    "{% set dt = as_datetime(item.get('datetime'), none) %}"
    "{% set date = as_local(dt).date() if dt is not none else none %}"
    "{% if date is none %}{% set dt = as_datetime(item.get('date'), none) %}{% set date = dt.date() if dt is not none else none %}{% endif %}"
    "{% if date is not none and date >= today %}{% set dates.items = dates.items + [{'date':date|string, 'item':item}] %}{% endif %}"
    "{% endfor %}"
    "{% set ns = namespace(entries=[], last='', unit='') %}"
    "{% for dated in dates.items | sort(attribute='date') %}"
    "{% if dated.date != ns.last and ns.entries|length < 8 %}"
    "{% set ns.last = dated.date %}{% set item = dated.item %}{% set values = namespace(high='', low='', high_key='', low_key='') %}"
    "{% for key in high_keys %}{% if values.high == '' and item.get(key) is number %}{% set values.high = item[key] %}{% set values.high_key = key %}{% endif %}{% endfor %}"
    "{% for key in low_keys %}{% if values.low == '' and item.get(key) is number %}{% set values.low = item[key] %}{% set values.low_key = key %}{% endif %}{% endfor %}"
    "{% if ns.unit == '' and (values.high is number or values.low is number) %}{% set ns.unit = source_unit(values.high_key or values.low_key, item) %}{% endif %}"
    "{% set ns.entries = ns.entries + [{'date':dated.date, 'item':item, 'values':values}] %}"
    "{% endif %}{% endfor %}"
    "{% set ns.unit = ns.unit or source_unit('', {}) %}"
    "{% macro temperature(value, key, item) %}"
    "{% if value is number %}{% set unit = source_unit(key, item)|lower %}{% set target = ns.unit|lower %}"
    "{{ (value - 32) * 5 / 9 if unit in ['°f','f','fahrenheit'] and target in ['°c','c','celsius'] else (value * 9 / 5 + 32 if unit in ['°c','c','celsius'] and target in ['°f','f','fahrenheit'] else value) }}"
    "{% endif %}{% endmacro %}"
    "{{ 'WF1|' ~ today ~ '|' ~ tomorrow ~ '|' ~ ns.unit ~ '|' ~ (state_attr(entity, 'precipitation_unit') or '') ~ '|' ~ (state_attr(entity, 'wind_speed_unit') or '') ~ '\\n' }}"
    "{% for entry in ns.entries %}{% set item = entry.item %}{% set values = entry['values'] %}"
    "{{ entry.date ~ '|' ~ (item.get('condition', '') or '')[:32] | replace('|', '-') | replace('\\n', ' ') }}"
    "{{ '|' ~ temperature(values.high, values.high_key, item) ~ '|' ~ temperature(values.low, values.low_key, item) }}"
    "{% for key in ['precipitation_probability', 'precipitation', 'wind_speed', 'humidity'] %}{% set value = item.get(key) %}{{ '|' ~ (value if value is number else '') }}{% endfor %}{{ '\\n' }}"
    "{% endfor %}";
}
