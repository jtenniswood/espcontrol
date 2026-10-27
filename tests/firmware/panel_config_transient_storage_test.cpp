#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "panel_config_espidf_storage.h"
#include "panel_config_transient_storage.h"
#include "storage_fixture.h"

using namespace espcontrol::configuration;
namespace fixture = storage_fixture;
namespace {
constexpr size_t CAPACITY = 256;
using Backend = TransientBlobStorageBackend<CAPACITY>;
size_t live_bytes = 0, peak_bytes = 0, allocations = 0;
bool fail_allocation = false;
uint8_t *allocate(size_t size) {
  if (fail_allocation)
    return nullptr;
  assert(size == CAPACITY && live_bytes == 0);
  auto *data = static_cast<uint8_t *>(std::malloc(size));
  assert(data);
  live_bytes += size;
  peak_bytes = std::max(peak_bytes, live_bytes);
  ++allocations;
  return data;
}
void release(uint8_t *data) {
  assert(data && live_bytes == CAPACITY);
  live_bytes = 0;
  std::free(data);
}
void begin(EspIdfPanelConfigBlobStorage &port, bool partition) {
  assert(partition ? port.begin_card_images_partition(CAPACITY) : port.begin());
}
const std::array<uint8_t, 7> OLD{{'o', 'r', 'i', 'g', 'i', 'n', 'l'}};
const std::array<uint8_t, 7> NEW{{'r', 'e', 'p', 'l', 'a', 'c', 'e'}};

void assert_load(bool partition, bool either = false) {
  EspIdfPanelConfigBlobStorage port;
  begin(port, partition);
  Backend backend(port, allocate, release);
  assert(backend.begin(CAPACITY));
  ConfigurationStore store(backend);
  std::array<uint8_t, OLD.size()> output{};
  const auto loaded = store.load(output.data(), output.size());
  assert(loaded.ok());
  assert(output == OLD || (either && output == NEW));
  assert(live_bytes == 0);
}

void seed_legacy(bool partition) {
  EspIdfPanelConfigBlobStorage port;
  begin(port, partition);
  BufferedBlobStorageBackend<CAPACITY> backend(port);
  std::array<uint8_t, 2 * CAPACITY> memory{};
  assert(backend.begin(memory.data(), memory.size()));
  ConfigurationStore store(backend);
  assert(store.commit(NEW.data(), NEW.size()).ok());
  assert(store.commit(OLD.data(), OLD.size()).ok());
}

void legacy_interoperability_and_memory(bool partition) {
  fixture::reset();
  seed_legacy(partition);
  assert_load(partition);
  const auto before_flash = fixture::flash;
  const auto before_blobs = fixture::blobs;
  // Get the exact bytes produced by the deployed writer for the same save.
  {
    EspIdfPanelConfigBlobStorage port;
    begin(port, partition);
    BufferedBlobStorageBackend<CAPACITY> backend(port);
    std::array<uint8_t, 2 * CAPACITY> memory{};
    assert(backend.begin(memory.data(), memory.size()));
    ConfigurationStore store(backend);
    assert(store.commit(NEW.data(), NEW.size()).ok());
  }
  const auto expected_flash = fixture::flash;
  const auto expected_blobs = fixture::blobs;
  fixture::flash = before_flash;
  fixture::blobs = before_blobs;
  const size_t allocations_before = allocations;
  {
    EspIdfPanelConfigBlobStorage port;
    begin(port, partition);
    Backend backend(port, allocate, release);
    assert(backend.begin(CAPACITY));
    ConfigurationStore store(backend);
    assert(store.commit_if_generation(2, NEW.data(), NEW.size()).ok());
    assert(live_bytes == 0);
    std::array<uint8_t, 7> output{};
    assert(store.load(output.data(), output.size()).ok() && output == NEW);
    assert(live_bytes == 0);
    assert(store.commit_if_generation(2, OLD.data(), OLD.size()).status ==
           StoreStatus::GENERATION_CONFLICT);
    assert(live_bytes == 0);
  }
  assert(fixture::flash == expected_flash && fixture::blobs == expected_blobs);
  assert(partition ? allocations == allocations_before
                   : allocations == allocations_before + 3);
  // Firmware with the old reader must see the new writer's generation too.
  EspIdfPanelConfigBlobStorage old_port;
  begin(old_port, partition);
  BufferedBlobStorageBackend<CAPACITY> old_backend(old_port);
  std::array<uint8_t, 2 * CAPACITY> memory{};
  assert(old_backend.begin(memory.data(), memory.size()));
  ConfigurationStore old_store(old_backend);
  std::array<uint8_t, 7> output{};
  assert(old_store.load(output.data(), output.size()).generation == 3);
  assert(output == NEW);
}

void power_loss_at_every_mutation(bool partition) {
  fixture::reset();
  seed_legacy(partition);
  const auto seed_flash = fixture::flash;
  const auto seed_blobs = fixture::blobs;
  size_t mutation_count = 0;
  for (unsigned percent : {0u, 50u, 100u}) {
    for (size_t cut = 1; cut <= (mutation_count ? mutation_count : 100);
         ++cut) {
      fixture::flash = seed_flash;
      fixture::blobs = seed_blobs;
      fixture::mutation = 0;
      fixture::cut_at = cut;
      fixture::cut_percent = percent;
      bool interrupted = false;
      try {
        EspIdfPanelConfigBlobStorage port;
        begin(port, partition);
        Backend backend(port, allocate, release);
        assert(backend.begin(CAPACITY));
        ConfigurationStore store(backend);
        assert(store.commit(NEW.data(), NEW.size()).ok());
      } catch (const fixture::PowerCut &) {
        interrupted = true;
      }
      assert(live_bytes == 0);
      fixture::cut_at = 0;
      assert_load(partition, true);
      if (!interrupted) {
        mutation_count = fixture::mutation;
        break;
      }
    }
  }
}

void failures_do_not_retain_memory_or_overwrite() {
  fixture::reset();
  seed_legacy(false);
  const auto original = fixture::blobs;
  EspIdfPanelConfigBlobStorage port;
  begin(port, false);
  Backend backend(port, allocate, release);
  assert(backend.begin(CAPACITY));
  ConfigurationStore store(backend);
  fail_allocation = true;
  assert(store.commit(NEW.data(), NEW.size()).status ==
         StoreStatus::READ_FAILED);
  fail_allocation = false;
  fixture::fail_read = true;
  assert(store.commit(NEW.data(), NEW.size()).status ==
         StoreStatus::READ_FAILED);
  fixture::fail_read = false;
  std::array<uint8_t, 1> small{};
  assert(store.load(small.data(), small.size()).status ==
         StoreStatus::BUFFER_TOO_SMALL);
  assert(live_bytes == 0 && fixture::blobs == original);
  assert_load(false);
  // NVS may expose bytes before commit reports failure. A failed final sync
  // must withdraw the new publication and allow a subsequent same-value retry.
  fixture::mutation = 0;
  fixture::fail_commit_at = 8;
  assert(store.commit(NEW.data(), NEW.size()).status ==
         StoreStatus::SYNC_FAILED);
  assert(live_bytes == 0);
  fixture::fail_commit_at = 0;
  assert_load(false);
  assert(store.commit(NEW.data(), NEW.size()).ok());
  assert(live_bytes == 0);
  // An unreadable raw slot must be an I/O error, never an empty migration.
  fixture::reset();
  seed_legacy(true);
  EspIdfPanelConfigBlobStorage raw;
  begin(raw, true);
  Backend raw_backend(raw, allocate, release);
  assert(raw_backend.begin(CAPACITY));
  ConfigurationStore raw_store(raw_backend);
  fixture::fail_read = true;
  assert(raw_store.load(nullptr, 0).status == StoreStatus::READ_FAILED);
  fixture::fail_read = false;
}

void bounded_partition_read_transactions() {
  fixture::reset();
  constexpr size_t capacity = 40 * 1024;
  EspIdfPanelConfigBlobStorage port;
  assert(port.begin_card_images_partition(capacity));
  TransientBlobStorageBackend<capacity> backend(port, allocate, release);
  assert(backend.begin(capacity));
  ConfigurationStore store(backend);
  std::vector<uint8_t> payload(store.maximum_payload_size(), 0xA5);
  const auto before = allocations;
  assert(store.commit(payload.data(), payload.size()).ok());
  payload.front() = 0x5A;
  payload.back() = 0x42;
  assert(store.commit(payload.data(), payload.size()).ok());
  fixture::read_calls = 0;
  std::vector<uint8_t> output(payload.size());
  assert(store.load(output.data(), output.size()).generation == 2);
  assert(output == payload && allocations == before && live_bytes == 0);
  // A raw-flash call costs far more than a RAM copy. Inspect both full slots
  // and load the winner within a bounded transaction budget, without slot RAM.
  std::cout << "40KiB slot load flash reads: " << fixture::read_calls
            << std::endl;
  assert(fixture::read_calls <= 256);
}
} // namespace

int main() {
  legacy_interoperability_and_memory(false);
  legacy_interoperability_and_memory(true);
  power_loss_at_every_mutation(false);
  power_loss_at_every_mutation(true);
  failures_do_not_retain_memory_or_overwrite();
  bounded_partition_read_transactions();
  assert(live_bytes == 0 && peak_bytes == CAPACITY);
  std::cout << "PASS legacy bytes, downgrade reads, operation-scoped memory "
               "and power cuts\n";
}
