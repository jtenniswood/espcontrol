#pragma once

#include <cstdint>

// RGB multipliers for display calibration; 100 leaves a channel unchanged.
constexpr int COLOR_CORRECTION_RED_PERCENT = 100;
constexpr int COLOR_CORRECTION_GREEN_PERCENT = 100;
constexpr int COLOR_CORRECTION_BLUE_PERCENT = 100;

constexpr uint32_t clamp_color_channel(uint32_t value) {
  return value > 255 ? 255 : value;
}

constexpr uint32_t correct_display_color(
    uint32_t rgb, int red_percent, int green_percent, int blue_percent) {
  uint32_t red = clamp_color_channel(((rgb >> 16) & 0xFF) * red_percent / 100);
  uint32_t green = clamp_color_channel(((rgb >> 8) & 0xFF) * green_percent / 100);
  uint32_t blue = clamp_color_channel((rgb & 0xFF) * blue_percent / 100);
  return (red << 16) | (green << 8) | blue;
}

constexpr uint32_t display_text_color_for_bg(uint32_t bg_color) {
  const uint32_t red = (bg_color >> 16) & 0xFF;
  const uint32_t green = (bg_color >> 8) & 0xFF;
  const uint32_t blue = bg_color & 0xFF;
  const uint32_t brightness = (red * 299 + green * 587 + blue * 114) / 1000;
  return brightness > 186 ? 0x212121 : 0xFFFFFF;
}

constexpr uint32_t lighter_card_color(uint32_t rgb) {
  const uint32_t red = (rgb >> 16) & 0xFF;
  const uint32_t green = (rgb >> 8) & 0xFF;
  const uint32_t blue = rgb & 0xFF;
  return ((red + ((255 - red) * 30 + 50) / 100) << 16) |
         ((green + ((255 - green) * 30 + 50) / 100) << 8) |
         (blue + ((255 - blue) * 30 + 50) / 100);
}

static_assert(lighter_card_color(0x000000) == 0x4D4D4D,
              "active card colour must round lightened channels consistently");
static_assert(lighter_card_color(0x3F51B5) == 0x7985CB,
              "active card colour must retain the card colour's hue");
static_assert(lighter_card_color(0xFFFFFF) == 0xFFFFFF,
              "white card colour must remain white");

constexpr uint32_t correct_display_color(uint32_t rgb) {
  return correct_display_color(
    rgb, COLOR_CORRECTION_RED_PERCENT, COLOR_CORRECTION_GREEN_PERCENT,
    COLOR_CORRECTION_BLUE_PERCENT);
}

static_assert(correct_display_color(0x123456, 100, 100, 100) == 0x123456,
              "neutral colour correction must not change RGB values");
static_assert(correct_display_color(0x123456, 0, 100, 100) == 0x003456,
              "red correction must only adjust the red channel");
static_assert(correct_display_color(0x123456, 100, 0, 100) == 0x120056,
              "green correction must only adjust the green channel");
static_assert(correct_display_color(0x123456, 100, 100, 0) == 0x123400,
              "blue correction must only adjust the blue channel");
static_assert(correct_display_color(0xF0F0F0, 200, 200, 200) == 0xFFFFFF,
              "colour correction must clamp channels at 255");
