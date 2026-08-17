#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "configuration_store.h"
#include "panel_config_storage_backend.h"

namespace {

using espcontrol::configuration::BlobLoadStatus;
using espcontrol::configuration::BlobLoadResult;
using espcontrol::configuration::BlobStorage;
using espcontrol::configuration::BufferedBlobStorageBackend;
using espcontrol::configuration::ConfigurationStore;

constexpr size_t kSlotCapacity = 96;

class FakeBlobStorage final : public BlobStorage {
 public:
  BlobLoadResult load_blob(uint8_t slot, uint8_t *output,
                           size_t capacity) override {
    if (slot >= blobs_.size() || capacity != kSlotCapacity) {
      return {BlobLoadStatus::FAILED};
    }
    if (!present_[slot]) return {BlobLoadStatus::MISSING};
    std::memcpy(output, blobs_[slot].data(), sizes_[slot]);
    std::memset(output + sizes_[slot], 0xFF, capacity - sizes_[slot]);
    return {BlobLoadStatus::OK, sizes_[slot]};
  }

  bool save_blob(uint8_t slot, const uint8_t *data, size_t size) override {
    if (fail_save_ || slot >= blobs_.size() || size > kSlotCapacity)
      return false;
    std::memcpy(blobs_[slot].data(), data, size);
    std::memset(blobs_[slot].data() + size, 0xFF, kSlotCapacity - size);
    sizes_[slot] = size;
    present_[slot] = true;
    ++save_calls_;
    return true;
  }

  bool sync() override {
    ++sync_calls_;
    return !fail_sync_;
  }

  bool present(uint8_t slot) const { return present_[slot]; }
  size_t save_calls() const { return save_calls_; }
  size_t stored_size(uint8_t slot) const { return sizes_[slot]; }
  size_t sync_calls() const { return sync_calls_; }
  void fail_sync(bool value) { fail_sync_ = value; }

 private:
  std::array<std::array<uint8_t, kSlotCapacity>, 2> blobs_{};
  std::array<bool, 2> present_{};
  std::array<size_t, 2> sizes_{};
  size_t save_calls_{0};
  size_t sync_calls_{0};
  bool fail_save_{false};
  bool fail_sync_{false};
};

bool missing_slots_behave_like_an_empty_store() {
  FakeBlobStorage blobs;
  BufferedBlobStorageBackend<kSlotCapacity> backend(blobs);
  std::array<uint8_t, kSlotCapacity * 2> memory{};
  if (!backend.begin(memory.data(), memory.size())) return false;
  ConfigurationStore store(backend);
  std::array<uint8_t, 32> output{};
  return store.load(output.data(), output.size()).status ==
             espcontrol::configuration::StoreStatus::EMPTY &&
         !blobs.present(0) && !blobs.present(1);
}

bool commits_are_buffered_until_the_store_sync_boundary() {
  FakeBlobStorage blobs;
  BufferedBlobStorageBackend<kSlotCapacity> backend(blobs);
  std::array<uint8_t, kSlotCapacity * 2> memory{};
  if (!backend.begin(memory.data(), memory.size())) return false;
  const std::array<uint8_t, 3> value{{'o', 'k', '!'}};
  if (!backend.write(0, 8, value.data(), value.size()) || blobs.present(0))
    return false;
  if (!backend.sync() || !blobs.present(0) || blobs.save_calls() != 1 ||
      blobs.sync_calls() != 1)
    return false;
  std::array<uint8_t, 3> loaded{};
  return backend.read(0, 8, loaded.data(), loaded.size()) && loaded == value;
}

bool persisted_blobs_are_compact_but_remain_readable() {
  FakeBlobStorage blobs;
  BufferedBlobStorageBackend<kSlotCapacity> backend(blobs);
  std::array<uint8_t, kSlotCapacity * 2> memory{};
  if (!backend.begin(memory.data(), memory.size())) return false;
  ConfigurationStore store(backend);
  const std::array<uint8_t, 3> value{{'o', 'k', '!'}};
  if (!store.commit(value.data(), value.size()).ok() ||
      blobs.stored_size(0) !=
          espcontrol::configuration::CONFIGURATION_ENVELOPE_HEADER_SIZE +
              value.size()) {
    return false;
  }

  BufferedBlobStorageBackend<kSlotCapacity> reloaded_backend(blobs);
  std::array<uint8_t, kSlotCapacity * 2> reloaded_memory{};
  if (!reloaded_backend.begin(reloaded_memory.data(), reloaded_memory.size()))
    return false;
  ConfigurationStore reloaded_store(reloaded_backend);
  std::array<uint8_t, value.size()> loaded{};
  const auto result = reloaded_store.load(loaded.data(), loaded.size());
  return result.ok() && loaded == value;
}

bool failed_sync_keeps_the_pending_slot_for_retry() {
  FakeBlobStorage blobs;
  BufferedBlobStorageBackend<kSlotCapacity> backend(blobs);
  std::array<uint8_t, kSlotCapacity * 2> memory{};
  if (!backend.begin(memory.data(), memory.size())) return false;
  const std::array<uint8_t, 2> value{{'v', '1'}};
  if (!backend.write(1, 0, value.data(), value.size())) return false;
  blobs.fail_sync(true);
  if (backend.sync()) return false;
  blobs.fail_sync(false);
  return backend.sync() && blobs.save_calls() == 2 && blobs.sync_calls() == 2;
}

bool withdrawing_a_slot_discards_stale_header_bytes() {
  FakeBlobStorage blobs;
  BufferedBlobStorageBackend<kSlotCapacity> backend(blobs);
  std::array<uint8_t, kSlotCapacity * 2> memory{};
  if (!backend.begin(memory.data(), memory.size())) return false;
  const std::array<uint8_t, 4> stale{{1, 2, 3, 4}};
  const std::array<uint8_t, 4> withdrawn{};
  if (!backend.write(0, 4, stale.data(), stale.size()) || !backend.sync() ||
      !backend.write(0, 0, withdrawn.data(), withdrawn.size()) ||
      !backend.sync() || blobs.stored_size(0) != withdrawn.size()) {
    return false;
  }
  std::array<uint8_t, stale.size()> retained{};
  return backend.read(0, 4, retained.data(), retained.size()) &&
         retained == std::array<uint8_t, stale.size()>{{0xFF, 0xFF, 0xFF, 0xFF}};
}

}  // namespace

int main() {
  return missing_slots_behave_like_an_empty_store() &&
                 commits_are_buffered_until_the_store_sync_boundary() &&
                 persisted_blobs_are_compact_but_remain_readable() &&
                 failed_sync_keeps_the_pending_slot_for_retry() &&
                 withdrawing_a_slot_discards_stale_header_bytes()
             ? 0
             : 1;
}
