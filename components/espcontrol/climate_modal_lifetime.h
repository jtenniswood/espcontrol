#pragma once

struct ClimateControlCtx;
// Optional selected-runtime bridge. Keep ownership outside ClimateControlCtx
// so the legacy renderer does not gain per-card lifetime fields. An installed
// bridge owns every climate modal and rejects commands for retired contexts.
struct ClimateModalLifetime {
  void *owner{nullptr};
  bool (*managed)(void *,const ClimateControlCtx *){nullptr};
  bool (*current)(void *,const ClimateControlCtx *){nullptr};
  bool (*action)(void *,const ClimateControlCtx *){nullptr};
  void (*closed)(void *,ClimateControlCtx *){nullptr};
  bool (*widgets)(void *,void *){nullptr};
};
inline ClimateModalLifetime &climate_modal_lifetime(){static ClimateModalLifetime observer;return observer;}
inline bool climate_modal_managed(const ClimateControlCtx *ctx) {
  const auto &observer=climate_modal_lifetime();return observer.owner&&observer.managed&&observer.managed(observer.owner,ctx);
}
inline bool climate_modal_context_current(const ClimateControlCtx *ctx) {
  const auto &observer=climate_modal_lifetime();return ctx&&(!observer.owner||(observer.current&&observer.current(observer.owner,ctx)));
}
inline bool climate_modal_action_allowed(const ClimateControlCtx *ctx) {
  const auto &observer=climate_modal_lifetime();return ctx&&(!observer.owner||(observer.action&&observer.action(observer.owner,ctx)));
}
inline void climate_modal_notify_closed(ClimateControlCtx *ctx) {
  const auto observer=climate_modal_lifetime();if(ctx&&observer.owner&&observer.closed)observer.closed(observer.owner,ctx);
}
inline bool climate_modal_widgets_ready(void *root) {
  const auto observer=climate_modal_lifetime();return !observer.owner||(observer.widgets&&observer.widgets(observer.owner,root));
}
