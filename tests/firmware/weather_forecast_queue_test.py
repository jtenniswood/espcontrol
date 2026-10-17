#!/usr/bin/env python3
"""Exercise production forecast queues across the main grid and all subpages."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
header = root / "components/espcontrol/button_grid_weather_forecast.h"
# Select the enabled implementation rather than the disabled-device no-op stubs.
disabled, source = header.read_text().split("\n#else\n", 1)
line_offset = disabled.count("\n") + 2
harness = r'''
#include <cassert>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>
#include "button_grid_limits.h"
#include "button_grid_weather_daily_data.h"
struct lv_obj_t {};
template<typename... Args> void log_warning(Args...) {}
#define ESP_LOGW(...) log_warning(__VA_ARGS__)
uint32_t now_ms = 0;
namespace esphome { uint32_t millis() { return now_ms; } }
bool connected = true;
bool ha_api_state_connected() { return connected; }
void weather_forecast_send_next_queued() {} // Inspect the queue before dispatch.
void weather_forecast_schedule_visual_refresh() {}
'''
for name in ("WeatherForecastCardRef", "WeatherForecastPendingRequest",
             "WeatherForecastQueuedRequest", "WeatherForecastRetryRequest"):
    match = re.search(rf"struct {name} \{{[\s\S]*?\n\}};", source)
    assert match, name
    harness += "\n" + match.group() + "\n"
constants = re.findall(r"^constexpr (?:int|uint32_t) WEATHER_FORECAST_(?:PENDING_MAX|WAITING_MAX|RETRY_DELAY_MS) = [^;]+;",
                       source, re.M)
harness += "\n" + "\n".join(constants) + "\n"
for name in ("weather_forecast_card_refs", "weather_forecast_card_count",
             "reset_weather_forecast_cards", "weather_forecast_request_matches",
             "weather_forecast_entity_id_safe", "weather_forecast_pending_requests",
             "weather_forecast_queued_requests", "weather_forecast_retry_requests",
             "weather_forecast_pending_key", "weather_forecast_queue_key",
             "weather_forecast_retry_key",
             "weather_forecast_clear_queue", "weather_forecast_clear_retry",
             "weather_forecast_clear_retries", "weather_forecast_schedule_retry",
             "weather_forecast_enqueue", "weather_forecast_dequeue",
             "weather_forecast_enqueue_due_retries",
             "refresh_weather_forecast_cards"):
    line, body = definition(source, name)
    harness += f'\n#line {line + line_offset} "{header.as_posix()}"\n{body}\n'
harness += r'''
#line 1 "weather_forecast_queue_test_harness.cpp"
int main() {
  auto *refs = weather_forecast_card_refs();
  const int total_cards = MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS;
  // Repeated refreshes must include the final card, not starve it indefinitely.
  for (int refresh = 0; refresh < 2; ++refresh) {
    for (int i = 0; i < total_cards; ++i) {
      refs[i].entity_id = "weather.card_" + std::to_string(i);
    }
    weather_forecast_card_count() = total_cards;
    refresh_weather_forecast_cards();
    refresh_weather_forecast_cards(); // Already queued entities are deduplicated.
    for (int i = 0; i < total_cards; ++i) {
      std::string entity, day;
      assert(weather_forecast_dequeue(entity, day));
      assert(entity == "weather.card_" + std::to_string(i));
      assert(day.empty()); // One daily forecast request serves both days.
    }
    std::string entity, day;
    assert(!weather_forecast_dequeue(entity, day));
  }

  // Today and Tomorrow cards sharing an entity use one queue slot.
  refs[0].entity_id = refs[1].entity_id = "weather.shared";
  refs[0].day = "today";
  refs[1].day = "tomorrow";
  weather_forecast_card_count() = 2;
  refresh_weather_forecast_cards();
  std::string entity, day;
  assert(weather_forecast_dequeue(entity, day) && entity == "weather.shared");
  assert(!weather_forecast_dequeue(entity, day));

  assert(weather_forecast_enqueue("weather.cleared", ""));
  weather_forecast_clear_queue();
  reset_weather_forecast_cards();
  assert(!weather_forecast_dequeue(entity, day));
  assert(weather_forecast_card_count() == 0);

  // Low-heap or failed requests must retain retries for every registered card.
  for (int i = 0; i < total_cards; ++i) {
    const auto key = "weather.retry_" + std::to_string(i);
    assert(weather_forecast_schedule_retry(key, "", "low internal heap"));
    assert(weather_forecast_schedule_retry(key, "", "duplicate"));
  }
  assert(!weather_forecast_enqueue_due_retries());
  now_ms += WEATHER_FORECAST_RETRY_DELAY_MS;
  connected = false;
  assert(!weather_forecast_enqueue_due_retries());
  connected = true;
  assert(weather_forecast_enqueue_due_retries());
  for (int i = 0; i < total_cards; ++i) {
    assert(weather_forecast_dequeue(entity, day));
    assert(entity == "weather.retry_" + std::to_string(i));
    assert(!weather_forecast_retry_key(entity, day));
  }
  assert(!weather_forecast_dequeue(entity, day));
  assert(weather_forecast_schedule_retry("weather.cleared", "", "failure"));
  weather_forecast_clear_retries();
  now_ms += WEATHER_FORECAST_RETRY_DELAY_MS;
  assert(!weather_forecast_enqueue_due_retries());

  // A saturated waiting queue must not discard a due retry.
  for (int i = 0; i < total_cards; ++i) {
    assert(weather_forecast_enqueue("weather.full_" + std::to_string(i), ""));
  }
  assert(!weather_forecast_enqueue("weather.overflow", ""));
  assert(weather_forecast_schedule_retry("weather.waiting", "", "failure"));
  now_ms += WEATHER_FORECAST_RETRY_DELAY_MS;
  assert(!weather_forecast_enqueue_due_retries());
  assert(weather_forecast_retry_key("weather.waiting", ""));
  weather_forecast_clear_queue();
  assert(weather_forecast_enqueue_due_retries());
  assert(weather_forecast_dequeue(entity, day) && entity == "weather.waiting");
  assert(!weather_forecast_retry_key(entity, day));
}
'''
with tempfile.TemporaryDirectory(prefix="weather-forecast-queue-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(harness)
    for grid_slots in (6, 9, 16, 25):
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        f"-DESPCONTROL_MAX_GRID_SLOTS={grid_slots}",
                        "-I", str(root / "components/espcontrol"), str(path / "test.cpp"),
                        "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
print("Production forecast queues serve every main-grid and subpage entity on 6-, 9-, 16-, and 25-slot devices")
