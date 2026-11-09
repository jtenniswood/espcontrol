#pragma once

#include <array>
#include <cstddef>

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
// Fixed main-loop fanout. Media freshness observes first, then image ownership
// can compare borrowed old/new URL bytes before the packed cache changes.
// Observers must not allocate, dispatch actions or replace configuration here.
// Remove an observer before destroying it; this set outlives the state graph.
template <size_t Count>
class PageEntityObserverSet final : public PageEntityObserver {
 public:
  static_assert(Count > 0);
  PageEntityObserverSet() = default;
  PageEntityObserverSet(const PageEntityObserverSet&) = delete;
  PageEntityObserverSet& operator=(const PageEntityObserverSet&) = delete;

  bool add(PageEntityObserver& observer) {
    if (dispatching_) return false;
    for (auto* item : observers_)
      if (item == &observer) return false;
    for (auto& item : observers_)
      if (!item) {
        item = &observer;
        return true;
      }
    return false;
  }
  void remove(PageEntityObserver& observer) {
    for (auto& item : observers_)
      if (item == &observer) item = nullptr;
  }
  void observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                std::string_view value, bool copied) override {
    Guard guard(dispatching_);
    for (size_t i = 0; i < Count; ++i)
      if (auto* observer = observers_[i]) observer->observed(generation, entity, attribute, value, copied);
  }
  void observed_update(uint32_t generation, std::string_view entity, std::string_view attribute,
                       std::string_view value, bool copied, std::string_view previous) override {
    Guard guard(dispatching_);
    // Fetch one pointer at a time: removing the next observer from a callback
    // prevents a later call into an object which has already been retired.
    for (size_t i = 0; i < Count; ++i)
      if (auto* observer = observers_[i])
        observer->observed_update(generation, entity, attribute, value, copied, previous);
  }

 private:
  struct Guard {
    bool& flag;
    bool previous;
    explicit Guard(bool& value) : flag(value), previous(value) {
      flag = true;
    }
    ~Guard() {
      flag = previous;
    }
  };
  std::array<PageEntityObserver*, Count> observers_{};
  bool dispatching_{false};
};
}  // namespace espcontrol::pages
