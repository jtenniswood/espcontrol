#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace espcontrol::corners {

// Continuous superellipses meet straight edges with zero curvature. Cards use
// a fourth-power profile; compact pills use a softer 2.5-power profile. The
// extent is twice the nominal radius for both.
inline uint8_t coverage(int x, int y, int extent, bool pill = false) {
  constexpr int samples = 4;
  int inside = 0;
  for (int sy = 0; sy < samples; ++sy) {
    const float dy = 1.0f - (y + (sy + 0.5f) / samples) / extent;
    const float dy2 = dy * dy;
    for (int sx = 0; sx < samples; ++sx) {
      const float dx = 1.0f - (x + (sx + 0.5f) / samples) / extent;
      const float dx2 = dx * dx;
      // Badges use a softer 2.5-power curve: closer to a pill than the
      // fourth-power card outline, while retaining a flattened end.
      inside += pill ? dx2 * std::sqrt(dx) + dy2 * std::sqrt(dy) <= 1.0f
                     : dx2 * dx2 + dy2 * dy2 <= 1.0f;
    }
  }
  return static_cast<uint8_t>((inside * 255 + samples * samples / 2) / (samples * samples));
}

inline void fill_mask(uint8_t *pixels, int width, int height, int stride, int radius, bool pill = false) {
  std::memset(pixels, 255, static_cast<size_t>(stride) * height);
  const int extent = std::min({radius * 2, width / 2, height / 2});
  for (int y = 0; y < extent; ++y) {
    for (int x = 0; x < extent; ++x) {
      const uint8_t alpha = coverage(x, y, extent, pill);
      pixels[y * stride + x] = alpha;
      pixels[y * stride + width - x - 1] = alpha;
      pixels[(height - y - 1) * stride + x] = alpha;
      pixels[(height - y - 1) * stride + width - x - 1] = alpha;
    }
  }
}

}  // namespace espcontrol::corners
