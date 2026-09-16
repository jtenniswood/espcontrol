#pragma once

// The user accent remains separate from the neutral runtime theme.

#include "theme_palette.h"

constexpr uint32_t DEFAULT_ACCENT_COLOR_RAW = 0xFF8C00;
constexpr uint32_t DEFAULT_ACCENT_COLOR = correct_display_color(DEFAULT_ACCENT_COLOR_RAW);
constexpr uint32_t CARD_ACCENT_TEXT_COLOR = 0xFFFFFF;
constexpr uint32_t CARD_CONTRAST_DARK_COLOR = theme_display_color(DARK_THEME.surface_secondary);

constexpr uint32_t readable_text_color_for_bg(uint32_t bg_color) {
  return display_text_color_for_bg(bg_color);
}

static_assert(readable_text_color_for_bg(0xFFFFFF) ==
                  theme_display_color(DARK_THEME.surface_secondary),
              "light backgrounds need dark text");
static_assert(readable_text_color_for_bg(0x000000) ==
                  DARK_THEME.text_primary,
              "dark backgrounds need light text");

// Notification card severity palette. Each background is paired with a fixed
// foreground because the generic brightness heuristic misjudges saturated
// yellow: dark text on the muted yellow reads at about 8.4:1, white text on
// the red at about 6.5:1. These are accessibility pairs, so they stay the same
// in both themes rather than following the active one. The information level
// is "normal card colours" and is read from the active theme where it is used.
// src/webserver/state/ui_tokens.ts repeats these values for the setup page;
// keep both in step.
constexpr uint32_t NOTIFICATION_WARNING_BG = correct_display_color(0xE3B341);
constexpr uint32_t NOTIFICATION_WARNING_TEXT = correct_display_color(0x212121);
constexpr uint32_t NOTIFICATION_ALERT_BG = correct_display_color(0xB3261E);
constexpr uint32_t NOTIFICATION_ALERT_TEXT = 0xFFFFFF;

inline uint32_t &current_button_primary_color_ref() {
  static uint32_t color = DEFAULT_ACCENT_COLOR;
  return color;
}

inline void set_current_button_primary_color(uint32_t color) {
  current_button_primary_color_ref() = color;
}

inline uint32_t current_button_primary_color() {
  return current_button_primary_color_ref();
}

// Card-specific values: the on color belongs to user configuration, while
// neutral defaults are sampled from the active theme when a card is built.
struct CardPalette {
  bool has_on = false;
  bool has_off = false;
  bool has_sensor_color = false;
  bool custom_background = false;
  uint32_t on_val = DEFAULT_ACCENT_COLOR;
  uint32_t off_val = theme_display_color(current_theme().surface_card);
  uint32_t sensor_val = theme_display_color(current_theme().surface_secondary);
  uint32_t surface_sensor_val = theme_display_color(current_theme().surface_sensor);
  uint32_t progress_val = DEFAULT_ACCENT_COLOR;
};
