#pragma once

#include <string>

#include "esphome/components/text/text.h"
#include "esphome/core/component.h"

#include "panel_config_text_bindings.h"

namespace espcontrol::configuration {

// Keep the renderer's text-state interface. These fields are never registered
// as entities and only notify the local layout refresh callbacks.
class PanelConfigField final : public esphome::text::Text,
                               public esphome::Component,
                               public PanelConfigTextValue {
 public:
  void set_initial_value(const char *value) { state = value; }
  const std::string &value() const override { return state; }

  bool set_value(const char *, size_t) override { return false; }
  bool publish_value(const char *value, size_t size) override {
    if (value == nullptr && size != 0) return false;
    state.assign(value == nullptr ? "" : value, size);
    set_has_state(true);
    state_callback_.call(state);
    return true;
  }

 protected:
  void control(const std::string &value) override {
    publish_value(value.data(), value.size());
  }
};

}  // namespace espcontrol::configuration
