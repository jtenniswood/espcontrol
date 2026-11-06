#include <cassert>
#include <cmath>
#include <initializer_list>
#include <limits>

#include "dpi_timing.h"

using esphome::mipi_dsi::DpiTiming;
using esphome::mipi_dsi::normalize_dpi_timing;

int main() {
  DpiTiming timing{};
  // Real 10.1-inch geometry. Keep its line period while aligning both clocks.
  for (float requested : {50.0f, 60.0f, 70.0f, 80.0f}) {
    assert(normalize_dpi_timing(240, requested, 20, 20, 800, 40, timing));
    const double before = 880.0 / requested;
    const double after = (840.0 + timing.front_porch) / timing.clock_mhz;
    assert(std::abs(before - after) <= 0.5 / timing.clock_mhz + 1e-6);
    const auto divider = std::round(240.0f / timing.clock_mhz);
    assert(240.0f / divider == timing.clock_mhz);
  }
  assert(normalize_dpi_timing(240, 70, 20, 20, 800, 40, timing));
  assert(timing.clock_mhz == 80 && timing.front_porch == 166);
  assert(normalize_dpi_timing(240, 60, 20, 20, 800, 40, timing));
  assert(timing.clock_mhz == 60 && timing.front_porch == 40);
  // Reapplying the workaround (or an upstream fix) cannot compensate twice.
  assert(normalize_dpi_timing(240, 80, 20, 20, 800, 166, timing));
  assert(timing.clock_mhz == 80 && timing.front_porch == 166);
  // A short porch cannot absorb negative compensation; never wrap it unsigned.
  assert(!normalize_dpi_timing(240, 50, 20, 20, 800, 1, timing));
  assert(!normalize_dpi_timing(240, 0, 20, 20, 800, 40, timing));
  assert(!normalize_dpi_timing(0, 70, 20, 20, 800, 40, timing));
  assert(!normalize_dpi_timing(240, std::numeric_limits<float>::quiet_NaN(), 20, 20, 800, 40, timing));
}
