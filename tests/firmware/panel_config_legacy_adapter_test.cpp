#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "panel_config_document.h"
#include "panel_config_legacy_adapter.h"
#include "panel_config_runtime_adapter.h"
#include "panel_config_text_bindings.h"

namespace {

using espcontrol::configuration::LegacyStatus;
using espcontrol::configuration::PanelConfigLegacyAdapter;
using espcontrol::configuration::PanelConfigRuntimeAdapter;
using espcontrol::configuration::PanelConfigTextBindings;
using espcontrol::configuration::PanelConfigTextValue;
using espcontrol::configuration::PanelConfigReader;
using espcontrol::configuration::PanelConfigRecord;
using espcontrol::configuration::PanelConfigRecordType;
using espcontrol::configuration::PanelConfigStatus;
using espcontrol::configuration::PanelConfigWriter;

static_assert(!std::is_base_of_v<
              espcontrol::configuration::ConfigurationRuntimeAdapter,
              PanelConfigLegacyAdapter>);
static_assert(std::is_base_of_v<
              espcontrol::configuration::ConfigurationRuntimeAdapter,
              PanelConfigRuntimeAdapter>);

class FakeText final : public PanelConfigTextValue {
 public:
  FakeText() = default;
  explicit FakeText(std::string state) : state_(std::move(state)) {}
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
  std::array<PanelConfigTextValue *, PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS>
      chunks{};
  chunks[0] = &subpage_a;
  chunks[1] = &subpage_b;

  PanelConfigTextBindings bindings;
  PanelConfigLegacyAdapter adapter(bindings);
  bindings.set_device_profile("guition-esp32-p4-jc1060p470");
  bindings.set_button_order(&order);
  bindings.set_button_on_color(&on_color);
  bindings.set_button(1, &button, chunks);
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
  std::array<PanelConfigTextValue *, PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS>
      chunks{};
  chunks[0] = &subpage_a;
  chunks[1] = &subpage_b;
  PanelConfigTextBindings bindings;
  PanelConfigLegacyAdapter adapter(bindings);
  PanelConfigRuntimeAdapter runtime(bindings);
  bindings.set_device_profile("profile");
  bindings.set_button_order(&order);
  bindings.set_button_on_color(&on_color);
  bindings.set_button(1, &button, chunks);

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
         runtime.apply(1, document.data(), document_size) &&
         button.persistent_writes == 1 && subpage_a.persistent_writes == 1 &&
         subpage_b.persistent_writes == 1 && order.persistent_writes == 1 &&
         on_color.persistent_writes == 1 && button.runtime_publishes == 0 &&
         subpage_a.runtime_publishes == 0 && subpage_b.runtime_publishes == 0 &&
         order.runtime_publishes == 0 && on_color.runtime_publishes == 0;
}

bool native_document_updates_live_grid_without_writing_legacy_preferences() {
  FakeText order("old-order");
  FakeText button("old-button");
  std::array<PanelConfigTextValue *, PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS>
      chunks{};
  PanelConfigTextBindings bindings;
  PanelConfigRuntimeAdapter adapter(bindings);
  bindings.set_device_profile("profile");
  bindings.set_button_order(&order);
  bindings.set_button(1, &button, chunks);

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

bool twenty_slot_document_reaches_order_and_subpage_settings() {
  constexpr size_t SLOT_COUNT = 20;
  FakeText order("old-order");
  FakeText on_color("old-color");
  std::array<FakeText, SLOT_COUNT> buttons{};
  FakeText subpage;
  std::array<std::array<PanelConfigTextValue *,
                        PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS>,
             SLOT_COUNT>
      chunks{};
  chunks[2][0] = &subpage;

  PanelConfigTextBindings bindings;
  PanelConfigLegacyAdapter legacy(bindings);
  PanelConfigRuntimeAdapter runtime(bindings);
  bindings.set_device_profile("guition-esp32-p4-jc8012p4a1");
  bindings.set_button_order(&order);
  bindings.set_button_on_color(&on_color);
  for (size_t index = 0; index < SLOT_COUNT; ++index) {
    bindings.set_button(static_cast<uint8_t>(index + 1), &buttons[index],
                        chunks[index]);
  }

  std::array<std::string, SLOT_COUNT> values{};
  for (size_t index = 0; index < SLOT_COUNT; ++index) {
    values[index] = index < 9 ? "card-" + std::to_string(index + 1)
                              : ";;Auto;Auto";
  }
  constexpr char DEVICE_PROFILE[] = "guition-esp32-p4-jc8012p4a1";
  constexpr char SUBPAGE[] = "~B,1,2|card-a|card-b";
  constexpr char BUTTON_ORDER[] = "1,2,6p,,,7,8,,,,9,4,,,,5";
  constexpr char BUTTON_ON_COLOR[] = "FF8C00";
  std::array<uint8_t, 4096> document{};
  PanelConfigWriter writer(document.data(), document.size());
  size_t document_size = 0;
  if (writer.begin() != PanelConfigStatus::OK ||
      writer.append_device_profile(
          reinterpret_cast<const uint8_t *>(DEVICE_PROFILE),
          sizeof(DEVICE_PROFILE) - 1) != PanelConfigStatus::OK) {
    return false;
  }
  for (size_t index = 0; index < SLOT_COUNT; ++index) {
    if (writer.append_button(
            static_cast<uint8_t>(index + 1),
            reinterpret_cast<const uint8_t *>(values[index].data()),
            values[index].size()) != PanelConfigStatus::OK) {
      return false;
    }
  }
  if (writer.append_subpage(3, reinterpret_cast<const uint8_t *>(SUBPAGE),
                            sizeof(SUBPAGE) - 1) != PanelConfigStatus::OK ||
      writer.append_setting(
          reinterpret_cast<const uint8_t *>("button_on_color"), 15,
          reinterpret_cast<const uint8_t *>(BUTTON_ON_COLOR),
          sizeof(BUTTON_ON_COLOR) - 1) != PanelConfigStatus::OK ||
      writer.append_setting(
          reinterpret_cast<const uint8_t *>("button_order"), 12,
          reinterpret_cast<const uint8_t *>(BUTTON_ORDER),
          sizeof(BUTTON_ORDER) - 1) != PanelConfigStatus::OK ||
      writer.finish(&document_size) != PanelConfigStatus::OK) {
    return false;
  }

  if (!legacy.mirror(1, document.data(), document_size) ||
      !runtime.apply(1, document.data(), document_size)) {
    return false;
  }
  for (size_t index = 0; index < SLOT_COUNT; ++index) {
    if (buttons[index].value() != values[index] ||
        buttons[index].persistent_writes != 1) {
      return false;
    }
  }
  return subpage.value() == SUBPAGE && order.value() == BUTTON_ORDER &&
         on_color.value() == BUTTON_ON_COLOR &&
         order.persistent_writes == 1 && on_color.persistent_writes == 1;
}

bool expanded_native_configuration_preserves_every_byte() {
  for (size_t chunk_count : {size_t{4}, size_t{8}}) {
    FakeText order("old-order");
    FakeText button("old-button");
    std::array<FakeText, PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS> fields{};
    std::array<PanelConfigTextValue *, PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS> chunks{};
    for (size_t index = 0; index < chunk_count; ++index) chunks[index] = &fields[index];
    PanelConfigTextBindings bindings;
    bindings.set_device_profile("profile");
    bindings.set_button_order(&order);
    bindings.set_button(1, &button, chunks);
    const std::string large_button(400, 'b');
    // 8191 bytes, with multi-byte characters crossing old chunk boundaries.
    const std::string large_subpage = std::string(250, 's') + "é😀" + std::string(7935, 'c');
    std::array<uint8_t, 10000> document{};
    PanelConfigWriter writer(document.data(), document.size());
    size_t size = 0;
    if (writer.begin() != PanelConfigStatus::OK ||
        writer.append_device_profile(reinterpret_cast<const uint8_t *>("profile"), 7) != PanelConfigStatus::OK ||
        writer.append_button(1, reinterpret_cast<const uint8_t *>(large_button.data()), large_button.size()) != PanelConfigStatus::OK ||
        writer.append_subpage(1, reinterpret_cast<const uint8_t *>(large_subpage.data()), large_subpage.size()) != PanelConfigStatus::OK ||
        writer.finish(&size) != PanelConfigStatus::OK) return false;
    // Rejected downgrade copies must not partially overwrite existing values.
    if (bindings.persist_document(document.data(), size) || order.value() != "old-order" ||
        button.value() != "old-button" || button.persistent_writes != 0) return false;
    if (!bindings.publish_document(document.data(), size) || button.value() != large_button) return false;
    std::string combined;
    for (const auto &field : fields) {
      if (!espcontrol::configuration::panel_config_valid_utf8(
          reinterpret_cast<const uint8_t *>(field.value().data()), field.value().size())) return false;
      combined += field.value();
      if (field.persistent_writes != 0) return false;
    }
    if (combined != large_subpage) return false;
    // Live text export must still reconstruct the entire native record.
    std::array<uint8_t, 10000> exported{};
    size_t exported_size = 0;
    if (!bindings.write_document(exported.data(), exported.size(), &exported_size) ||
        exported_size != size || std::memcmp(document.data(), exported.data(), size) != 0) return false;
    if (writer.begin() != PanelConfigStatus::OK ||
        writer.append_device_profile(reinterpret_cast<const uint8_t *>("profile"), 7) != PanelConfigStatus::OK ||
        writer.append_subpage(1, reinterpret_cast<const uint8_t *>("short"), 5) != PanelConfigStatus::OK ||
        writer.finish(&size) != PanelConfigStatus::OK ||
        !bindings.publish_document(document.data(), size)) return false;
    if (fields[0].value() != "short" || !button.value().empty()) return false;
    for (size_t index = 1; index < fields.size(); ++index)
      if (!fields[index].value().empty()) return false;
    // Once a layout fits again, unchanged live values must still reach the
    // preference copy that was skipped during the larger save.
    if (!bindings.persist_document(document.data(), size) || fields[0].persistent_writes != 1 ||
        button.persistent_writes != 1 || order.persistent_writes != 1) return false;
    if (!bindings.persist_document(document.data(), size) || fields[0].persistent_writes != 1) return false;
  }
  return true;
}

bool legacy_unicode_capacity_is_checked_before_writing() {
  FakeText order("old-order");
  FakeText button("old-button");
  FakeText first("old-first");
  FakeText second("old-second");
  PanelConfigTextBindings bindings;
  bindings.set_device_profile("profile");
  bindings.set_button_order(&order);
  std::array<PanelConfigTextValue *, PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS> chunks{};
  chunks[0] = &first;
  chunks[1] = &second;
  bindings.set_button(1, &button, chunks);
  for (bool fits : {false, true}) {
    std::string value;
    for (size_t index = 0; index < (fits ? 254 : 127); ++index) value += fits ? "é" : "😀";
    std::array<uint8_t, 1024> document{};
    PanelConfigWriter writer(document.data(), document.size());
    size_t size = 0;
    if (writer.begin() != PanelConfigStatus::OK ||
        writer.append_device_profile(reinterpret_cast<const uint8_t *>("profile"), 7) != PanelConfigStatus::OK ||
        writer.append_button(1, reinterpret_cast<const uint8_t *>("new"), 3) != PanelConfigStatus::OK ||
        writer.append_subpage(1, reinterpret_cast<const uint8_t *>(value.data()), value.size()) != PanelConfigStatus::OK ||
        writer.finish(&size) != PanelConfigStatus::OK) return false;
    if (bindings.persist_document(document.data(), size) != fits) return false;
    if (fits) {
      if (first.value() + second.value() != value || first.value().size() != 254 || second.value().size() != 254) return false;
    } else if (button.persistent_writes != 0 || first.persistent_writes != 0 || second.persistent_writes != 0 ||
               first.value() != "old-first" || second.value() != "old-second") return false;
  }
  return true;
}

}  // namespace

int main() {
  return imports_legacy_button_subpage_and_order() &&
                 native_document_mirrors_back_to_legacy_entities_for_downgrade() &&
                 native_document_updates_live_grid_without_writing_legacy_preferences() &&
                 twenty_slot_document_reaches_order_and_subpage_settings() &&
                 expanded_native_configuration_preserves_every_byte() &&
                 legacy_unicode_capacity_is_checked_before_writing()
             ? 0
             : 1;
}
