#pragma once

#include <algorithm>
#include <cmath>
#include <functional>

// Keep the on-device control readable even at its lowest setting.
constexpr int SETTINGS_BACKLIGHT_MIN_PERCENT = 10;

enum class SettingsBacklightLevel { MANUAL, DAYTIME, NIGHTTIME };

struct SettingsBacklightState {
  bool available = false;
  SettingsBacklightLevel level = SettingsBacklightLevel::MANUAL;
  float percent = 100;
};

inline int settings_backlight_percent(SettingsBacklightLevel level, float value) {
  const int step = level == SettingsBacklightLevel::MANUAL ? 1 : 5;
  if (!std::isfinite(value)) value = 100;
  return std::max(SETTINGS_BACKLIGHT_MIN_PERCENT,
                  std::min(100, static_cast<int>(std::lround(value / step)) * step));
}

struct SettingsBacklightService {
  std::function<SettingsBacklightState()> read;
  std::function<void(SettingsBacklightLevel, int)> write;
};

inline SettingsBacklightService &settings_backlight_service() {
  static SettingsBacklightService service;
  return service;
}

inline bool settings_backlight_commit(SettingsBacklightLevel dragged_level,
                                      int percent, bool allowed) {
  auto &service = settings_backlight_service();
  if (!allowed || !service.read || !service.write) return false;
  const auto active = service.read();
  if (!active.available || active.level != dragged_level) return false;
  service.write(dragged_level,
                settings_backlight_percent(dragged_level, percent));
  return true;
}
