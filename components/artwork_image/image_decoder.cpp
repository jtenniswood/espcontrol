#include "image_decoder.h"
#include "artwork_image.h"

#include "esphome/core/log.h"

namespace esphome {
namespace artwork_image {

static const char *const TAG = "artwork_image.decoder";

bool ImageDecoder::set_size(int width, int height) {
  bool success = this->image_->resize_(width, height) > 0;
  if (!success) {
    this->failed_ = true;
    return false;
  }
  int content_width = this->image_->decode_content_width_ > 0 ? this->image_->decode_content_width_
                                                              : this->image_->decode_buffer_width_;
  int content_height = this->image_->decode_content_height_ > 0 ? this->image_->decode_content_height_
                                                                : this->image_->decode_buffer_height_;
  this->x_offset_ = this->image_->decode_offset_x_;
  this->y_offset_ = this->image_->decode_offset_y_;
  this->x_scale_ = static_cast<double>(content_width) / width;
  this->y_scale_ = static_cast<double>(content_height) / height;
  ESP_LOGI(TAG, "Decoder geometry: source=%dx%d content=%dx%d offset=%d,%d scale=%.4f,%.4f",
           width, height, content_width, content_height, this->x_offset_, this->y_offset_, this->x_scale_,
           this->y_scale_);
  return success;
}

void ImageDecoder::draw(int x, int y, int w, int h, const Color &color) {
  if (this->failed_) {
    return;
  }
  auto width = std::min(this->image_->decode_buffer_width_,
                        this->x_offset_ + static_cast<int>(std::ceil((x + w) * this->x_scale_)));
  auto height = std::min(this->image_->decode_buffer_height_,
                         this->y_offset_ + static_cast<int>(std::ceil((y + h) * this->y_scale_)));
  int start_x = std::max(0, this->x_offset_ + static_cast<int>(x * this->x_scale_));
  int start_y = std::max(0, this->y_offset_ + static_cast<int>(y * this->y_scale_));
  for (int i = start_x; i < width; i++) {
    for (int j = start_y; j < height; j++) {
      this->image_->draw_pixel_(i, j, color);
    }
  }
}

void ImageDecoder::draw_rgb565_block(int x, int y, int w, int h, const uint8_t *data) {
  if (this->failed_) {
    return;
  }
  int bpp_bytes = this->image_->get_bpp() / 8;

  if (this->x_scale_ == 1.0 && this->y_scale_ == 1.0 && bpp_bytes == 2) {
    for (int row = 0; row < h; row++) {
      int dy = this->y_offset_ + y + row;
      if (dy < 0 || dy >= this->image_->decode_buffer_height_)
        continue;
      int start_x = std::max(0, this->x_offset_ + x);
      int end_x = std::min(this->x_offset_ + x + w, this->image_->decode_buffer_width_);
      if (start_x >= end_x)
        continue;
      int copy_w = end_x - start_x;
      int src_offset = (row * w + (start_x - this->x_offset_ - x)) * 2;
      int dst_pos = this->image_->get_position_(start_x, dy);
      memcpy(this->image_->decode_buffer_ + dst_pos, data + src_offset, copy_w * 2);
    }
    return;
  }

  for (int row = 0; row < h; row++) {
    for (int col = 0; col < w; col++) {
      int src_x = x + col;
      int src_y = y + row;
      int src_offset = (row * w + col) * 2;

      int target_x0 = std::max(0, this->x_offset_ + static_cast<int>(src_x * this->x_scale_));
      int target_y0 = std::max(0, this->y_offset_ + static_cast<int>(src_y * this->y_scale_));
      auto target_w = std::min(this->image_->decode_buffer_width_,
                               this->x_offset_ + static_cast<int>(std::ceil((src_x + 1) * this->x_scale_)));
      auto target_h = std::min(this->image_->decode_buffer_height_,
                               this->y_offset_ + static_cast<int>(std::ceil((src_y + 1) * this->y_scale_)));
      for (int dy = target_y0; dy < target_h; dy++) {
        for (int dx = target_x0; dx < target_w; dx++) {
          int dst_pos = this->image_->get_position_(dx, dy);
          memcpy(this->image_->decode_buffer_ + dst_pos, data + src_offset, 2);
          if (bpp_bytes > 2) {
            this->image_->decode_buffer_[dst_pos + 2] = 0xFF;
          }
        }
      }
    }
  }
}

void ImageDecoder::draw_rgb565_frame(int width, int height, size_t stride_bytes,
                                     const uint8_t *data) {
  if (this->failed_ || !data || width <= 0 || height <= 0 ||
      stride_bytes < static_cast<size_t>(width) * 2) {
    return;
  }
  int bpp_bytes = this->image_->get_bpp() / 8;
  if (bpp_bytes < 2) return;

  int content_width = this->image_->decode_content_width_ > 0
                          ? this->image_->decode_content_width_
                          : this->image_->decode_buffer_width_;
  int content_height = this->image_->decode_content_height_ > 0
                           ? this->image_->decode_content_height_
                           : this->image_->decode_buffer_height_;
  if (content_width <= 0 || content_height <= 0) return;

  int start_x = std::max(0, this->x_offset_);
  int start_y = std::max(0, this->y_offset_);
  int end_x = std::min(this->image_->decode_buffer_width_, this->x_offset_ + content_width);
  int end_y = std::min(this->image_->decode_buffer_height_, this->y_offset_ + content_height);
  if (bpp_bytes == 2 && this->x_offset_ == 0 && this->y_offset_ == 0 &&
      width == this->image_->decode_buffer_width_ && height == this->image_->decode_buffer_height_ &&
      content_width == width && content_height == height) {
    size_t row_bytes = static_cast<size_t>(width) * 2;
    if (stride_bytes == row_bytes) {
      memcpy(this->image_->decode_buffer_, data, row_bytes * height);
    } else {
      for (int y = 0; y < height; y++) {
        memcpy(this->image_->decode_buffer_ + static_cast<size_t>(y) * row_bytes,
               data + static_cast<size_t>(y) * stride_bytes, row_bytes);
      }
    }
    return;
  }

  std::vector<size_t> source_x_offsets(static_cast<size_t>(std::max(0, end_x - start_x)));
  for (int dst_x = start_x; dst_x < end_x; dst_x++) {
    int src_x = std::min(width - 1, (dst_x - this->x_offset_) * width / content_width);
    source_x_offsets[dst_x - start_x] = static_cast<size_t>(src_x) * 2;
  }
  for (int dst_y = start_y; dst_y < end_y; dst_y++) {
    int src_y = std::min(height - 1, (dst_y - this->y_offset_) * height / content_height);
    const uint8_t *source_row = data + static_cast<size_t>(src_y) * stride_bytes;
    uint8_t *destination =
        this->image_->decode_buffer_ + this->image_->get_position_(start_x, dst_y);
    for (int dst_x = start_x; dst_x < end_x; dst_x++) {
      const uint8_t *source = source_row + source_x_offsets[dst_x - start_x];
      destination[0] = source[0];
      destination[1] = source[1];
      if (bpp_bytes > 2) destination[2] = 0xFF;
      destination += bpp_bytes;
    }
  }
}

DownloadBuffer::DownloadBuffer(size_t size) : size_(size) {
  this->buffer_ = this->allocator_.allocate(size);
  this->reset();
  if (!this->buffer_) {
    ESP_LOGE(TAG, "Initial allocation of download buffer failed!");
    this->size_ = 0;
  }
}

uint8_t *DownloadBuffer::data(size_t offset) {
  if (offset > this->size_) {
    ESP_LOGE(TAG, "Tried to access beyond download buffer bounds!!!");
    return this->buffer_;
  }
  return this->buffer_ + offset;
}

size_t DownloadBuffer::read(size_t len) {
  if (len > this->unread_) {
    ESP_LOGE(TAG, "Decoder consumed %zu bytes, but only %zu were buffered", len, this->unread_);
    len = this->unread_;
  }
  this->unread_ -= len;
  if (len > 0 && this->unread_ > 0) {
    memmove(this->data(), this->data(len), this->unread_);
  }
  return this->unread_;
}

size_t DownloadBuffer::resize(size_t size) {
  if (this->size_ >= size) {
    return this->size_;
  }
  uint8_t *new_buffer = this->allocator_.allocate(size);
  if (new_buffer) {
    if (this->buffer_ && this->unread_ > 0) {
      memcpy(new_buffer, this->buffer_, this->unread_);
    }
    this->allocator_.deallocate(this->buffer_, this->size_);
    this->buffer_ = new_buffer;
    this->size_ = size;
    return size;
  } else {
    ESP_LOGE(TAG, "allocation of %zu bytes failed. Biggest block in heap: %zu Bytes", size,
             this->allocator_.get_max_free_block_size());
    this->allocator_.deallocate(this->buffer_, this->size_);
    this->buffer_ = nullptr;
    this->size_ = 0;
    this->reset();
    return 0;
  }
}

void DownloadBuffer::shrink_to(size_t size) {
  this->reset();
  if (this->size_ <= size) {
    return;
  }
  this->allocator_.deallocate(this->buffer_, this->size_);
  this->buffer_ = nullptr;
  this->size_ = 0;
  if (size == 0) {
    return;
  }
  this->buffer_ = this->allocator_.allocate(size);
  if (!this->buffer_) {
    ESP_LOGW(TAG, "allocation of shrunken download buffer failed: %zu bytes", size);
    return;
  }
  this->size_ = size;
}

bool DownloadBuffer::adopt(uint8_t *buffer, size_t size) {
  if (!buffer || size == 0) return false;
  if (buffer != this->buffer_) {
    this->allocator_.deallocate(this->buffer_, this->size_);
    this->buffer_ = buffer;
  }
  this->size_ = size;
  this->unread_ = size;
  return true;
}

}  // namespace artwork_image
}  // namespace esphome
