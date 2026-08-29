#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "configuration_service.h"
#include "panel_config_service_validator.h"

namespace {

using namespace espcontrol::configuration;

class MemoryBackend final : public StorageBackend {
 public:
  explicit MemoryBackend(size_t capacity)
      : slots_{std::vector<uint8_t>(capacity, 0xFF),
               std::vector<uint8_t>(capacity, 0xFF)} {}

  size_t slot_capacity() const override { return slots_[0].size(); }
  bool read(uint8_t slot, size_t offset, uint8_t *output,
            size_t size) override {
    if (slot >= slots_.size() || offset > slots_[slot].size() ||
        size > slots_[slot].size() - offset) {
      return false;
    }
    if (size > 0) std::memcpy(output, slots_[slot].data() + offset, size);
    return true;
  }
  bool write(uint8_t slot, size_t offset, const uint8_t *data,
             size_t size) override {
    if (fail_writes_ || slot >= slots_.size() ||
        offset > slots_[slot].size() ||
        size > slots_[slot].size() - offset) {
      return false;
    }
    if (size > 0) std::memcpy(slots_[slot].data() + offset, data, size);
    return true;
  }
  bool sync() override { return true; }
  void fail_writes(bool value) { fail_writes_ = value; }

 private:
  std::array<std::vector<uint8_t>, CONFIGURATION_SLOT_COUNT> slots_;
  bool fail_writes_{false};
};

class FakeLegacy final : public LegacyConfigurationAdapter {
 public:
  LegacyLoadResult load(uint8_t *output, size_t output_capacity) override {
    ++load_calls;
    if (read_failed) return {LegacyStatus::READ_FAILED, version, value.size()};
    if (value.empty()) return {LegacyStatus::EMPTY, version, 0};
    if (value.size() > output_capacity) {
      return {LegacyStatus::BUFFER_TOO_SMALL, version, value.size()};
    }
    std::copy(value.begin(), value.end(), output);
    return {LegacyStatus::OK, version, value.size()};
  }

  bool mirror(uint16_t document_version, const uint8_t *document,
              size_t document_size) override {
    ++mirror_calls;
    mirrored_version = document_version;
    mirrored.assign(document, document + document_size);
    return !mirror_failed;
  }

  std::vector<uint8_t> value;
  std::vector<uint8_t> mirrored;
  uint16_t version{CURRENT_CONFIGURATION_DOCUMENT_VERSION};
  uint16_t mirrored_version{0};
  size_t load_calls{0};
  size_t mirror_calls{0};
  bool read_failed{false};
  bool mirror_failed{false};
};

class FakeRuntime final : public ConfigurationRuntimeAdapter {
 public:
  bool apply(uint16_t document_version, const uint8_t *document,
             size_t document_size) override {
    ++apply_calls;
    applied_version = document_version;
    applied.assign(document, document + document_size);
    return !apply_failed;
  }

  std::vector<uint8_t> applied;
  uint16_t applied_version{0};
  size_t apply_calls{0};
  bool apply_failed{false};
};

std::vector<uint8_t> bytes(const char *value) {
  return std::vector<uint8_t>(value, value + std::strlen(value));
}

std::vector<uint8_t> panel_config_document() {
  std::array<uint8_t, 128> buffer{};
  PanelConfigWriter writer(buffer.data(), buffer.size());
  if (writer.begin() != PanelConfigStatus::OK ||
      writer.append_device_profile(
          reinterpret_cast<const uint8_t *>("esp32-p4-86"), 11) !=
          PanelConfigStatus::OK ||
      writer.append_button(1, reinterpret_cast<const uint8_t *>("light.kitchen"),
                           13) != PanelConfigStatus::OK) {
    return {};
  }
  size_t document_size = 0;
  if (writer.finish(&document_size) != PanelConfigStatus::OK) return {};
  return {buffer.begin(), buffer.begin() + document_size};
}

bool legacy_is_imported_once() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.value = bytes("legacy-document");
  ConfigurationService service(store, legacy);
  std::array<uint8_t, 64> output{};
  const ServiceLoadResult imported = service.load(output.data(), output.size());
  if (!imported.ok() || !imported.imported_legacy() ||
      imported.generation != 1 || legacy.load_calls != 1 ||
      !std::equal(legacy.value.begin(), legacy.value.end(), output.begin())) {
    return false;
  }

  legacy.value.clear();
  output.fill(0);
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  const std::vector<uint8_t> expected = bytes("legacy-document");
  return loaded.status == ServiceStatus::OK && loaded.generation == 1 &&
         legacy.load_calls == 1 &&
         std::equal(expected.begin(), expected.end(), output.begin());
}

bool partial_migration_refreshes_the_native_shadow() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.value = bytes("initial-editor-document");
  ConfigurationService service(store, legacy);
  std::array<uint8_t, 64> output{};
  if (!service.load(output.data(), output.size()).imported_legacy()) {
    return false;
  }

  legacy.value = bytes("updated-editor-document");
  const ServiceLoadResult refreshed =
      service.refresh_legacy_shadow(output.data(), output.size());
  if (refreshed.status != ServiceStatus::SYNCED_LEGACY ||
      refreshed.generation != 2 || legacy.mirror_calls != 0) {
    return false;
  }

  output.fill(0);
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return loaded.status == ServiceStatus::OK && loaded.generation == 2 &&
         std::equal(legacy.value.begin(), legacy.value.end(), output.begin());
}

bool failed_legacy_mirror_keeps_the_native_save_durable() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.mirror_failed = true;
  ConfigurationService service(store, legacy);
  const std::vector<uint8_t> expected = bytes("new-document");
  const ServiceSaveResult saved =
      service.save_current(expected.data(), expected.size());
  if (saved.status != ServiceStatus::LEGACY_MIRROR_FAILED ||
      !saved.durable() || saved.generation != 1 || legacy.mirror_calls != 1) {
    return false;
  }

  std::array<uint8_t, 64> output{};
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return loaded.ok() && loaded.generation == 1 &&
         std::equal(expected.begin(), expected.end(), output.begin());
}

bool failed_durable_save_never_updates_legacy() {
  MemoryBackend backend(256);
  backend.fail_writes(true);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  ConfigurationService service(store, legacy);
  const std::vector<uint8_t> expected = bytes("must-not-mirror");
  const ServiceSaveResult saved =
      service.save_current(expected.data(), expected.size());
  return saved.status == ServiceStatus::STORE_FAILED && !saved.durable() &&
         legacy.mirror_calls == 0;
}

bool successful_save_dual_writes() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  ConfigurationService service(store, legacy);
  const std::vector<uint8_t> expected = bytes("dual-write");
  const ServiceSaveResult saved =
      service.save_current(expected.data(), expected.size());
  return saved.ok() && saved.generation == 1 && legacy.mirror_calls == 1 &&
         legacy.mirrored_version == CURRENT_CONFIGURATION_DOCUMENT_VERSION &&
         legacy.mirrored == expected;
}

bool successful_save_updates_the_native_runtime() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  FakeRuntime runtime;
  ConfigurationService service(store, legacy);
  service.set_runtime_adapter(&runtime);
  const std::vector<uint8_t> expected = bytes("live-native-document");
  const ServiceSaveResult saved =
      service.save_current(expected.data(), expected.size());
  return saved.ok() && runtime.apply_calls == 1 &&
         runtime.applied_version == CURRENT_CONFIGURATION_DOCUMENT_VERSION &&
         runtime.applied == expected;
}

bool stored_document_can_be_applied_to_the_runtime_atomically() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  FakeRuntime runtime;
  ConfigurationService service(store, legacy);
  service.set_runtime_adapter(&runtime);
  const std::vector<uint8_t> expected = bytes("boot-native-document");
  if (!service.save_current(expected.data(), expected.size()).ok()) {
    return false;
  }

  std::array<uint8_t, 64> output{};
  const ServiceLoadResult loaded =
      service.load_and_apply_runtime(output.data(), output.size());
  return loaded.ok() && loaded.generation == 1 && runtime.apply_calls == 2 &&
         runtime.applied_version == CURRENT_CONFIGURATION_DOCUMENT_VERSION &&
         runtime.applied == expected &&
         std::equal(expected.begin(), expected.end(), output.begin());
}

bool runtime_is_updated_even_if_the_legacy_mirror_fails() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.mirror_failed = true;
  FakeRuntime runtime;
  ConfigurationService service(store, legacy);
  service.set_runtime_adapter(&runtime);
  const std::vector<uint8_t> expected = bytes("durable-native-document");
  const ServiceSaveResult saved =
      service.save_current(expected.data(), expected.size());
  return saved.status == ServiceStatus::LEGACY_MIRROR_FAILED &&
         saved.durable() && runtime.apply_calls == 1 &&
         runtime.applied == expected;
}

bool read_import_only_preserves_upgrade_import_without_legacy_writes() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  FakeRuntime runtime;
  ConfigurationService service(
      store, legacy, nullptr, nullptr, 0,
      LegacyConfigurationMode::READ_IMPORT_ONLY);
  service.set_runtime_adapter(&runtime);
  const std::vector<uint8_t> expected = bytes("native-only-save");
  if (!service.save_current(expected.data(), expected.size()).ok() ||
      service.legacy_writes_enabled() || legacy.mirror_calls != 0 ||
      runtime.apply_calls != 1 || runtime.applied != expected) {
    return false;
  }
  legacy.value = bytes("older-legacy-value");
  std::array<uint8_t, 64> output{};
  const ServiceLoadResult native = service.load(output.data(), output.size());
  if (!native.ok() || native.document_size != expected.size() ||
      !std::equal(expected.begin(), expected.end(), output.begin())) {
    return false;
  }

  MemoryBackend upgrade_backend(256);
  ConfigurationStore upgrade_store(upgrade_backend);
  FakeLegacy upgrade_legacy;
  upgrade_legacy.value = bytes("upgrade-legacy-value");
  ConfigurationService upgrade_service(
      upgrade_store, upgrade_legacy, nullptr, nullptr, 0,
      LegacyConfigurationMode::READ_IMPORT_ONLY);
  output.fill(0);
  const ServiceLoadResult imported =
      upgrade_service.load(output.data(), output.size());
  return imported.imported_legacy() && upgrade_legacy.mirror_calls == 0;
}

bool conditional_save_rejects_a_stale_generation() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  ConfigurationService service(store, legacy);
  const std::vector<uint8_t> first = bytes("first-document");
  const std::vector<uint8_t> second = bytes("second-document");
  if (!service.save_current(first.data(), first.size()).ok()) return false;

  const ServiceSaveResult rejected = service.save_if_generation(
      0, CURRENT_CONFIGURATION_DOCUMENT_VERSION, second.data(), second.size());
  if (rejected.status != ServiceStatus::GENERATION_CONFLICT ||
      rejected.store_status != StoreStatus::GENERATION_CONFLICT ||
      rejected.generation != 1 || legacy.mirror_calls != 1) {
    return false;
  }

  const ServiceSaveResult saved = service.save_if_generation(
      1, CURRENT_CONFIGURATION_DOCUMENT_VERSION, second.data(), second.size());
  std::array<uint8_t, 64> output{};
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return saved.ok() && saved.generation == 2 && legacy.mirror_calls == 2 &&
         loaded.ok() && loaded.generation == 2 &&
         std::equal(second.begin(), second.end(), output.begin());
}

bool version_and_buffer_failures_are_explicit() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  ConfigurationService service(store, legacy);
  const std::vector<uint8_t> expected = bytes("versioned-document");
  if (service.save(CURRENT_CONFIGURATION_DOCUMENT_VERSION + 1,
                   expected.data(), expected.size()).status !=
      ServiceStatus::UNSUPPORTED_VERSION) {
    return false;
  }
  if (!service.save_current(expected.data(), expected.size()).ok()) return false;
  std::array<uint8_t, 2> output{};
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return loaded.status == ServiceStatus::BUFFER_TOO_SMALL &&
         loaded.document_size == expected.size();
}

bool malformed_store_document_is_not_treated_as_legacy() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.value = bytes("legacy-fallback");
  const std::vector<uint8_t> malformed = bytes("not-a-versioned-document");
  if (!store.commit(malformed.data(), malformed.size()).ok()) return false;
  ConfigurationService service(store, legacy);
  std::array<uint8_t, 64> output{};
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return loaded.status == ServiceStatus::INVALID_DOCUMENT &&
         legacy.load_calls == 0;
}

bool panel_config_validator_protects_the_atomic_store() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  PanelConfigDocumentValidator validator;
  ConfigurationService service(store, legacy, &validator);
  const std::vector<uint8_t> invalid = bytes("not-a-panel-config");
  const ServiceSaveResult rejected =
      service.save_current(invalid.data(), invalid.size());
  if (rejected.status != ServiceStatus::INVALID_DOCUMENT ||
      legacy.mirror_calls != 0) {
    return false;
  }

  const std::vector<uint8_t> expected = panel_config_document();
  if (expected.empty()) return false;
  const ServiceSaveResult saved =
      service.save_current(expected.data(), expected.size());
  if (!saved.ok() || legacy.mirrored != expected) return false;

  std::array<uint8_t, 128> output{};
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return loaded.ok() && loaded.document_size == expected.size() &&
         std::equal(expected.begin(), expected.end(), output.begin());
}

bool panel_config_validator_rejects_invalid_stored_documents() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  const std::vector<uint8_t> invalid = bytes("not-a-panel-config");
  ConfigurationService compatibility_service(store, legacy);
  if (!compatibility_service.save_current(invalid.data(), invalid.size()).ok()) {
    return false;
  }

  PanelConfigDocumentValidator validator;
  ConfigurationService service(store, legacy, &validator);
  std::array<uint8_t, 64> output{};
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return loaded.status == ServiceStatus::INVALID_DOCUMENT &&
         loaded.generation == 1 && legacy.load_calls == 0;
}

bool panel_config_validator_rejects_invalid_legacy_imports() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.value = bytes("not-a-panel-config");
  PanelConfigDocumentValidator validator;
  ConfigurationService service(store, legacy, &validator);
  std::array<uint8_t, 64> output{};
  if (service.load(output.data(), output.size()).status !=
      ServiceStatus::INVALID_DOCUMENT) {
    return false;
  }

  ConfigurationService compatibility_service(store, legacy);
  return compatibility_service.load(output.data(), output.size()).status ==
         ServiceStatus::IMPORTED_LEGACY;
}

}  // namespace

int main() {
  const bool passed =
      legacy_is_imported_once() &&
      partial_migration_refreshes_the_native_shadow() &&
      failed_legacy_mirror_keeps_the_native_save_durable() &&
      failed_durable_save_never_updates_legacy() &&
      successful_save_dual_writes() &&
      successful_save_updates_the_native_runtime() &&
      stored_document_can_be_applied_to_the_runtime_atomically() &&
      runtime_is_updated_even_if_the_legacy_mirror_fails() &&
      read_import_only_preserves_upgrade_import_without_legacy_writes() &&
      conditional_save_rejects_a_stale_generation() &&
      version_and_buffer_failures_are_explicit() &&
      malformed_store_document_is_not_treated_as_legacy() &&
      panel_config_validator_protects_the_atomic_store() &&
      panel_config_validator_rejects_invalid_stored_documents() &&
      panel_config_validator_rejects_invalid_legacy_imports();
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
