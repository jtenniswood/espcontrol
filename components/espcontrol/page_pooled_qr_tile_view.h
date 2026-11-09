#pragma once

#include "page_lvgl_qr_frames.h"
#include "page_qr_tile_view.h"
#include "wifi_qr_layout.h"

namespace espcontrol::pages {
class PagePooledQrTileView final : public PageQrTileView {
 public:
  PagePooledQrTileView(PageQrRuntime<>& runtime, PageLvglQrFrames& frames)
      : runtime_(runtime), frames_(frames) {}
  bool ready() const override {
    return runtime_.available();
  }
  size_t capacity() const override {
    return runtime_.capacity();
  }
  void bind(PageBindingToken token, std::string_view payload) override {
    runtime_.bind(token.shell, token, payload);
  }
  void unbind(PageBindingToken token) override {
    runtime_.unbind(token.shell, token);
  }
  void size(PageBindingToken token, uint16_t width, uint16_t height, uint8_t rows, uint8_t columns) override {
    const int side = std::min<int>(width, height - 2 * wifi_qr_tile_vertical_inset(rows, columns));
    runtime_.size(token.shell, token, side >= 48 ? side : 0, rows == 1 && columns == 1);
  }
  PageQrSnapshot snapshot(PageBindingToken token) const override {
    return runtime_.snapshot(token.shell, token);
  }
  bool draw(PageBindingToken token, uint64_t asset, lv_obj_t* surface, lv_layer_t* layer) const override {
    const auto state = snapshot(token);
    return state.ready && asset && state.asset == asset && frames_.draw(token.shell, surface, layer);
  }
  uint64_t revision() const override {
    return runtime_.revision();
  }

 private:
  PageQrRuntime<>& runtime_;
  PageLvglQrFrames& frames_;
};
}  // namespace espcontrol::pages
