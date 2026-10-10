#include "scanline_resampler.h"

#include <cassert>
#include <cmath>
#include <vector>
#include <limits>

using namespace esphome::artwork_image;

static std::vector<ResampleColor> resize(const std::vector<ResampleColor> &source,
                                       int sw, int sh, int cw, int ch,
                                       int tw, int th, int ox = 0, int oy = 0, bool fast = false, bool cached = false, bool bounded = false) {
  std::vector<ResampleColor> result(tw * th, {7, 11, 13});
  std::vector<uint64_t> memory((ScanlineResampler::workspace_size(tw) + 7) / 8);
  ScanlineResampler scaler;
  assert(scaler.configure(sw, sh, cw, ch, tw, th, ox, oy, memory.data(), memory.size() * 8));
  std::vector<ResampleAxis32> axes(tw);
  if (cached && scaler.fast_dimensions_supported())
    assert(scaler.cache_horizontal_axes(axes.data(), axes.size()));
  std::vector<bool> emitted(tw * th);
  size_t call_pixels = 0;
  for (int y = 0; y < sh; y++) {
    const auto read = [&](int x) {
      assert(x >= 0 && x < sw);
      return source[y * sw + x];
    };
    const auto emit = [&](int x, int dy, ResampleColor color) {
      assert(x >= 0 && x < tw && dy >= 0 && dy < th);
      assert(!emitted[dy * tw + x]);
      emitted[dy * tw + x] = true;
      ++call_pixels;
      result[dy * tw + x] = color;
    };
    if (bounded) {
      ScanlineResampler::RowResult state;
      do {
        call_pixels = 0;
        state = scaler.push_row_fast_bounded(y, read, emit, 2);
        assert(state != ScanlineResampler::RowResult::ERROR);
        assert(call_pixels <= static_cast<size_t>(tw) * 2);
      } while (state == ScanlineResampler::RowResult::MORE);
    } else {
      assert(fast ? scaler.push_row_fast(y, read, emit) : scaler.push_row(y, read, emit));
    }
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
  // Exhaust common normalization divisors and adversarial full-width inputs.
  ResampleDivider32 divider;
  assert(!divider.configure(0));
  uint32_t random = 0x9834512;
  for (uint32_t divisor = 1; divisor <= 65536; ++divisor) {
    assert(divider.configure(divisor));
    for (uint32_t value : {uint32_t{0}, uint32_t{1}, divisor - 1, divisor,
                           std::numeric_limits<uint32_t>::max(),
                           std::numeric_limits<uint32_t>::max() / 2})
      assert(divider.divide(value) == value / divisor);
    for (int sample = 0; sample < 8; ++sample) {
      random = random * 1664525 + 1013904223;
      assert(divider.divide(random) == random / divisor);
    }
  }
  for (uint32_t divisor : {uint32_t{417792}, uint32_t{8388608}, uint32_t{0x80000000},
                           uint32_t{0xfffffffe}, uint32_t{0xffffffff}}) {
    assert(divider.configure(divisor));
    for (int sample = 0; sample < 1000; ++sample) {
      random = random * 1664525 + 1013904223;
      assert(divider.divide(random) == random / divisor);
    }
    assert(divider.divide(std::numeric_limits<uint32_t>::max()) ==
           std::numeric_limits<uint32_t>::max() / divisor);
  }
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
        for (bool cached : {false, true}) for (bool bounded : {false, true}) {
          const auto fast = resize(source, sw, sh, cw, ch, tw, th, offset, -offset, true, cached, bounded);
          assert(!std::memcmp(result.data(), fast.data(), result.size() * sizeof(ResampleColor)));
        }
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

  // A single tiny source row expands to a full display. Each resumable call
  // emits at most two output rows, including the wide-arithmetic fallback.
  for (int source_height : {1, 2}) {
    const std::vector<ResampleColor> source(2 * source_height, {213, 41, 109});
    for (int content_height : {800, 20000}) {
      const auto wide = resize(source, 2, source_height, 1280, content_height, 1280, 800);
      const auto bounded = resize(source, 2, source_height, 1280, content_height, 1280, 800, 0, 0, true, true, true);
      assert(!std::memcmp(wide.data(), bounded.data(), wide.size() * sizeof(ResampleColor)));
    }
  }

  // Large reductions must not overflow or lose the brightness of a flat field.
  const auto white = resize(std::vector<ResampleColor>(65535, {255,255,255}), 65535, 1, 1, 1, 1, 1);
  assert(white[0].r == 255 && white[0].g == 255 && white[0].b == 255);

  // Actual radar tile/expanded geometry, including cropped edge pixels.
  std::vector<ResampleColor> radar(640 * 640);
  for (size_t i = 0; i < radar.size(); ++i) radar[i] = {
    static_cast<uint8_t>(i * 71 + 19), static_cast<uint8_t>(i * 31 + 101),
    static_cast<uint8_t>(i * 53 + 3)};
  for (bool expanded : {false, true}) {
    const int content = expanded ? 792 : 524;
    const int width = expanded ? 790 : 524, height = expanded ? 610 : 403;
    const int x = expanded ? -1 : 0, y = expanded ? -91 : -60;
    const auto wide = resize(radar, 640, 640, content, content, width, height, x, y);
    const auto fast = resize(radar, 640, 640, content, content, width, height, x, y, true, true, true);
    assert(!std::memcmp(wide.data(), fast.data(), wide.size() * sizeof(ResampleColor)));
  }

  // Native-arithmetic bounds, extreme fit dimensions and fallback must
  // preserve the wide filter exactly, including fully bright colour sums.
  for (const auto &dimensions : {std::pair{16384, 1}, std::pair{1, 16384},
                                 std::pair{16385, 1}, std::pair{1, 16385},
                                 std::pair{65535, 1}, std::pair{1, 65535}}) {
    const int sw = dimensions.first, cw = dimensions.second;
    const auto source = std::vector<ResampleColor>(sw, {255,255,255});
    const auto wide = resize(source, sw, 1, cw, 1, cw, 1);
    const auto fast = resize(source, sw, 1, cw, 1, cw, 1, 0, 0, true, true, true);
    assert(!std::memcmp(wide.data(), fast.data(), wide.size() * sizeof(ResampleColor)));
  }
  for (int ch : {16384, 16385}) {
    const auto source = std::vector<ResampleColor>(2, {255,255,255});
    const auto wide = resize(source, 1, 2, 1, ch, 1, ch);
    const auto fast = resize(source, 1, 2, 1, ch, 1, ch, 0, 0, true, true);
    assert(!std::memcmp(wide.data(), fast.data(), wide.size() * sizeof(ResampleColor)));
  }

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
