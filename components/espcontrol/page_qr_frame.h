#pragma once

#include "lvgl.h"
#include "page_qr_decoder.h"
#include "page_qr_matrix.h"
#include "src/misc/cache/instance/lv_image_cache.h"

namespace espcontrol::pages {
// Persistent descriptor per visible slot. Caller guards the full binding/asset
// before drawing, hides before unbind, then fences draw work before overwriting
// either this descriptor or its matrix. Never pass a stack descriptor to LVGL.
class PageQrFrame {
 public:
  explicit PageQrFrame(PageQrDecoder& decoder) : decoder_(decoder) {}
  ~PageQrFrame() {
    assert(!registered_);
  }
  PageQrFrame(const PageQrFrame&) = delete;
  PageQrFrame& operator=(const PageQrFrame&) = delete;
  void hide() {
    shown_ = false;
  }
  void reset_after_draw() {
    lv_image_cache_drop(&image_);
    if (registered_) {
      decoder_.release_frame();
      registered_ = false;
    }
    image_ = {};
    layout_ = {};
    shown_ = false;
    data_ = {};
  }
  void assign_after_draw(const PageQrMatrix& matrix) {
    reset_after_draw();
    if (!matrix.layout.valid() || !decoder_.ready()) return;
    layout_ = matrix.layout;
    image_.header.magic = LV_IMAGE_HEADER_MAGIC;
    image_.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;
    image_.header.flags = LV_IMAGE_FLAGS_USER8;
    image_.header.w = image_.header.h = layout_.side;
    data_.owner = &decoder_;
    data_.matrix = &matrix;
    image_.data_size = sizeof(data_);
    image_.data = reinterpret_cast<const uint8_t*>(&data_);
    decoder_.retain_frame();
    registered_ = shown_ = true;
  }
  bool draw(lv_obj_t* surface, lv_layer_t* layer) const {
    if (!shown_ || !surface || !layer || !image_.data) return false;
    const auto width = lv_obj_get_width(surface), height = lv_obj_get_height(surface);
    if (width < layout_.side || height < layout_.side) return false;
    lv_area_t box;
    lv_obj_get_coords(surface, &box);
    const int32_t x = box.x1 + (width - layout_.side) / 2;
    const int32_t y = box.y1 + (height - layout_.side) / 2;
    const lv_area_t area{x, y, x + layout_.side - 1, y + layout_.side - 1};
    lv_draw_image_dsc_t image;
    lv_draw_image_dsc_init(&image);
    image.base.layer = layer;
    lv_obj_init_draw_image_dsc(surface, LV_PART_MAIN, &image);
    image.src = &image_;
    image.scale_x = image.scale_y = LV_SCALE_NONE;
    image.pivot = {0, 0};
    image.antialias = false;
    image.image_area = area;
    image.recolor = lv_color_black();
    image.recolor_opa = LV_OPA_COVER;
    lv_draw_image(layer, &image, &area);
    return true;
  }

 private:
  PageQrDecoder& decoder_;
  PageQrImageData data_;
  lv_image_dsc_t image_{};
  PageQrLayout layout_;
  bool shown_{false}, registered_{false};
};
}  // namespace espcontrol::pages
