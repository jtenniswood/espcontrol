#include <cassert>
#include <cmath>

#include "button_grid_style.h"

struct ThemeSample {
  uint32_t background = 0;
  uint32_t text = 0;
  int applications = 0;
  const ThemePalette *previous = nullptr;
};

void apply_theme_sample(void *context, const ThemePalette &theme) {
  auto &sample = *static_cast<ThemeSample *>(context);
  sample.background = theme.background;
  sample.text = theme.text_primary;
  sample.previous = &theme_refresh_previous();
  ++sample.applications;
}

double theme_test_luminance(uint32_t rgb) {
  const auto channel = [](uint32_t value) {
    const double normalized = static_cast<double>(value) / 255.0;
    return normalized <= 0.04045 ? normalized / 12.92
                                 : std::pow((normalized + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * channel((rgb >> 16) & 0xFF) +
         0.7152 * channel((rgb >> 8) & 0xFF) +
         0.0722 * channel(rgb & 0xFF);
}

double theme_test_contrast(uint32_t foreground, uint32_t background) {
  const double a = theme_test_luminance(foreground);
  const double b = theme_test_luminance(background);
  return (std::fmax(a, b) + 0.05) / (std::fmin(a, b) + 0.05);
}

int main() {
  assert(&current_theme() == &DARK_THEME);
  assert(current_theme().background == 0x000000);
  assert(current_theme().surface_primary == 0x313131);
  assert(current_theme().surface_secondary == 0x212121);
  assert(current_theme().surface_sensor == 0x212121);
  assert(current_theme().surface_card == 0x313131);
  assert(current_theme().text_primary == 0xFFFFFF);
  assert(current_theme().text_muted == 0xB0B0B0);
  assert(current_theme().text_inverted == 0x000000);
  assert(current_theme().text_disabled == 0x707070);
  assert(current_theme().border == 0x313131);
  assert(current_theme().control_neutral == 0x313131);
  assert(current_theme().track_background == 0x313131);
  assert(current_theme().overlay == 0x000000);
  assert(current_theme().setup_action == 0x333333);
  assert(readable_text_color_for_bg(0xFFFFFF) ==
         theme_display_color(DARK_THEME.surface_secondary));
  assert(readable_text_color_for_bg(0x000000) == DARK_THEME.text_primary);
  assert(LIGHT_THEME.background == 0xE3E3E3);
  assert(LIGHT_THEME.surface_primary == 0xB0B0B0);
  assert(LIGHT_THEME.surface_secondary == 0xFFFFFF);
  assert(LIGHT_THEME.surface_sensor == 0xF5F5F5);
  assert(LIGHT_THEME.surface_card == 0xFFFFFF);
  assert(LIGHT_THEME.text_primary == 0x333333);
  assert(LIGHT_THEME.clockbar_text == 0x333333);
  assert(LIGHT_THEME.text_muted == 0x606060);
  assert(LIGHT_THEME.text_inverted == 0xFFFFFF);
  assert(LIGHT_THEME.text_disabled == 0x9A9A9A);
  assert(LIGHT_THEME.border == 0xD0D0D0);
  assert(LIGHT_THEME.control_neutral == 0xE0E0E0);
  assert(LIGHT_THEME.track_background == 0xD0D0D0);
  assert(LIGHT_THEME.overlay == 0x000000);
  assert(LIGHT_THEME.setup_action == 0xE0E0E0);
  assert(theme_test_contrast(LIGHT_THEME.clockbar_text, LIGHT_THEME.background) >= 4.5);
  assert(theme_test_contrast(LIGHT_THEME.text_primary, LIGHT_THEME.surface_primary) >= 4.5);
  assert(theme_test_contrast(LIGHT_THEME.text_muted, LIGHT_THEME.surface_card) >= 4.5);
  assert(theme_test_contrast(LIGHT_THEME.text_muted, LIGHT_THEME.surface_secondary) >= 4.5);
  // Disabled text and subtle borders are intentionally lower emphasis; their
  // practical legibility still needs review on the physical displays.

  set_current_button_primary_color(0xAABBCC);
  ThemeSample sample;
  assert(register_theme_refresh(&sample, apply_theme_sample, &sample));
  assert(register_theme_refresh(&sample, apply_theme_sample, &sample));
  ThemeSample others[16];
  for (int i = 0; i < 15; ++i)
    assert(register_theme_refresh(&others[i], apply_theme_sample, &others[i]));
  assert(!register_theme_refresh(&others[15], apply_theme_sample, &others[15]));
  apply_current_theme();
  apply_current_theme();
  assert(sample.applications == 0);  // Dark -> Dark changes no live styles.

  ThemePalette alternate = DARK_THEME;
  alternate.background = 0x123456;
  alternate.surface_primary = 0x445566;
  alternate.surface_secondary = 0x112233;
  alternate.surface_sensor = 0x445566;
  alternate.surface_card = 0x778899;
  alternate.text_primary = 0xEEEEEE;
  set_active_theme_palette(alternate);
  apply_current_theme();
  assert(sample.background == 0x123456 && sample.text == 0xEEEEEE);
  assert(sample.applications == 1);
  assert(sample.previous == &DARK_THEME);
  for (int i = 0; i < 15; ++i) assert(others[i].applications == 1);
  unregister_theme_refresh(&others[0]);
  assert(register_theme_refresh(&others[15], apply_theme_sample, &others[15]));
  apply_current_theme();
  assert(others[15].applications == 1);
  apply_current_theme();
  assert(sample.applications == 1);
  CardPalette card;
  assert(card.on_val == DEFAULT_ACCENT_COLOR);
  assert(card.off_val == theme_display_color(alternate.surface_card));
  assert(card.sensor_val == theme_display_color(alternate.surface_secondary));
  assert(card.surface_sensor_val == theme_display_color(alternate.surface_sensor));
  assert(current_button_primary_color() == 0xAABBCC);
  assert(CARD_ACCENT_TEXT_COLOR == 0xFFFFFF);
  assert(readable_text_color_for_bg(0x000000) == CARD_ACCENT_TEXT_COLOR);
  assert(readable_text_color_for_bg(0xFFFFFF) == CARD_CONTRAST_DARK_COLOR);

  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  assert(sample.background == 0x000000 && sample.text == 0xFFFFFF);
  assert(sample.applications == 2);
  assert(sample.previous == &alternate);
  assert(others[0].applications == 1);
  assert(others[15].applications == 2);
  unregister_theme_refresh(&sample);
  for (int i = 1; i < 16; ++i) unregister_theme_refresh(&others[i]);

  ThemeSample production_switch;
  assert(register_theme_refresh(&production_switch, apply_theme_sample, &production_switch));
  set_active_theme_palette(LIGHT_THEME);
  apply_current_theme();
  assert(&current_theme() == &LIGHT_THEME);
  assert(production_switch.applications == 1);
  assert(production_switch.previous == &DARK_THEME);
  assert(production_switch.background == 0xE3E3E3);
  CardPalette light_card;
  assert(light_card.off_val == theme_display_color(LIGHT_THEME.surface_card));
  assert(light_card.sensor_val == theme_display_color(LIGHT_THEME.surface_secondary));
  assert(light_card.surface_sensor_val == theme_display_color(LIGHT_THEME.surface_sensor));
  assert(light_card.on_val == DEFAULT_ACCENT_COLOR);
  assert(current_button_primary_color() == 0xAABBCC);
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  assert(production_switch.applications == 2);
  assert(production_switch.previous == &LIGHT_THEME);
  assert(production_switch.background == DARK_THEME.background);
  unregister_theme_refresh(&production_switch);
  return 0;
}
