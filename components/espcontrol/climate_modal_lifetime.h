#pragma once

#include "modal_context_lifetime.h"

struct ClimateControlCtx;
// Optional selected-runtime bridge. Keep ownership outside ClimateControlCtx
// so the legacy renderer does not gain per-card lifetime fields. An installed
// bridge owns every climate modal and rejects commands for retired contexts.
using ClimateModalLifetime=espcontrol::ModalContextLifetime<ClimateControlCtx>;
inline ClimateModalLifetime &climate_modal_lifetime(){static ClimateModalLifetime observer;return observer;}
inline bool climate_modal_managed(const ClimateControlCtx *ctx) {
  return climate_modal_lifetime().managed_context(ctx);
}
inline bool climate_modal_context_current(const ClimateControlCtx *ctx) {
  return climate_modal_lifetime().current_context(ctx);
}
inline bool climate_modal_action_allowed(const ClimateControlCtx *ctx) {
  return climate_modal_lifetime().action_allowed(ctx);
}
inline void climate_modal_notify_closed(ClimateControlCtx *ctx) {
  climate_modal_lifetime().notify_closed(ctx);
}
inline bool climate_modal_widgets_ready(void *root) {
  return climate_modal_lifetime().widgets_ready(root);
}
