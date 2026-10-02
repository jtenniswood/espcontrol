#include <cassert>

#include "theme_settings.h"

struct ThemeCounter { int calls = 0; };
void count_theme(void *context, const ThemePalette &) {
  ++static_cast<ThemeCounter *>(context)->calls;
}

int main() {
  int minute = -1;
  assert(parse_theme_time("00:00", minute) && minute == 0);
  assert(parse_theme_time("23:59", minute) && minute == 1439);
  for (const char *invalid : {"24:00", "23:60", "7:00", "07:0", "07:00x", "ab:cd"})
    assert(!parse_theme_time(invalid, minute));
  assert(parse_theme_mode("Dark") == ThemeMode::DARK);
  assert(parse_theme_mode("Light") == ThemeMode::LIGHT);
  assert(parse_theme_mode("Auto") == ThemeMode::AUTO);
  assert(parse_theme_auto_method("Time") == ThemeAutoMethod::TIME);
  assert(parse_theme_auto_method("Sunrise / Sunset") == ThemeAutoMethod::SUNRISE_SUNSET);
  assert(parse_theme_mode("") == ThemeMode::DARK);  // Older saved configuration.

  ThemeResolver resolver;
  ThemeSettings settings;
  ThemeConditions conditions;
  ThemeCounter counter;
  assert(register_theme_refresh(&counter, count_theme, &counter));
  assert(!apply_theme_resolution(resolver, settings, conditions));
  settings.mode = ThemeMode::LIGHT;
  assert(apply_theme_resolution(resolver, settings, conditions));
  assert(&current_theme() == &LIGHT_THEME && counter.calls == 1);
  assert(!apply_theme_resolution(resolver, settings, conditions));
  settings.mode = ThemeMode::DARK;
  assert(apply_theme_resolution(resolver, settings, conditions));
  assert(&current_theme() == &DARK_THEME && counter.calls == 2);

  settings.mode = ThemeMode::AUTO;
  conditions.time_valid = true;
  for (int now : {0, 419, 1200, 1380}) {
    conditions.local_minute = now;
    assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::DARK);
  }
  for (int now : {420, 720, 1199}) {
    conditions.local_minute = now;
    assert(resolve_theme(settings, conditions, EffectiveTheme::DARK) == EffectiveTheme::LIGHT);
  }
  settings.light_start = 1200;
  settings.dark_start = 420;
  for (int now : {0, 419, 1200, 1439}) {
    conditions.local_minute = now;
    assert(resolve_theme(settings, conditions, EffectiveTheme::DARK) == EffectiveTheme::LIGHT);
  }
  for (int now : {420, 1199}) {
    conditions.local_minute = now;
    assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::DARK);
  }
  settings.dark_start = settings.light_start;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::DARK);
  conditions.time_valid = false;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::LIGHT);

  settings.auto_method = ThemeAutoMethod::SUNRISE_SUNSET;
  conditions.time_valid = true;
  conditions.sunrise_minute = 360;
  conditions.sunset_minute = 1080;
  conditions.sun_valid = true;
  conditions.local_minute = 600;
  assert(resolve_theme(settings, conditions, EffectiveTheme::DARK) == EffectiveTheme::LIGHT);
  conditions.local_minute = 1200;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::DARK);
  conditions.sun_valid = false;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::LIGHT);
  assert(resolve_theme(settings, conditions, ThemeResolver{}.effective) == EffectiveTheme::DARK);
  assert(!apply_theme_resolution(resolver, settings, conditions));
  conditions.sun_valid = true;
  conditions.sunrise_minute = 360;
  conditions.sunset_minute = 1080;
  settings.sunrise_offset = 30;
  conditions.local_minute = 389;
  assert(resolve_theme(settings, conditions, EffectiveTheme::DARK) == EffectiveTheme::DARK);
  conditions.local_minute = 390;
  assert(resolve_theme(settings, conditions, EffectiveTheme::DARK) == EffectiveTheme::LIGHT);
  settings.sunrise_offset = -20;
  conditions.local_minute = 340;
  assert(resolve_theme(settings, conditions, EffectiveTheme::DARK) == EffectiveTheme::LIGHT);
  settings.sunset_offset = 45;
  conditions.local_minute = 1124;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::LIGHT);
  conditions.local_minute = 1125;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::DARK);
  settings.sunset_offset = -15;
  conditions.local_minute = 1065;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::DARK);
  assert(theme_normalize_minute(1430 + 30) == 20);
  assert(theme_normalize_minute(10 - 30) == 1420);
  conditions.sunrise_minute = 1430;
  conditions.sunset_minute = 600;
  settings.sunrise_offset = 30;
  settings.sunset_offset = -15;
  conditions.local_minute = 20;
  assert(resolve_theme(settings, conditions, EffectiveTheme::DARK) == EffectiveTheme::LIGHT);
  conditions.local_minute = 19;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::DARK);
  assert(theme_sun_offset_valid(-180) && theme_sun_offset_valid(180));
  assert(!theme_sun_offset_valid(-181) && !theme_sun_offset_valid(181));
  settings.sunrise_offset = 181;
  assert(resolve_theme(settings, conditions, EffectiveTheme::LIGHT) == EffectiveTheme::LIGHT);
  unregister_theme_refresh(&counter);
}
