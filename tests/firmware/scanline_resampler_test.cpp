#include "scanline_resampler.h"

#include <cassert>
#include <cmath>
#include <vector>

using namespace esphome::artwork_image;

static std::vector<ResampleColor> resize(const std::vector<ResampleColor> &source,
                                       int sw, int sh, int cw, int ch,
                                       int tw, int th, int ox = 0, int oy = 0) {
  std::vector<ResampleColor> result(tw * th, {7, 11, 13});
  std::vector<uint64_t> memory((ScanlineResampler::workspace_size(tw) + 7) / 8);
  ScanlineResampler scaler;
  assert(scaler.configure(sw, sh, cw, ch, tw, th, ox, oy, memory.data(), memory.size() * 8));
  for (int y = 0; y < sh; y++) {
    assert(scaler.push_row(y, [&](int x) {
      assert(x >= 0 && x < sw);
      return source[y * sw + x];
    }, [&](int x, int dy, ResampleColor color) {
      assert(x >= 0 && x < tw && dy >= 0 && dy < th);
      result[dy * tw + x] = color;
    }));
  }
  return result;
}

// Independent floating-point filter oracle; exercises fractional footprints
// and both axes without relying on the streaming implementation's weights.
static double weight(int s, int d, int i, int j) {
  if (s >= d) {
    const double left = double(j) * s / d, right = double(j + 1) * s / d;
    return std::max(0.0, std::min(right, double(i + 1)) - std::max(left, double(i))) / (right - left);
  }
  const double position = std::clamp((j + 0.5) * s / d - 0.5, 0.0, double(s - 1));
  return std::max(0.0, 1.0 - std::abs(position - i));
}

int main() {
  const std::vector<ResampleColor> checker = {{0,0,0}, {255,255,255}, {255,255,255}, {0,0,0}};
  const auto average = resize(checker, 2, 2, 1, 1, 1, 1);
  assert(average[0].r == 128 && average[0].g == 128 && average[0].b == 128);

  const auto ramp = resize({{0,0,0}, {255,255,255}}, 2, 1, 4, 1, 4, 1);
  assert(ramp[0].r == 0 && ramp[1].r == 64 && ramp[2].r == 191 && ramp[3].r == 255);
  const auto fractional = resize({{0,0,0}, {255,255,255}, {0,0,0}}, 3, 1, 2, 1, 2, 1);
  assert(fractional[0].r == 85 && fractional[1].r == 85);

  // Identity, odd dimensions, mixed up/down scaling, cropping and letterboxes.
  for (int sw = 1; sw <= 8; sw++) for (int sh = 1; sh <= 8; sh++) {
    std::vector<ResampleColor> source(sw * sh);
    for (int i = 0; i < sw * sh; i++) source[i] = {
      static_cast<uint8_t>(i * 71 + 19), static_cast<uint8_t>(i * 31 + 101), static_cast<uint8_t>(i * 53 + 3)};
    for (int cw = 1; cw <= 9; cw++) for (int ch = 1; ch <= 9; ch++) {
      for (int offset : {-1, 0, 1}) {
        if (cw + offset <= 0 || ch - offset <= 0) continue;
        const int tw = cw + 2, th = ch + 2;
        const auto result = resize(source, sw, sh, cw, ch, tw, th, offset, -offset);
        for (int y = 0; y < th; y++) for (int x = 0; x < tw; x++) {
          const auto actual = result[y * tw + x];
          if (x < offset || x >= cw + offset || y < -offset || y >= ch - offset) {
            assert(actual.r == 7 && actual.g == 11 && actual.b == 13);
            continue;
          }
          double r = 0, g = 0, b = 0;
          for (int sy = 0; sy < sh; sy++) for (int sx = 0; sx < sw; sx++) {
            const auto color = source[sy * sw + sx];
            const double w = weight(sw, cw, sx, x - offset) * weight(sh, ch, sy, y + offset);
            r += color.r * w; g += color.g * w; b += color.b * w;
          }
          assert(std::abs(actual.r - r) <= 0.51);
          assert(std::abs(actual.g - g) <= 0.51);
          assert(std::abs(actual.b - b) <= 0.51);
        }
      }
    }
  }

  // Large reductions must not overflow or lose the brightness of a flat field.
  const auto white = resize(std::vector<ResampleColor>(65535, {255,255,255}), 65535, 1, 1, 1, 1, 1);
  assert(white[0].r == 255 && white[0].g == 255 && white[0].b == 255);

  ScanlineResampler invalid;
  uint64_t workspace[32]{};
  assert(!invalid.configure(0, 1, 1, 1, 1, 1, 0, 0, workspace, sizeof(workspace)));
  assert(!invalid.configure(1, 1, 1, 1, 1, 1, 0, 0, nullptr, sizeof(workspace)));
  assert(!invalid.configure(1, 1, 1, 1, 1, 1, 0, 0, workspace, 1));

  // These half-size decodes used to be chosen and immediately enlarged again.
  assert(!jpeg_decode_size_sufficient(960, 540, 1280, 800, false));
  assert(!jpeg_decode_size_sufficient(800, 450, 1024, 600, false));
  assert(jpeg_decode_size_sufficient(1920, 1080, 1280, 800, false));
  assert(jpeg_decode_size_sufficient(480, 270, 480, 480, false));
  assert(!jpeg_decode_size_sufficient(480, 270, 480, 480, true));
  assert(jpeg_decode_size_sufficient(960, 540, 480, 480, true));
  assert(jpeg_decode_size_sufficient(270, 480, 480, 480, false));
}
