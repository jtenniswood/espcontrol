#pragma once

#include <string>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "panel_config_text_bindings.h"

namespace espcontrol::configuration {

// The renderer and its refresh automations share this volatile layout field.
class PanelConfigField final : public esphome::Component,
                               public PanelConfigTextValue {
 public:
  std::string state;

  void set_initial_value(const char *value) { state = value; }
  const std::string &value() const override { return state; }

  template<typename F> void add_on_state_callback(F &&callback) {
    callbacks_.add(std::forward<F>(callback));
  }

  bool set_value(const char *, size_t) override { return false; }
  bool publish_value(const char *value, size_t size) override {
    if (size == 0) {
      state.clear();
    } else {
      if (value == nullptr) return false;
      state.assign(value, size);
    }
    callbacks_.call(state);
    return true;
  }

 private:
  esphome::LazyCallbackManager<void(const std::string &)> callbacks_;
};

}  // namespace espcontrol::configuration
