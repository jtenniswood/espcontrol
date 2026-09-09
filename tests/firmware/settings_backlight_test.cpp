#include "../../components/espcontrol/settings_backlight.h"

#include <cassert>
#include <limits>

int main() {
  using Level = SettingsBacklightLevel;
  assert(settings_backlight_percent(Level::MANUAL, 0) == 1);
  assert(settings_backlight_percent(Level::DAYTIME, 0) == 10);
  assert(settings_backlight_percent(Level::NIGHTTIME, 73) == 75);
  assert(settings_backlight_percent(Level::MANUAL, 73) == 73);
  assert(settings_backlight_percent(Level::DAYTIME, 150) == 100);
  assert(settings_backlight_percent(
             Level::NIGHTTIME,
             std::numeric_limits<float>::quiet_NaN()) == 100);

  SettingsBacklightState state{true, Level::MANUAL, 50};
  int writes = 0;
  int value = 0;
  Level target = Level::MANUAL;
  auto &service = settings_backlight_service();
  service.read = [&] { return state; };
  service.write = [&](Level level, int percent) {
    ++writes;
    target = level;
    value = percent;
  };

  for (Level level : {Level::MANUAL, Level::DAYTIME, Level::NIGHTTIME}) {
    state.level = level;
    assert(settings_backlight_commit(level, 73, true));
    assert(target == level);
    assert(value == (level == Level::MANUAL ? 73 : 75));
  }

  const int before = writes;
  state.level = Level::NIGHTTIME;
  assert(!settings_backlight_commit(Level::DAYTIME, 100, true));
  assert(!settings_backlight_commit(Level::NIGHTTIME, 100, false));
  state.available = false;
  assert(!settings_backlight_commit(Level::NIGHTTIME, 100, true));
  assert(writes == before);
}
