#pragma once

#include <array>
#include <string>

#include "esphome/components/artwork_image/artwork_image.h"
#include "page_image_runtime.h"

namespace espcontrol::pages {
// Claims the existing YAML components once, before preparing a reusable bank.
// Components outlive this adapter, which outlives its PageImageRuntime. This
// adapter cannot share components with the legacy grid or animation callbacks.
template <size_t Slots>
class PageArtworkRequests final : public PageImageRequests {
 public:
  static_assert(Slots > 0 && Slots <= UINT16_MAX);
  ~PageArtworkRequests() override {
    shutdown();
  }
  PageArtworkRequests() = default;
  PageArtworkRequests(const PageArtworkRequests&) = delete;
  PageArtworkRequests& operator=(const PageArtworkRequests&) = delete;
  bool initialize(esphome::artwork_image::ArtworkImage* const* images, size_t count) {
    if (initialized_ || images == nullptr || count != Slots) return false;
    for (size_t i = 0; i < Slots; ++i) {
      auto& s = slots_[i];
      s.slot = i;
      s.adapter = this;
      if (images[i] == nullptr || !images[i]->claim_owner(s)) {
        shutdown();
        return false;
      }
      s.image = images[i];
      auto* slot = &s;
      s.image->set_animation_callbacks(
          [slot]() { return slot->adapter->frames_ && slot->adapter->frames_->image_visible(slot->slot); },
          [slot]() {
            if (slot->adapter->frames_) slot->adapter->frames_->image_redraw(slot->slot);
          },
          [slot]() {
            return slot->adapter->frames_ && slot->adapter->frames_->image_screen_active(slot->slot);
          });
    }
    initialized_ = true;
    return true;
  }
  bool claim_frames(PageImageFrames& owner) override {
    if (frames_ && frames_ != &owner) return false;
    frames_ = &owner;
    return true;
  }
  void release_frames(PageImageFrames& owner) override {
    if (frames_ == &owner) frames_ = nullptr;
  }
  bool start(size_t slot, uint64_t request, std::string_view url, PageImageOptions options, void* owner,
             Receive receive) override {
    if (!initialized_ || slot >= Slots || request == 0 || url.empty() || owner == nullptr ||
        receive == nullptr || options.width == 0 || options.height == 0)
      return false;
    auto& s = slots_[slot];
    s.owner = owner;
    s.receive = receive;
    s.image->set_target_size(options.width, options.height);
    s.image->set_resize_mode(options.cover ? esphome::artwork_image::ImageResizeMode::COVER
                                           : esphome::artwork_image::ImageResizeMode::FIT);
    s.image->set_fit_background_color(options.background);
    s.image->set_request_priority(esphome::artwork_image::ImageRequestPriority::COVER_ART);
    return s.image->request_owned_update(s, request, std::string(url));
  }
  void cancel(size_t slot) override {
    if (slot >= Slots || slots_[slot].image == nullptr) return;
    auto& s = slots_[slot];
    s.owner = nullptr;
    s.receive = nullptr;
    s.image->cancel_update();
  }
  void release_surface(size_t slot) override {
    if (slot >= Slots || slots_[slot].image == nullptr) return;
#ifdef USE_LVGL
    // Maintenance runs outside refresh/draw callbacks, after the view detached
    // the descriptor. Finish hardware draw-unit reads before freeing pixels.
    lv_draw_wait_for_finish();
#endif
    slots_[slot].image->release();
  }
  bool initialized() const {
    return initialized_;
  }
  esphome::artwork_image::ArtworkImage* image(size_t slot) const {
    return slot < Slots ? slots_[slot].image : nullptr;
  }

 private:
  struct Slot final : esphome::artwork_image::ImageResultObserver {
    PageArtworkRequests* adapter{nullptr};
    esphome::artwork_image::ArtworkImage* image{nullptr};
    void* owner{nullptr};
    Receive receive{nullptr};
    size_t slot{0};
    void image_result(esphome::artwork_image::ImageResult result) override {
      // Take the callback by value; retirement can clear/rebind this record.
      const auto callback = receive;
      void* target = owner;
      if (callback != nullptr && target != nullptr)
        callback(target, slot, result.request, result.success && image->has_image(), result.cached);
    }
  };
  void shutdown() {
    for (size_t i = 0; i < Slots; ++i) {
      auto& s = slots_[i];
      if (s.image == nullptr) continue;
      cancel(i);
      s.image->set_animation_callbacks({}, {}, {});
      s.image->release_owner(s);
      s.image = nullptr;
    }
    initialized_ = false;
  }
  std::array<Slot, Slots> slots_{};
  PageImageFrames* frames_{nullptr};
  bool initialized_{false};
};
}  // namespace espcontrol::pages
