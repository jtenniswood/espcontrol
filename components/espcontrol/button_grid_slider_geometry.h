#pragma once

#include <cstdint>

namespace espcontrol {
namespace slider_geometry {

inline int32_t vertical_edge_inset(int32_t height) {
  if (height <= 0) return 0;

  int32_t inset = (height * 8 + 50) / 100;
  if (inset < 8) inset = 8;
  if (inset > 24) inset = 24;

  const int32_t small_slider_limit = height / 4;
  if (inset > small_slider_limit) inset = small_slider_limit;
  return inset;
}

inline bool vertical_pointer_percent(int32_t pointer_y, int32_t top, int32_t bottom,
                                     int &percent) {
  if (bottom < top) return false;

  const int32_t height = bottom - top + 1;
  const int32_t inset = vertical_edge_inset(height);
  const int32_t safe_top = top + inset;
  const int32_t safe_bottom = bottom - inset;
  if (safe_bottom <= safe_top) return false;

  if (pointer_y <= safe_top) {
    percent = 100;
    return true;
  }
  if (pointer_y >= safe_bottom) {
    percent = 0;
    return true;
  }

  const int32_t span = safe_bottom - safe_top;
  const int32_t distance_from_bottom = safe_bottom - pointer_y;
  percent = static_cast<int>((distance_from_bottom * 100 + span / 2) / span);
  return true;
}

}  // namespace slider_geometry
}  // namespace espcontrol
