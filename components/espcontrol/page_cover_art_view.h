#pragma once

#include "lvgl.h"
#include "page_image_runtime.h"

namespace espcontrol::pages {
struct PageCoverArtSurface {
  const lv_image_dsc_t* source{nullptr};
  uint64_t asset{0};
  uint32_t tint{0};
};
// A borrowed bridge to the one shared artwork pool. The fixed grid bank owns
// only draw commands, never component descriptors or downloaded pixel buffers.
class PageCoverArtView {
 public:
  virtual ~PageCoverArtView() = default;
  virtual bool ready() const = 0;
  virtual void bind(PageBindingToken) = 0;
  virtual void unbind(PageBindingToken) = 0;
  virtual void size(PageBindingToken, uint16_t width, uint16_t height) = 0;
  virtual PageCoverArtSurface surface(PageBindingToken) = 0;
  virtual uint64_t revision() const = 0;
};
}  // namespace espcontrol::pages
