#pragma once

#include <string>

// Large retains the panel's configured radius. Round smaller presets to the
// same whole pixels in firmware and the setup preview.
inline int card_curve_radius(int base_radius, const std::string &preset,
                             int small_radius = -1, int medium_radius = -1) {
  if (preset == "Small") return small_radius >= 0 ? small_radius : (base_radius + 1) / 2;
  if (preset == "Medium") return medium_radius >= 0 ? medium_radius : (base_radius * 3 + 2) / 4;
  return base_radius;
}
