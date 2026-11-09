#pragma once
// Include after the firmware-lifetime internal relay registry helpers.
#include "page_local_relay_runtime.h"
namespace espcontrol::pages {
class PageInternalRelayProvider final : public PageLocalRelayProvider {
 public:
  PageLocalRelaySnapshot read(std::string_view key) const override {
    for (const auto& relay : internal_relay_registry())
      if (std::string_view(relay.key) == key) return {relay.is_on ? relay.is_on() : false, relay.label};
    return {};
  }
};
inline PageLocalRelayProvider& internal_relay_provider() {
  static PageInternalRelayProvider provider;
  return provider;
}
}  // namespace espcontrol::pages
