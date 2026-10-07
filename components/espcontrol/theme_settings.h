#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <initializer_list>
#include <string>

#include "theme_palette.h"

enum class ThemeMode : uint8_t { DARK, LIGHT, AUTO };
enum class ThemeAutoMethod : uint8_t { TIME, SUNRISE_SUNSET };
enum class EffectiveTheme : uint8_t { DARK, LIGHT };

inline constexpr int THEME_SUN_OFFSET_LIMIT_MINUTES = 180;

struct ThemeSettings {
  ThemeMode mode = ThemeMode::DARK;
  ThemeAutoMethod auto_method = ThemeAutoMethod::TIME;
  int light_start = 7 * 60;
  int dark_start = 20 * 60;
  int sunrise_offset = 0;
  int sunset_offset = 0;
};

// ESPHome template selects restore option indexes, not option strings. The
// former Sun option occupied index 3, outside today's Dark/Light/Auto list.
// Index 2 already restores as Auto with the default Time method (Schedule).
inline bool migrate_legacy_theme_index(size_t index, ThemeSettings &settings) {
  if (index != 3) return false;
  settings.mode = ThemeMode::AUTO;
  settings.auto_method = ThemeAutoMethod::SUNRISE_SUNSET;
  return true;
}

struct ThemeConditions {
  bool time_valid = false;
  int local_minute = 0;
  bool sun_valid = false;
  int sunrise_minute = 0;
  int sunset_minute = 0;
};

// An unavailable clock or solar calculation retains the last effective color.
// A fresh boot starts Dark; the configured mode is never replaced by fallback.
struct ThemeResolver {
  EffectiveTheme effective = EffectiveTheme::DARK;
};

inline bool parse_theme_time(const std::string &text, int &minute) {
  if (text.size() != 5 || text[2] != ':') return false;
  for (int i : {0, 1, 3, 4}) {
    if (text[i] < '0' || text[i] > '9') return false;
  }
  const int hour = (text[0] - '0') * 10 + (text[1] - '0');
  const int min = (text[3] - '0') * 10 + (text[4] - '0');
  if (hour > 23 || min > 59) return false;
  minute = hour * 60 + min;
  return true;
}

inline ThemeMode parse_theme_mode(const std::string &mode) {
  if (mode == "Light") return ThemeMode::LIGHT;
  if (mode == "Auto") return ThemeMode::AUTO;
  return ThemeMode::DARK;
}

inline ThemeAutoMethod parse_theme_auto_method(const std::string &method) {
  return method == "Sunrise / Sunset" ? ThemeAutoMethod::SUNRISE_SUNSET
                                        : ThemeAutoMethod::TIME;
}

inline bool theme_sun_offset_valid(int offset) {
  return offset >= -THEME_SUN_OFFSET_LIMIT_MINUTES &&
         offset <= THEME_SUN_OFFSET_LIMIT_MINUTES;
}

inline bool parse_theme_sun_offset(float value, int &offset) {
  if (!std::isfinite(value) || value < -THEME_SUN_OFFSET_LIMIT_MINUTES ||
      value > THEME_SUN_OFFSET_LIMIT_MINUTES || std::floor(value) != value) return false;
  offset = static_cast<int>(value);
  return true;
}

inline int theme_normalize_minute(int minute) {
  const int normalized = minute % 1440;
  return normalized < 0 ? normalized + 1440 : normalized;
}

inline bool theme_light_between(int now, int light_start, int dark_start) {
  if (light_start == dark_start) return false;  // Equal boundaries mean Dark.
  if (light_start < dark_start)
    return now >= light_start && now < dark_start;
  return now >= light_start || now < dark_start;
}

inline EffectiveTheme resolve_theme(const ThemeSettings &settings,
                                   const ThemeConditions &conditions,
                                   EffectiveTheme previous) {
  switch (settings.mode) {
    case ThemeMode::DARK: return EffectiveTheme::DARK;
    case ThemeMode::LIGHT: return EffectiveTheme::LIGHT;
    case ThemeMode::AUTO:
      if (!conditions.time_valid) return previous;
      if (settings.auto_method == ThemeAutoMethod::TIME)
        return theme_light_between(conditions.local_minute, settings.light_start,
                                   settings.dark_start)
                   ? EffectiveTheme::LIGHT : EffectiveTheme::DARK;
      if (!conditions.sun_valid || !theme_sun_offset_valid(settings.sunrise_offset) ||
          !theme_sun_offset_valid(settings.sunset_offset)) return previous;
      return theme_light_between(
                 conditions.local_minute,
                 theme_normalize_minute(conditions.sunrise_minute + settings.sunrise_offset),
                 theme_normalize_minute(conditions.sunset_minute + settings.sunset_offset))
                 ? EffectiveTheme::LIGHT : EffectiveTheme::DARK;
  }
  return EffectiveTheme::DARK;
}

inline bool apply_theme_resolution(ThemeResolver &resolver,
                                   const ThemeSettings &settings,
                                   const ThemeConditions &conditions) {
  const auto next = resolve_theme(settings, conditions, resolver.effective);
  const ThemePalette &palette = next == EffectiveTheme::LIGHT ? LIGHT_THEME : DARK_THEME;
  resolver.effective = next;
  if (&current_theme() == &palette) return false;
  set_active_theme_palette(palette);
  apply_current_theme();
  return true;
}

inline ThemeResolver &current_theme_resolver() {
  static ThemeResolver resolver{};
  return resolver;
}
