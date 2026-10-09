#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome::artwork_image::gif {

// A bounded GIF87a/89a reader. The caller owns the compressed data and two
// RGB565 canvases. No file field can request an allocation or grow the LZW table.
class Player {
 public:
  enum class Result { MORE, FRAME, END, ERROR };
  static constexpr size_t MAX_PIXELS = 640 * 640;
  // The transfer pipeline applies the tighter 2 MiB limit on ESP32-S3.
  static constexpr size_t MAX_BYTES = 8 * 1024 * 1024;
  static constexpr uint32_t MIN_DELAY_MS = 100;

  static bool dimensions(const uint8_t *data, size_t size, uint16_t &width, uint16_t &height) {
    if (!data || size < 13 || size > MAX_BYTES ||
        (std::memcmp(data, "GIF87a", 6) && std::memcmp(data, "GIF89a", 6))) return false;
    width = data[6] | (data[7] << 8);
    height = data[8] | (data[9] << 8);
    return width && height && width <= 1024 && height <= 1024 &&
           static_cast<size_t>(width) * height <= MAX_PIXELS;
  }

  bool open(const uint8_t *data, size_t size, uint16_t *canvas, uint16_t *restore) {
    if (!dimensions(data, size, width_, height_) || !canvas || !restore) return false;
    data_ = data; size_ = size; canvas_ = canvas; restore_ = restore; pos_ = 13;
    if (data[10] & 0x80) {
      global_colors_ = 1u << ((data[10] & 7) + 1);
      if (!palette(global_palette_, global_colors_)) return false;
    }
    background_ = data[11] < global_colors_ ? global_palette_[data[11]] : 0;
    start_ = pos_;
    std::fill_n(canvas_, static_cast<size_t>(width_) * height_, background_);
    return true;
  }

  uint16_t width() const { return width_; }
  uint16_t height() const { return height_; }
  const uint16_t *canvas() const { return canvas_; }
  uint32_t delay_ms() const { return std::max(MIN_DELAY_MS, static_cast<uint32_t>(delay_) * 10); }

  Result step(size_t pixel_budget = 4096) {
    if (error_) return Result::ERROR;
    if (ended_) return Result::END;
    if (!decoding_) {
      auto result = begin_frame();
      if (result != Result::MORE) return result;
    }
    size_t work_budget = pixel_budget * 2;
    while (pixel_budget && work_budget--) {
      if (stack_size_) {
        const uint8_t index = stack_[--stack_size_];
        if (pixels_ >= static_cast<size_t>(frame_width_) * frame_height_ || index >= colors_) return fail();
        if (!transparent_ || index != transparent_index_)
          canvas_[static_cast<size_t>(top_ + row_) * width_ + left_ + column_] = local_palette_[index];
        ++pixels_; --pixel_budget;
        if (++column_ == frame_width_) {
          column_ = 0;
          if (!interlaced_) ++row_;
          else {
            static constexpr uint16_t starts[] = {0, 4, 2, 1};
            static constexpr uint16_t strides[] = {8, 8, 4, 2};
            row_ += strides[pass_];
            while (row_ >= frame_height_ && pass_ < 3) row_ = starts[++pass_];
          }
        }
        continue;
      }
      uint16_t code;
      if (!read_code(code)) return fail();
      if (code == clear_) {
        code_bits_ = min_bits_ + 1; next_ = clear_ + 2; previous_ = -1;
        continue;
      }
      if (code == clear_ + 1) {
        if (pixels_ != static_cast<size_t>(frame_width_) * frame_height_ || !finish_blocks()) return fail();
        decoding_ = false;
        ++frames_;
        return Result::FRAME;
      }
      if (code > next_ || code >= 4096 || (previous_ < 0 && code >= clear_)) return fail();
      const uint16_t original = code;
      if (code == next_) {
        if (previous_ < 0 || !push(first_)) return fail();
        code = static_cast<uint16_t>(previous_);
      }
      while (code >= clear_) {
        if (code >= next_ || !push(suffix_[code]) || prefix_[code] >= code) return fail();
        code = prefix_[code];
      }
      first_ = static_cast<uint8_t>(code);
      if (!push(first_)) return fail();
      if (previous_ >= 0 && next_ < 4096) {
        prefix_[next_] = static_cast<uint16_t>(previous_);
        suffix_[next_++] = first_;
        if (next_ == (1u << code_bits_) && code_bits_ < 12) ++code_bits_;
      }
      previous_ = original;
    }
    return Result::MORE;
  }

 private:
  Result fail() { error_ = true; return Result::ERROR; }
  bool byte(uint8_t &value) {
    if (pos_ >= size_) return false;
    value = data_[pos_++]; return true;
  }
  bool word(uint16_t &value) {
    if (size_ - pos_ < 2) return false;
    value = data_[pos_] | (data_[pos_ + 1] << 8); pos_ += 2; return true;
  }
  bool skip(size_t count) {
    if (count > size_ - pos_) return false;
    pos_ += count; return true;
  }
  bool blocks() {
    uint8_t count;
    while (byte(count)) {
      if (!count) return true;
      if (!skip(count)) return false;
    }
    return false;
  }
  bool palette(std::array<uint16_t, 256> &table, size_t count) {
    if (count * 3 > size_ - pos_) return false;
    for (size_t i = 0; i < count; ++i) {
      const auto *rgb = data_ + pos_; pos_ += 3;
      table[i] = ((rgb[0] & 0xf8) << 8) | ((rgb[1] & 0xfc) << 3) | (rgb[2] >> 3);
    }
    return true;
  }
  Result begin_frame() {
    // Disposal applies before the following image, not while displaying this one.
    if (disposal_ == 2) {
      for (size_t y = top_; y < static_cast<size_t>(top_) + frame_height_; ++y)
        std::fill_n(canvas_ + y * width_ + left_, frame_width_, background_);
    } else if (disposal_ == 3) {
      std::memcpy(canvas_, restore_, static_cast<size_t>(width_) * height_ * 2);
    }
    uint8_t disposal = 0, transparent_index = 0;
    uint16_t delay = 0;
    bool transparent = false;
    uint8_t marker;
    while (byte(marker)) {
      if (marker == 0x3b) {
        if (!frames_) return fail();
        if (!loop_configured_ || (repeats_ && completed_loops_ >= repeats_)) {
          ended_ = true; return Result::END;
        }
        ++completed_loops_;
        pos_ = start_; frames_ = 0;
        std::fill_n(canvas_, static_cast<size_t>(width_) * height_, background_);
        continue;
      }
      if (marker == 0x21) {
        uint8_t label;
        if (!byte(label)) return fail();
        if (label == 0xf9) {
          uint8_t length, packed, terminator;
          if (!byte(length) || length != 4 || !byte(packed) || !word(delay) ||
              !byte(transparent_index) || !byte(terminator) || terminator) return fail();
          disposal = (packed >> 2) & 7;
          if (disposal > 3) return fail();
          transparent = packed & 1;
        } else if (label == 0xff) {
          uint8_t length;
          if (!byte(length) || length > size_ - pos_) return fail();
          bool netscape = length == 11 && (!std::memcmp(data_ + pos_, "NETSCAPE2.0", 11) ||
                                                   !std::memcmp(data_ + pos_, "ANIMEXTS1.0", 11));
          if (!skip(length)) return fail();
          if (netscape) {
            uint8_t count, id, end; uint16_t repeats;
            if (!byte(count) || count != 3 || !byte(id) || id != 1 || !word(repeats) ||
                !byte(end) || end) return fail();
            if (!loop_configured_) { repeats_ = repeats; loop_configured_ = true; }
          } else if (!blocks()) return fail();
        } else if (label == 0xfe) {
          if (!blocks()) return fail();
        } else {
          // Plain-text graphics cannot be represented by a camera snapshot.
          return fail();
        }
        continue;
      }
      if (marker != 0x2c || frames_ >= 512) return fail();
      uint8_t packed;
      if (!word(left_) || !word(top_) || !word(frame_width_) || !word(frame_height_) || !byte(packed) ||
          !frame_width_ || !frame_height_ || static_cast<size_t>(left_) + frame_width_ > width_ ||
          static_cast<size_t>(top_) + frame_height_ > height_) return fail();
      colors_ = global_colors_; local_palette_ = global_palette_;
      if (packed & 0x80) {
        colors_ = 1u << ((packed & 7) + 1);
        if (!palette(local_palette_, colors_)) return fail();
      }
      if (!colors_ || !byte(min_bits_) || min_bits_ < 2 || min_bits_ > 8) return fail();
      disposal_ = disposal; delay_ = delay; transparent_ = transparent;
      transparent_index_ = transparent_index; interlaced_ = packed & 0x40;
      if (disposal_ == 3) std::memcpy(restore_, canvas_, static_cast<size_t>(width_) * height_ * 2);
      clear_ = 1u << min_bits_; next_ = clear_ + 2; code_bits_ = min_bits_ + 1;
      previous_ = -1; stack_size_ = 0; bit_buffer_ = 0; bit_count_ = 0;
      block_remaining_ = 0; blocks_ended_ = false; pixels_ = 0;
      column_ = 0; row_ = 0; pass_ = 0; decoding_ = true;
      return Result::MORE;
    }
    return fail();
  }
  bool push(uint8_t value) {
    if (stack_size_ == stack_.size()) return false;
    stack_[stack_size_++] = value; return true;
  }
  bool read_code(uint16_t &code) {
    while (bit_count_ < code_bits_) {
      if (!block_remaining_) {
        if (!byte(block_remaining_) || !block_remaining_) { blocks_ended_ = true; return false; }
        if (block_remaining_ > size_ - pos_) return false;
      }
      uint8_t value;
      if (!byte(value)) return false;
      --block_remaining_;
      bit_buffer_ |= static_cast<uint32_t>(value) << bit_count_; bit_count_ += 8;
    }
    code = bit_buffer_ & ((1u << code_bits_) - 1);
    bit_buffer_ >>= code_bits_; bit_count_ -= code_bits_; return true;
  }
  bool finish_blocks() {
    if (blocks_ended_) return false;
    if (!skip(block_remaining_)) return false;
    block_remaining_ = 0;
    return blocks();
  }

  const uint8_t *data_{nullptr};
  size_t size_{0}, pos_{0}, start_{0}, pixels_{0}, stack_size_{0};
  uint16_t *canvas_{nullptr}, *restore_{nullptr};
  std::array<uint16_t, 256> global_palette_{}, local_palette_{};
  std::array<uint16_t, 4096> prefix_{};
  std::array<uint8_t, 4096> suffix_{}, stack_{};
  uint16_t width_{0}, height_{0}, global_colors_{0}, colors_{0}, background_{0};
  uint16_t left_{0}, top_{0}, frame_width_{0}, frame_height_{0}, row_{0}, column_{0};
  uint16_t delay_{0}, clear_{0}, next_{0}, repeats_{0}, frames_{0};
  uint32_t completed_loops_{0}, bit_buffer_{0};
  int previous_{-1};
  uint8_t disposal_{0}, transparent_index_{0}, pass_{0}, min_bits_{0}, code_bits_{0};
  uint8_t first_{0}, bit_count_{0}, block_remaining_{0};
  bool transparent_{false}, interlaced_{false}, decoding_{false}, blocks_ended_{false};
  bool loop_configured_{false}, ended_{false}, error_{false};
};

}  // namespace esphome::artwork_image::gif
