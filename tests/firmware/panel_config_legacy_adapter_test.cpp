#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "panel_config_document.h"
#include "panel_config_legacy_adapter.h"

namespace {

using espcontrol::configuration::LegacyStatus;
using espcontrol::configuration::LegacyTextValue;
using espcontrol::configuration::PanelConfigLegacyAdapter;
using espcontrol::configuration::PanelConfigReader;
using espcontrol::configuration::PanelConfigRecord;
using espcontrol::configuration::PanelConfigRecordType;
using espcontrol::configuration::PanelConfigStatus;
using espcontrol::configuration::PanelConfigWriter;

class FakeText final : public LegacyTextValue {
 public:
  explicit FakeText(std::string state = {}) : state_(std::move(state)) {}
  const std::string &value() const override { return state_; }
  bool set_value(const char *value, size_t value_size) override {
    if (value == nullptr && value_size > 0) return false;
    state_.assign(value, value_size);
    ++persistent_writes;
    return true;
  }
  bool publish_value(const char *value, size_t value_size) override {
    if (value == nullptr && value_size > 0) return false;
    state_.assign(value, value_size);
    ++runtime_publishes;
    return true;
  }

  size_t persistent_writes{0};
  size_t runtime_publishes{0};

 private:
  std::string state_;
};

bool imports_legacy_button_subpage_and_order() {
  FakeText order("2,1");
  FakeText on_color("FF8800");
  FakeText button("light.kitchen;Kitchen");
  FakeText subpage_a("media.living_room;");
  FakeText subpage_b("media.office");
  std::array<LegacyTextValue *, PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
      chunks{};
  chunks[0] = &subpage_a;
  chunks[1] = &subpage_b;

  PanelConfigLegacyAdapter adapter;
  adapter.set_device_profile("guition-esp32-p4-jc1060p470");
  adapter.set_button_order(&order);
  adapter.set_button_on_color(&on_color);
  adapter.set_button(1, &button, chunks);
  std::array<uint8_t, 512> document{};
  const auto loaded = adapter.load(document.data(), document.size());
  if (loaded.status != LegacyStatus::OK || loaded.document_size == 0)
    return false;

  PanelConfigReader reader(document.data(), loaded.document_size);
  if (reader.begin() != PanelConfigStatus::OK) return false;
  bool button_found = false;
  bool subpage_found = false;
  bool order_found = false;
  bool on_color_found = false;
  PanelConfigRecord record;
  while (reader.next(&record) == PanelConfigStatus::OK) {
    if (record.type == PanelConfigRecordType::BUTTON && record.slot == 1)
      button_found = std::string(reinterpret_cast<const char *>(record.value),
                                 record.value_size) == "light.kitchen;Kitchen";
    if (record.type == PanelConfigRecordType::SUBPAGE && record.slot == 1)
      subpage_found =
          std::string(reinterpret_cast<const char *>(record.value), record.value_size) ==
          "media.living_room;media.office";
    if (record.type == PanelConfigRecordType::SETTING)
      order_found = order_found || (
          std::string(reinterpret_cast<const char *>(record.key), record.key_size) ==
              "button_order" &&
          std::string(reinterpret_cast<const char *>(record.value), record.value_size) ==
              "2,1");
    if (record.type == PanelConfigRecordType::SETTING)
      on_color_found = on_color_found || (
          std::string(reinterpret_cast<const char *>(record.key), record.key_size) ==
              "button_on_color" &&
          std::string(reinterpret_cast<const char *>(record.value), record.value_size) ==
              "FF8800");
  }
  return button_found && subpage_found && order_found && on_color_found;
}

bool native_document_mirrors_back_to_legacy_entities_for_downgrade() {
  FakeText order("old");
  FakeText on_color("old-color");
  FakeText button("old");
  FakeText subpage_a("old");
  FakeText subpage_b("old");
  std::array<LegacyTextValue *, PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
      chunks{};
  chunks[0] = &subpage_a;
  chunks[1] = &subpage_b;
  PanelConfigLegacyAdapter adapter;
  adapter.set_device_profile("profile");
  adapter.set_button_order(&order);
  adapter.set_button_on_color(&on_color);
  adapter.set_button(1, &button, chunks);

  std::array<uint8_t, 512> document{};
  PanelConfigWriter writer(document.data(), document.size());
  size_t document_size = 0;
  if (writer.begin() != PanelConfigStatus::OK ||
      writer.append_device_profile(reinterpret_cast<const uint8_t *>("profile"),
                                   7) != PanelConfigStatus::OK ||
      writer.append_button(1, reinterpret_cast<const uint8_t *>("new-button"),
                           10) != PanelConfigStatus::OK ||
      writer.append_subpage(1,
                            reinterpret_cast<const uint8_t *>("123456789"),
                            9) != PanelConfigStatus::OK ||
      writer.append_setting(reinterpret_cast<const uint8_t *>("button_order"),
                            12, reinterpret_cast<const uint8_t *>("1"), 1) !=
          PanelConfigStatus::OK ||
      writer.append_setting(reinterpret_cast<const uint8_t *>("button_on_color"),
                            15, reinterpret_cast<const uint8_t *>("0088FF"), 6) !=
          PanelConfigStatus::OK ||
      writer.finish(&document_size) != PanelConfigStatus::OK) {
    return false;
  }
  if (!adapter.mirror(1, document.data(), document_size) ||
      button.value() != "new-button" || subpage_a.value() != "123456789" ||
      !subpage_b.value().empty() || order.value() != "1" ||
      on_color.value() != "0088FF") {
    return false;
  }
  // A native save mirrors the document for downgrade compatibility, then
  // applies it to the live grid. Neither stage should republish unchanged
  // text entities and flood the panel with grid refreshes.
  return adapter.mirror(1, document.data(), document_size) &&
         adapter.apply(1, document.data(), document_size) &&
         button.persistent_writes == 1 && subpage_a.persistent_writes == 1 &&
         subpage_b.persistent_writes == 1 && order.persistent_writes == 1 &&
         on_color.persistent_writes == 1 && button.runtime_publishes == 0 &&
         subpage_a.runtime_publishes == 0 && subpage_b.runtime_publishes == 0 &&
         order.runtime_publishes == 0 && on_color.runtime_publishes == 0;
}

bool native_document_updates_live_grid_without_writing_legacy_preferences() {
  FakeText order("old-order");
  FakeText button("old-button");
  std::array<LegacyTextValue *, PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
      chunks{};
  PanelConfigLegacyAdapter adapter;
  adapter.set_device_profile("profile");
  adapter.set_button_order(&order);
  adapter.set_button(1, &button, chunks);

  std::array<uint8_t, 256> document{};
  PanelConfigWriter writer(document.data(), document.size());
  size_t document_size = 0;
  if (writer.begin() != PanelConfigStatus::OK ||
      writer.append_device_profile(reinterpret_cast<const uint8_t *>("profile"),
                                   7) != PanelConfigStatus::OK ||
      writer.append_button(1, reinterpret_cast<const uint8_t *>("new-button"),
                           10) != PanelConfigStatus::OK ||
      writer.append_setting(reinterpret_cast<const uint8_t *>("button_order"),
                            12, reinterpret_cast<const uint8_t *>("1"), 1) !=
          PanelConfigStatus::OK ||
      writer.finish(&document_size) != PanelConfigStatus::OK) {
    return false;
  }
  return adapter.apply(1, document.data(), document_size) &&
         button.value() == "new-button" && order.value() == "1" &&
         button.persistent_writes == 0 && order.persistent_writes == 0 &&
         button.runtime_publishes == 1 && order.runtime_publishes == 1;
}

}  // namespace

int main() {
  return imports_legacy_button_subpage_and_order() &&
                 native_document_mirrors_back_to_legacy_entities_for_downgrade() &&
                 native_document_updates_live_grid_without_writing_legacy_preferences()
             ? 0
             : 1;
}
