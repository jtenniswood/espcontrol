#pragma once

#include <cstdint>

namespace esphome::artwork_image {
struct ImageResult {
  uint64_t request{0};
  bool success{false}, cached{false};
};

class ImageResultObserver {
 public:
  virtual ~ImageResultObserver() = default;
  virtual void image_result(ImageResult result) = 0;
};

// An exclusive, main-loop-only completion channel for a reusable image slot.
// The observer outlives its claim. No configuration/URL/widget is retained here.
// Cancel before releasing a claim; workers never call this channel directly.
class ImageResultChannel {
 public:
  ImageResultChannel() = default;
  ImageResultChannel(const ImageResultChannel&) = delete;
  ImageResultChannel& operator=(const ImageResultChannel&) = delete;
  bool claim(ImageResultObserver& observer) {
    if (observer_ != nullptr && observer_ != &observer) return false;
    observer_ = &observer;
    return true;
  }
  bool owned_by(const ImageResultObserver& observer) const {
    return observer_ == &observer;
  }
  bool claimed() const {
    return observer_ != nullptr;
  }
  void clear() {
    request_ = 0;
  }
  void release(ImageResultObserver& observer) {
    if (!owned_by(observer)) return;
    clear();
    observer_ = nullptr;
  }
  bool arm(ImageResultObserver& observer, uint32_t generation, uint64_t request) {
    if (!owned_by(observer) || request == 0) return false;
    generation_ = generation;
    request_ = request;
    return true;
  }
  bool deliver(uint32_t generation, bool success, bool cached) {
    if (observer_ == nullptr || request_ == 0 || generation != generation_) return false;
    const ImageResult result{request_, success, cached};
    auto* observer = observer_;
    // Reentrant retirement/rebind may arm a replacement. Consume first and do
    // not access the observer or change channel state after dispatch.
    clear();
    observer->image_result(result);
    return true;
  }

 private:
  ImageResultObserver* observer_{nullptr};
  uint64_t request_{0};
  uint32_t generation_{0};
};
}  // namespace esphome::artwork_image
