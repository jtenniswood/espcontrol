#pragma once
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>

#include "image_result.h"

#ifdef USE_LVGL
void lv_draw_wait_for_finish();
extern size_t artwork_test_draw_fences;
#endif

namespace esphome::artwork_image {
enum class ImageResizeMode { FIT, COVER };
enum class ImageRequestPriority { COVER_ART };
// Only transfer/component I/O is replaced. The production channel and adapter
// run unchanged, with a retained result available for late-delivery tests.
class ArtworkImage {
 public:
  ImageResultChannel channel;
  std::string url;
  uint64_t request{0};
  uint32_t generation{0}, background{0};
  int width{0}, height{0};
  ImageResizeMode mode{ImageResizeMode::FIT};
  ImageRequestPriority priority{ImageRequestPriority::COVER_ART};
  size_t cancels{0}, releases{0};
  bool ready{false}, legacy_owned{false}, accept{true};
  bool claim_owner(ImageResultObserver& owner) {
    return !legacy_owned && !visible && !redraw && !screen_active && channel.claim(owner);
  }
  void release_owner(ImageResultObserver& owner) {
    if (!channel.owned_by(owner)) return;
    cancel_update();
    channel.release(owner);
  }
  bool request_owned_update(ImageResultObserver& owner, uint64_t token, const std::string& source) {
    assert(channel.owned_by(owner));
    cancel_update();
    if (!accept) return false;
    request = token;
    url = source;
    return channel.arm(owner, ++generation, token);
  }
  void set_target_size(int w, int h) {
    width = w;
    height = h;
  }
  void set_resize_mode(ImageResizeMode value) {
    mode = value;
  }
  void set_fit_background_color(uint32_t value) {
    background = value;
  }
  void set_request_priority(ImageRequestPriority value) {
    priority = value;
  }
  std::function<bool()> visible, screen_active;
  std::function<void()> redraw;
  void set_animation_callbacks(std::function<bool()> shown, std::function<void()> frame,
                               std::function<bool()> active = {}, bool = false) {
    visible = std::move(shown);
    redraw = std::move(frame);
    screen_active = std::move(active);
  }
  bool has_image() const {
    return ready;
  }
  void cancel_update() {
    ++cancels;
    channel.clear();
  }
  void release() {
#ifdef USE_LVGL
    assert(artwork_test_draw_fences > releases);
#endif
    cancel_update();
    ++releases;
    ready = false;
  }
  void reply(bool success = true, bool cached = false) {
    channel.deliver(generation, success, cached);
  }
};
}  // namespace esphome::artwork_image
