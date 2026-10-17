#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace esphome::mipi_dsi {

struct DpiTiming {
  float clock_mhz;
  uint32_t front_porch;
};

// ESP-IDF rounds the DPI divider, then compensates the bridge's front porch.
// Supplying that achievable clock and compensated porch ourselves keeps the
// host and bridge on the same timing without changing the intended frame rate.
// See espressif/esp-idf#19188 (host timing used the requested clock instead).
inline bool normalize_dpi_timing(float source_mhz, float requested_mhz, uint32_t sync, uint32_t back_porch,
                                 uint32_t width, uint32_t front_porch, DpiTiming &timing) {
  if (!std::isfinite(source_mhz) || !std::isfinite(requested_mhz) || source_mhz <= 0 || requested_mhz <= 0)
    return false;
  const float divider = std::round(source_mhz / requested_mhz);
  if (divider < 1) return false;
  const float actual_mhz = source_mhz / divider;
  const uint64_t fixed_pixels = uint64_t(sync) + back_porch + width;
  const double total = std::round(double(actual_mhz) / requested_mhz * (fixed_pixels + front_porch));
  if (total < fixed_pixels || total - fixed_pixels > std::numeric_limits<uint16_t>::max()) return false;
  timing = {actual_mhz, static_cast<uint32_t>(total - fixed_pixels)};
  return true;
}

}  // namespace esphome::mipi_dsi
