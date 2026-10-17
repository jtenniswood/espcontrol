#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <vector>

#include "page_store_runtime.h"
#include "panel_config_service_validator.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;

namespace {

class Allocator final : public PageArenaAllocator {
 public:
  void *allocate(size_t bytes) override {
    if (fail) return nullptr;
    void *arena = std::malloc(bytes);
    if (arena != nullptr) allocations.insert(arena);
    return arena;
  }
  void release(void *arena, size_t) override {
    assert(allocations.erase(arena) == 1);
    std::free(arena);
  }
  bool fail{false};
  std::unordered_set<void *> allocations;
};

class Backend final : public StorageBackend {
 public:
  size_t slot_capacity() const override { return slots[0].size(); }
  bool read(uint8_t slot, size_t offset, uint8_t *out, size_t size) override {
    if (slot >= 2 || offset > slot_capacity() || size > slot_capacity() - offset) return false;
    std::memcpy(out, slots[slot].data() + offset, size);
    return true;
  }
  bool write(uint8_t slot, size_t offset, const uint8_t *in, size_t size) override {
    if (fail || slot >= 2 || offset > slot_capacity() || size > slot_capacity() - offset) return false;
    std::memcpy(slots[slot].data() + offset, in, size);
    return true;
  }
  bool sync() override { return true; }
  Backend() { for (auto &slot : slots) slot.fill(0xff); }
  std::array<std::array<uint8_t, 1024>, 2> slots{};
  bool fail{false};
};

class Legacy final : public LegacyConfigurationAdapter {
 public:
  LegacyLoadResult load(uint8_t *, size_t) override { return {}; }
  bool mirror(uint16_t, const uint8_t *, size_t) override {
    ++mirrors;
    // The active generation must be visible before compatibility mirroring.
    assert(runtime == nullptr || runtime->active().generation() == mirrors);
    return !fail;
  }
  size_t mirrors{0};
  bool fail{false};
  PageStoreRuntime *runtime{nullptr};
};

std::vector<uint8_t> document(const DeviceUiLimits &limits, const char *label) {
  std::array<uint8_t, 512> buffer{};
  PanelConfigWriter writer(buffer.data(), buffer.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),
      std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  const std::string card = std::string("light.test;") + label + ";Auto;Auto;;;;;";
  assert(writer.append_button(1, reinterpret_cast<const uint8_t *>(card.data()), card.size()) == PanelConfigStatus::OK);
  size_t size;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  return {buffer.begin(), buffer.begin() + size};
}

void staged_service_lifecycle() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  Allocator allocator;
  Backend backend;
  ConfigurationStore store(backend);
  Legacy legacy;
  PanelConfigDocumentValidator validator;
  {
    PageStoreRuntime runtime(limits, allocator, 8192);
    legacy.runtime = &runtime;
    ConfigurationService service(store, legacy, &validator);
    service.set_runtime_adapter(&runtime);
    const auto original = document(limits, "Original");
    assert(service.save_current(original.data(), original.size()).ok());
    assert(runtime.active().generation() == 1 && allocator.allocations.size() == 1);
    assert(runtime.active().field(*runtime.active().card(0), CardField::LABEL) == "Original");
    const auto replacement = document(limits, "Replacement");
    allocator.fail = true;
    assert(service.save_current(replacement.data(), replacement.size()).status ==
        ServiceStatus::RUNTIME_PREPARE_FAILED);
    allocator.fail = false;
    assert(runtime.active().generation() == 1 && legacy.mirrors == 1);
    assert(service.save_if_generation(0, 1, replacement.data(), replacement.size()).status ==
        ServiceStatus::GENERATION_CONFLICT);
    assert(allocator.allocations.size() == 1 && runtime.memory().staged_reserved_bytes == 0);
    backend.fail = true;
    assert(service.save_current(replacement.data(), replacement.size()).status == ServiceStatus::STORE_FAILED);
    backend.fail = false;
    assert(allocator.allocations.size() == 1 && runtime.active().generation() == 1);
    legacy.fail = true;
    const auto saved = service.save_if_generation(1, 1, replacement.data(), replacement.size());
    assert(saved.status == ServiceStatus::LEGACY_MIRROR_FAILED && saved.durable());
    assert(runtime.active().generation() == 2 && allocator.allocations.size() == 1);
    assert(runtime.active().card(ConfigurationHandle{1, 0}) == nullptr);
    assert(runtime.active().field(*runtime.active().card(0), CardField::LABEL) == "Replacement");
    std::array<uint8_t, 512> restored{};
    assert(service.load_and_apply_runtime(restored.data(), restored.size()).ok());
    assert(runtime.active().generation() == 2 && allocator.allocations.size() == 1);
    assert(runtime.memory().active_reserved_bytes == 8192);
    assert(runtime.memory().peak_reserved_bytes == 16384);
    assert(runtime.memory().preparation_failures == 1);
  }
  assert(allocator.allocations.empty());
}

void bounded_replacements_and_validation() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  Allocator allocator;
  const auto a = document(limits, "A");
  const auto b = document(limits, "B");
  {
    PageStoreRuntime runtime(limits, allocator, 8192);
    for (uint32_t generation = 1; generation <= 1000; ++generation) {
      const auto &source = generation % 2 ? a : b;
      assert(runtime.prepare(1, source.data(), source.size()));
      assert(allocator.allocations.size() <= 2);
      assert(runtime.activate_prepared(generation, 1, source.data(), source.size()));
      runtime.discard_prepared();
      assert(runtime.active().generation() == generation);
      assert(allocator.allocations.size() == 1);
    }
    const auto mismatch = document(DEVICE_UI_PROFILES[0], "Mismatch");
    assert(!runtime.prepare(1, mismatch.data(), mismatch.size()));
    assert(runtime.last_stage_status() == PageStoreStatus::PROFILE_MISMATCH);
    assert(runtime.active().generation() == 1000 && allocator.allocations.size() == 1);
    assert(!runtime.prepare(2, a.data(), a.size()));
    assert(!runtime.apply(1, a.data(), a.size()));
    assert(runtime.prepare(1, a.data(), a.size()));
    assert(!runtime.activate_prepared(1001, 1, b.data(), b.size()));
    runtime.discard_prepared();
    assert(runtime.active().generation() == 1000 && allocator.allocations.size() == 1);
  }
  assert(allocator.allocations.empty());
  {
    PageStoreRuntime too_small(limits, allocator, 4);
    assert(!too_small.prepare(1, a.data(), a.size()));
    assert(too_small.last_stage_status() == PageStoreStatus::BUFFER_TOO_SMALL);
    assert(allocator.allocations.empty());
    assert(too_small.active().generation() == 0);
  }
}

}  // namespace

int main() {
  staged_service_lifecycle();
  bounded_replacements_and_validation();
}
