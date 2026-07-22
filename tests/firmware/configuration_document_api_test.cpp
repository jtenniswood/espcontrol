#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "configuration_document_api.h"

namespace {

using namespace espcontrol::configuration;

class MemoryBackend final : public StorageBackend {
 public:
  explicit MemoryBackend(size_t capacity)
      : slots_{std::vector<uint8_t>(capacity, 0xFF),
               std::vector<uint8_t>(capacity, 0xFF)} {}

  size_t slot_capacity() const override { return slots_[0].size(); }
  bool read(uint8_t slot, size_t offset, uint8_t *output, size_t size) override {
    if (slot >= slots_.size() || offset > slots_[slot].size() ||
        size > slots_[slot].size() - offset) {
      return false;
    }
    if (size > 0) std::memcpy(output, slots_[slot].data() + offset, size);
    return true;
  }
  bool write(uint8_t slot, size_t offset, const uint8_t *data,
             size_t size) override {
    if (slot >= slots_.size() || offset > slots_[slot].size() ||
        size > slots_[slot].size() - offset) {
      return false;
    }
    if (size > 0) std::memcpy(slots_[slot].data() + offset, data, size);
    return true;
  }
  bool sync() override { return true; }

 private:
  std::array<std::vector<uint8_t>, CONFIGURATION_SLOT_COUNT> slots_;
};

class EmptyLegacy final : public LegacyConfigurationAdapter {
 public:
  LegacyLoadResult load(uint8_t *, size_t) override {
    return {LegacyStatus::EMPTY, CURRENT_CONFIGURATION_DOCUMENT_VERSION, 0};
  }
  bool mirror(uint16_t, const uint8_t *, size_t) override {
    ++mirror_count;
    return true;
  }

  size_t mirror_count = 0;
};

bool snapshot_and_replace_form_one_revisioned_boundary() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  EmptyLegacy legacy;
  ConfigurationService service(store, legacy);
  ConfigurationDocumentApi api(service);
  std::array<uint8_t, 64> output{};

  const DocumentSnapshot empty = api.snapshot(output.data(), output.size());
  if (empty.status != DocumentApiStatus::EMPTY || empty.revision != 0) {
    return false;
  }

  constexpr char candidate[] = R"({"version":1,"cards":[]})";
  const DocumentSave saved = api.replace(
      empty.revision, CURRENT_CONFIGURATION_DOCUMENT_VERSION,
      reinterpret_cast<const uint8_t *>(candidate), sizeof(candidate) - 1);
  if (!saved.ok() || saved.revision != 1 || legacy.mirror_count != 1) {
    return false;
  }

  const DocumentSnapshot snapshot = api.snapshot(output.data(), output.size());
  return snapshot.ok() && snapshot.revision == saved.revision &&
         snapshot.document_version == CURRENT_CONFIGURATION_DOCUMENT_VERSION &&
         snapshot.document_size == sizeof(candidate) - 1 &&
         std::memcmp(output.data(), candidate, sizeof(candidate) - 1) == 0;
}

bool stale_replacement_reports_current_revision() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  EmptyLegacy legacy;
  ConfigurationService service(store, legacy);
  ConfigurationDocumentApi api(service);
  constexpr char first[] = "first";
  constexpr char stale[] = "stale";

  if (!api.replace(0, CURRENT_CONFIGURATION_DOCUMENT_VERSION,
                   reinterpret_cast<const uint8_t *>(first),
                   sizeof(first) - 1)
           .ok()) {
    return false;
  }
  const DocumentSave rejected = api.replace(
      0, CURRENT_CONFIGURATION_DOCUMENT_VERSION,
      reinterpret_cast<const uint8_t *>(stale), sizeof(stale) - 1);
  return rejected.status == DocumentApiStatus::CONFLICT &&
         rejected.revision == 1 && legacy.mirror_count == 1;
}

bool snapshot_capacity_failure_reports_required_size() {
  MemoryBackend backend(256);
  ConfigurationStore store(backend);
  EmptyLegacy legacy;
  ConfigurationService service(store, legacy);
  ConfigurationDocumentApi api(service);
  constexpr char value[] = "configuration-document";
  if (!api.replace(0, CURRENT_CONFIGURATION_DOCUMENT_VERSION,
                   reinterpret_cast<const uint8_t *>(value),
                   sizeof(value) - 1)
           .ok()) {
    return false;
  }

  std::array<uint8_t, 2> output{};
  const DocumentSnapshot snapshot = api.snapshot(output.data(), output.size());
  return snapshot.status == DocumentApiStatus::TOO_LARGE &&
         snapshot.revision == 1 && snapshot.document_size == sizeof(value) - 1;
}

}  // namespace

int main() {
  return snapshot_and_replace_form_one_revisioned_boundary() &&
                 stale_replacement_reports_current_revision() &&
                 snapshot_capacity_failure_reports_required_size()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
