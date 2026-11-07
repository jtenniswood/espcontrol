#pragma once

#include "page_basic_lvgl_port.h"
#include "page_card_commands.h"

namespace espcontrol::pages {
// Application-owned overlay boundary. Keep full origin bindings for accepted
// panel feedback while revoking every old grid intent and held gesture.
template <size_t Slots>
class PageGridInput {
 public:
  PageGridInput(PageBasicLvglPort<Slots>& view, PageRendererRuntime<Slots>& renderer,
                PageCardCommands& commands)
      : view_(view), renderer_(renderer), commands_(commands) {}
  PageGridInput(const PageGridInput&) = delete;
  PageGridInput& operator=(const PageGridInput&) = delete;
  // Scalar-only cancellation policies: safe inside HA/LVGL/native callbacks.
  void cover(bool covered) {
    view_.set_input_covered(covered);
    if (covered) commands_.cancel();
  }
  bool settle() { return view_.settle_input(); }
  bool valid(PageBindingToken origin) const { return view_.input_enabled() && renderer_.valid(origin); }
  bool bound(PageBindingToken origin) const { return renderer_.bound(origin); }

 private:
  PageBasicLvglPort<Slots>& view_;
  PageRendererRuntime<Slots>& renderer_;
  PageCardCommands& commands_;
};
}  // namespace espcontrol::pages
