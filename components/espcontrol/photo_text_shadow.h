#pragma once

#if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR >= 9 && LV_USE_SNAPSHOT
#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include "esp_heap_caps.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "src/misc/cache/instance/lv_image_header_cache.h"

namespace espcontrol {

class PhotoTextShadow {
 public:
  void update(lv_obj_t *label, bool visible, bool large) {
    if (!visible || lv_obj_has_flag(label, LV_OBJ_FLAG_HIDDEN)) {
      hide();
      return;
    }

    const int width = lv_obj_get_width(label);
    const int height = lv_obj_get_height(label);
    const char *text = lv_label_get_text(label);
    const std::array<int, 3> radii = large ? std::array<int, 3>{2, 3, 3}
                                           : std::array<int, 3>{2, 2, 2};
    const int padding = radii[0] + radii[1] + radii[2];
    if (!pixels_ || width != width_ || height != height_ || text_ != text) {
      hide();
      if (!render(label, radii, padding)) return;
      text_ = lv_label_get_text(label);
      width_ = width;
      height_ = height;
      if (!image_) {
        image_ = lv_image_create(lv_obj_get_parent(label));
        lv_obj_move_to_index(image_, 0);
        lv_obj_set_style_image_recolor(image_, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_image_opa(image_, LV_OPA_70, LV_PART_MAIN);
      }
      ::lv_image_set_src(image_, &mask_);
    }

    const int inset_x = (static_cast<int>(mask_.header.w) - width) / 2;
    const int inset_y = (static_cast<int>(mask_.header.h) - height) / 2;
    lv_obj_set_pos(image_, lv_obj_get_x(label) + 2 - inset_x,
                   lv_obj_get_y(label) + (large ? 4 : 3) - inset_y);
    lv_obj_clear_flag(image_, LV_OBJ_FLAG_HIDDEN);
  }

 private:
  void hide() {
    if (!pixels_) return;
    lv_obj_add_flag(image_, LV_OBJ_FLAG_HIDDEN);
    ::lv_image_set_src(image_, static_cast<const void *>(nullptr));
    lv_image_cache_drop(&mask_);
    lv_image_header_cache_drop(&mask_);
    pixels_.reset();
    text_.clear();
  }

  // Slide a box filter along each row or column, treating the outer samples as
  // transparent. Three box filters approximate a Gaussian with fixed work per pixel.
  static void blur_lines(const uint8_t *from, uint8_t *to, int lines, int length,
                         int next_line, int next_pixel, int radius) {
    const int diameter = 2 * radius + 1;
    for (int line = 0; line < lines; ++line, from += next_line, to += next_line) {
      int sum = 0;
      for (int i = 0; i < radius && i < length; ++i) sum += from[i * next_pixel];
      for (int i = 0; i < length; ++i) {
        if (i + radius < length) sum += from[(i + radius) * next_pixel];
        to[i * next_pixel] = (sum + radius) / diameter;
        if (i >= radius) sum -= from[(i - radius) * next_pixel];
      }
    }
  }

  bool render(lv_obj_t *label, const std::array<int, 3> &radii, int padding) {
    // Let LVGL render the original label, including its language, line wrapping
    // and ellipsis. The cached result contains only alpha, at one byte per pixel.
    using Snapshot = std::unique_ptr<lv_draw_buf_t, decltype(&lv_draw_buf_destroy)>;
    Snapshot snapshot(lv_snapshot_take(label, LV_COLOR_FORMAT_ARGB8888), lv_draw_buf_destroy);
    if (!snapshot) return false;

    const int width = snapshot->header.w + 2 * padding;
    const int height = snapshot->header.h + 2 * padding;
    const int stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_A8);
    const uint32_t bytes = stride * height;
    pixels_.reset(static_cast<uint8_t *>(
        heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
    if (!pixels_) return false;
    auto *alpha = pixels_.get();
    std::memset(alpha, 0, bytes);
    for (int y = 0; y < snapshot->header.h; ++y) {
      const auto *row = reinterpret_cast<const lv_color32_t *>(
          snapshot->data + y * snapshot->header.stride);
      auto *out = alpha + (y + padding) * stride + padding;
      for (int x = 0; x < snapshot->header.w; ++x) out[x] = row[x].alpha;
    }

    // Reuse the temporary snapshot storage for the horizontal pass when it fits.
    std::unique_ptr<uint8_t, decltype(&std::free)> scratch(nullptr, std::free);
    if (bytes > snapshot->header.stride * snapshot->header.h) {
      snapshot.reset();
      scratch.reset(static_cast<uint8_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
      if (!scratch) {
        pixels_.reset();
        return false;
      }
    }
    auto *intermediate = snapshot ? snapshot->data : scratch.get();
    for (int radius : radii) {
      blur_lines(alpha, intermediate, height, width, stride, 1, radius);
      blur_lines(intermediate, alpha, width, height, 1, stride, radius);
    }

    mask_.header.magic = LV_IMAGE_HEADER_MAGIC;
    mask_.header.cf = LV_COLOR_FORMAT_A8;
    mask_.header.w = width;
    mask_.header.h = height;
    mask_.header.stride = stride;
    mask_.data_size = bytes;
    mask_.data = alpha;
    return true;
  }

  std::unique_ptr<uint8_t, decltype(&std::free)> pixels_{nullptr, std::free};
  lv_obj_t *image_ = nullptr;
  lv_image_dsc_t mask_{};
  std::string text_;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace espcontrol
#endif
