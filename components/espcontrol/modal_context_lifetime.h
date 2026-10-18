#pragma once

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
  bool managed_context(const Context *ctx) const {return owner&&managed&&managed(owner,ctx);}
  bool current_context(const Context *ctx) const {return ctx&&(!owner||(current&&current(owner,ctx)));}
  bool action_allowed(const Context *ctx) const {return ctx&&(!owner||(action&&action(owner,ctx)));}
  void notify_closed(Context *ctx) const {
    const auto observer=*this;
    if(ctx&&observer.owner&&observer.closed)observer.closed(observer.owner,ctx);
  }
  bool widgets_ready(void *root) const {
    const auto observer=*this;
    return !observer.owner||(observer.widgets&&observer.widgets(observer.owner,root));
  }
};
}  // namespace espcontrol
