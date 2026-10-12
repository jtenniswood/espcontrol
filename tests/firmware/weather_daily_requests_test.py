"""Execute production request queue/reset helpers against compact shared sources."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HEADERS = ROOT / 'components/espcontrol'
forecast = (HEADERS / 'button_grid_weather_forecast.h').read_text()


def function(name):
    # Select the implementation after the disabled-feature stubs.
    matches = list(re.finditer(r'inline [^\n]*?\b' + re.escape(name) + r'\(', forecast))
    start = matches[-1].start()
    body = forecast.index('{', start)
    depth, end = 1, body + 1
    while depth:
        depth += (forecast[end] == '{') - (forecast[end] == '}')
        end += 1
    return forecast[start:end] + '\n'


functions = ''.join(function(name) for name in [
    'weather_forecast_entity_id_safe', 'weather_forecast_request_matches',
    'weather_forecast_pending_requests', 'weather_forecast_queued_requests',
    'weather_forecast_retry_requests', 'weather_forecast_action_ready_ms',
    'weather_forecast_pending_key', 'weather_forecast_queue_key', 'weather_forecast_any_pending',
    'weather_forecast_clear_queue', 'weather_forecast_clear_retry', 'weather_forecast_clear_retries',
    'weather_forecast_enqueue', 'weather_forecast_dequeue',
    'weather_forecast_cancel_pending_requests',
])
source = r'''
#include <cassert>
#include <cstdint>
#include <set>
#include "button_grid_weather_daily_data.h"
#define ESP_LOGW(...) ((void)0)
constexpr int WEATHER_FORECAST_PENDING_MAX = 8;
struct WeatherForecastPendingRequest { uint32_t call_id=0, started_ms=0; std::string entity_id,day; };
struct WeatherForecastQueuedRequest { std::string entity_id,day; };
struct WeatherForecastRetryRequest { std::string entity_id,day; uint32_t due_ms=0; };
struct WeatherForecastCardRef { std::string entity_id; } refs[1];
WeatherForecastCardRef *weather_forecast_card_refs() { return refs; }
int weather_forecast_card_count() { return 0; } // All three modes now use the compact source registry.
int refreshes=0, cancelled=0;
void weather_forecast_schedule_visual_refresh() { ++refreshes; }
void ha_cancel_action_response_callback(uint32_t, const char *) { ++cancelled; }
bool ready=false;
bool weather_forecast_actions_ready() { return ready; }
bool weather_forecast_enqueue_due_retries() { return false; }
''' + functions + r'''
std::vector<std::string> sent;
void request_weather_forecast_entity(const std::string &entity, const std::string &day) {
  sent.push_back(entity);
  weather_forecast_pending_requests()[0] = {1, 0, entity, day};
}
''' + function('weather_forecast_send_next_queued') + function('refresh_weather_forecast_cards') + r'''
int main() {
  for(size_t i=0;i<espcontrol::weather::MAX_SOURCES;++i)
    assert(weather_daily_acquire_source("weather.test_"+std::to_string(i)));
  assert(!weather_daily_acquire_source("weather.ninth"));
  auto *empty=weather_daily_sources()[0].get();
  auto *cached=weather_daily_sources()[1].get();cached->forecast.count=1;cached->loading=false;
  weather_forecast_action_ready_ms()=123;
  weather_forecast_cancel_pending_requests(false);
  assert(empty->loading && !cached->stale && !cached->loading && weather_forecast_action_ready_ms()==0);
  refresh_weather_forecast_cards();
  assert(sent.empty() && empty->loading);
  ready=true;weather_forecast_send_next_queued();
  refresh_weather_forecast_cards(); // Do not duplicate the request already in flight/queued.
  for(int i=0;i<8;++i) {
    weather_forecast_pending_requests()[0]={};
    weather_forecast_send_next_queued();
  }
  assert(sent.size()==8 && std::set<std::string>(sent.begin(),sent.end()).size()==8);
  assert(weather_forecast_queued_requests()[0].entity_id.empty());
  weather_forecast_pending_requests()[0]={1,0,"weather.test_0",""};
  weather_forecast_cancel_pending_requests();
  assert(cancelled==1 && !empty->loading && cached->stale);
  ready=false;refresh_weather_forecast_cards();assert(empty->loading);
  weather_forecast_cancel_pending_requests(false);assert(empty->loading && cached->stale);
  while(!weather_daily_sources().empty())weather_daily_release_source(weather_daily_sources().back().get());
}
'''
with tempfile.TemporaryDirectory(prefix='weather-requests-') as directory:
    cpp, binary = Path(directory)/'test.cpp', Path(directory)/'test'
    cpp.write_text(source)
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++17','-Wall','-Wextra','-Werror',
                    '-I',str(HEADERS),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
# Keep the phase wiring tied to the executable reset semantics above.
grid=(HEADERS/'button_grid_grid.h').read_text()
phase1=grid.split('inline void grid_phase1(',1)[1].split('inline void grid_phase2(',1)[0]
assert phase1.index('weather_daily_reset_cards();') < phase1.index('setup_card_visual(')
assert grid.count('weather_forecast_cancel_pending_requests(false);')==2
print('Forecast request queue: all eight shared entities, readiness wait, deduplication and rebuild/disconnect reset passed.')
