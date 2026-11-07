#pragma once

#include <array>

#include "page_card_commands.h"

namespace espcontrol::pages {
struct PageCardControlRoute {
  PageCardCommandKind kind{PageCardCommandKind::NONE};
  PageCardControlPolicy* policy{nullptr};
};
// One application-owned table, independent of configured cards and visible
// shells. Routes/policies are immutable and outlive the queue/native adapter.
template <size_t Count>
class PageCardControlRouter final : public PageCardControlPolicy {
 public:
  static_assert(Count > 0);
  explicit PageCardControlRouter(std::array<PageCardControlRoute, Count> routes) : routes_(routes) {
    ready_ = true;
    for (size_t i = 0; i < Count; ++i) {
      if (!routes_[i].policy || !PageCardCommands::control(routes_[i].kind)) ready_ = false;
      for (size_t j = 0; j < i; ++j)
        if (routes_[i].kind == routes_[j].kind) ready_ = false;
    }
  }
  PageCardControlRouter(const PageCardControlRouter&) = delete;
  PageCardControlRouter& operator=(const PageCardControlRouter&) = delete;
  bool ready() const { return ready_; }
  bool admit(const PageStore& s, PageBindingToken t, PageCardCommandKind k,
             PageCardCommandValue& v) override {
    auto* p = find(k);
    return p && p->admit(s, t, k, v);
  }
  bool valid(const PageStore& s, PageBindingToken t, PageCardCommandKind k,
             const PageCardCommandValue& v) override {
    auto* p = find(k);
    return p && p->valid(s, t, k, v);
  }
  void discard(PageBindingToken t, PageCardCommandKind k, const PageCardCommandValue& v) override {
    if (auto* p = find(k)) p->discard(t, k, v);
  }
  void accepted(PageBindingToken t, PageCardCommandKind k, const PageCardCommandValue& v) override {
    if (auto* p = find(k)) p->accepted(t, k, v);
  }

 private:
  PageCardControlPolicy* find(PageCardCommandKind k) const {
    if (ready_)
      for (const auto& r : routes_)
        if (r.kind == k) return r.policy;
    return nullptr;
  }
  const std::array<PageCardControlRoute, Count> routes_;
  bool ready_{false};
};
}  // namespace espcontrol::pages
