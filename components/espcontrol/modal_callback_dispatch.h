#pragma once

#include <cassert>
#include <cstdint>

namespace espcontrol {
// Action responses can reenter configuration replacement outside LVGL or HA
// read dispatch. Keep retired panels alive until these callbacks also return.
inline uint32_t& modal_callback_depth() {
  static uint32_t depth = 0;
  return depth;
}
inline bool modal_callback_dispatching() {
  return modal_callback_depth() != 0;
}
class ModalCallbackDispatchScope {
 public:
  ModalCallbackDispatchScope() {
    ++modal_callback_depth();
  }
  ~ModalCallbackDispatchScope() {
    assert(modal_callback_depth() != 0);
    --modal_callback_depth();
  }
  ModalCallbackDispatchScope(const ModalCallbackDispatchScope&) = delete;
  ModalCallbackDispatchScope& operator=(const ModalCallbackDispatchScope&) = delete;
};
}  // namespace espcontrol
