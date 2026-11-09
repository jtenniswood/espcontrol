#pragma once

#include "esphome/core/defines.h"
#ifdef USE_ARTWORK_IMAGE_GIF_SUPPORT
#include "gif_player.h"
#include "image_decoder.h"
#include "esphome/core/helpers.h"

namespace esphome::artwork_image {

class GifDecoder : public ImageDecoder {
 public:
  explicit GifDecoder(ArtworkImage *image) : ImageDecoder(image), source_(0) {
#if defined(USE_ESP_IDF) && defined(CONFIG_IDF_TARGET_ESP32S3)
    // Hot interpolation rows and lookup axes compete with RGB scanout in
    // PSRAM. Rows are required; unavailable lookup storage uses the existing
    // exact arithmetic fallback instead of an external-memory cache.
    resample_allocator_ = RAMAllocator<uint8_t>{RAMAllocator<uint8_t>::ALLOC_INTERNAL};
    axis_allocator_ = RAMAllocator<ResampleAxis32>{RAMAllocator<ResampleAxis32>::ALLOC_INTERNAL};
#endif
  }
  ~GifDecoder() override;
  int decode(uint8_t *buffer, size_t size) override;
  bool is_decoding() const override { return (scanning_ || initialized_) && !first_frame_ready_; }
  gif::Player::Result advance();
  void pause() { decode_loop_.stop(); }
  bool suspend_working_state(bool completed_frame = false);
  void reset_render_target(bool completed_frame = false);
  uint32_t delay_ms() const { return player_ ? player_->delay_ms() : suspended_delay_ms_; }
  bool is_animated() const { return suspended_ || (player_ && player_->has_more_frames()); }
  bool retain_source(DownloadBuffer &transfer);

 protected:
  gif::Player::Result advance_step_();
  int initialize_player_(uint8_t *buffer, size_t size);
  void release_working_state_();
  HighFrequencyLoopRequester decode_loop_;
  RAMAllocator<gif::Player> player_allocator_{RAMAllocator<gif::Player>::ALLOC_EXTERNAL};
  gif::Player *player_{nullptr};
  DownloadBuffer source_;
  size_t source_length_{0};
  size_t restore_scan_position_{0};
  bool scanning_{false};
  bool restore_required_{false}, suspended_{false};
  uint32_t suspended_delay_ms_{0};
  gif::Player::PlaybackPosition replay_position_{0, 0};
  uint16_t replay_remaining_frames_{0};
  bool replay_current_frame_{false}, replaying_{false};
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
