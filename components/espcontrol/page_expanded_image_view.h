#pragma once

#include "lvgl.h"
#include "page_expanded_image_source.h"

namespace espcontrol::pages {
struct PageExpandedImageSurface {
  PageExpandedImageSnapshot policy;
  const lv_image_dsc_t* source{nullptr};
  uint64_t preview_asset{0};
  // Separate components can produce identical numeric assets. Role and full
  // preview binding must match too before a queued draw can use these pixels.
  bool same_frame(const PageExpandedImageSurface& other) const {
    return (policy.asset || policy.preview.binding) && policy.bound && other.policy.bound &&
           policy.status == PageImageTileStatus::READY && other.policy.status == PageImageTileStatus::READY &&
           policy.asset == other.policy.asset && preview_asset == other.preview_asset && source &&
           source == other.source &&
           ((!policy.preview.binding && !other.policy.preview.binding) ||
            PageBindingImageIdentity::same(policy.preview, other.policy.preview));
  }
};
class PageExpandedImageView {
 public:
  virtual ~PageExpandedImageView() = default;
  virtual bool ready() const = 0;
  virtual bool bind(PageModalToken, PageImageTileSource) = 0;
  virtual void unbind(PageModalToken) = 0;
  virtual void size(PageModalToken, uint16_t width, uint16_t height, uint32_t background) = 0;
  virtual PageExpandedImageSurface surface(PageModalToken) = 0;
  virtual uint64_t revision() const = 0;
};
}  // namespace espcontrol::pages
