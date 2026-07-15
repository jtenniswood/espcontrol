#include "bmp_image.h"

#ifdef USE_ARTWORK_IMAGE_BMP_SUPPORT

#include "esphome/components/display/display.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace artwork_image {

static const char *const TAG = "artwork_image.bmp";

int HOT BmpDecoder::decode(uint8_t *buffer, size_t size) {
  size_t index = 0;
  if (this->current_index_ == 0 && size > 14) {
    if (buffer[0] != 'B' || buffer[1] != 'M') {
      ESP_LOGE(TAG, "Not a BMP file");
      return DECODE_ERROR_INVALID_TYPE;
    }
    this->download_size_ = encode_uint32(buffer[5], buffer[4], buffer[3], buffer[2]);
    this->data_offset_ = encode_uint32(buffer[13], buffer[12], buffer[11], buffer[10]);
    this->current_index_ = 14;
    index = 14;
  }

  if (this->current_index_ == 14 && index == 14 && size > this->data_offset_) {
    this->width_ = encode_uint32(buffer[21], buffer[20], buffer[19], buffer[18]);
    this->height_ = encode_uint32(buffer[25], buffer[24], buffer[23], buffer[22]);
    this->bits_per_pixel_ = encode_uint16(buffer[29], buffer[28]);
    this->compression_method_ = encode_uint32(buffer[33], buffer[32], buffer[31], buffer[30]);

    if (this->width_ <= 0 || this->height_ <= 0) {
      ESP_LOGE(TAG, "Unsupported BMP orientation or dimensions: %dx%d", this->width_, this->height_);
      return DECODE_ERROR_UNSUPPORTED_FORMAT;
    }
    if (this->bits_per_pixel_ == 24) {
      size_t row_bytes = static_cast<size_t>(this->width_) * 3;
      this->padding_bytes_ = static_cast<uint8_t>((4 - row_bytes % 4) % 4);
    } else if (this->bits_per_pixel_ != 1) {
      ESP_LOGE(TAG, "Unsupported BMP depth: %u bits", this->bits_per_pixel_);
      return DECODE_ERROR_UNSUPPORTED_FORMAT;
    }
    if (this->compression_method_ != 0) {
      ESP_LOGE(TAG, "Unsupported BMP compression method: %" PRIu32, this->compression_method_);
      return DECODE_ERROR_UNSUPPORTED_FORMAT;
    }
    if (!this->set_size(this->width_, this->height_)) return DECODE_ERROR_OUT_OF_MEMORY;
    this->current_index_ = this->data_offset_;
    index = this->data_offset_;
  }

  if (this->bits_per_pixel_ == 1) {
    while (index < size) {
      uint8_t current_byte = buffer[index++];
      for (uint8_t bit = 0; bit < 8 && this->paint_index_ < static_cast<size_t>(this->width_) * this->height_; bit++) {
        size_t x = this->paint_index_ % this->width_;
        size_t y = (this->height_ - 1) - (this->paint_index_ / this->width_);
        Color color = (current_byte & (1 << (7 - bit))) ? display::COLOR_ON : display::COLOR_OFF;
        this->draw(x, y, 1, 1, color);
        this->paint_index_++;
      }
    }
  } else if (this->bits_per_pixel_ == 24) {
    while (index < size) {
      if (index + 2 >= size) {
        this->decoded_bytes_ += index;
        return index;
      }
      uint8_t blue = buffer[index];
      uint8_t green = buffer[index + 1];
      uint8_t red = buffer[index + 2];
      size_t x = this->paint_index_ % this->width_;
      size_t y = (this->height_ - 1) - (this->paint_index_ / this->width_);
      this->draw(x, y, 1, 1, Color(red, green, blue));
      this->paint_index_++;
      this->current_index_ += 3;
      index += 3;
      if (x == static_cast<size_t>(this->width_ - 1) && this->padding_bytes_ > 0) {
        if (index + this->padding_bytes_ > size) {
          this->decoded_bytes_ += index;
          return index;
        }
        index += this->padding_bytes_;
        this->current_index_ += this->padding_bytes_;
      }
    }
  }

  this->decoded_bytes_ += size;
  return size;
}

}  // namespace artwork_image
}  // namespace esphome

#endif
