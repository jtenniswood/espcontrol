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

// sRGB channels linearized with the WCAG formula, scaled by 1,000,000.
// The lookup keeps contrast selection constexpr and avoids runtime exponentiation.
// https://www.w3.org/WAI/WCAG22/Techniques/general/G18
constexpr uint32_t SRGB_LINEAR_CHANNEL[256] = {
  0, 304, 607, 911, 1214, 1518, 1821, 2125,
  2428, 2732, 3035, 3347, 3677, 4025, 4391, 4777,
  5182, 5605, 6049, 6512, 6995, 7499, 8023, 8568,
  9134, 9721, 10330, 10960, 11612, 12286, 12983, 13702,
  14444, 15209, 15996, 16807, 17642, 18500, 19382, 20289,
  21219, 22174, 23153, 24158, 25187, 26241, 27321, 28426,
  29557, 30713, 31896, 33105, 34340, 35601, 36889, 38204,
  39546, 40915, 42311, 43735, 45186, 46665, 48172, 49707,
  51269, 52861, 54480, 56128, 57805, 59511, 61246, 63010,
  64803, 66626, 68478, 70360, 72272, 74214, 76185, 78187,
  80220, 82283, 84376, 86500, 88656, 90842, 93059, 95307,
  97587, 99899, 102242, 104616, 107023, 109462, 111932, 114435,
  116971, 119538, 122139, 124772, 127438, 130136, 132868, 135633,
  138432, 141263, 144128, 147027, 149960, 152926, 155926, 158961,
  162029, 165132, 168269, 171441, 174647, 177888, 181164, 184475,
  187821, 191202, 194618, 198069, 201556, 205079, 208637, 212231,
  215861, 219526, 223228, 226966, 230740, 234551, 238398, 242281,
  246201, 250158, 254152, 258183, 262251, 266356, 270498, 274677,
  278894, 283149, 287441, 291771, 296138, 300544, 304987, 309469,
  313989, 318547, 323143, 327778, 332452, 337164, 341914, 346704,
  351533, 356400, 361307, 366253, 371238, 376262, 381326, 386429,
  391572, 396755, 401978, 407240, 412543, 417885, 423268, 428690,
  434154, 439657, 445201, 450786, 456411, 462077, 467784, 473531,
  479320, 485150, 491021, 496933, 502886, 508881, 514918, 520996,
  527115, 533276, 539479, 545724, 552011, 558340, 564712, 571125,
  577580, 584078, 590619, 597202, 603827, 610496, 617207, 623960,
  630757, 637597, 644480, 651406, 658375, 665387, 672443, 679542,
  686685, 693872, 701102, 708376, 715694, 723055, 730461, 737910,
  745404, 752942, 760525, 768151, 775822, 783538, 791298, 799103,
  806952, 814847, 822786, 830770, 838799, 846873, 854993, 863157,
  871367, 879622, 887923, 896269, 904661, 913099, 921582, 930111,
  938686, 947307, 955973, 964686, 973445, 982251, 991102, 1000000,
};

constexpr uint32_t display_relative_luminance(uint32_t rgb) {
  return (static_cast<uint64_t>(SRGB_LINEAR_CHANNEL[(rgb >> 16) & 0xFF]) * 2126 +
          static_cast<uint64_t>(SRGB_LINEAR_CHANNEL[(rgb >> 8) & 0xFF]) * 7152 +
          static_cast<uint64_t>(SRGB_LINEAR_CHANNEL[rgb & 0xFF]) * 722) / 10000;
}

constexpr uint32_t display_text_color_for_bg(uint32_t bg_color) {
  // Prefer white on mid-tone card colours; switch to dark grey on pale fills.
  // Match cardPreviewTextColor (luminance scaled by 1,000,000).
  return display_relative_luminance(bg_color) < 450000 ? 0xFFFFFF : 0x212121;
}

// Progress fill direction is independent of the foreground contrast choice.
constexpr bool display_progress_uses_dark_fill(uint32_t bg_color) {
  const uint32_t red = (bg_color >> 16) & 0xFF;
  const uint32_t green = (bg_color >> 8) & 0xFF;
  const uint32_t blue = bg_color & 0xFF;
  const bool blue_green_tone = green >= red + 32 && blue >= red + 32;
  const uint32_t crossover = blue_green_tone ? 550000 : 450000;
  return display_relative_luminance(bg_color) >= crossover;
}

constexpr uint32_t lighter_card_color(uint32_t rgb) {
  const uint32_t red = (rgb >> 16) & 0xFF;
  const uint32_t green = (rgb >> 8) & 0xFF;
  const uint32_t blue = rgb & 0xFF;
  return ((red + ((255 - red) * 30 + 50) / 100) << 16) |
         ((green + ((255 - green) * 30 + 50) / 100) << 8) |
         (blue + ((255 - blue) * 30 + 50) / 100);
}

// Give custom cards a distinct on state, including pale colours and white.
// Match activeCardColor in the browser before applying display correction.
constexpr uint32_t active_card_color(uint32_t rgb) {
  const uint32_t red = (rgb >> 16) & 0xFF;
  const uint32_t green = (rgb >> 8) & 0xFF;
  const uint32_t blue = rgb & 0xFF;
  const bool pale = red * 299 + green * 587 + blue * 114 >= 160000;
  const auto active_channel = [pale](uint32_t channel) {
    return pale ? (channel * 75 + 50) / 100
                : channel + ((255 - channel) * 40 + 50) / 100;
  };
  return (active_channel(red) << 16) | (active_channel(green) << 8) |
         active_channel(blue);
}

// Slider labels span the base and fill without changing checked state.
// Keep the active shade only when the base foreground remains readable.
constexpr uint32_t card_slider_fill_color(uint32_t rgb) {
  const uint32_t active = active_card_color(rgb);
  const uint32_t foreground = display_text_color_for_bg(rgb);
  const uint32_t text = display_relative_luminance(foreground);
  const auto readable = [text](uint32_t color) {
    const uint32_t fill = display_relative_luminance(color);
    const uint32_t light = fill > text ? fill : text;
    const uint32_t dark = fill > text ? text : fill;
    return uint64_t(light + 50000) * 1000 >= uint64_t(dark + 50000) * 4500;
  };
  if (active != rgb && readable(active)) return active;
  // Move the fill away from the foreground until it reaches 4.5:1.
  for (uint32_t amount = 10; amount <= 100; amount += 10) {
    const auto channel = [foreground, amount](uint32_t value) {
      return foreground == 0xFFFFFF ? (value * (100 - amount) + 50) / 100
        : value + ((255 - value) * amount + 50) / 100;
    };
    const uint32_t shade = (channel((active >> 16) & 255) << 16) |
      (channel((active >> 8) & 255) << 8) | channel(active & 255);
    if (shade != rgb && readable(shade)) return shade;
  }
  return active;
}

// Stripes must preserve the foreground contrast of the active background.
constexpr uint32_t card_pattern_highlight_color(uint32_t rgb) {
  const bool white_text = display_text_color_for_bg(rgb) == 0xFFFFFF;
  const auto channel = [white_text](uint32_t value) {
    return white_text ? value * 65 / 100 : value + (255 - value) * 35 / 100;
  };
  return (channel((rgb >> 16) & 255) << 16) |
         (channel((rgb >> 8) & 255) << 8) | channel(rgb & 255);
}

// Progress must stay visible on both dark and pale custom backgrounds.
constexpr uint32_t card_progress_color(uint32_t rgb) {
  if (!display_progress_uses_dark_fill(rgb)) return lighter_card_color(rgb);
  return (((((rgb >> 16) & 0xFF) * 70 + 50) / 100) << 16) |
         (((((rgb >> 8) & 0xFF) * 70 + 50) / 100) << 8) |
         (((rgb & 0xFF) * 70 + 50) / 100);
}

static_assert(card_progress_color(0x00BCD4) == 0x4DD0E1,
              "dark progress backgrounds use a lighter fill");
static_assert(card_progress_color(0xFFFFFF) == 0xB3B3B3,
              "white progress backgrounds need a darker fill");
static_assert(card_progress_color(0xFFEC16) == 0xB3A50F,
              "pale progress backgrounds use a darker shade of the same hue");

static_assert(lighter_card_color(0x000000) == 0x4D4D4D,
              "lighter card colour must round channels consistently");
static_assert(lighter_card_color(0x3F51B5) == 0x7985CB,
              "lighter card colour must retain the card colour's hue");
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
