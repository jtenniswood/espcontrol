#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <functional>
#include <thread>

#include "configuration_main_loop.h"
#include "panel_config_upload.h"

using namespace espcontrol::configuration;
using namespace std::chrono_literals;
namespace {
struct Backend : StorageBackend {
  Backend() { for (auto &slot : slots) slot.fill(0xff); }
  size_t slot_capacity() const override { return slots[0].size(); }
  bool read(uint8_t slot, size_t offset, uint8_t *data, size_t size) override {
    assert(std::this_thread::get_id() == main_thread);
    if (slot >= 2 || offset > slots[slot].size() || size > slots[slot].size() - offset) return false;
    std::memcpy(data, slots[slot].data() + offset, size); return true;
  }
  bool write(uint8_t slot, size_t offset, const uint8_t *data, size_t size) override {
    assert(std::this_thread::get_id() == main_thread);
    if (fail || slot >= 2 || offset > slots[slot].size() || size > slots[slot].size() - offset) return false;
    std::memcpy(slots[slot].data() + offset, data, size); return true;
  }
  bool sync() override { return !fail; }
  std::thread::id main_thread{std::this_thread::get_id()};
  std::array<std::array<uint8_t, 1024>, 2> slots;
  bool fail{false};
};
struct Legacy : LegacyConfigurationAdapter {
  LegacyLoadResult load(uint8_t *, size_t) override { return {}; }
  bool mirror(uint16_t, const uint8_t *, size_t) override {
    assert(std::this_thread::get_id() == main_thread); ++mirrors; return !fail;
  }
  std::thread::id main_thread{std::this_thread::get_id()};
  unsigned mirrors{0}; bool fail{false};
};
struct Runtime : ConfigurationRuntimeAdapter {
  bool prepare(uint16_t, const uint8_t *data, size_t size) override {
    assert(std::this_thread::get_id() == main_thread); ++preparations;
    if (hook) hook();
    assert(size == expected.size() && std::memcmp(data, expected.data(), size) == 0);
    return !fail;
  }
  bool activate_prepared(uint32_t generation, uint16_t, const uint8_t *, size_t) override {
    assert(std::this_thread::get_id() == main_thread); active_generation = generation; return true;
  }
  bool apply(uint16_t, const uint8_t *, size_t) override { assert(false); return false; }
  void discard_prepared() override { ++discards; }
  std::thread::id main_thread{std::this_thread::get_id()};
  std::function<void()> hook;
  std::array<uint8_t, 4> expected{{'t','e','s','t'}};
  unsigned preparations{0}, discards{0}; uint32_t active_generation{0}; bool fail{false};
};
struct Fixture {
  Fixture() { service.set_runtime_adapter(&runtime); }
  Backend backend; ConfigurationStore store{backend}; Legacy legacy; Runtime runtime;
  ConfigurationService service{store, legacy}; ConfigurationMainLoop queue;
};
void wait_pending(ConfigurationMainLoop &queue) {
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (!queue.pending() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
  assert(queue.pending());
}
ConfigurationSaveTransaction save(Fixture &f, uint32_t generation) {
  ConfigurationSaveTransaction result;
  std::thread worker([&] { result = f.queue.save_if_generation(generation, 1,
      f.runtime.expected.data(), f.runtime.expected.size()); });
  wait_pending(f.queue); assert(f.queue.process(f.service)); worker.join(); return result;
}
void round_trips_and_outcomes() {
  Fixture f;
  assert(!f.queue.process(f.service));
  for (uint32_t i = 0; i < 1000; ++i) {
    const auto saved = save(f, i);
    assert(saved.dispatch == ConfigurationDispatchStatus::COMPLETED && saved.result.ok());
    assert(saved.result.generation == i + 1 && f.runtime.active_generation == i + 1);
    std::array<uint8_t, 32> output{}; ConfigurationLoadTransaction loaded;
    std::thread reader([&] { loaded = f.queue.load(output.data(), output.size()); });
    wait_pending(f.queue); assert(f.queue.process(f.service)); reader.join();
    assert(loaded.dispatch == ConfigurationDispatchStatus::COMPLETED && loaded.result.ok());
    assert(loaded.result.generation == i + 1 && loaded.result.document_size == 4);
    assert(std::memcmp(output.data(), f.runtime.expected.data(), 4) == 0);
  }
  const auto conflict = save(f, 0);
  assert(conflict.result.status == ServiceStatus::GENERATION_CONFLICT && conflict.result.generation == 1000);
  assert(f.runtime.active_generation == 1000);
  f.runtime.fail = true;
  assert(save(f, 1000).result.status == ServiceStatus::RUNTIME_PREPARE_FAILED);
  f.runtime.fail = false; f.backend.fail = true;
  assert(save(f, 1000).result.status == ServiceStatus::STORE_FAILED);
  assert(f.runtime.active_generation == 1000);
  f.backend.fail = false; f.legacy.fail = true;
  const auto warning = save(f, 1000);
  assert(warning.result.status == ServiceStatus::LEGACY_MIRROR_FAILED && warning.result.durable());
  assert(f.runtime.active_generation == 1001);
}
void admission_and_shutdown() {
  Fixture f;
  const auto expired = f.queue.save_if_generation(0, 1, f.runtime.expected.data(), 4, 0ms);
  assert(expired.dispatch == ConfigurationDispatchStatus::EXPIRED);
  assert(!f.queue.process(f.service) && f.runtime.preparations == 0);
  ConfigurationSaveTransaction first;
  std::thread worker([&] { first = f.queue.save_if_generation(0, 1, f.runtime.expected.data(), 4); });
  wait_pending(f.queue);
  assert(f.queue.load(nullptr, 0, 0ms).dispatch == ConfigurationDispatchStatus::BUSY);
  f.queue.stop(); worker.join();
  assert(first.dispatch == ConfigurationDispatchStatus::STOPPED && !f.queue.process(f.service));
  assert(f.runtime.preparations == 0 && f.legacy.mirrors == 0);
  assert(f.queue.load(nullptr, 0).dispatch == ConfigurationDispatchStatus::STOPPED);
}
void running_request_retains_buffer() {
  Fixture f; std::atomic<bool> in_prepare{false}, release{false}, returned{false};
  f.runtime.hook = [&] {
    assert(!f.queue.process(f.service));
    assert(f.queue.load(nullptr, 0).dispatch == ConfigurationDispatchStatus::BUSY);
    in_prepare.store(true);
    while (!release.load()) std::this_thread::yield();
    // stop() inside an executing callback cannot publish an early completion.
    f.queue.stop(); assert(!returned.load());
  };
  ConfigurationSaveTransaction result;
  std::thread worker([&] {
    PanelConfigUpload borrowed(4, ConfigurationBuffer::allocate_host,
                               ConfigurationBuffer::release_host);
    assert(borrowed.append(f.runtime.expected.data(), 4, 0, 4) &&
           borrowed.complete());
    result = f.queue.save_if_generation(0, 1, borrowed.data(), borrowed.size(), 50ms);
    returned.store(true);
    // The request-owned allocation is released when the actual dispatch
    // completes.
  });
  wait_pending(f.queue);
  std::thread observer([&] {
    while (!in_prepare.load()) std::this_thread::yield();
    std::this_thread::sleep_for(75ms);
    assert(!returned.load()); release.store(true);
  });
  assert(f.queue.process(f.service)); worker.join(); observer.join();
  assert(result.dispatch == ConfigurationDispatchStatus::COMPLETED && result.result.ok());
  assert(f.runtime.active_generation == 1 && returned.load());
  assert(f.queue.load(nullptr, 0).dispatch == ConfigurationDispatchStatus::STOPPED);
}
}
int main() { round_trips_and_outcomes(); admission_and_shutdown(); running_request_retains_buffer(); }
