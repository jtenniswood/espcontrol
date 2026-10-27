#pragma once

#include <cstddef>
#include <string>

#include "esphome/components/text/text.h"

#include "legacy_text_preference.h"
#include "panel_config_text_bindings.h"

#ifdef USE_ESP32
#include "esphome/core/preferences.h"
#endif

namespace espcontrol::configuration {

class EspHomePanelConfigTextValue final : public PanelConfigTextValue {
 public:
   void bind(esphome::text::Text *text, bool template_preference = false) {
     text_ = text;
     template_preference_ = template_preference;
   }

  const std::string &value() const override {
    return text_ == nullptr ? empty_ : text_->state;
  }

  bool set_value(const char *value, size_t value_size) override {
    if (text_ == nullptr || (value == nullptr && value_size > 0)) return false;
#ifdef USE_ESP32
    if (template_preference_) {
      auto *preferences = esphome::global_preferences;
      if (!preferences || !preferences->nvs_handle)
        return false;
      // Match the pinned SDK TemplateText::setup preference identity and
      // length-prefixed payload, including the smaller order/color fields.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      uint32_t key = text_->get_preference_hash();
#pragma GCC diagnostic pop
      key += text_->traits.get_min_length() << 2;
      key += text_->traits.get_max_length() << 4;
      key += esphome::fnv1_hash(text_->traits.get_pattern_c_str()) << 6;
      // The ESP32 SDK backend is a value containing its key and NVS handle.
      // Keeping it on the stack avoids allocating a second backend per field.
      esphome::ESPPreferenceBackend backend;
      backend.key = key;
      backend.nvs_handle = preferences->nvs_handle;
      if (!persist_legacy_text(
              backend, [preferences] { return preferences->sync(); }, value,
              value_size, text_->traits.get_max_length()))
        return false;
      const auto &current = text_->state;
      if (current.size() != value_size ||
          (value_size && std::memcmp(current.data(), value, value_size) != 0))
        text_->publish_state(
            std::string(value == nullptr ? "" : value, value_size));
      return true;
    }
#endif
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
  bool template_preference_{false};
  std::string empty_;
};

// Temporary source-compatible name while device wiring moves away from the
// legacy configuration path.
using EspHomeLegacyTextValue = EspHomePanelConfigTextValue;

}  // namespace espcontrol::configuration
