#pragma once

#include <array>
#include <cassert>

#include "page_modal_runtime.h"

namespace espcontrol::pages {
struct PageModalRoute {
  ControlModalKind kind;
  PageModalPort* port;
};

// One application-owned table, one demanded context, no per-card routing data.
// Install every family on the same PageModalRuntime after constructing it with
// this port. The table, families and origin validator outlive that runtime.
template <size_t Count>
class PageModalRouter final : public PageModalPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageModalRouter(std::array<PageModalRoute, Count> routes, void* owner, OriginValid valid)
      : routes_(routes), owner_(owner), valid_(valid) {
    configured_ = Count != 0 && valid_;
    for (size_t i = 0; i < Count; ++i) {
      if (!routes_[i].port || routes_[i].kind == ControlModalKind::NONE) configured_ = false;
      for (size_t j = 0; j < i; ++j)
        if (routes_[i].kind == routes_[j].kind) configured_ = false;
    }
  }
  ~PageModalRouter() override { assert(!context_ && !creating_ && !destroying_); }
  PageModalRouter(const PageModalRouter&) = delete;
  PageModalRouter& operator=(const PageModalRouter&) = delete;

  bool valid_origin(PageBindingToken token) const override { return configured_ && valid_(owner_, token); }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
    return kind(store, card, ControlModalKind::NONE);
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle card,
                        ControlModalKind requested) const override {
    if (!configured_) return ControlModalKind::NONE;
    if (requested != ControlModalKind::NONE) {
      auto* port = route(requested);
      return port && port->kind(store, card, requested) == requested ? requested : ControlModalKind::NONE;
    }
    ControlModalKind selected = ControlModalKind::NONE;
    for (const auto& entry : routes_) {
      if (entry.port->kind(store, card) != entry.kind) continue;
      // An ambiguous default must be fixed at the family boundary. Never let
      // table order choose a different panel or connection policy.
      if (selected != ControlModalKind::NONE) return ControlModalKind::NONE;
      selected = entry.kind;
    }
    return selected;
  }
  ControlModalKind local_kind(ControlModalKind requested) const override {
    auto* port = route(requested);
    return port && port->local_kind(requested) == requested ? requested : ControlModalKind::NONE;
  }
  bool valid_local_origin(PageLocalModalOrigin origin, ControlModalKind requested) const override {
    auto* port = route(requested);
    return port && port->local_kind(requested) == requested && port->valid_local_origin(origin, requested);
  }
  bool ready() const override {
    if (!configured_ || creating_) return false;
    // This includes deferred image/QR decoder leases in a retiring family.
    for (const auto& entry : routes_)
      if (!entry.port->ready()) return false;
    return true;
  }
  // The runtime must use the admitted family, including offline local panels.
  bool connected() const override { return false; }
  uint32_t connection_generation() const override { return 0; }
  bool connected_for(ControlModalKind kind) const override {
    auto* port = route(kind);
    return port && port->connected_for(kind);
  }
  uint32_t connection_generation_for(ControlModalKind kind) const override {
    auto* port = route(kind);
    return port ? port->connection_generation_for(kind) : 0;
  }
  void* create(const PageStore& store, ConfigurationHandle card, PageModalToken token) override {
    auto* port = route(token.kind);
    if (!token.opening || token.configuration != card.generation || token.card != card.index ||
        !can_create(port) || port->kind(store, card, token.kind) != token.kind)
      return nullptr;
    creating_ = true;
    auto* context = port->create(store, card, token);
    creating_ = false;
    return adopt(port, context, token);
  }
  void* create_local(const PageStore& store, PageLocalModalOrigin origin, PageModalToken token) override {
    auto* port = route(token.kind);
    if (!token.opening || token.configuration != origin.configuration || !can_create(port) ||
        token.card != NO_CARD || port->local_kind(token.kind) != token.kind)
      return nullptr;
    creating_ = true;
    auto* context = port->create_local(store, origin, token);
    creating_ = false;
    return adopt(port, context, token);
  }
  bool initialize(void* context, PageModalToken token) override {
    return live(context, token) && active_->initialize(context, token);
  }
  bool visible(void* context) const override {
    return context && context == context_ && !retired_ && active_->visible(context);
  }
  void update(void* context, PageModalToken token) override {
    if (live(context, token)) active_->update(context, token);
  }
  ControlModalDismissPolicy dismiss_policy(void* context) const override {
    return context && context == context_ ? active_->dismiss_policy(context)
                                          : ControlModalDismissPolicy::DISMISS;
  }
  uint64_t source_revision(void* context) const override {
    return context && context == context_ ? active_->source_revision(context) : 0;
  }
  void retire(void* context, PageModalClose reason) override {
    if (!context || context != context_ || retired_) return;
    retired_ = true;
    active_->retire(context, reason);
  }
  void destroy(void* context) override {
    assert(context && context == context_ && retired_ && !creating_ && !destroying_);
    // Keep the route until destruction returns: a final climate edit queries
    // its admitted family's connection and readiness during this call.
    destroying_ = true;
    active_->destroy(context);
    destroying_ = false;
    active_ = nullptr;
    context_ = nullptr;
    token_ = {};
    retired_ = false;
  }

 private:
  PageModalPort* route(ControlModalKind kind) const {
    if (!configured_ || kind == ControlModalKind::NONE) return nullptr;
    for (const auto& entry : routes_)
      if (entry.kind == kind) return entry.port;
    return nullptr;
  }
  bool can_create(PageModalPort* port) const {
    return port && !context_ && !creating_ && !destroying_ && ready();
  }
  void* adopt(PageModalPort* port, void* context, PageModalToken token) {
    if (context) {
      active_ = port;
      context_ = context;
      token_ = token;
      retired_ = false;
    }
    return context;
  }
  bool live(void* context, PageModalToken token) const {
    return context && context == context_ && !retired_ && token.opening &&
           token.configuration == token_.configuration && token.opening == token_.opening &&
           token.card == token_.card && token.kind == token_.kind;
  }
  const std::array<PageModalRoute, Count> routes_;
  void* owner_;
  OriginValid valid_;
  PageModalPort* active_{nullptr};
  void* context_{nullptr};
  PageModalToken token_;
  bool configured_{false}, creating_{false}, destroying_{false}, retired_{false};
};
}  // namespace espcontrol::pages
