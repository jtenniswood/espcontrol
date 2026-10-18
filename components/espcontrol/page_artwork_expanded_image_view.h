#pragma once

#include "page_artwork_requests.h"
#include "page_expanded_image_view.h"
#include "page_image_tile_view.h"

namespace espcontrol::pages {
template <size_t Slots>
class PageArtworkExpandedImageView final : public PageExpandedImageView {
 public:
  PageArtworkExpandedImageView(PageExpandedImageSource<Slots>& source, PageArtworkRequests<1>& requests,
                               PageImageTileView& tiles)
      : source_(source), requests_(requests), tiles_(tiles) {}
  bool ready() const override {
    return source_.available() && requests_.initialized() && tiles_.ready();
  }
  bool bind(PageModalToken token, PageImageTileSource source) override {
    return source_.bind(token, source);
  }
  void unbind(PageModalToken token) override {
    source_.unbind(token);
  }
  void size(PageModalToken token, uint16_t w, uint16_t h, uint32_t background) override {
    source_.size(token, w, h, background);
  }
  PageExpandedImageSurface surface(PageModalToken token) override {
    PageExpandedImageSurface result;
    result.policy = source_.snapshot(token);
    if (result.policy.status != PageImageTileStatus::READY) return result;
    if (result.policy.asset) {
      auto* image = requests_.image(0);
      if (image) result.source = image->get_lv_image_dsc();
    } else if (result.policy.preview.binding) {
      const auto tile = tiles_.surface(result.policy.preview);
      if (tile.policy.status == PageImageTileStatus::READY) {
        result.source = tile.source;
        result.preview_asset = tile.policy.asset;
      }
    }
    if (result.source && !result.source->data) result.source = nullptr;
    return result;
  }
  uint64_t revision() const override {
    return source_.revision();
  }

 private:
  PageExpandedImageSource<Slots>& source_;
  PageArtworkRequests<1>& requests_;
  PageImageTileView& tiles_;
};
}  // namespace espcontrol::pages
