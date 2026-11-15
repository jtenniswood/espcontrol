#pragma once

#include "panel_config_storage_backend.h"

namespace espcontrol::configuration {

// Run before exposing the destination store to ConfigurationService (which
// imports legacy text when empty). Reuse the startup buffers, never modify the
// source, and re-open the destination buffers to verify durable reads rather
// than the write cache. A valid destination always wins on subsequent boots.
template <size_t MaximumSlotCapacity>
StoreStatus migrate_panel_config_storage(BlobStorage &source,
                                         BlobStorage &destination,
                                         size_t destination_capacity,
                                         uint8_t *memory,
                                         size_t memory_size) {
  if (memory == nullptr || destination_capacity > MaximumSlotCapacity ||
      destination_capacity < CONFIGURATION_ENVELOPE_HEADER_SIZE ||
      memory_size / destination_capacity < 4)
    return StoreStatus::INVALID_ARGUMENT;

  uint8_t *const payload = memory + destination_capacity * 2;
  uint8_t *const verified_payload = memory + destination_capacity * 3;
  BufferedBlobStorageBackend<MaximumSlotCapacity> target_backend(destination);
  if (!target_backend.begin(memory, destination_capacity * 2,
                            destination_capacity))
    return StoreStatus::INVALID_ARGUMENT;
  ConfigurationStore target(target_backend);
  const LoadResult existing = target.load(payload, destination_capacity);
  if (existing.status != StoreStatus::EMPTY) return existing.status;

  // Source and destination slot caches share memory only during these
  // sequential stages. Reset the target cache after reading the old store.
  BufferedBlobStorageBackend<MaximumSlotCapacity> source_backend(source);
  // NVS blobs are compact, so use the destination's full capacity rather
  // than today's smaller NVS write limit. Older firmware may have saved more.
  if (!source_backend.begin(memory, destination_capacity * 2, destination_capacity))
    return StoreStatus::INVALID_ARGUMENT;
  ConfigurationStore original(source_backend);
  const LoadResult loaded = original.load(payload, destination_capacity);
  if (!loaded.ok()) return loaded.status;

  target_backend.begin(memory, destination_capacity * 2, destination_capacity);
  const CommitResult copied = target.commit(payload, loaded.payload_size);
  if (!copied.ok()) return copied.status;
  target_backend.begin(memory, destination_capacity * 2, destination_capacity);
  const LoadResult verified = target.load(verified_payload, destination_capacity);
  if (!verified.ok() || verified.payload_size != loaded.payload_size ||
      std::memcmp(payload, verified_payload, loaded.payload_size) != 0)
    return StoreStatus::VERIFY_FAILED;
  return StoreStatus::OK;
}

}  // namespace espcontrol::configuration
