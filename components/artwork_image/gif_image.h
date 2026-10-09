#pragma once

#include "esphome/core/defines.h"
#ifdef USE_ARTWORK_IMAGE_GIF_SUPPORT
#include "gif_player.h"
#include "image_decoder.h"
#include "esphome/core/helpers.h"

namespace esphome::artwork_image {

class GifDecoder : public ImageDecoder {
 public:
  explicit GifDecoder(ArtworkImage *image) : ImageDecoder(image), source_(0) {}
  ~GifDecoder() override;
  int decode(uint8_t *buffer, size_t size) override;
  bool is_decoding() const override { return initialized_ && !first_frame_ready_; }
  gif::Player::Result advance();
  void pause() { decode_loop_.stop(); }
  void reset_render_target(bool completed_frame = false);
  uint32_t delay_ms() const { return player_->delay_ms(); }
  bool retain_source(uint8_t *data, size_t size) { return source_.adopt(data, size); }

 protected:
  gif::Player::Result advance_step_();
  HighFrequencyLoopRequester decode_loop_;
  RAMAllocator<gif::Player> player_allocator_{RAMAllocator<gif::Player>::ALLOC_EXTERNAL};
  gif::Player *player_{nullptr};
  DownloadBuffer source_;
  RAMAllocator<uint16_t> canvas_allocator_{RAMAllocator<uint16_t>::ALLOC_EXTERNAL};
  uint16_t *workspace_{nullptr};
  size_t workspace_pixels_{0};
  RAMAllocator<ResampleAxis32> axis_allocator_{RAMAllocator<ResampleAxis32>::ALLOC_EXTERNAL};
  ResampleAxis32 *horizontal_axes_{nullptr};
  size_t horizontal_axes_size_{0};
  int render_row_{0};
  uint32_t decode_work_us_{0}, resize_work_us_{0};
  bool initialized_{false}, rendering_{false}, render_target_ready_{false}, first_frame_ready_{false};
};

}  // namespace esphome::artwork_image
#endif
