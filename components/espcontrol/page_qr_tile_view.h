#pragma once

#include "lvgl.h"
#include "page_qr_runtime.h"

namespace espcontrol::pages {
class PageQrTileView {
 public:
  virtual ~PageQrTileView() = default;
  virtual bool ready() const = 0;
  virtual size_t capacity() const = 0;
  virtual void bind(PageBindingToken, std::string_view payload) = 0;
  virtual void unbind(PageBindingToken) = 0;
  virtual void size(PageBindingToken, uint16_t width, uint16_t height, uint8_t rows, uint8_t columns) = 0;
  virtual PageQrSnapshot snapshot(PageBindingToken) const = 0;
  virtual bool draw(PageBindingToken, uint64_t asset, lv_obj_t*, lv_layer_t*) const = 0;
  virtual uint64_t revision() const = 0;
};
}  // namespace espcontrol::pages
