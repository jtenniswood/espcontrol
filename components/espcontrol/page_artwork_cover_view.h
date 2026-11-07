#pragma once

#include "cover_art.h"
#include "page_artwork_requests.h"
#include "page_cover_art_runtime.h"
#include "page_cover_art_view.h"

namespace espcontrol::pages {
template <size_t Slots>
class PageArtworkCoverView final : public PageCoverArtView {
 public:
  PageArtworkCoverView(PageCoverArtRuntime<Slots>& runtime, PageArtworkRequests<Slots>& requests)
      : runtime_(runtime), requests_(requests) {}
  bool ready() const override {
    return requests_.initialized() && runtime_.available();
  }
  void bind(PageBindingToken token) override {
    runtime_.bind(token);
  }
  void unbind(PageBindingToken token) override {
    runtime_.unbind(token);
  }
  void size(PageBindingToken token, uint16_t width, uint16_t height) override {
    runtime_.size(token, width, height);
  }
  PageCoverArtSurface surface(PageBindingToken token) override {
    const auto snapshot = runtime_.snapshot(token);
    if (!snapshot.ready || !snapshot.asset) return {};
    const auto slot = runtime_.find(token);
    if (slot >= Slots) return {};
    auto* image = requests_.image(slot);
    if (!image) return {};
    // Animated frames invalidate the component's descriptor. Rebuild it on
    // every pull; the address remains component-owned and stable until release.
    const auto* source = image->get_lv_image_dsc();
    if (!source || !source->data) return {};
    auto& accent = accents_[slot];
    if (accent.asset != snapshot.asset) {
      const auto color = cover_art::darken_accent_color(cover_art::extract_accent_color_rgb565(
          source->data, image->get_width(), image->get_height(), image->is_big_endian(),
          image->get_content_offset_x(), image->get_content_offset_y(), image->get_content_width(),
          image->get_content_height()));
      accent.asset = snapshot.asset;
      accent.color = color.valid ? (static_cast<uint32_t>(color.red) << 16) |
                                       (static_cast<uint32_t>(color.green) << 8) | color.blue
                                 : 0;
    }
    return {source, snapshot.asset, accent.color};
  }
  uint64_t revision() const override {
    return runtime_.revision();
  }

 private:
  struct Accent {
    uint64_t asset{0};
    uint32_t color{0};
  };
  PageCoverArtRuntime<Slots>& runtime_;
  PageArtworkRequests<Slots>& requests_;
  std::array<Accent, Slots> accents_{};
};
}  // namespace espcontrol::pages
