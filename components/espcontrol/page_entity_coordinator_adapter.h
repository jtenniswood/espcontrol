#pragma once

#include <string>

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {

// Reuse the application's deduplicated transport channels across staged and
// active configurations. Only the configuration cache retains these values;
// legacy widget/modal consumers can continue to share the same channels.
// PageEntityStateRuntime explicitly copies live values during staging. Skip
// coordinator initial replay so an unrelated retained consumer cannot revive
// metadata that the configuration-owned media lifecycle has invalidated.
template<typename Coordinator>
class CoordinatorPageEntitySubscriptions final : public PageEntitySubscriptions {
 public:
  CoordinatorPageEntitySubscriptions(Coordinator &coordinator, uint32_t scope)
      : coordinator_(coordinator), scope_(scope) {}
  bool subscribe(const char *entity, const char *attribute, void *owner,
      uint32_t revision, uint32_t channel, Deliver deliver) override {
    return coordinator_.subscribe(std::string(entity), std::string(attribute),
        [owner, revision, channel, deliver](typename Coordinator::State value) {
          deliver(owner, revision, channel, {value.c_str(), value.size()});
        }, scope_, owner, false, false, false);
  }
  void release(void *owner) override { coordinator_.release_owner(owner); }
  uint32_t connection_generation() const override { return coordinator_.connection_generation(); }

 private:
  Coordinator &coordinator_;
  uint32_t scope_;
};
}  // namespace espcontrol::pages
