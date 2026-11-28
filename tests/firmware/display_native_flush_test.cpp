#include <cassert>

#include "display_native_flush.h"

using espcontrol::display_native_flush::Region;
using espcontrol::display_native_flush::plan_clockwise_region;

int main() {
  constexpr size_t quarter = 512000;
  Region result{};
  assert(plan_clockwise_region({0, 0, 1280, 200}, 800, 1280, quarter, result));
  assert(result.x == 600 && result.y == 0 && result.width == 200 && result.height == 1280);
  assert(plan_clockwise_region({1110, 620, 170, 180}, 800, 1280, quarter, result));
  assert(result.x == 0 && result.y == 1110 && result.width == 180 && result.height == 170);
  assert(!plan_clockwise_region({0, 0, 1280, 202}, 800, 1280, quarter, result));
  assert(!plan_clockwise_region({0, 0, 800, 127}, 800, 1280, quarter, result));
  assert(!plan_clockwise_region({1200, 0, 100, 100}, 800, 1280, quarter, result));
  assert(!plan_clockwise_region({0, 750, 100, 100}, 800, 1280, quarter, result));
  assert(!plan_clockwise_region({-1, 0, 100, 100}, 800, 1280, quarter, result));
}
