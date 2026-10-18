#pragma once

#include "modal_context_lifetime.h"
struct FanCardCtx;
using FanControlModalLifetime = espcontrol::ModalContextLifetime<FanCardCtx>;
inline FanControlModalLifetime& fan_control_modal_lifetime() {
  static FanControlModalLifetime observer;
  return observer;
}
inline bool fan_modal_managed(const FanCardCtx* ctx) {
  return fan_control_modal_lifetime().managed_context(ctx);
}
inline bool fan_modal_context_current(const FanCardCtx* ctx) {
  return fan_control_modal_lifetime().current_context(ctx);
}
inline bool fan_modal_action_allowed(const FanCardCtx* ctx) {
  return fan_control_modal_lifetime().action_allowed(ctx);
}
inline void fan_modal_notify_closed(FanCardCtx* ctx) {
  fan_control_modal_lifetime().notify_closed(ctx);
}
inline bool fan_modal_widgets_ready(void* root) {
  return fan_control_modal_lifetime().widgets_ready(root);
}
