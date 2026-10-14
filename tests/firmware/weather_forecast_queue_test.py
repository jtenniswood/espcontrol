#!/usr/bin/env python3
"""Exercise production forecast queuing for a full grid of distinct entities."""
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
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>
#include "button_grid_limits.h"
struct lv_obj_t {};
#define ESP_LOGW(...) ((void)0)
void weather_forecast_send_next_queued() {} // Inspect the queue before dispatch.
'''
for name in ("WeatherForecastCardRef", "WeatherForecastPendingRequest",
             "WeatherForecastQueuedRequest", "WeatherForecastRetryRequest"):
    match = re.search(rf"struct {name} \{{[\s\S]*?\n\}};", source)
    assert match, name
    harness += "\n" + match.group() + "\n"
capacity = re.search(r"^constexpr int WEATHER_FORECAST_PENDING_MAX = [^;]+;", source, re.M)
assert capacity
harness += "\n" + capacity.group() + "\n"
for name in ("weather_forecast_card_refs", "weather_forecast_card_count",
             "reset_weather_forecast_cards", "weather_forecast_request_matches",
             "weather_forecast_entity_id_safe", "weather_forecast_pending_requests",
             "weather_forecast_queued_requests", "weather_forecast_retry_requests",
             "weather_forecast_pending_key", "weather_forecast_queue_key",
             "weather_forecast_clear_queue", "weather_forecast_clear_retry",
             "weather_forecast_enqueue", "weather_forecast_dequeue",
             "refresh_weather_forecast_cards"):
    line, body = definition(source, name)
    harness += f'\n#line {line + line_offset} "{header.as_posix()}"\n{body}\n'
harness += r'''
#line 1 "weather_forecast_queue_test_harness.cpp"
int main() {
  auto *refs = weather_forecast_card_refs();
  // Repeated refreshes must include the final card, not starve it indefinitely.
  for (int refresh = 0; refresh < 2; ++refresh) {
    for (int i = 0; i < MAX_GRID_SLOTS; ++i) {
      refs[i].entity_id = "weather.card_" + std::to_string(i);
    }
    weather_forecast_card_count() = MAX_GRID_SLOTS;
    refresh_weather_forecast_cards();
    refresh_weather_forecast_cards(); // Already queued entities are deduplicated.
    for (int i = 0; i < MAX_GRID_SLOTS; ++i) {
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
}
'''
with tempfile.TemporaryDirectory(prefix="weather-forecast-queue-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(harness)
    for grid_slots in (9, 16, 25):
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        f"-DESPCONTROL_MAX_GRID_SLOTS={grid_slots}",
                        "-I", str(root / "components/espcontrol"), str(path / "test.cpp"),
                        "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
print("Production forecast queue serves every entity on 9-, 16-, and 25-slot grids")
