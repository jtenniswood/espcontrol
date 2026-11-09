#pragma once

#include "modal_context_lifetime.h"
struct LightControlCtx;
struct CoverControlCtx;
using LightModalLifetime=espcontrol::ModalContextLifetime<LightControlCtx>;
using CoverModalLifetime=espcontrol::ModalContextLifetime<CoverControlCtx>;
inline LightModalLifetime &light_modal_lifetime(){static LightModalLifetime observer;return observer;}
inline CoverModalLifetime &cover_modal_lifetime(){static CoverModalLifetime observer;return observer;}
inline bool light_modal_managed(const LightControlCtx *ctx){return light_modal_lifetime().managed_context(ctx);}
inline bool light_modal_context_current(const LightControlCtx *ctx){return light_modal_lifetime().current_context(ctx);}
inline bool light_modal_action_allowed(const LightControlCtx *ctx){return light_modal_lifetime().action_allowed(ctx);}
inline void light_modal_notify_closed(LightControlCtx *ctx){light_modal_lifetime().notify_closed(ctx);}
inline bool light_modal_widgets_ready(void *root){return light_modal_lifetime().widgets_ready(root);}
inline bool cover_modal_managed(const CoverControlCtx *ctx){return cover_modal_lifetime().managed_context(ctx);}
inline bool cover_modal_context_current(const CoverControlCtx *ctx){return cover_modal_lifetime().current_context(ctx);}
inline bool cover_modal_action_allowed(const CoverControlCtx *ctx){return cover_modal_lifetime().action_allowed(ctx);}
inline void cover_modal_notify_closed(CoverControlCtx *ctx){cover_modal_lifetime().notify_closed(ctx);}
inline bool cover_modal_widgets_ready(void *root){return cover_modal_lifetime().widgets_ready(root);}
