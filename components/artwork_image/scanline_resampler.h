#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome {
namespace artwork_image {

struct ResampleColor {
  uint8_t r, g, b;
};

// Area averaging when reducing an image; centre-aligned linear interpolation
// when enlarging it. Integer weights sum exactly to total, including edges.
struct ResampleAxis {
  int first, last;
  int64_t begin, end, total;
  int source_size, target_size;

  static ResampleAxis at(int source, int target, int pixel) {
    if (source >= target) {
      const int64_t begin = static_cast<int64_t>(pixel) * source;
      const int64_t end = begin + source;
      return {static_cast<int>(begin / target), static_cast<int>((end - 1) / target),
              begin, end, source, source, target};
    }
    const int64_t total = static_cast<int64_t>(target) * 2;
    const int64_t position = std::clamp(
        (static_cast<int64_t>(pixel) * 2 + 1) * source - target,
        int64_t{0}, static_cast<int64_t>(source - 1) * total);
    const int first = position / total;
    const int64_t fraction = position % total;
    return {first, first + (fraction != 0), fraction, 0, total, source, target};
  }

  uint64_t weight(int pixel) const {
    if (source_size < target_size) return pixel == first ? total - begin : begin;
    return std::min(end, static_cast<int64_t>(pixel + 1) * target_size) -
           std::max(begin, static_cast<int64_t>(pixel) * target_size);
  }
};

// Consumes source rows in order, so software JPEG decoding needs no full-size
// intermediate image. Workspace is two horizontal rows plus one accumulator,
// proportional to the visible target width (about 17 KiB at 480 pixels).
class ScanlineResampler {
 public:
  static size_t workspace_size(int visible_width) {
    return visible_width > 0 ? static_cast<size_t>(visible_width) * 3 *
                                  (sizeof(uint64_t) + 2 * sizeof(uint16_t)) : 0;
  }

  bool configure(int source_width, int source_height, int content_width, int content_height,
                 int target_width, int target_height, int offset_x, int offset_y,
                 void *workspace, size_t workspace_bytes) {
    if (source_width <= 0 || source_height <= 0 || content_width <= 0 || content_height <= 0 ||
        target_width <= 0 || target_height <= 0) return false;
    x0_ = std::max(0, offset_x);
    x1_ = std::min<int64_t>(target_width, static_cast<int64_t>(offset_x) + content_width);
    y_ = std::max(0, offset_y);
    y1_ = std::min<int64_t>(target_height, static_cast<int64_t>(offset_y) + content_height);
    if (x1_ <= x0_ || y1_ <= y_ || !workspace ||
        workspace_bytes < workspace_size(x1_ - x0_)) return false;
    source_width_ = source_width;
    source_height_ = source_height;
    content_width_ = content_width;
    content_height_ = content_height;
    offset_x_ = offset_x;
    offset_y_ = offset_y;
    channels_ = static_cast<size_t>(x1_ - x0_) * 3;
    sum_ = static_cast<uint64_t *>(workspace);
    rows_[0] = reinterpret_cast<uint16_t *>(sum_ + channels_);
    rows_[1] = rows_[0] + channels_;
    std::memset(sum_, 0, channels_ * sizeof(uint64_t));
    accumulated_ = false;
    next_source_y_ = 0;
    return true;
  }

  // read_pixel(x) returns an RGB888 colour from the current source row.
  // emit(x, y, colour) writes directly into the already-allocated target.
  template<typename ReadPixel, typename Emit>
  bool push_row(int source_y, ReadPixel read_pixel, Emit emit) {
    if (!sum_ || source_y != next_source_y_ || source_y >= source_height_) return false;
    next_source_y_++;
    if (y_ >= y1_) return true;
    auto vertical = ResampleAxis::at(source_height_, content_height_, y_ - offset_y_);
    if (source_y < vertical.first) return true;

    uint16_t *row = rows_[source_y % 2];
    for (int x = x0_; x < x1_; x++) {
      const auto horizontal = ResampleAxis::at(source_width_, content_width_, x - offset_x_);
      uint64_t r = 0, g = 0, b = 0;
      for (int sx = horizontal.first; sx <= horizontal.last; sx++) {
        const auto color = read_pixel(sx);
        const auto weight = horizontal.weight(sx);
        r += color.r * weight;
        g += color.g * weight;
        b += color.b * weight;
      }
      const size_t index = static_cast<size_t>(x - x0_) * 3;
      row[index] = (r * 256 + horizontal.total / 2) / horizontal.total;
      row[index + 1] = (g * 256 + horizontal.total / 2) / horizontal.total;
      row[index + 2] = (b * 256 + horizontal.total / 2) / horizontal.total;
    }

    while (y_ < y1_ && vertical.first <= source_y) {
      // Adjacent enlarged output rows can share the previous source row.
      if (!accumulated_ && vertical.first < source_y) {
        if (vertical.first != source_y - 1 || source_height_ >= content_height_) return false;
        accumulate_(rows_[(source_y - 1) % 2], vertical.weight(source_y - 1));
      }
      accumulate_(row, vertical.weight(source_y));
      accumulated_ = true;
      if (source_y < vertical.last) break;
      const uint64_t divisor = static_cast<uint64_t>(vertical.total) * 256;
      for (int x = x0_; x < x1_; x++) {
        const size_t index = static_cast<size_t>(x - x0_) * 3;
        emit(x, y_, ResampleColor{
            static_cast<uint8_t>((sum_[index] + divisor / 2) / divisor),
            static_cast<uint8_t>((sum_[index + 1] + divisor / 2) / divisor),
            static_cast<uint8_t>((sum_[index + 2] + divisor / 2) / divisor)});
      }
      std::memset(sum_, 0, channels_ * sizeof(uint64_t));
      accumulated_ = false;
      y_++;
      if (y_ < y1_) vertical = ResampleAxis::at(source_height_, content_height_, y_ - offset_y_);
    }
    return true;
  }

 private:
  void accumulate_(const uint16_t *row, uint64_t weight) {
    for (size_t i = 0; i < channels_; i++) sum_[i] += row[i] * weight;
  }
  int source_width_{0}, source_height_{0}, content_width_{0}, content_height_{0};
  int offset_x_{0}, offset_y_{0}, x0_{0}, x1_{0}, y_{0}, y1_{0}, next_source_y_{0};
  size_t channels_{0};
  uint64_t *sum_{nullptr};
  uint16_t *rows_[2]{nullptr, nullptr};
  bool accumulated_{false};
};

// A reduced JPEG must still have enough pixels for the fitted content, not
// necessarily the letterboxed canvas. If even the original is too small,
// retain the original rather than reducing it further.
constexpr bool jpeg_decode_size_sufficient(int width, int height, int target_width,
                                           int target_height, bool cover) {
  return cover ? width >= target_width && height >= target_height
               : width >= target_width || height >= target_height;
}

}  // namespace artwork_image
}  // namespace esphome
