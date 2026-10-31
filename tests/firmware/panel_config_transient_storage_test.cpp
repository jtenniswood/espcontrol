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
size_t expected_capacity = CAPACITY;
uint8_t *allocate(size_t size) {
  if (fail_allocation)
    return nullptr;
  assert(size == expected_capacity && live_bytes == 0);
  auto *data = static_cast<uint8_t *>(std::malloc(size));
  assert(data);
  live_bytes += size;
  peak_bytes = std::max(peak_bytes, live_bytes);
  ++allocations;
  return data;
}
void release(uint8_t *data) {
  assert(data && live_bytes == expected_capacity);
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
         StoreStatus::ALLOCATION_FAILED);
  std::array<uint8_t, OLD.size()> failed_output{};
  assert(store.load(failed_output.data(), failed_output.size()).status ==
         StoreStatus::ALLOCATION_FAILED);
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

// Observe the store interface even for NVS: the NVS API itself still requires
// whole-blob I/O and exactly one transient slot buffer per operation.
class WindowBackend final : public StorageBackend {
 public:
  explicit WindowBackend(StorageBackend &inner) : inner_(inner) {}
  size_t slot_capacity() const override { return inner_.slot_capacity(); }
  bool begin_operation() override { return inner_.begin_operation(); }
  void end_operation() override { inner_.end_operation(); }
  StoreStatus operation_failure() const override { return inner_.operation_failure(); }
  bool read(uint8_t slot, size_t offset, uint8_t *out, size_t size) override {
    max_read = std::max(max_read, size);
    ++reads;
    if (fail_read_at == reads) return false;
    return inner_.read(slot, offset, out, size);
  }
  bool write(uint8_t slot, size_t offset, const uint8_t *data, size_t size) override {
    max_write = std::max(max_write, size);
    if (offset >= CONFIGURATION_ENVELOPE_HEADER_SIZE && ++payload_writes == cut_payload) {
      const size_t prefix = size * cut_percent / 100;
      if (prefix) assert(inner_.write(slot, offset, data, prefix));
      throw fixture::PowerCut{};
    }
    return inner_.write(slot, offset, data, size);
  }
  bool sync() override { return inner_.sync(); }
  size_t max_read{0}, max_write{0}, reads{0}, payload_writes{0};
  size_t cut_payload{0}, fail_read_at{0};
  unsigned cut_percent{0};
 private:
  StorageBackend &inner_;
};

void large_windows_and_power_cuts(bool partition) {
  fixture::reset();
  constexpr size_t capacity = 8192, payload_size = capacity - 20;
  expected_capacity = capacity;
  using LargeBackend = TransientBlobStorageBackend<capacity>;
  auto payload = [](unsigned seed) {
    std::vector<uint8_t> result(payload_size);
    for (size_t i = 0; i < result.size(); ++i) result[i] = (i * 37 + seed) % 256;
    return result;
  };
  const auto old = payload(17), next = payload(91);
  // Independent frozen envelope goldens: CRC32 generated with Python zlib,
  // including the metadata bytes. Neither reader nor writer under test makes
  // these expected bytes; this detects an accidental storage-format change.
  const std::array<uint8_t, 20> old_header{{69,67,70,71,1,0,20,0,1,0,0,0,236,31,0,0,67,99,41,73}};
  const std::array<uint8_t, 20> new_header{{69,67,70,71,1,0,20,0,2,0,0,0,236,31,0,0,17,192,188,254}};
  std::vector<uint8_t> old_envelope(old_header.begin(), old_header.end());
  old_envelope.insert(old_envelope.end(), old.begin(), old.end());
  std::vector<uint8_t> new_envelope(new_header.begin(), new_header.end());
  new_envelope.insert(new_envelope.end(), next.begin(), next.end());
  auto open = [partition](EspIdfPanelConfigBlobStorage &port) {
    assert(partition ? port.begin_card_images_partition(capacity) : port.begin());
  };
  {
    EspIdfPanelConfigBlobStorage port; open(port);
    assert(port.save_blob(0, old_envelope.data(), old_envelope.size()) && port.sync());
  }
  const auto seed_flash = fixture::flash;
  const auto seed_blobs = fixture::blobs;
  auto restore_seed = [&]() { fixture::reset(); fixture::flash = seed_flash; fixture::blobs = seed_blobs; };
  auto read_back = [&](bool either) {
    EspIdfPanelConfigBlobStorage port; open(port);
    LargeBackend backend(port, allocate, release); assert(backend.begin(capacity));
    WindowBackend spy(backend); ConfigurationStore store(spy);
    std::vector<uint8_t> output(payload_size);
    const auto loaded = store.load(output.data(), output.size());
    assert(loaded.ok() && ((loaded.generation == 1 && output == old) ||
           (either && loaded.generation == 2 && output == next)));
    assert(spy.max_read <= 1024 && live_bytes == 0);
  };
  read_back(false);
  {
    EspIdfPanelConfigBlobStorage port; open(port);
    LargeBackend backend(port, allocate, release); assert(backend.begin(capacity));
    WindowBackend spy(backend); ConfigurationStore store(spy);
    const size_t before = allocations;
    assert(store.commit_if_generation(1, next.data(), next.size()).ok());
    assert(live_bytes == 0 && allocations == before + (partition ? 0 : 1));
    assert(spy.max_read <= 1024 && spy.max_write <= 1024 && spy.payload_writes == 8);
    // The deployed whole-blob port can read the new envelope exactly.
    std::vector<uint8_t> old_reader(capacity);
    const auto loaded = port.load_blob(1, old_reader.data(), old_reader.size());
    assert(loaded.status == BlobLoadStatus::OK && loaded.size == capacity && old_reader == new_envelope);
    read_back(true);
  }
  size_t cuts = 0;
  // Every store payload chunk, before/halfway/after its write. NVS discards
  // unsynced slot RAM on interruption; raw flash keeps a torn unpublished slot.
  for (unsigned percent : {0u, 50u, 100u}) {
    for (size_t chunk = 1; chunk <= 8; ++chunk) {
      restore_seed();
      bool interrupted = false;
      try {
        EspIdfPanelConfigBlobStorage port; open(port);
        LargeBackend backend(port, allocate, release); assert(backend.begin(capacity));
        WindowBackend spy(backend); spy.cut_payload = chunk; spy.cut_percent = percent;
        ConfigurationStore store(spy);
        store.commit(next.data(), next.size());
      } catch (const fixture::PowerCut &) { interrupted = true; }
      assert(interrupted && live_bytes == 0); ++cuts;
      read_back(false);
    }
  }
  // Also cut every real storage mutation, including erase, metadata, the
  // final publication marker and NVS commit, rather than just RAM-copy calls.
  for (unsigned percent : {0u, 50u, 100u}) {
    for (size_t cut = 1; ; ++cut) {
      restore_seed(); fixture::cut_at = cut; fixture::cut_percent = percent;
      bool interrupted = false;
      try {
        EspIdfPanelConfigBlobStorage port; open(port);
        LargeBackend backend(port, allocate, release); assert(backend.begin(capacity));
        WindowBackend spy(backend); ConfigurationStore store(spy);
        assert(store.commit(next.data(), next.size()).ok());
      } catch (const fixture::PowerCut &) { interrupted = true; }
      assert(live_bytes == 0); fixture::cut_at = 0;
      read_back(true);
      if (!interrupted) break;
      ++cuts;
    }
  }
  // Every read in a load (inspection plus winner payload) fails explicitly,
  // cleans its one temporary NVS buffer, and leaves durable bytes unchanged.
  restore_seed(); size_t total_reads = 0;
  {
    EspIdfPanelConfigBlobStorage port; open(port);
    LargeBackend backend(port, allocate, release); assert(backend.begin(capacity));
    WindowBackend spy(backend); ConfigurationStore store(spy);
    std::vector<uint8_t> output(payload_size);
    assert(store.load(output.data(), output.size()).ok()); total_reads = spy.reads;
  }
  for (size_t read = 1; read <= total_reads; ++read) {
    EspIdfPanelConfigBlobStorage port; open(port);
    LargeBackend backend(port, allocate, release); assert(backend.begin(capacity));
    WindowBackend spy(backend); spy.fail_read_at = read; ConfigurationStore store(spy);
    std::vector<uint8_t> output(payload_size);
    const auto failed = store.load(output.data(), output.size());
    // A read error in the unused empty slot may still leave a verified winner.
    assert(failed.status == StoreStatus::READ_FAILED || (failed.ok() && output == old));
    assert(live_bytes == 0 && fixture::flash == seed_flash && fixture::blobs == seed_blobs);
  }
  std::cout << "PASS " << (partition ? "raw" : "NVS") << " bounded I/O, independent envelope, "
            << cuts << " interruptions, " << total_reads << " read failures\n";
  expected_capacity = CAPACITY;
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
  large_windows_and_power_cuts(false);
  large_windows_and_power_cuts(true);
  std::cout << "PASS legacy bytes, downgrade reads, operation-scoped memory "
               "and power cuts\n";
}
