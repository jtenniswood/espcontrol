#pragma once

#include "lvgl.h"
#include "page_image_tile_runtime.h"

namespace espcontrol::pages {
struct PageImageTileSurface {
  PageImageTileSnapshot policy;
  const lv_image_dsc_t* source{nullptr};
};
class PageImageTileView {
 public:
  virtual ~PageImageTileView() = default;
  virtual bool ready() const = 0;
  virtual void bind(PageBindingToken, PageImageTileSource) = 0;
  virtual void unbind(PageBindingToken) = 0;
  virtual void size(PageBindingToken, uint16_t width, uint16_t height, uint32_t background) = 0;
  virtual PageImageTileSurface surface(PageBindingToken) = 0;
  virtual uint64_t revision() const = 0;
};
}  // namespace espcontrol::pages
