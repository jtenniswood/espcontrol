#pragma once
#include "esphome/core/color.h"

namespace esphome {
namespace artwork_image {

enum DecodeError : int {
  DECODE_ERROR_INVALID_TYPE = -1,
  DECODE_ERROR_UNSUPPORTED_FORMAT = -2,
  DECODE_ERROR_OUT_OF_MEMORY = -3,
};

class ArtworkImage;

/**
 * @brief Class to abstract decoding different image formats.
 */
class ImageDecoder {
 public:
  /**
   * @brief Construct a new Image Decoder object
   *
   * @param image The image to decode the stream into.
   */
  ImageDecoder(ArtworkImage *image) : image_(image) {}
  virtual ~ImageDecoder() = default;

  /**
   * @brief Initialize the decoder.
   *
   * @param download_size The total number of bytes that need to be downloaded for the image.
   * @return int          Returns 0 on success, a {@see DecodeError} value in case of an error.
   */
  virtual int prepare(size_t download_size) {
    this->download_size_ = download_size;
    return 0;
  }

  /**
   * @brief Decode a part of the image. It will try reading from the buffer.
   * There is no guarantee that the whole available buffer will be read/decoded;
   * the method will return the amount of bytes actually decoded, so that the
   * unread content can be moved to the beginning.
   *
   * @param buffer The buffer to read from.
   * @param size   The maximum amount of bytes that can be read from the buffer.
   * @return int   The amount of bytes read. It can be 0 if the buffer does not have enough content to meaningfully
   *               decode anything, or negative in case of a decoding error.
   */
  virtual int decode(uint8_t *buffer, size_t size) = 0;

  /**
   * @brief Request the image to be resized once the actual dimensions are known.
   * Called by the callback functions, to be able to access the parent Image class.
   *
   * @param width The image's width.
   * @param height The image's height.
   * @return true if the image was resized, false otherwise.
   */
  bool set_size(int width, int height);
  bool has_failed() const { return this->failed_; }

  /**
   * @brief Fill a rectangle on the display_buffer using the defined color.
   * Will check the given coordinates for out-of-bounds, and clip the rectangle accordingly.
   * In case of binary displays, the color will be converted to binary as well.
   * Called by the callback functions, to be able to access the parent Image class.
   *
   * @param x The left-most coordinate of the rectangle.
   * @param y The top-most coordinate of the rectangle.
   * @param w The width of the rectangle.
   * @param h The height of the rectangle.
   * @param color The fill color
   */
  void draw(int x, int y, int w, int h, const Color &color);

  /**
   * @brief Write a block of pre-formatted RGB565 pixel data directly into the image buffer.
   * Uses memcpy for 1:1 scale (no per-pixel conversion overhead).
   * Falls back to per-pixel copy when scaling is required.
   *
   * @param x Block x position in source image coordinates.
   * @param y Block y position in source image coordinates.
   * @param w Block width in pixels.
   * @param h Block height in pixels.
   * @param data Pointer to packed RGB565 pixel data (2 bytes per pixel, row-major).
   */
  void draw_rgb565_block(int x, int y, int w, int h, const uint8_t *data);

  /** Copy or scale a complete hardware-decoded RGB565 frame into the image surface. */
  void draw_rgb565_frame(int width, int height, size_t stride_bytes, const uint8_t *data);

  bool is_finished() const { return this->download_size_ > 0 && this->decoded_bytes_ >= this->download_size_; }
  bool has_unknown_download_size() const { return this->download_size_ == 0; }
  void set_download_size(size_t download_size) { this->download_size_ = download_size; }
  virtual bool is_decoding() const { return false; }

 protected:
  ArtworkImage *image_;
  // Initializing to 1, to ensure it is distinguishable from initial "decoded_bytes_".
  // Will be overwritten anyway once the download size is known.
  size_t download_size_ = 1;
  size_t decoded_bytes_ = 0;
  double x_scale_ = 1.0;
  double y_scale_ = 1.0;
  int x_offset_ = 0;
  int y_offset_ = 0;
  bool failed_{false};
};

class DownloadBuffer {
 public:
  DownloadBuffer(size_t size);

  virtual ~DownloadBuffer() { this->allocator_.deallocate(this->buffer_, this->size_); }

  uint8_t *data(size_t offset = 0);

  uint8_t *append() { return this->data(this->unread_); }

  size_t unread() const { return this->unread_; }
  size_t size() const { return this->size_; }
  size_t free_capacity() const { return this->size_ - this->unread_; }

  size_t read(size_t len);
  size_t write(size_t len) {
    this->unread_ += len;
    return this->unread_;
  }

  void reset() { this->unread_ = 0; }

  size_t resize(size_t size);
  void shrink_to(size_t size);

 protected:
  RAMAllocator<uint8_t> allocator_{};
  uint8_t *buffer_;
  size_t size_;
  /** Total number of downloaded bytes not yet read. */
  size_t unread_;
};

}  // namespace artwork_image
}  // namespace esphome
