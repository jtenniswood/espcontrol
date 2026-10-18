#pragma once

#include "page_artwork_requests.h"
#include "page_image_tile_view.h"

namespace espcontrol::pages {
template <size_t Slots>
class PageArtworkImageTileView final : public PageImageTileView {
 public:
  PageArtworkImageTileView(PageImageTileRuntime<Slots>& runtime, PageArtworkRequests<Slots>& requests)
      : runtime_(runtime), requests_(requests) {}
  bool ready() const override {
    return requests_.initialized() && runtime_.available();
  }
  void bind(PageBindingToken token, PageImageTileSource source) override {
    runtime_.bind(token, source);
  }
  void unbind(PageBindingToken token) override {
    runtime_.unbind(token);
  }
  void size(PageBindingToken token, uint16_t width, uint16_t height, uint32_t background) override {
    runtime_.size(token, width, height, background);
  }
  PageImageTileSurface surface(PageBindingToken token) override {
    PageImageTileSurface result{runtime_.snapshot(token)};
    if (result.policy.status != PageImageTileStatus::READY || !result.policy.asset) return result;
    const auto slot = runtime_.find(token);
    if (slot >= Slots) return result;
    auto* image = requests_.image(slot);
    if (!image) return result;
    const auto* source = image->get_lv_image_dsc();
    if (source && source->data) result.source = source;
    return result;
  }
  uint64_t revision() const override {
    return runtime_.revision();
  }

 private:
  PageImageTileRuntime<Slots>& runtime_;
  PageArtworkRequests<Slots>& requests_;
};
}  // namespace espcontrol::pages
