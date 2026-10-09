#include "gif_image.h"
#ifdef USE_ARTWORK_IMAGE_GIF_SUPPORT
#include "artwork_image.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::artwork_image {
static const char *const TAG = "artwork_image.gif";

GifDecoder::~GifDecoder() {
  this->pause();
  if (player_) {
    player_->~Player();
    player_allocator_.deallocate(player_, 1);
  }
  canvas_allocator_.deallocate(workspace_, workspace_pixels_);
  axis_allocator_.deallocate(horizontal_axes_, horizontal_axes_size_);
}

int GifDecoder::decode(uint8_t *buffer, size_t size) {
  // GIF playback needs the complete compressed file, including unknown-length
  // HTTP responses. Keep staging bytes unread until the first frame is ready.
  if (!download_size_ || size < download_size_) return 0;
  if (!initialized_) {
    uint16_t width = 0, height = 0;
    if (!gif::Player::dimensions(buffer, size, width, height)) {
      ESP_LOGE(TAG, "Invalid GIF or source exceeds limits: source=%ux%u compressed=%zu max_pixels=%zu",
               width, height, size, gif::Player::MAX_PIXELS);
      return DECODE_ERROR_UNSUPPORTED_FORMAT;
    }
    ESP_LOGI(TAG, "GIF source: %ux%u compressed=%zu bytes", width, height, size);
    player_ = player_allocator_.allocate(1);
    if (!player_) return DECODE_ERROR_OUT_OF_MEMORY;
    new (player_) gif::Player();
    workspace_pixels_ = static_cast<size_t>(width) * height * 2;
    workspace_ = canvas_allocator_.allocate(workspace_pixels_);
    if (!workspace_) return DECODE_ERROR_OUT_OF_MEMORY;
    if (!player_->open(buffer, size, workspace_, workspace_ + workspace_pixels_ / 2))
      return DECODE_ERROR_INVALID_TYPE;
    initialized_ = true;
  }
  const auto result = advance();
  if (result == gif::Player::Result::ERROR || result == gif::Player::Result::END)
    return DECODE_ERROR_INVALID_TYPE;
  if (result == gif::Player::Result::FRAME) {
    first_frame_ready_ = true;
    decoded_bytes_ = download_size_;
  }
  return 0;
}

gif::Player::Result GifDecoder::advance() {
  // Avoid the normal 16 ms component-loop delay between bounded decode slices.
  // Other components still run between slices; idle/hidden animations release
  // the request so ordinary firmware scheduling resumes.
  decode_loop_.start();
  const uint32_t started = millis();
  for (int slice = 0; slice < 8; ++slice) {
    const auto result = advance_step_();
    if (result != gif::Player::Result::MORE) {
      this->pause();
      return result;
    }
    if (millis() - started >= 4) return result;
  }
  return gif::Player::Result::MORE;
}

void GifDecoder::reset_render_target(bool completed_frame) {
  if (completed_frame) rendering_ = true;
  this->pause();
  release_filtered_resize();
  render_row_ = 0;
  render_target_ready_ = false;
}

gif::Player::Result GifDecoder::advance_step_() {
  if (!rendering_) {
    const auto result = player_->step();
    if (result != gif::Player::Result::FRAME) return result;
    rendering_ = true;
  }
  if (!render_target_ready_) {
    if (!set_size(player_->width(), player_->height()) ||
        !prepare_filtered_resize(player_->width(), player_->height())) return gif::Player::Result::ERROR;
    if (resampler_.fast_dimensions_supported()) {
      const size_t count = resampler_.visible_width();
      if (count != horizontal_axes_size_) {
        axis_allocator_.deallocate(horizontal_axes_, horizontal_axes_size_);
        horizontal_axes_ = axis_allocator_.allocate(count);
        horizontal_axes_size_ = horizontal_axes_ ? count : 0;
      }
      // Cache allocation is optional; native arithmetic also works without it.
      resampler_.cache_horizontal_axes(horizontal_axes_, horizontal_axes_size_);
    }
    render_target_ready_ = true;
    render_row_ = 0;
  }
  // Resize only a few source rows per loop so GIF decoding yields to touch,
  // network and display work. Composite disposal/transparency precedes resize.
  const int end = std::min(render_row_ + 8, static_cast<int>(player_->height()));
  while (render_row_ < end) {
    const auto *source = player_->canvas() + static_cast<size_t>(render_row_) * player_->width();
    draw_fast_filtered_rgb565_row(render_row_++, source);
    if (has_failed()) return gif::Player::Result::ERROR;
  }
  if (render_row_ != player_->height()) return gif::Player::Result::MORE;
  release_filtered_resize();
  rendering_ = false;
  render_target_ready_ = false;
  return gif::Player::Result::FRAME;
}
}  // namespace esphome::artwork_image
#endif
