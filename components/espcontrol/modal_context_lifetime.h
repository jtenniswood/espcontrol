#pragma once

#include <cstdint>

namespace espcontrol {
// Optional observer shared by demand-created legacy modal adapters. Keep it
// outside card contexts: the unselected renderer gains no per-card fields.
// Each family's facade owns its own observer, even if a test aliases contexts.
template<typename Context> struct ModalContextLifetime {
  void *owner{nullptr};
  bool (*managed)(void *,const Context *){nullptr};
  bool (*current)(void *,const Context *){nullptr};
  bool (*action)(void *,const Context *){nullptr};
  void (*closed)(void *,Context *){nullptr};
  bool (*widgets)(void *,void *){nullptr};
  void (*numeric_action)(void *,const Context *,float){nullptr};
  uint64_t (*identity)(void *,const Context *){nullptr};
  void (*state_confirmed)(void *,const Context *,bool){nullptr};
  bool (*refresh)(void *,const Context *){nullptr};
  uint64_t context_identity(const Context *ctx) const {
    return ctx&&owner&&identity?identity(owner,ctx):0;
  }
  bool managed_context(const Context *ctx) const {return owner&&managed&&managed(owner,ctx);}
  bool current_context(const Context *ctx) const {return ctx&&(!owner||(current&&current(owner,ctx)));}
  bool action_allowed(const Context *ctx) const {return ctx&&(!owner||(action&&action(owner,ctx)));}
  void notify_closed(Context *ctx) const {
    const auto observer=*this;
    if(ctx&&observer.owner&&observer.closed)observer.closed(observer.owner,ctx);
  }
  // The owner maps this exact live context to its opening token before
  // forwarding an optimistic numeric value. Never resolve a replacement card.
  void notify_numeric_action(const Context *ctx,float value) const {
    const auto observer=*this;
    if(ctx&&observer.owner&&observer.numeric_action&&observer.action_allowed(ctx))
      observer.numeric_action(observer.owner,ctx,value);
  }
  bool refresh_current(const Context* ctx) const {
    const auto observer = *this;
    return ctx && (!observer.owner || (observer.refresh && observer.refresh(observer.owner, ctx)));
  }
  void notify_state_confirmed(const Context* ctx, bool value) const {
    const auto observer = *this;
    if (ctx && observer.owner && observer.state_confirmed && observer.current_context(ctx))
      observer.state_confirmed(observer.owner, ctx, value);
  }
  bool widgets_ready(void *root) const {
    const auto observer=*this;
    return !observer.owner||(observer.widgets&&observer.widgets(observer.owner,root));
  }
};
}  // namespace espcontrol
