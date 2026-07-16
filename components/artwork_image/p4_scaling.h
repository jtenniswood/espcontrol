#pragma once

#include <cstdint>

namespace esphome {
namespace artwork_image {
namespace p4 {

static constexpr uint32_t SCALE_FRACTION_STEPS = 16;
static constexpr uint32_t MAX_SCALE_STEPS = 16 * SCALE_FRACTION_STEPS;

// PPA stores each scale factor as an integer plus a four-bit fraction. Return
// the exact hardware step count, or zero when the requested output size would
// be rounded or is outside the supported [1/16, 16) range.
constexpr uint32_t exact_scale_steps(uint32_t input_size, uint32_t output_size) {
  if (input_size == 0 || output_size == 0) return 0;
  uint64_t numerator = static_cast<uint64_t>(output_size) * SCALE_FRACTION_STEPS;
  if (numerator % input_size != 0) return 0;
  uint64_t steps = numerator / input_size;
  return steps > 0 && steps < MAX_SCALE_STEPS ? static_cast<uint32_t>(steps) : 0;
}

}  // namespace p4
}  // namespace artwork_image
}  // namespace esphome
