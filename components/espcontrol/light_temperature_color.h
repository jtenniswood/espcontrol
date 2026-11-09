#pragma once
#include <cstdint>
namespace espcontrol::light_temperature {
// Absolute 2000..6500 K gradient; configured slider ranges do not stretch it.
inline uint32_t fill_color(int kelvin) {
  if (kelvin <= 2000) return 0xFF8012;
  if (kelvin >= 6500) return 0xB8CCFF;
  const float t = static_cast<float>(kelvin - 2000) / 4500.0f;
  const auto r = static_cast<uint8_t>(0xFF + t * static_cast<float>(0xB8 - 0xFF) + 0.5f);
  const auto g = static_cast<uint8_t>(0x80 + t * static_cast<float>(0xCC - 0x80) + 0.5f);
  const auto b = static_cast<uint8_t>(0x12 + t * static_cast<float>(0xFF - 0x12) + 0.5f);
  return static_cast<uint32_t>(r) << 16 | static_cast<uint32_t>(g) << 8 | b;
}
}  // namespace espcontrol::light_temperature
