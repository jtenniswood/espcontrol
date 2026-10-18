#pragma once

#include "page_qr_frame.h"
#include "page_qr_runtime.h"

namespace espcontrol::pages {
// Fixed descriptors in the scene's external arena, one per visible lease.
// The scene locks LVGL around preflight, maintenance and final shutdown.
class PageLvglQrFrames final : public PageQrFrames {
 public:
  explicit PageLvglQrFrames(PageArenaAllocator& allocator) : allocator_(allocator) {}
  ~PageLvglQrFrames() override {
    const bool stopped = shutdown_after_draw();
    assert(stopped);
    (void)stopped;
  }
  PageLvglQrFrames(const PageLvglQrFrames&) = delete;
  PageLvglQrFrames& operator=(const PageLvglQrFrames&) = delete;
  bool initialize(size_t count) override {
    if (frames_) return count == count_;
    if (!count || count > UINT8_MAX || !decoder_.initialize()) return false;
    void* memory = allocator_.allocate(count * sizeof(PageQrFrame));
    if (!memory) {
      const bool stopped = decoder_.shutdown();
      assert(stopped);
      (void)stopped;
      return false;
    }
    frames_ = static_cast<PageQrFrame*>(memory);
    count_ = count;
    for (size_t i = 0; i < count_; ++i) new (frames_ + i) PageQrFrame(decoder_);
    return true;
  }
  void hide(size_t slot) override {
    if (slot < count_) frames_[slot].hide();
  }
  bool wait_for_draw() override {
    lv_draw_wait_for_finish();
    return decoder_.sessions() == 0;
  }
  void reset_after_draw(size_t slot) override {
    if (slot < count_) frames_[slot].reset_after_draw();
  }
  bool assign_after_draw(size_t slot, const PageQrMatrix& matrix) override {
    if (slot >= count_ || !matrix.layout.valid() || !decoder_.ready()) return false;
    frames_[slot].assign_after_draw(matrix);
    return true;
  }
  bool shutdown_after_draw() override {
    if (!decoder_.shutdown()) return false;
    for (size_t i = 0; i < count_; ++i) frames_[i].~PageQrFrame();
    if (frames_) allocator_.release(frames_, count_ * sizeof(PageQrFrame));
    frames_ = nullptr;
    count_ = 0;
    return true;
  }
  bool draw(size_t slot, lv_obj_t* surface, lv_layer_t* layer) const {
    return slot < count_ && frames_[slot].draw(surface, layer);
  }
  bool idle() const {
    return decoder_.sessions() == 0;
  }
  size_t reserved_bytes() const {
    return count_ * sizeof(PageQrFrame);
  }

 private:
  PageArenaAllocator& allocator_;
  PageQrDecoder decoder_;
  PageQrFrame* frames_{nullptr};
  size_t count_{0};
};
}  // namespace espcontrol::pages
