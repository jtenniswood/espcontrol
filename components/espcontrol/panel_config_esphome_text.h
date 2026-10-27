#pragma once

#include <cstddef>
#include <string>

#include "esphome/components/text/text.h"

#include "legacy_text_preference.h"
#include "panel_config_text_bindings.h"

#ifdef USE_ESP32
#include "esphome/core/preferences.h"
#include <nvs.h>
#endif

namespace espcontrol::configuration {

class EspHomePanelConfigTextValue final : public PanelConfigTextValue {
 public:
   void bind(esphome::text::Text *text, bool template_preference = false,
             bool empty_default = false) {
     text_ = text;
     template_preference_ = template_preference;
     empty_default_ = template_preference && empty_default;
   }

  const std::string &value() const override {
    return text_ == nullptr ? empty_ : text_->state;
  }

  bool prepare_empty_value() override {
#ifdef USE_ESP32
    if (empty_default_)
      return erase_empty_default();
#endif
    return true;
  }

  bool can_persist_value(size_t size) const override {
    return text_ && (!template_preference_ ||
                     (text_->traits.get_max_length() <= UINT8_MAX &&
                      size <= text_->traits.get_max_length()));
  }

  bool set_value(const char *value, size_t value_size) override {
    if (text_ == nullptr || (value == nullptr && value_size > 0)) return false;
#ifdef USE_ESP32
    if (template_preference_) {
      auto *preferences = esphome::global_preferences;
      if (!preferences || !preferences->nvs_handle)
        return false;
      if (empty_default_ && value_size == 0) {
        if (!erase_empty_default())
          return false;
        if (!text_->state.empty())
          text_->publish_state("");
        return true;
      }
      // The ESP32 SDK backend is a value containing its key and NVS handle.
      // Keeping it on the stack avoids allocating a second backend per field.
      esphome::ESPPreferenceBackend backend;
      backend.key = preference_key();
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
#ifdef USE_ESP32
   uint32_t preference_key() const {
     // Match TemplateText::setup, including order/color's smaller allocations.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
     uint32_t key = text_->get_preference_hash();
#pragma GCC diagnostic pop
     key += text_->traits.get_min_length() << 2;
     key += text_->traits.get_max_length() << 4;
     key += esphome::fnv1_hash(text_->traits.get_pattern_c_str()) << 6;
     return key;
   }

   bool erase_empty_default() {
     auto *preferences = esphome::global_preferences;
     if (!text_ || !preferences || !preferences->nvs_handle)
       return false;
     // The pinned SDK clears unsuccessful queued writes too. Still reclaim this
     // empty key after a failed drain so a full partition can recover on retry,
     // while returning the failure to the compatibility caller.
     const bool drained = preferences->sync();
     char key[11];
     esphome::uint32_to_str(key, preference_key());
     const auto erased = nvs_erase_key(preferences->nvs_handle, key);
     const bool durable =
         erased == ESP_ERR_NVS_NOT_FOUND ||
         (erased == ESP_OK && nvs_commit(preferences->nvs_handle) == ESP_OK);
     return drained && durable;
   }
#endif
   esphome::text::Text *text_{nullptr};
   bool template_preference_{false};
   bool empty_default_{false};
   // Unbound adapters share a read-only fallback instead of retaining an
   // unused std::string in every compatibility field.
   inline static const std::string empty_{};
};

// Temporary source-compatible name while device wiring moves away from the
// legacy configuration path.
using EspHomeLegacyTextValue = EspHomePanelConfigTextValue;

}  // namespace espcontrol::configuration
