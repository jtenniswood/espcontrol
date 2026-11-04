#include "gif_image.h"
#ifdef USE_ARTWORK_IMAGE_GIF_SUPPORT
#include "artwork_image.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::artwork_image {
static const char *const TAG = "artwork_image.gif";

GifDecoder::~GifDecoder() { this->release_working_state_(); }

void GifDecoder::release_working_state_() {
  this->pause();
  if (player_) {
    player_->~Player();
    player_allocator_.deallocate(player_, 1);
  }
  canvas_allocator_.deallocate(workspace_, workspace_pixels_);
  axis_allocator_.deallocate(horizontal_axes_, horizontal_axes_size_);
  release_filtered_resize();
  player_ = nullptr; workspace_ = nullptr; workspace_pixels_ = 0;
  horizontal_axes_ = nullptr; horizontal_axes_size_ = 0;
  initialized_ = false; rendering_ = false; render_target_ready_ = false;
  render_row_ = 0; decode_work_us_ = resize_work_us_ = 0;
}

bool GifDecoder::suspend_working_state(bool completed_frame) {
  if (suspended_ || !player_ || !first_frame_ready_ || !source_.data()) return false;
  suspended_delay_ms_ = player_->delay_ms();
  // A second cover during reconstruction must keep the original destination.
  if (!replaying_) {
    replay_position_ = player_->playback_position();
    replay_current_frame_ = rendering_ || completed_frame;
  }
  this->release_working_state_();
  suspended_ = true;
  return true;
}

int GifDecoder::initialize_player_(uint8_t *buffer, size_t size) {
  this->release_working_state_();
  uint16_t width = 0, height = 0;
  if (!gif::Player::dimensions(buffer, size, width, height)) return DECODE_ERROR_INVALID_TYPE;
  player_ = player_allocator_.allocate(1);
  if (!player_) return DECODE_ERROR_OUT_OF_MEMORY;
  new (player_) gif::Player();
  const size_t canvas_pixels = static_cast<size_t>(width) * height;
  workspace_pixels_ = canvas_pixels * (restore_required_ ? 2u : 1u);
  workspace_ = canvas_allocator_.allocate(workspace_pixels_);
  if (!workspace_) return DECODE_ERROR_OUT_OF_MEMORY;
  if (!player_->open(buffer, size, workspace_, restore_required_ ? workspace_ + canvas_pixels : nullptr))
    return DECODE_ERROR_INVALID_TYPE;
  source_length_ = size;
  initialized_ = true;
  suspended_ = false;
  return 0;
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
      this->pause();
      scanning_ = false;
      return DECODE_ERROR_UNSUPPORTED_FORMAT;
    }
    if (!scanning_) {
      ESP_LOGI(TAG, "GIF source: %ux%u compressed=%zu bytes", width, height, size);
      scanning_ = true;
    }
    decode_loop_.start();
    // Check time between small slices, including on repetitive compressed data.
    // A call examines at most 64 KiB of positions; other components run between
    // calls even when source admission reaches the 8 MiB file ceiling.
    constexpr size_t SCAN_POSITIONS_PER_SLICE = 4 * 1024;
    const uint32_t started = millis();
    auto scan = gif::Player::RestoreScan::MORE;
    for (unsigned slice = 0; slice < 16; ++slice) {
      scan = gif::Player::scan_restore_canvas(
          buffer, size, restore_scan_position_, SCAN_POSITIONS_PER_SLICE);
      if (scan != gif::Player::RestoreScan::MORE || millis() - started >= 4) break;
    }
    if (scan == gif::Player::RestoreScan::MORE) return 0;
    this->pause();
    scanning_ = false;
    restore_required_ = scan == gif::Player::RestoreScan::REQUIRED;
    const int initialized = this->initialize_player_(buffer, size);
    if (initialized < 0) return initialized;
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

bool GifDecoder::retain_source(DownloadBuffer &transfer) {
  if (!player_ || !transfer.data() || transfer.size() < source_length_) return false;
  transfer.compact(source_length_);  // Keep the safe allocation if shrinking fails.
  const size_t capacity = transfer.size();
  auto *data = transfer.detach();
  source_.adopt(data, capacity);
  player_->rebind_source(data);
  return true;
}

gif::Player::Result GifDecoder::advance() {
  if (suspended_) {
    if (!source_.data() || source_.size() < source_length_ ||
        this->initialize_player_(source_.data(), source_length_) < 0)
      return gif::Player::Result::ERROR;
    player_->restore_replayed_loop_count(replay_position_.loops);
    replay_remaining_frames_ = replay_position_.frames;
    replaying_ = true;
  }
  if (!player_) return gif::Player::Result::ERROR;
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
  if (replaying_) {
    if (!replay_remaining_frames_) {
      replaying_ = false;
      rendering_ = replay_current_frame_ && replay_position_.frames != 0;
      return gif::Player::Result::MORE;
    }
    const auto result = player_->step();
    if (result == gif::Player::Result::FRAME) {
      --replay_remaining_frames_;
      return gif::Player::Result::MORE;
    }
    return result;
  }
  if (!rendering_) {
    const uint32_t started = micros();
    const auto result = player_->step();
    decode_work_us_ += micros() - started;
    if (result != gif::Player::Result::FRAME) return result;
    rendering_ = true;
  }
  const uint32_t resize_started = micros();
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
  // A tiny source can expand into a whole screen. Bound output work as well
  // as source work; resume this row before accepting another source row.
  const auto *source = player_->canvas() + static_cast<size_t>(render_row_) * player_->width();
  const auto row_result = draw_fast_filtered_rgb565_row(render_row_, source, 2);
  if (row_result == ScanlineResampler::RowResult::ERROR) return gif::Player::Result::ERROR;
  if (row_result == ScanlineResampler::RowResult::DONE) ++render_row_;
  resize_work_us_ += micros() - resize_started;
  if (render_row_ != player_->height()) return gif::Player::Result::MORE;
  ESP_LOGD(TAG, "GIF frame work: decode=%lu ms resize=%lu ms",
           static_cast<unsigned long>(decode_work_us_ / 1000),
           static_cast<unsigned long>(resize_work_us_ / 1000));
  decode_work_us_ = resize_work_us_ = 0;
#if !defined(USE_ESP_IDF) || !defined(CONFIG_IDF_TARGET_ESP32S3)
  release_filtered_resize();
#endif
  rendering_ = false;
  render_target_ready_ = false;
  return gif::Player::Result::FRAME;
}
}  // namespace esphome::artwork_image
#endif
