#pragma once

#include "esphome/core/defines.h"
#ifdef USE_ARTWORK_IMAGE_BMP_SUPPORT

#include "image_decoder.h"

namespace esphome {
namespace artwork_image {

class BmpDecoder : public ImageDecoder {
 public:
  explicit BmpDecoder(ArtworkImage *image) : ImageDecoder(image) {}
  int HOT decode(uint8_t *buffer, size_t size) override;

 protected:
  size_t current_index_{0};
  size_t paint_index_{0};
  int32_t width_{0};
  int32_t height_{0};
  uint16_t bits_per_pixel_{0};
  uint32_t compression_method_{0};
  size_t data_offset_{0};
  size_t row_bytes_{0};
  size_t row_stride_{0};
  Color palette_[2]{Color(0, 0, 0), Color(255, 255, 255)};
};

}  // namespace artwork_image
}  // namespace esphome

#endif
