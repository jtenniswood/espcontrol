#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "page_counted_arena.h"
#include "page_store_runtime.h"
#include "panel_config_service_validator.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;

namespace {

class Allocator final : public PageArenaAllocator {
 public:
  void *allocate(size_t bytes) override {
    if (fail || ++attempts == fail_at)
      return nullptr;
    void *arena = std::malloc(bytes);
    if (arena != nullptr) {
      allocations.emplace(arena, bytes);
      live_bytes += bytes;
      peak_bytes = std::max(peak_bytes, live_bytes);
    }
    return arena;
  }
  void release(void *arena, size_t bytes) override {
    assert(allocations.at(arena) == bytes);
    live_bytes -= bytes;
    assert(allocations.erase(arena) == 1);
    std::free(arena);
  }
  bool fail{false};
  size_t attempts{0}, fail_at{0}, live_bytes{0}, peak_bytes{0};
  std::unordered_map<void *, size_t> allocations;
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
  PageCountedArena counted(allocator);
  Backend backend;
  ConfigurationStore store(backend);
  Legacy legacy;
  PanelConfigDocumentValidator validator;
  {
    PageStoreRuntime runtime(limits, counted, 8192);
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
        ServiceStatus::ALLOCATION_FAILED);
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
    assert(runtime.memory().active_reserved_bytes ==
           runtime.active().memory().arena_bytes);
    assert(runtime.memory().active_reserved_bytes < 8192);
    assert(runtime.memory().peak_reserved_bytes == allocator.peak_bytes);
    assert(runtime.memory().preparation_failures == 1);
    assert(counted.memory().reserved_bytes == allocator.live_bytes);
    assert(counted.memory().peak_reserved_bytes == allocator.peak_bytes);
    assert(counted.memory().allocation_failures == 1);
  }
  assert(allocator.allocations.empty());
  assert(counted.memory().reserved_bytes == 0 &&
         counted.memory().live_allocations == 0);
  assert(counted.memory().accounting_errors == 0);
}

class Resources final : public PageRuntimeResources {
public:
  bool prepare(const PageStore &store) override {
    ++prepares;
    prepared = store;
    assert(prepared.field(*prepared.card(0), CardField::LABEL) ==
           "Replacement");
    return !fail;
  }
  void discard_prepared() override { prepared = {}; }
  void activate_prepared(const PageStore &store) override {
    assert(prepared.source_document() == store.source_document());
    assert(prepared.field(*prepared.card(0), CardField::LABEL) ==
           "Replacement");
    active = store;
    prepared = {};
  }
  void retire_active() override { active = {}; }
  PageStore prepared, active;
  bool fail{false};
  size_t prepares{0};
};

void compact_preparation_and_failures() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  Allocator allocator;
  Resources resources;
  const auto original = document(limits, "Original");
  const auto replacement = document(limits, "Replacement");
  {
    PageStoreRuntime runtime(limits, allocator, 8192);
    assert(runtime.prepare(1, original.data(), original.size()));
    assert(runtime.activate_prepared(1, 1, original.data(), original.size()));
    const size_t original_bytes = allocator.live_bytes;
    const auto *original_source = runtime.active().source_document();
    // Fail the exact-size allocation, after a successful ceiling-sized parse.
    allocator.fail_at = allocator.attempts + 2;
    assert(!runtime.prepare(1, replacement.data(), replacement.size()));
    assert(runtime.last_stage_status() == PageStoreStatus::BUFFER_TOO_SMALL);
    assert(allocator.live_bytes == original_bytes &&
           allocator.allocations.size() == 1);
    assert(runtime.memory().staged_reserved_bytes == 0);
    assert(runtime.active().source_document() == original_source);
    assert(runtime.active().field(*runtime.active().card(0),
                                  CardField::LABEL) == "Original");
    assert(runtime.prepare(1, replacement.data(), replacement.size()));
    const size_t replacement_bytes = runtime.memory().staged_reserved_bytes;
    assert(allocator.live_bytes == original_bytes + replacement_bytes);
    assert(runtime.memory().peak_reserved_bytes ==
           original_bytes + 8192 + replacement_bytes);
    assert(runtime.memory().peak_reserved_bytes == allocator.peak_bytes);
    // Publication must need no additional arena allocation.
    allocator.fail = true;
    assert(runtime.activate_prepared(2, 1, replacement.data(),
                                     replacement.size()));
    assert(allocator.live_bytes == replacement_bytes);
    assert(runtime.active().memory().arena_bytes == replacement_bytes);
    assert(runtime.memory().active_reserved_bytes == replacement_bytes);
    assert(runtime.active().field(*runtime.active().card(0),
                                  CardField::LABEL) == "Replacement");
    allocator.fail = false;
  }
  assert(allocator.allocations.empty() && allocator.live_bytes == 0);
  {
    PageStoreRuntime runtime(limits, allocator, 8192, &resources);
    resources.fail = true;
    assert(!runtime.prepare(1, replacement.data(), replacement.size()));
    assert(runtime.last_stage_status() ==
           PageStoreStatus::RESOURCE_UNAVAILABLE);
    assert(allocator.allocations.empty() && allocator.live_bytes == 0);
    resources.fail = false;
    assert(runtime.prepare(1, replacement.data(), replacement.size()));
    // Resources receive only the retained compact view, which survives
    // activation.
    assert(allocator.allocations.size() == 1);
    assert(allocator.live_bytes == resources.prepared.memory().arena_bytes);
    allocator.fail = true;
    assert(runtime.activate_prepared(3, 1, replacement.data(),
                                     replacement.size()));
    assert(resources.active.field(*resources.active.card(0),
                                  CardField::LABEL) == "Replacement");
    allocator.fail = false;
  }
  assert(allocator.allocations.empty() && allocator.live_bytes == 0);
  // A document filling its ceiling needs no compaction allocation.
  alignas(std::max_align_t) std::array<uint8_t, 8192> scratch{};
  PageStore sized;
  assert(PageStore::stage_v1(replacement.data(), replacement.size(), limits, 0,
                             scratch.data(), scratch.size(),
                             &sized) == PageStoreStatus::OK);
  {
    PageStoreRuntime runtime(limits, allocator, sized.memory().arena_bytes);
    allocator.fail_at = allocator.attempts + 2;
    assert(runtime.prepare(1, replacement.data(), replacement.size()));
    assert(runtime.activate_prepared(1, 1, replacement.data(),
                                     replacement.size()));
    assert(runtime.memory().active_reserved_bytes ==
           sized.memory().arena_bytes);
    assert(runtime.memory().peak_reserved_bytes == sized.memory().arena_bytes);
  }
  assert(allocator.allocations.empty() && allocator.live_bytes == 0);
}

void write_admission_preserves_boot_and_inactive_source() {
  struct ResourcesSpy:PageRuntimeResources {
    size_t prepares=0;
    bool prepare(const PageStore&)override{++prepares;return true;}
    void discard_prepared()override{}
    void activate_prepared(const PageStore&)override{}
    void retire_active()override{}
  };
  for(const auto& limits:DEVICE_UI_PROFILES)for(bool inactive:{false,true}) {
    Allocator backing;PageCountedArena arena(backing);ResourcesSpy resources;
    const auto original=document(limits,"Original");
    std::array<uint8_t,8192> bytes{};PanelConfigWriter writer(bytes.data(),bytes.size());
    assert(writer.begin()==PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
    const std::string root=inactive?"light.one;Dormant;Auto;Auto;;;;;":";Folder;Auto;Auto;;;subpage;;";
    assert(writer.append_button(1,reinterpret_cast<const uint8_t*>(root.data()),root.size())==PanelConfigStatus::OK);
    std::string children="~B";
    for(size_t i=0;i<=limits.cards_per_subpage;++i)children+="|,switch.child_"+std::to_string(i)+",Child,Auto,Auto,,,,";
    assert(writer.append_subpage(1,reinterpret_cast<const uint8_t*>(children.data()),children.size())==PanelConfigStatus::OK);
    size_t size=0;assert(writer.finish(&size)==PanelConfigStatus::OK);
    PanelConfigRepresentation converted(ConfigurationBuffer::allocate_host,ConfigurationBuffer::release_host);
    assert(converted.convert(1,2,bytes.data(),size,PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
    for(uint16_t version:{1,2}) {
      PageStoreRuntime runtime(limits,arena,128*1024,&resources);
      assert(runtime.prepare_write(1,original.data(),original.size()));
      assert(runtime.activate_prepared(1,1,original.data(),original.size()));
      const auto reserved=arena.memory().reserved_bytes,prepares=resources.prepares;
      const auto* source=version==1?bytes.data():converted.data();const size_t source_size=version==1?size:converted.size();
      const bool admitted=runtime.prepare_write(version,source,source_size);
      if(inactive) {
        assert(admitted&&!runtime.preparation_capacity_exceeded());runtime.discard_prepared();
      } else {
        assert(!admitted&&runtime.preparation_capacity_exceeded()&&!runtime.preparation_allocation_failed());
        assert(resources.prepares==prepares&&arena.memory().reserved_bytes==reserved&&runtime.active().generation()==1);
      }
      // The read/boot path keeps the complete historical document and its
      // prior semantics. Strict write admission never clips the saved source.
      assert(runtime.prepare(version,source,source_size));
      assert(!runtime.preparation_capacity_exceeded());
      assert(runtime.activate_prepared(2,version,source,source_size));
      assert(runtime.active().source_size()==source_size&&std::memcmp(runtime.active().source_document(),source,source_size)==0);
      assert(runtime.active().page(1)->card_count==limits.cards_per_subpage+1);
    }
    assert(backing.allocations.empty()&&arena.memory().reserved_bytes==0&&arena.memory().accounting_errors==0);
  }
}

void bounded_replacements_and_validation() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  Allocator allocator;
  PageCountedArena counted(allocator);
  const auto a = document(limits, "A");
  const auto b = document(limits, "B");
  {
    PageStoreRuntime runtime(limits, counted, 8192);
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
    assert(runtime.prepare(1, a.data(), a.size()));
    assert(runtime.activate_prepared(0, 1, a.data(), a.size()));
    assert(runtime.active().card(ConfigurationHandle{0, 0}) != nullptr);
    assert(runtime.active().card(ConfigurationHandle{1000, 0}) == nullptr);
  }
  assert(allocator.allocations.empty());
  assert(counted.memory().reserved_bytes == 0 &&
         counted.memory().live_allocations == 0);
  assert(counted.memory().peak_reserved_bytes == allocator.peak_bytes);
  assert(counted.memory().accounting_errors == 0);
  {
    PageStoreRuntime too_small(limits, allocator, 4);
    assert(!too_small.prepare(1, a.data(), a.size()));
    assert(too_small.last_stage_status() == PageStoreStatus::BUFFER_TOO_SMALL);
    assert(!too_small.preparation_allocation_failed());
    assert(!too_small.preparation_capacity_exceeded());
    assert(!too_small.prepare_write(1, a.data(), a.size()));
    assert(too_small.preparation_capacity_exceeded());
    assert(!too_small.preparation_allocation_failed());
    PanelConfigRepresentation converted(ConfigurationBuffer::allocate_host, ConfigurationBuffer::release_host);
    assert(converted.convert(1, 2, a.data(), a.size(), PANEL_CONFIG_V2_HTTP_MAX_BYTES) == PanelConfigRepresentationStatus::OK);
    assert(!too_small.prepare_write(2, converted.data(), converted.size()));
    assert(too_small.preparation_capacity_exceeded());
    assert(!too_small.preparation_allocation_failed());
    assert(!too_small.prepare(2, converted.data(), converted.size()));
    assert(too_small.last_stage_status() == PageStoreStatus::BUFFER_TOO_SMALL);
    assert(!too_small.preparation_capacity_exceeded());
    assert(allocator.allocations.empty());
    assert(too_small.active().generation() == 0);
  }
}

}  // namespace

int main() {
  write_admission_preserves_boot_and_inactive_source();
  staged_service_lifecycle();
  compact_preparation_and_failures();
  bounded_replacements_and_validation();
}
