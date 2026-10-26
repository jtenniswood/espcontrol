#pragma once
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>

#include "image_result.h"
#ifdef ESPCONTROL_REAL_LVGL_TEST
#include "lvgl.h"
#endif

#if defined(USE_LVGL) && !defined(ESPCONTROL_REAL_LVGL_TEST)
void lv_draw_wait_for_finish();
extern size_t artwork_test_draw_fences;
#endif

namespace esphome::artwork_image {
enum class ImageResizeMode { FIT, COVER };
enum class ImageRequestPriority { BACKGROUND, TILE, COVER_ART, MODAL };
// Only transfer/component I/O is replaced. The production channel and adapter
// run unchanged, with a retained result available for late-delivery tests.
class ArtworkImage {
 public:
  ImageResultChannel channel;
  std::string url;
  uint64_t request{0};
  uint32_t generation{0}, background{0};
  int width{0}, height{0}, max_source_dimension{0};
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
  bool request_owned_update(ImageResultObserver& owner, uint64_t token, const std::string& source,
                            int maximum = 0) {
    assert(channel.owned_by(owner));
    cancel_update();
    if (!accept) return false;
    request = token;
    url = source;
    max_source_dimension = maximum;
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
#if defined(USE_LVGL) && !defined(ESPCONTROL_REAL_LVGL_TEST)
    assert(artwork_test_draw_fences > releases);
#endif
    cancel_update();
    ++releases;
    ready = false;
  }
#ifdef ESPCONTROL_REAL_LVGL_TEST
  const lv_image_dsc_t* get_lv_image_dsc() {
    descriptor = {};
    descriptor.header.magic = LV_IMAGE_HEADER_MAGIC;
    descriptor.header.cf = LV_COLOR_FORMAT_RGB565;
    descriptor.header.w = descriptor.header.h = 2;
    descriptor.header.stride = 4;
    descriptor.data_size = sizeof(pixels);
    descriptor.data = reinterpret_cast<const uint8_t*>(pixels);
    return &descriptor;
  }
  int get_width() const { return 2; }
  int get_height() const { return 2; }
  int get_content_offset_x() const { return 0; }
  int get_content_offset_y() const { return 0; }
  int get_content_width() const { return 2; }
  int get_content_height() const { return 2; }
  bool is_big_endian() const { return false; }
  uint16_t pixels[4]{0xFFFF, 0xABCD, 0x1234, 0xFFFF};
  lv_image_dsc_t descriptor{};
#endif
  void reply(bool success = true, bool cached = false) {
    channel.deliver(generation, success, cached);
  }
};
}  // namespace esphome::artwork_image
