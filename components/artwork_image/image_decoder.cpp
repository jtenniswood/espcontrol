#include "image_decoder.h"
#include "artwork_image.h"
#include "rgb565_scaler.h"

#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace artwork_image {

static const char *const TAG = "artwork_image.decoder";

ImageDecoder::~ImageDecoder() { this->release_filtered_resize(); }

void ImageDecoder::release_filtered_resize() {
  if (this->resample_workspace_) {
    this->resample_allocator_.deallocate(this->resample_workspace_, this->resample_workspace_size_);
    this->resample_workspace_ = nullptr;
  }
  this->resample_workspace_size_ = 0;
  this->resampler_ = ScanlineResampler{};
}

bool ImageDecoder::prepare_filtered_resize(int width, int height) {
  this->release_filtered_resize();
  const int content_width = this->image_->decode_content_width_;
  const int content_height = this->image_->decode_content_height_;
  const int start_x = std::max(0, this->x_offset_);
  const int end_x = std::min(this->image_->decode_buffer_width_, this->x_offset_ + content_width);
  this->resample_workspace_size_ = ScanlineResampler::workspace_size(end_x - start_x);
  this->resample_workspace_ = this->resample_allocator_.allocate(this->resample_workspace_size_);
  if (!this->resampler_.configure(width, height, content_width, content_height,
                                 this->image_->decode_buffer_width_, this->image_->decode_buffer_height_,
                                 this->x_offset_, this->y_offset_, this->resample_workspace_,
                                 this->resample_workspace_size_)) {
    this->failed_ = true;
    this->release_filtered_resize();
    ESP_LOGE(TAG, "Could not allocate filtered image resize workspace");
    return false;
  }
  return true;
}

void ImageDecoder::draw_filtered_rgb888_row(int y, const uint8_t *data) {
  if (!this->resampler_.push_row(y, [data](int x) {
        return ResampleColor{data[x * 3], data[x * 3 + 1], data[x * 3 + 2]};
      }, [this](int x, int y, ResampleColor color) {
        this->image_->draw_pixel_(x, y, Color(color.r, color.g, color.b, 0xFF));
      })) this->failed_ = true;
}

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

  draw_scaled_rgb565_block(
      this->image_->decode_buffer_, this->image_->decode_buffer_width_,
      this->image_->decode_buffer_height_, bpp_bytes, this->x_offset_,
      this->y_offset_, this->x_scale_, this->y_scale_, x, y, w, h, data);
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

  if (!this->prepare_filtered_resize(width, height)) return;
  const bool big_endian = this->image_->is_big_endian();
  for (int y = 0; y < height; y++) {
    const uint8_t *row = data + static_cast<size_t>(y) * stride_bytes;
    if (!this->resampler_.push_row(y, [row, big_endian](int x) {
          const uint16_t pixel = big_endian ? (row[x * 2] << 8) | row[x * 2 + 1]
                                            : row[x * 2] | (row[x * 2 + 1] << 8);
          const uint8_t r = (pixel >> 11) & 31, g = (pixel >> 5) & 63, b = pixel & 31;
          return ResampleColor{static_cast<uint8_t>((r << 3) | (r >> 2)),
                               static_cast<uint8_t>((g << 2) | (g >> 4)),
                               static_cast<uint8_t>((b << 3) | (b >> 2))};
        }, [this](int x, int y, ResampleColor color) {
          this->image_->draw_pixel_(x, y, Color(color.r, color.g, color.b, 0xFF));
        })) {
      this->failed_ = true;
      break;
    }
    App.feed_wdt();
  }
  this->release_filtered_resize();
}

DownloadBuffer::DownloadBuffer(size_t size) : buffer_(nullptr), size_(size) {
  // ArtworkImage intentionally starts with no staging allocation and grows on
  // first use. A zero-sized allocator returning nullptr is therefore expected.
  if (size == 0) {
    this->reset();
    return;
  }
  this->buffer_ = this->allocator_.allocate(size);
  this->reset();
  if (!this->buffer_) {
    ESP_LOGE(TAG, "Initial allocation of download buffer failed!");
    this->size_ = 0;
  }
}

uint8_t *DownloadBuffer::data(size_t offset) {
  if (!this->buffer_ || offset > this->size_) {
    ESP_LOGE(TAG, "Download buffer is unavailable or access is beyond its bounds");
    return nullptr;
  }
  return this->buffer_ + offset;
}

size_t DownloadBuffer::read(size_t len) {
  if (!this->buffer_) {
    this->reset();
    return 0;
  }
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
