#pragma once

#include "configuration_buffer.h"
#include "panel_config_transient_storage.h"

namespace espcontrol::configuration {

// Run before the service can import legacy text. A valid destination wins,
// and the old NVS store is only read. Raw flash verifies publication through
// the store's checksummed readback without reserving a second document buffer.
template <size_t MaximumSlotCapacity>
StoreStatus migrate_panel_config_storage(
    BlobStorage &source, StorageBackend &destination,
    ConfigurationBuffer::Allocate allocate,
    ConfigurationBuffer::Release release) {
  ConfigurationStore target(destination);
  const auto existing = target.load_prefix(nullptr, 0);
  if (existing.status != StoreStatus::EMPTY) return existing.status;

  const size_t capacity = destination.slot_capacity();
  TransientBlobStorageBackend<MaximumSlotCapacity> source_backend(
      source, allocate, release);
  // Older firmware could save larger NVS blobs than the current write limit.
  if (!source_backend.begin(capacity)) return StoreStatus::INVALID_ARGUMENT;

  ConfigurationBuffer buffer(allocate, release);
  auto *payload = buffer.allocate(capacity);
  if (payload == nullptr) return StoreStatus::ALLOCATION_FAILED;

  ConfigurationStore original(source_backend);
  const auto loaded = original.load(payload, capacity);
  if (!loaded.ok()) return loaded.status;
  return target.commit_if_generation(0, payload, loaded.payload_size).status;
}

}  // namespace espcontrol::configuration
