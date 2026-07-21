#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "configuration_service.h"

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

std::vector<uint8_t> bytes(const char *value) {
  return std::vector<uint8_t>(value, value + std::strlen(value));
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

bool saves_are_durable_before_the_legacy_mirror() {
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

bool revision_aware_save_rejects_stale_writers() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  ConfigurationService service(store, legacy);
  const std::vector<uint8_t> first = bytes("first");
  const std::vector<uint8_t> stale = bytes("stale");
  if (!service.save_current_if_revision(0, first.data(), first.size()).ok()) {
    return false;
  }

  const ServiceSaveResult rejected = service.save_current_if_revision(
      0, stale.data(), stale.size());
  if (rejected.status != ServiceStatus::REVISION_CONFLICT ||
      rejected.generation != 1 || legacy.mirror_calls != 1) {
    return false;
  }

  std::array<uint8_t, 32> output{};
  const ServiceLoadResult loaded = service.load(output.data(), output.size());
  return loaded.ok() && loaded.generation == 1 &&
         std::equal(first.begin(), first.end(), output.begin());
}

bool revision_aware_save_imports_legacy_before_comparing() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.value = bytes("legacy");
  ConfigurationService service(store, legacy);
  const std::vector<uint8_t> replacement = bytes("replacement");

  const ServiceSaveResult stale = service.save_current_if_revision(
      0, replacement.data(), replacement.size());
  if (stale.status != ServiceStatus::REVISION_CONFLICT ||
      stale.generation != 1 || legacy.load_calls != 1 ||
      legacy.mirror_calls != 0) {
    return false;
  }

  const ServiceSaveResult saved = service.save_current_if_revision(
      1, replacement.data(), replacement.size());
  return saved.ok() && saved.generation == 2 && legacy.load_calls == 1 &&
         legacy.mirror_calls == 1 && legacy.mirrored == replacement;
}

bool fixed_scratch_supports_import_save_and_aliased_load() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  FakeLegacy legacy;
  legacy.value = bytes("fixed-scratch-legacy");
  std::array<uint8_t, 256> scratch{};
  ConfigurationService service(store, legacy, scratch.data(), scratch.size());

  const ServiceLoadResult imported = service.load(scratch.data(), scratch.size());
  if (!imported.imported_legacy() || imported.generation != 1 ||
      !std::equal(legacy.value.begin(), legacy.value.end(), scratch.begin())) {
    return false;
  }

  const std::vector<uint8_t> replacement = bytes("fixed-scratch-replacement");
  const ServiceSaveResult saved = service.save_current_if_revision(
      imported.generation, replacement.data(), replacement.size());
  if (!saved.ok() || saved.generation != 2 || legacy.mirrored != replacement) {
    return false;
  }

  scratch.fill(0);
  const ServiceLoadResult loaded = service.load(scratch.data(), scratch.size());
  return loaded.ok() && loaded.generation == 2 &&
         std::equal(replacement.begin(), replacement.end(), scratch.begin());
}

}  // namespace

int main() {
  const bool passed =
      legacy_is_imported_once() &&
      saves_are_durable_before_the_legacy_mirror() &&
      failed_durable_save_never_updates_legacy() &&
      successful_save_dual_writes() &&
      version_and_buffer_failures_are_explicit() &&
      malformed_store_document_is_not_treated_as_legacy() &&
      revision_aware_save_rejects_stale_writers() &&
      revision_aware_save_imports_legacy_before_comparing() &&
      fixed_scratch_supports_import_save_and_aliased_load();
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
