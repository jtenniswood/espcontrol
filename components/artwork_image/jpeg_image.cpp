#include "jpeg_image.h"
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT

#include "esphome/components/display/display_buffer.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esp_heap_caps.h"

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "artwork_image.h"
static const char *const TAG = "artwork_image.jpeg";

namespace esphome {
namespace artwork_image {

static void jpeg_error_exit(j_common_ptr cinfo) {
  auto *err = reinterpret_cast<JpegErrorMgr *>(cinfo->err);
  (*(cinfo->err->format_message))(cinfo, err->message);
  longjmp(err->setjmp_buffer, 1);
}

static constexpr size_t MAX_JPEG_DOWNLOAD_SIZE = 2 * 1024 * 1024;  // 2 MB
static constexpr uint32_t JPEG_DECODE_BUDGET_MS = 12;
static constexpr int JPEG_SCANLINE_BATCH = 8;

JpegDecoder::~JpegDecoder() { this->cleanup_(); }

int JpegDecoder::prepare(size_t download_size) {
  if (download_size > MAX_JPEG_DOWNLOAD_SIZE) {
    ESP_LOGE(TAG, "JPEG too large to decode: %zu bytes (max %zu). Consider using a smaller image URL.",
             download_size, MAX_JPEG_DOWNLOAD_SIZE);
    return DECODE_ERROR_OUT_OF_MEMORY;
  }
  ImageDecoder::prepare(download_size);
  auto size = this->image_->resize_download_buffer(download_size);
  if (size < download_size) {
    ESP_LOGE(TAG, "Download buffer resize failed!");
    return DECODE_ERROR_OUT_OF_MEMORY;
  }
  return 0;
}

int HOT JpegDecoder::decode(uint8_t *buffer, size_t size) {
  if (this->download_size_ == 0) {
    ESP_LOGV(TAG, "Waiting for HTTP transfer to finish before decoding JPEG with unknown length");
    return 0;
  }
  if (size < this->download_size_) {
    ESP_LOGV(TAG, "Download not complete. Size: %zu/%zu", size, this->download_size_);
    return 0;
  }
  if (!this->decode_started_) {
    int ret = this->start_decode_(buffer, size);
    if (ret < 0) {
      return ret;
    }
  }
  return this->decode_scanlines_();
}

int JpegDecoder::start_decode_(uint8_t *buffer, size_t size) {
  ESP_LOGD(TAG, "JPEG decode start: %zu bytes", size);

  std::memset(&this->cinfo_, 0, sizeof(this->cinfo_));
  std::memset(&this->jerr_, 0, sizeof(this->jerr_));
  this->cinfo_.err = jpeg_std_error(&this->jerr_.pub);
  this->jerr_.pub.error_exit = jpeg_error_exit;

  if (setjmp(this->jerr_.setjmp_buffer)) {
    ESP_LOGE(TAG, "JPEG decode error: %s", this->jerr_.message);
    this->cleanup_();
    return DECODE_ERROR_UNSUPPORTED_FORMAT;
  }

  this->cinfo_created_ = true;
  jpeg_create_decompress(&this->cinfo_);
  jpeg_mem_src(&this->cinfo_, buffer, size);

  if (jpeg_read_header(&this->cinfo_, TRUE) != JPEG_HEADER_OK) {
    ESP_LOGE(TAG, "Could not read JPEG header");
    this->cleanup_();
    return DECODE_ERROR_INVALID_TYPE;
  }

  int src_w = this->cinfo_.image_width;
  int src_h = this->cinfo_.image_height;
  ESP_LOGD(TAG, "JPEG header: %dx%d, components=%d, progressive=%s",
           src_w, src_h, this->cinfo_.num_components,
           this->cinfo_.progressive_mode ? "yes" : "no");
  // Request RGB output regardless of input colorspace
  this->cinfo_.out_color_space = JCS_RGB;
  // Use fast integer IDCT — slightly lower quality but faster on ESP32
  // and avoids pulling in the float IDCT code path.
  this->cinfo_.dct_method = JDCT_IFAST;

  // Use IDCT scaling to downscale during decode.
  int target_w = this->image_->get_fixed_width();
  int target_h = this->image_->get_fixed_height();
  if (target_w > 0 && target_h > 0) {
    // Choose the IDCT output closest to the target artwork size. When two
    // options are equally close, prefer the smaller decode to reduce libjpeg's
    // temporary memory peak on ESP32-S3.
    constexpr unsigned int denoms[] = {1, 2, 4, 8};
    unsigned int best_denom = 1;
    int best_w = 0;
    int best_h = 0;
    long best_score = LONG_MAX;
    uint64_t best_area = UINT64_MAX;
    for (unsigned int denom : denoms) {
      this->cinfo_.scale_num = 1;
      this->cinfo_.scale_denom = denom;
      jpeg_calc_output_dimensions(&this->cinfo_);
      int candidate_w = static_cast<int>(this->cinfo_.output_width);
      int candidate_h = static_cast<int>(this->cinfo_.output_height);
      long score = std::labs(candidate_w - target_w) + std::labs(candidate_h - target_h);
      uint64_t area = static_cast<uint64_t>(candidate_w) * static_cast<uint64_t>(candidate_h);
      if (score < best_score || (score == best_score && area < best_area)) {
        best_score = score;
        best_area = area;
        best_denom = denom;
        best_w = candidate_w;
        best_h = candidate_h;
      }
    }
    this->cinfo_.scale_num = 1;
    this->cinfo_.scale_denom = best_denom;
    jpeg_calc_output_dimensions(&this->cinfo_);
    if (best_denom > 1 && (best_w < target_w || best_h < target_h)) {
      ESP_LOGD(TAG, "Using smaller JPEG decode to reduce memory peak: target=%dx%d",
               target_w, target_h);
    }
    if (this->cinfo_.output_width == 0 || this->cinfo_.output_height == 0) {
      this->cinfo_.scale_denom = 1;
      jpeg_calc_output_dimensions(&this->cinfo_);
    }
  } else {
    jpeg_calc_output_dimensions(&this->cinfo_);
  }

  this->out_w_ = this->cinfo_.output_width;
  this->out_h_ = this->cinfo_.output_height;
  if (this->out_w_ != src_w || this->out_h_ != src_h) {
    ESP_LOGD(TAG, "Using IDCT downscale: %dx%d -> %dx%d", src_w, src_h, this->out_w_, this->out_h_);
  }

  if (!this->set_size(this->out_w_, this->out_h_)) {
    this->cleanup_();
    return DECODE_ERROR_OUT_OF_MEMORY;
  }

  jpeg_start_decompress(&this->cinfo_);

  size_t row_stride = static_cast<size_t>(this->out_w_) * 3;
  this->row_buffer_ = static_cast<uint8_t *>(heap_caps_malloc(row_stride, MALLOC_CAP_8BIT));
  if (this->row_buffer_ == nullptr) {
    ESP_LOGE(TAG, "JPEG row buffer allocation failed: %zu bytes", row_stride);
    this->cleanup_();
    return DECODE_ERROR_OUT_OF_MEMORY;
  }

  this->use_rgb565_ = (this->image_->image_type() == image::ImageType::IMAGE_TYPE_RGB565);
  this->big_endian_ = this->image_->is_big_endian();
  this->source_size_ = size;
  this->y_ = 0;
  this->decode_started_ = true;
  return 0;
}

int JpegDecoder::decode_scanlines_() {
  if (setjmp(this->jerr_.setjmp_buffer)) {
    ESP_LOGE(TAG, "JPEG decode error: %s", this->jerr_.message);
    this->cleanup_();
    return DECODE_ERROR_UNSUPPORTED_FORMAT;
  }

  const uint32_t start = millis();
  while (this->cinfo_.output_scanline < this->cinfo_.output_height) {
    for (int i = 0; i < JPEG_SCANLINE_BATCH && this->cinfo_.output_scanline < this->cinfo_.output_height; i++) {
      uint8_t *row_ptr = this->row_buffer_;
      jpeg_read_scanlines(&this->cinfo_, &row_ptr, 1);

      if (this->use_rgb565_) {
        // Convert RGB888 -> RGB565 in-place (2 bpp fits within the 3 bpp
        // source buffer, so no separate allocation needed). We read forward
        // and write forward; the write pointer never overtakes the read
        // pointer because 2 < 3.
        uint8_t *dst = this->row_buffer_;
        for (int x = 0; x < this->out_w_; x++) {
          uint8_t r = this->row_buffer_[x * 3 + 0];
          uint8_t g = this->row_buffer_[x * 3 + 1];
          uint8_t b = this->row_buffer_[x * 3 + 2];
          uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
          if (this->big_endian_) {
            dst[0] = rgb565 >> 8;
            dst[1] = rgb565 & 0xFF;
          } else {
            dst[0] = rgb565 & 0xFF;
            dst[1] = rgb565 >> 8;
          }
          dst += 2;
        }
        this->draw_rgb565_block(0, this->y_, this->out_w_, 1, this->row_buffer_);
      } else {
        for (int x = 0; x < this->out_w_; x++) {
          Color color(this->row_buffer_[x * 3 + 0], this->row_buffer_[x * 3 + 1], this->row_buffer_[x * 3 + 2]);
          this->draw(x, this->y_, 1, 1, color);
        }
      }
      this->y_++;
    }
    App.feed_wdt();
    if (millis() - start >= JPEG_DECODE_BUDGET_MS) {
      break;
    }
  }

  if (this->cinfo_.output_scanline < this->cinfo_.output_height) {
    return 0;
  }

  jpeg_finish_decompress(&this->cinfo_);
  ESP_LOGD(TAG, "JPEG decode finished: output=%dx%d", this->out_w_, this->out_h_);
  size_t decoded = this->source_size_;
  this->decoded_bytes_ = decoded;
  this->cleanup_();
  return static_cast<int>(decoded);
}

void JpegDecoder::cleanup_() {
  if (this->row_buffer_ != nullptr) {
    free(this->row_buffer_);
    this->row_buffer_ = nullptr;
  }
  if (this->cinfo_created_) {
    if (setjmp(this->jerr_.setjmp_buffer) == 0) {
      jpeg_destroy_decompress(&this->cinfo_);
    }
    this->cinfo_created_ = false;
  }
  this->decode_started_ = false;
  this->source_size_ = 0;
  this->out_w_ = 0;
  this->out_h_ = 0;
  this->y_ = 0;
  this->use_rgb565_ = false;
  this->big_endian_ = false;
}

}  // namespace artwork_image
}  // namespace esphome

#endif  // USE_ARTWORK_IMAGE_JPEG_SUPPORT
