#pragma once

#include "page_card_commands.h"

namespace espcontrol::pages {
// Borrows the existing timer history; the view never treats queue acceptance as
// a sent cancellation. Other command kinds are deliberately unsupported here.
class PageTimerCommandPolicy final : public PageCardControlPolicy {
 public:
  PageTimerCommandPolicy(PageEntityStateRuntime& graph, PageTimerRuntime& timer)
      : graph_(graph), timer_(timer) {}
  bool admit(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             PageCardCommandValue& value) override {
    return valid(store, origin, kind, value);
  }
  bool valid(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             const PageCardCommandValue& value) override {
    if (kind != PageCardCommandKind::TIMER_START && kind != PageCardCommandKind::TIMER_CANCEL) return false;
    if (!PageTimerRuntime::supported(store, origin.card)) return false;
    const auto* card = store.runtime_card({origin.configuration, origin.card});
    return card && current(origin, kind, value, store.field(*card, CardField::ENTITY));
  }
  void discard(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) override {}
  void accepted(PageBindingToken origin, PageCardCommandKind kind,
                const PageCardCommandValue& value) override {
    if (kind != PageCardCommandKind::TIMER_CANCEL) return;
    timer_.loop();
    const ConfigurationHandle handle{origin.configuration, origin.card};
    const auto snapshot = timer_.read(handle);
    if (snapshot.mode == PageTimerMode::ACTIVE && snapshot.confirmation_revision == value.write_revision &&
        (!snapshot.confirmation || value.integer == 1))
      timer_.cancel_sent(handle, true);
  }

 private:
  bool current(PageBindingToken origin, PageCardCommandKind kind, const PageCardCommandValue& value,
               std::string_view entity) {
    timer_.loop();
    const ConfigurationHandle handle{origin.configuration, origin.card};
    const auto snapshot = timer_.read(handle);
    for (const auto attribute : {"", "duration", "remaining", "finishes_at"})
      if (graph_.read_attribute(handle, entity, attribute).status == PageEntityStatus::EXHAUSTED)
        return false;
    if (snapshot.confirmation_revision != value.write_revision) return false;
    if (kind == PageCardCommandKind::TIMER_START)
      return snapshot.mode == PageTimerMode::IDLE || snapshot.mode == PageTimerMode::PAUSED;
    return snapshot.mode == PageTimerMode::ACTIVE && (!snapshot.confirmation || value.integer == 1);
  }
  PageEntityStateRuntime& graph_;
  PageTimerRuntime& timer_;
};
}  // namespace espcontrol::pages
