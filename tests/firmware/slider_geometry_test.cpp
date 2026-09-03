#include <cstdlib>

#include "button_grid_slider_geometry.h"

namespace {

bool maps_to(int32_t pointer_y, int32_t top, int32_t bottom, int expected) {
  int actual = -1;
  return espcontrol::slider_geometry::vertical_pointer_percent(
           pointer_y, top, bottom, actual) && actual == expected;
}

bool maps_position_to(int32_t pointer_y, int32_t top, int32_t bottom,
                      int max_position, int expected) {
  int actual = -1;
  return espcontrol::slider_geometry::vertical_pointer_position(
           pointer_y, top, bottom, max_position, actual) && actual == expected;
}

}  // namespace

int main() {
  using espcontrol::slider_geometry::vertical_edge_inset;
  using espcontrol::slider_geometry::vertical_pointer_percent;
  using espcontrol::slider_geometry::vertical_pointer_position;

  if (vertical_edge_inset(0) != 0) return EXIT_FAILURE;
  if (vertical_edge_inset(20) != 5) return EXIT_FAILURE;
  if (vertical_edge_inset(100) != 8) return EXIT_FAILURE;
  if (vertical_edge_inset(200) != 16) return EXIT_FAILURE;
  if (vertical_edge_inset(500) != 24) return EXIT_FAILURE;

  // A 121 px card has a 10 px inset and a 100 px logical track.
  constexpr int32_t top = 1000;
  constexpr int32_t bottom = 1120;
  if (!maps_to(999, top, bottom, 100)) return EXIT_FAILURE;
  if (!maps_to(1010, top, bottom, 100)) return EXIT_FAILURE;
  if (!maps_to(1011, top, bottom, 99)) return EXIT_FAILURE;
  if (!maps_to(1015, top, bottom, 95)) return EXIT_FAILURE;
  if (!maps_to(1060, top, bottom, 50)) return EXIT_FAILURE;
  if (!maps_to(1105, top, bottom, 5)) return EXIT_FAILURE;
  if (!maps_to(1109, top, bottom, 1)) return EXIT_FAILURE;
  if (!maps_to(1110, top, bottom, 0)) return EXIT_FAILURE;
  if (!maps_to(1121, top, bottom, 0)) return EXIT_FAILURE;

  // Non-integral percentages round to the nearest whole percent.
  if (!maps_to(45, 0, 100, 56)) return EXIT_FAILURE;
  if (!maps_to(46, 0, 100, 55)) return EXIT_FAILURE;

  // Numeric sliders map directly to their full position range instead of first
  // rounding through a whole percentage.
  int position = -1;
  if (!vertical_pointer_position(99, 0, 200, 1000, position)) return EXIT_FAILURE;
  if (position != 506) return EXIT_FAILURE;
  if (!vertical_pointer_position(-1, 0, 200, 1000, position)) return EXIT_FAILURE;
  if (position != 1000) return EXIT_FAILURE;
  if (!vertical_pointer_position(201, 0, 200, 1000, position)) return EXIT_FAILURE;
  if (position != 0) return EXIT_FAILURE;

  // Higher-resolution sliders map directly to their full position range rather
  // than first collapsing the pointer coordinate to a whole percentage.
  if (!maps_position_to(500, 0, 1000, 1000, 500)) return EXIT_FAILURE;
  if (!maps_position_to(499, 0, 1000, 1000, 501)) return EXIT_FAILURE;
  if (!maps_position_to(24, 0, 1000, 1000, 1000)) return EXIT_FAILURE;
  if (!maps_position_to(976, 0, 1000, 1000, 0)) return EXIT_FAILURE;

  int unchanged = 42;
  if (vertical_pointer_percent(0, 10, 9, unchanged)) return EXIT_FAILURE;
  if (unchanged != 42) return EXIT_FAILURE;
  if (vertical_pointer_percent(0, 0, 0, unchanged)) return EXIT_FAILURE;
  if (unchanged != 42) return EXIT_FAILURE;
  if (vertical_pointer_position(0, 0, 100, 0, unchanged)) return EXIT_FAILURE;
  if (vertical_pointer_position(0, 0, 100, -1, unchanged)) return EXIT_FAILURE;
  if (unchanged != 42) return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
