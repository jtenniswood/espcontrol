#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "configuration_store.h"

namespace {

using espcontrol::configuration::CONFIGURATION_ENVELOPE_HEADER_SIZE;
using espcontrol::configuration::CONFIGURATION_SLOT_COUNT;
using espcontrol::configuration::CommitResult;
using espcontrol::configuration::ConfigurationStore;
using espcontrol::configuration::LoadResult;
using espcontrol::configuration::StorageBackend;
using espcontrol::configuration::StoreStatus;

constexpr size_t STORED_GENERATION_OFFSET = 8;

class MemoryBackend final : public StorageBackend {
 public:
  explicit MemoryBackend(size_t capacity)
      : slots_{std::vector<uint8_t>(capacity, 0xFF),
               std::vector<uint8_t>(capacity, 0xFF)} {}

  size_t slot_capacity() const override { return slots_[0].size(); }

  bool read(uint8_t slot, size_t offset, uint8_t *output,
            size_t size) override {
    if (fail_reads_ || slot >= slots_.size() ||
        offset > slots_[slot].size() ||
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
    if (write_budget_ != std::numeric_limits<size_t>::max()) {
      const size_t writable = std::min(size, write_budget_);
      if (writable > 0) {
        std::memcpy(slots_[slot].data() + offset, data, writable);
        write_budget_ -= writable;
      }
      if (writable != size) return false;
    } else if (size > 0) {
      std::memcpy(slots_[slot].data() + offset, data, size);
    }
    return true;
  }

  bool sync() override {
    ++sync_calls_;
    return fail_sync_call_ == 0 || sync_calls_ != fail_sync_call_;
  }

  void fail_reads(bool fail) { fail_reads_ = fail; }
  void fail_writes_after(size_t bytes) { write_budget_ = bytes; }
  void clear_write_failure() {
    write_budget_ = std::numeric_limits<size_t>::max();
  }
  void fail_sync_on_call(size_t call) { fail_sync_call_ = call; }
  void corrupt(uint8_t slot, size_t offset) { slots_[slot][offset] ^= 0x55; }

 private:
  std::array<std::vector<uint8_t>, CONFIGURATION_SLOT_COUNT> slots_;
  bool fail_reads_{false};
  size_t write_budget_{std::numeric_limits<size_t>::max()};
  size_t sync_calls_{0};
  size_t fail_sync_call_{0};
};

std::vector<uint8_t> bytes(const char *value) {
  return std::vector<uint8_t>(value, value + std::strlen(value));
}

bool payload_equals(const std::vector<uint8_t> &actual,
                    const std::vector<uint8_t> &expected,
                    const LoadResult &result) {
  return result.ok() && result.payload_size == expected.size() &&
         std::equal(expected.begin(), expected.end(), actual.begin());
}

bool empty_store_is_reported() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  std::array<uint8_t, 16> output{};
  return store.load(output.data(), output.size()).status == StoreStatus::EMPTY;
}

bool commit_and_load_round_trip() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> expected = bytes("configuration-v1");
  const CommitResult committed = store.commit(expected.data(), expected.size());
  std::vector<uint8_t> output(expected.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return committed.ok() && committed.generation == 1 && committed.slot == 0 &&
         loaded.generation == committed.generation &&
         payload_equals(output, expected, loaded);
}

bool newest_generation_wins() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> first = bytes("first");
  const std::vector<uint8_t> second = bytes("second");
  if (!store.commit(first.data(), first.size()).ok()) return false;
  const CommitResult committed = store.commit(second.data(), second.size());
  std::vector<uint8_t> output(second.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return committed.ok() && committed.generation == 2 && committed.slot == 1 &&
         loaded.slot == 1 && payload_equals(output, second, loaded);
}

bool corrupt_newest_falls_back() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> first = bytes("known-good");
  const std::vector<uint8_t> second = bytes("new-value");
  if (!store.commit(first.data(), first.size()).ok()) return false;
  const CommitResult newest = store.commit(second.data(), second.size());
  if (!newest.ok()) return false;
  backend.corrupt(newest.slot, CONFIGURATION_ENVELOPE_HEADER_SIZE + 1);
  std::vector<uint8_t> output(first.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return loaded.generation == 1 && payload_equals(output, first, loaded);
}

bool corrupt_stale_generation_cannot_promote_old_payload() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> stale = bytes("stale");
  const std::vector<uint8_t> current = bytes("current");
  const CommitResult first = store.commit(stale.data(), stale.size());
  const CommitResult second = store.commit(current.data(), current.size());
  if (!first.ok() || !second.ok()) return false;

  // A generation bit flip used to make an intact stale payload look newer,
  // because only payload bytes were protected by the checksum.
  backend.corrupt(first.slot, STORED_GENERATION_OFFSET);
  std::vector<uint8_t> output(current.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return loaded.slot == second.slot && loaded.generation == second.generation &&
         payload_equals(output, current, loaded);
}

bool partial_payload_write_preserves_previous() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> first = bytes("stable-value");
  const std::vector<uint8_t> second = bytes("replacement-value");
  if (!store.commit(first.data(), first.size()).ok()) return false;
  backend.fail_writes_after(4);
  const CommitResult failed = store.commit(second.data(), second.size());
  backend.clear_write_failure();
  std::vector<uint8_t> output(first.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return failed.status == StoreStatus::WRITE_FAILED &&
         loaded.generation == 1 && payload_equals(output, first, loaded);
}

bool partial_header_write_preserves_previous() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> first = bytes("stable-value");
  const std::vector<uint8_t> second = bytes("replacement");
  if (!store.commit(first.data(), first.size()).ok()) return false;
  backend.fail_writes_after(sizeof(uint32_t) + second.size() + 5);
  const CommitResult failed = store.commit(second.data(), second.size());
  backend.clear_write_failure();
  std::vector<uint8_t> output(first.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return failed.status == StoreStatus::WRITE_FAILED &&
         loaded.generation == 1 && payload_equals(output, first, loaded);
}

bool torn_metadata_cannot_promote_stale_checksum() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> stale = bytes("shared-prefix");
  const std::vector<uint8_t> current = bytes("current-value");
  const std::vector<uint8_t> replacement = bytes("shared-prefix-with-new-tail");
  if (!store.commit(stale.data(), stale.size()).ok()) return false;
  if (!store.commit(current.data(), current.size()).ok()) return false;

  // Stop after the new generation is written but before payload size and
  // checksum metadata. Without a final validity-marker write, slot zero can
  // look like generation three while retaining generation one's checksum.
  backend.fail_writes_after(sizeof(uint32_t) + replacement.size() + 8);
  const CommitResult failed =
      store.commit(replacement.data(), replacement.size());
  backend.clear_write_failure();

  std::vector<uint8_t> output(current.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return failed.status == StoreStatus::WRITE_FAILED && loaded.slot == 1 &&
         loaded.generation == 2 && payload_equals(output, current, loaded);
}

bool failed_publication_sync_preserves_previous() {
  MemoryBackend backend(128);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> stable = bytes("stable-value");
  const std::vector<uint8_t> replacement = bytes("replacement-value");
  if (!store.commit(stable.data(), stable.size()).ok()) return false;

  // Each commit has four sync points. Fail the final publication sync of the
  // second commit after its marker bytes have already become readable.
  backend.fail_sync_on_call(8);
  const CommitResult failed =
      store.commit(replacement.data(), replacement.size());

  std::vector<uint8_t> output(stable.size());
  const LoadResult loaded = store.load(output.data(), output.size());
  return failed.status == StoreStatus::SYNC_FAILED && loaded.generation == 1 &&
         payload_equals(output, stable, loaded);
}

bool size_and_argument_errors_are_explicit() {
  MemoryBackend backend(48);
  ConfigurationStore store(backend);
  const std::vector<uint8_t> too_large(store.maximum_payload_size() + 1, 1);
  if (store.commit(nullptr, 1).status != StoreStatus::INVALID_ARGUMENT)
    return false;
  if (store.commit(too_large.data(), too_large.size()).status !=
      StoreStatus::PAYLOAD_TOO_LARGE)
    return false;

  const std::vector<uint8_t> value = bytes("buffer-check");
  if (!store.commit(value.data(), value.size()).ok()) return false;
  std::array<uint8_t, 2> small{};
  const LoadResult loaded = store.load(small.data(), small.size());
  return loaded.status == StoreStatus::BUFFER_TOO_SMALL &&
         loaded.payload_size == value.size();
}

bool io_and_sync_errors_are_explicit() {
  MemoryBackend unreadable(128);
  unreadable.fail_reads(true);
  ConfigurationStore unreadable_store(unreadable);
  if (unreadable_store.load(nullptr, 0).status != StoreStatus::READ_FAILED)
    return false;

  MemoryBackend unsynced(128);
  unsynced.fail_sync_on_call(1);
  ConfigurationStore unsynced_store(unsynced);
  const std::vector<uint8_t> value = bytes("not-published");
  return unsynced_store.commit(value.data(), value.size()).status ==
         StoreStatus::SYNC_FAILED;
}

}  // namespace

int main() {
  const bool passed =
      empty_store_is_reported() && commit_and_load_round_trip() &&
      newest_generation_wins() && corrupt_newest_falls_back() &&
      corrupt_stale_generation_cannot_promote_old_payload() &&
      partial_payload_write_preserves_previous() &&
      partial_header_write_preserves_previous() &&
      torn_metadata_cannot_promote_stale_checksum() &&
      failed_publication_sync_preserves_previous() &&
      size_and_argument_errors_are_explicit() &&
      io_and_sync_errors_are_explicit();
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
