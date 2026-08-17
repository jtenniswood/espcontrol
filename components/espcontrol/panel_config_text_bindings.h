#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "panel_config_document.h"

namespace espcontrol::configuration {

// A live text field used by the existing YAML grid wiring. Persistence and
// runtime publication are deliberately separate operations: compatibility
// releases may mirror a document for downgrade support, while every release
// still needs to refresh the running panel without rewriting preferences.
class PanelConfigTextValue {
 public:
  virtual ~PanelConfigTextValue() = default;

  virtual const std::string &value() const = 0;
  virtual bool set_value(const char *value, size_t value_size) = 0;
  virtual bool publish_value(const char *value, size_t value_size) = 0;
};

class PanelConfigTextBindings {
 public:
  static constexpr size_t MAX_SUBPAGE_CHUNKS = 8;

  void set_device_profile(const char *device_profile);
  void set_button_order(PanelConfigTextValue *button_order) {
    button_order_ = button_order;
  }
  void set_button_on_color(PanelConfigTextValue *button_on_color) {
    button_on_color_ = button_on_color;
  }
  void set_button(
      uint8_t slot, PanelConfigTextValue *button,
      const std::array<PanelConfigTextValue *, MAX_SUBPAGE_CHUNKS>
          &subpage_chunks);

  bool configured() const;
  bool write_document(uint8_t *output, size_t output_capacity,
                      size_t *document_size) const;
  bool persist_document(const uint8_t *document, size_t document_size);
  bool publish_document(const uint8_t *document, size_t document_size);

 private:
  struct ButtonSources {
    PanelConfigTextValue *button{nullptr};
    std::array<PanelConfigTextValue *, MAX_SUBPAGE_CHUNKS> subpage_chunks{};
  };

  bool apply_document(const uint8_t *document, size_t document_size,
                      bool persist);
  static bool write_value(PanelConfigTextValue *target, const char *value,
                          size_t value_size, bool persist);

  std::string device_profile_;
  PanelConfigTextValue *button_order_{nullptr};
  PanelConfigTextValue *button_on_color_{nullptr};
  std::array<ButtonSources, PANEL_CONFIG_MAX_SLOT_COUNT> buttons_{};
};

}  // namespace espcontrol::configuration
