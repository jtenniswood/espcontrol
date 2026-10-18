#pragma once

#include "modal_context_lifetime.h"
struct FanCardCtx;
using FanControlModalLifetime = espcontrol::ModalContextLifetime<FanCardCtx>;
inline FanControlModalLifetime& fan_control_modal_lifetime() {
  static FanControlModalLifetime observer;
  return observer;
}
inline FanControlModalLifetime& fan_preset_modal_lifetime() {
  static FanControlModalLifetime observer;
  return observer;
}
// Both fan panel families share a legacy context type, but own separate records.
// Route by that exact record; an installed observer rejects all other contexts.
inline FanControlModalLifetime* fan_modal_observer(const FanCardCtx* ctx) {
  if (fan_control_modal_lifetime().managed_context(ctx)) return &fan_control_modal_lifetime();
  if (fan_preset_modal_lifetime().managed_context(ctx)) return &fan_preset_modal_lifetime();
  return nullptr;
}
inline bool fan_modal_observed() {
  return fan_control_modal_lifetime().owner || fan_preset_modal_lifetime().owner;
}
inline bool fan_modal_managed(const FanCardCtx* ctx) {
  return fan_modal_observer(ctx);
}
inline bool fan_modal_context_current(const FanCardCtx* ctx) {
  const auto* observer = fan_modal_observer(ctx);
  return observer ? observer->current_context(ctx) : ctx && !fan_modal_observed();
}
inline bool fan_modal_action_allowed(const FanCardCtx* ctx) {
  const auto* observer = fan_modal_observer(ctx);
  return observer ? observer->action_allowed(ctx) : ctx && !fan_modal_observed();
}
inline uint64_t fan_modal_context_identity(const FanCardCtx* ctx) {
  const auto* observer = fan_modal_observer(ctx);
  return observer ? observer->context_identity(ctx) : 0;
}
inline void fan_modal_notify_closed(FanCardCtx* ctx) {
  if (const auto* observer = fan_modal_observer(ctx)) observer->notify_closed(ctx);
}
inline bool fan_modal_widgets_ready(const FanCardCtx* ctx, void* root) {
  const auto* observer = fan_modal_observer(ctx);
  return observer ? observer->widgets_ready(root) : !fan_modal_observed();
}
