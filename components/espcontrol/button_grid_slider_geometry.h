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

inline bool vertical_pointer_position(int32_t pointer_y, int32_t top, int32_t bottom,
                                      int32_t position_count, int &position) {
  if (bottom < top || position_count <= 0) return false;

  const int32_t height = bottom - top + 1;
  const int32_t inset = vertical_edge_inset(height);
  const int32_t safe_top = top + inset;
  const int32_t safe_bottom = bottom - inset;
  if (safe_bottom <= safe_top) return false;

  if (pointer_y <= safe_top) {
    position = position_count;
    return true;
  }
  if (pointer_y >= safe_bottom) {
    position = 0;
    return true;
  }

  const int32_t span = safe_bottom - safe_top;
  const int32_t distance_from_bottom = safe_bottom - pointer_y;
  position = static_cast<int>(
    (static_cast<int64_t>(distance_from_bottom) * position_count + span / 2) / span);
  return true;
}

inline bool vertical_pointer_percent(int32_t pointer_y, int32_t top, int32_t bottom,
                                     int &percent) {
  return vertical_pointer_position(pointer_y, top, bottom, 100, percent);
}

}  // namespace slider_geometry
}  // namespace espcontrol
