#pragma once

#include "image_decoder.h"
#include "esphome/core/defines.h"
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
#include <jpeglib.h>
#include <csetjmp>
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace artwork_image {

struct JpegErrorMgr {
  jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
  char message[JMSG_LENGTH_MAX];
};

/**
 * @brief Image decoder specialization for JPEG images.
 */
class JpegDecoder : public ImageDecoder {
 public:
  /**
   * @brief Construct a new JPEG Decoder object.
   *
   * @param display The image to decode the stream into.
   */
  JpegDecoder(ArtworkImage *image) : ImageDecoder(image) {}
  ~JpegDecoder() override;

  int prepare(size_t download_size) override;
  int HOT decode(uint8_t *buffer, size_t size) override;
  bool is_decoding() const override { return this->decode_started_ && !this->is_finished(); }

 protected:
#if defined(USE_ESP_IDF) && defined(CONFIG_IDF_TARGET_ESP32P4)
  int decode_hardware_(uint8_t *buffer, size_t size);
#endif
  int start_decode_(uint8_t *buffer, size_t size);
  int decode_scanlines_();
  void cleanup_();

  jpeg_decompress_struct cinfo_{};
  JpegErrorMgr jerr_{};
  uint8_t *row_buffer_{nullptr};
  size_t source_size_{0};
  int out_w_{0};
  int out_h_{0};
  int y_{0};
  bool cinfo_created_{false};
  bool decode_started_{false};
  bool use_rgb565_{false};
  bool big_endian_{false};
};

}  // namespace artwork_image
}  // namespace esphome

#endif  // USE_ARTWORK_IMAGE_JPEG_SUPPORT
