#pragma once

#include <cstddef>
#include <string>

#include "esphome/components/text/text.h"

#include "panel_config_text_bindings.h"

namespace espcontrol::configuration {

class EspHomePanelConfigTextValue final : public PanelConfigTextValue {
 public:
  void bind(esphome::text::Text *text) { text_ = text; }

  const std::string &value() const override {
    return text_ == nullptr ? empty_ : text_->state;
  }

  bool set_value(const char *value, size_t value_size) override {
    if (text_ == nullptr || (value == nullptr && value_size > 0)) return false;
    text_->make_call().set_value(value, value_size).perform();
    return true;
  }

  bool publish_value(const char *value, size_t value_size) override {
    if (text_ == nullptr || (value == nullptr && value_size > 0)) return false;
    text_->publish_state(std::string(value == nullptr ? "" : value, value_size));
    return true;
  }

 private:
  esphome::text::Text *text_{nullptr};
  std::string empty_;
};

// Temporary source-compatible name while device wiring moves away from the
// legacy configuration path.
using EspHomeLegacyTextValue = EspHomePanelConfigTextValue;

}  // namespace espcontrol::configuration
