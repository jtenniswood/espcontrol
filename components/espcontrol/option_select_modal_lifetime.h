#pragma once
#include "modal_context_lifetime.h"
struct OptionSelectCtx;
using OptionSelectModalLifetime = espcontrol::ModalContextLifetime<OptionSelectCtx>;
inline OptionSelectModalLifetime& option_select_modal_lifetime() {
  static OptionSelectModalLifetime observer;
  return observer;
}
inline bool option_select_context_current(const OptionSelectCtx* ctx) {
  return option_select_modal_lifetime().current_context(ctx);
}
inline bool option_select_action_allowed(const OptionSelectCtx* ctx) {
  return option_select_modal_lifetime().action_allowed(ctx);
}
