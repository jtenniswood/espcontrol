#pragma once

#include <string>

// Large retains the panel's configured radius. Round smaller presets to the
// same whole pixels in firmware and the setup preview.
inline int card_curve_radius(int base_radius, const std::string &preset) {
  if (preset == "Small") return (base_radius + 1) / 2;
  if (preset == "Medium") return (base_radius * 3 + 2) / 4;
  return base_radius;
}
