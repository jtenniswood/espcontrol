#pragma once
// Include after the Home Assistant coordinator helpers.
#include "page_media_state_runtime.h"

namespace espcontrol::pages {
class PageHaMediaTransport final : public PageMediaTransport {
 public:
  bool connected() const override {
    return ha_api_state_connected();
  }
  uint32_t connection_generation() const override {
    return ha_read_coordinator().connection_generation();
  }
  void reannounce() override {
    ha_reannounce_state_subscriptions();
  }
};
}  // namespace espcontrol::pages
