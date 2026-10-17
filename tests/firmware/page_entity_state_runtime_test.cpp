#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ha_read_coordinator.h"
#include "page_entity_coordinator_adapter.h"
#include "panel_config_document.h"

static size_t allocation_calls = 0;
void *operator new(size_t bytes) {
  ++allocation_calls;
  if (void *p = std::malloc(bytes ? bytes : 1)) return p;
  std::abort();
}
void *operator new[](size_t bytes) { return ::operator new(bytes); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
namespace {
struct Arena final : PageArenaAllocator {
  void *allocate(size_t bytes) override {
    if (fail) return nullptr;
    ++calls;
    auto *p = std::malloc(bytes); assert(p); live[p] = bytes; return p;
  }
  void release(void *p, size_t bytes) override { assert(live.at(p) == bytes); live.erase(p); std::free(p); }
  bool fail{false}; size_t calls{0}; std::unordered_map<void *, size_t> live;
};
struct Source final : PageDependencySource {
  struct Item { CardId card; std::string entity, attribute; };
  bool enumerate(const PageStore &, PageDependencySink &sink) const override {
    if (fail) return false;
    for (const auto &i : items) if (!sink.add(i.card, i.entity, i.attribute)) return false;
    return true;
  }
  std::vector<Item> items;
  bool fail{false};
};
struct Subscriptions final : PageEntitySubscriptions {
  struct Item { std::string entity, attribute; void *owner; uint32_t revision, channel; Deliver deliver; };
  bool subscribe(const char *entity, const char *attribute, void *owner, uint32_t revision,
                 uint32_t channel, Deliver deliver) override {
    if (remaining == 0) return false;
    if (remaining > 0) --remaining;
    entries.push_back({entity, attribute, owner, revision, channel, deliver}); return true;
  }
  void release(void *owner) override {
    entries.erase(std::remove_if(entries.begin(), entries.end(),
        [owner](const Item &e) { return e.owner == owner; }), entries.end());
  }
  uint32_t connection_generation() const override { return connection; }
  void send(std::string entity, std::string attribute, const std::string &value) {
    const auto snapshot = entries;
    for (const auto &e : snapshot) if (e.entity == entity && e.attribute == attribute)
      e.deliver(e.owner, e.revision, e.channel, value);
  }
  std::vector<Item> entries; int remaining{-1}; uint32_t connection{1};
};
struct Fixture {
  Fixture() {
    PanelConfigWriter writer(document.data(), document.size()); assert(writer.begin() == PanelConfigStatus::OK);
    const auto &limits = DEVICE_UI_PROFILES[8];
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),
        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    for (uint8_t slot = 1; slot <= 3; ++slot) {
      const std::string config = "light.test;Light;Auto;Auto;;;;;";
      assert(writer.append_button(slot, reinterpret_cast<const uint8_t *>(config.data()), config.size()) == PanelConfigStatus::OK);
    }
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 17, arena.data(), arena.size(), &store) == PageStoreStatus::OK);
  }
  alignas(std::max_align_t) std::array<uint8_t, 8192> arena{};
  std::array<uint8_t, 1024> document{}; size_t size{0}; PageStore store;
};
void lifetime_and_state_budget() {
  Fixture f; Arena arena; Subscriptions transport; Source source;
  source.items = {{0,"sensor.one",""},{1,"sensor.one",""},{0,"sensor.one",""},
                  {0,"sensor.one","unit_of_measurement"},{2,"sensor.two",""}};
  {
    PageEntityStateRuntime state(source, arena, transport, {12, 4, 16});
    assert(state.prepare(f.store)); assert(transport.entries.size() == 3);
    assert(state.dependency_count({17,0}) == 0); // Staged values cannot replace the active configuration.
    transport.send("sensor.one", "", "12.5"); transport.send("sensor.two", "", "abc");
    const auto allocations = arena.calls, all_allocations = allocation_calls;
    state.activate_prepared(17); assert(arena.calls == allocations); assert(allocation_calls == all_allocations);
    assert(state.dependency_count({17,0}) == 2); assert(state.dependency_count({17,1}) == 1);
    assert(state.memory().active_dependencies == 4); assert(state.memory().active_channels == 3);
    assert(state.dependency({17,0},0).value == "12.5"); assert(state.dependency({17,1},0).value == "12.5");
    transport.send("sensor.one", "unit_of_measurement", "°C");
    transport.send("sensor.one", "", "1234567890"); // Compact the other values without corrupting them.
    assert(state.dependency({17,2},0).value == "abc"); assert(state.dependency({17,0},1).value == "°C");
    transport.send("sensor.one", "", "12345678901234567");
    assert(state.dependency({17,0},0).status == PageEntityStatus::EXHAUSTED);
    assert(state.dependency({17,1},0).value.empty()); assert(state.dependency({17,2},0).value == "abc");
    assert(state.memory().active_state_bytes == 6); assert(state.memory().state_overflows == 1);
    transport.send("sensor.one", "", "on"); assert(state.dependency({17,0},0).value == "on");
    ++transport.connection;
    assert(state.dependency({17,0},0).status == PageEntityStatus::UNKNOWN);
    assert(state.memory().active_state_bytes == 0);
    transport.send("sensor.one", "", "unavailable");
    assert(state.dependency({17,0},0).value == "unavailable");
    assert(arena.calls == allocations); // Receiving and querying never allocate an arena.
    assert(state.dependency({16,0},0).status == PageEntityStatus::STALE);
  }
  assert(transport.entries.empty()); assert(arena.live.empty());
}
void preparation_and_replacements() {
  Fixture f; Arena arena; Subscriptions transport; Source source;
  source.items = {{0,"sensor.shared",""},{1,"sensor.old",""}};
  PageEntityStateRuntime state(source, arena, transport, {8, 4, 128});
  assert(state.prepare(f.store)); state.activate_prepared(1);
  transport.send("sensor.shared", "", "live"); const auto stale = transport.entries.back();
  source.items = {{2,"sensor.shared",""},{0,"sensor.new",""}};
  assert(state.prepare(f.store)); assert(transport.entries.size() == 4);
  assert(state.dependency({1,0},0).value == "live");
  const auto allocations = arena.calls; state.activate_prepared(2); assert(arena.calls == allocations);
  assert(transport.entries.size() == 2); assert(arena.live.size() == 1);
  assert(state.dependency({2,2},0).value == "live");
  assert(state.dependency({1,0},0).status == PageEntityStatus::STALE);
  stale.deliver(stale.owner, stale.revision, stale.channel, "old");
  assert(state.memory().stale_deliveries == 1);
  for (uint32_t generation = 3; generation < 1003; ++generation) {
    source.items = {{0,"sensor.generation_" + std::to_string(generation),""}};
    assert(state.prepare(f.store)); state.activate_prepared(generation);
    assert(transport.entries.size() == 1); assert(arena.live.size() == 1);
    stale.deliver(stale.owner, stale.revision, stale.channel, "old");
    assert(state.dependency({generation,0},0).status == PageEntityStatus::UNKNOWN);
  }
  const auto memory = state.memory(); assert(memory.active_channels == 1);
  assert(memory.peak_arena_bytes <= 2 * 1024);
  transport.remaining = 1; source.items = {{0,"sensor.a",""},{1,"sensor.b",""}};
  assert(!state.prepare(f.store)); assert(transport.entries.size() == 1); assert(arena.live.size() == 1);
  assert(state.dependency_count({1002,0}) == 1); assert(state.memory().staged_arena_bytes == 0);
  transport.remaining = -1; arena.fail = true; assert(!state.prepare(f.store)); arena.fail = false;
  source.fail = true; assert(!state.prepare(f.store)); source.fail = false;
  source.items = {{NO_CARD,"sensor.bad",""}}; assert(!state.prepare(f.store));
  source.items = {{0,std::string("sensor.bad\0suffix",17),""}}; assert(!state.prepare(f.store));
  assert(state.memory().preparation_failures == 5);
  assert(transport.entries.size() == 1); state.retire_active(); assert(transport.entries.empty());
  assert(arena.live.empty());
}
void page_store_prepares_resources_before_activation() {
  Fixture f; Arena arena; Subscriptions transport; Source source;
  source.items = {{0,"sensor.one",""}};
  PageEntityStateRuntime state(source, arena, transport, {8, 4, 128});
  {
    PageStoreRuntime runtime(DEVICE_UI_PROFILES[8], arena, 8192, &state);
    assert(runtime.prepare(1, f.document.data(), f.size));
    const auto allocations = arena.calls, all_allocations = allocation_calls;
    assert(runtime.activate_prepared(8,1,f.document.data(),f.size)); assert(arena.calls == allocations);
    assert(allocation_calls == all_allocations);
    transport.send("sensor.one", "", "stable"); assert(state.dependency({8,0},0).value == "stable");
    transport.remaining = 0; source.items = {{0,"sensor.two",""}};
    assert(!runtime.prepare(1,f.document.data(),f.size));
    assert(runtime.last_stage_status() == PageStoreStatus::RESOURCE_UNAVAILABLE);
    assert(runtime.active().generation() == 8); assert(state.dependency({8,0},0).value == "stable");
    assert(arena.live.size() == 2); // Exactly the active configuration and state arenas.
  }
  assert(arena.live.empty()); assert(transport.entries.empty());
}
struct Transport {
  using State = std::string; using Callback = std::function<void(State)>;
  static constexpr bool RECLAIMABLE_SUBSCRIPTIONS = true;
  struct Entry { std::string entity,attribute; Callback callback; uint32_t token; };
  bool available() const { return true; } bool state_connected() const { return true; }
  uint32_t subscribe(const std::string &e,const std::string &a,Callback cb) {
    const auto token = ++next; entries.push_back({e,a,std::move(cb),token}); return token;
  }
  void unsubscribe(uint32_t token) {
    entries.erase(std::remove_if(entries.begin(),entries.end(),[token](const Entry &e){return e.token==token;}),entries.end());
  }
  bool request(const std::string &,const std::string &) { return true; }
  uint32_t next{0}; std::vector<Entry> entries;
};
struct Heap { bool available(const char *,size_t,size_t) { return true; } };
void coordinator_deduplicates_generations_without_an_extra_cache() {
  Fixture f; Arena arena; Source source; source.items={{0,"sensor.shared",""},{1,"sensor.shared",""}};
  HaReadCoordinator<Transport,Heap> coordinator;
  CoordinatorPageEntitySubscriptions adapter(coordinator,1u<<4);
  {
    PageEntityStateRuntime state(source,arena,adapter,{8,4,128});
    assert(state.prepare(f.store)); state.activate_prepared(1);
    assert(coordinator.subscription_channel_count()==1); assert(coordinator.subscription_count()==1);
    coordinator.transport().entries[0].callback("live");
    assert(state.dependency({1,1},0).value=="live");
    assert(state.prepare(f.store)); assert(coordinator.subscription_channel_count()==1);
    assert(coordinator.subscription_count()==2);
    const auto all_allocations = allocation_calls;
    state.activate_prepared(2); assert(allocation_calls == all_allocations);
    assert(coordinator.subscription_count()==1); assert(state.dependency({2,0},0).value=="live");
    bool replayed=false;
    assert(coordinator.subscribe("sensor.shared","",[&](const std::string &){replayed=true;},1));
    assert(!replayed); // The configuration cache, not coordinator replay, owns this state.
    coordinator.reset_subscriptions(1); coordinator.invalidate_retained_state();
    assert(state.dependency({2,0},0).status==PageEntityStatus::UNKNOWN);
    coordinator.transport().entries[0].callback("fresh"); assert(state.dependency({2,0},0).value=="fresh");
  }
  coordinator.reclaim_unused_channels(); assert(coordinator.transport().entries.empty()); assert(arena.live.empty());
}
void packed_state_matches_reference_without_allocations() {
  Fixture f; Arena arena; Subscriptions transport; Source source;
  source.items = {{0,"sensor.a",""},{1,"sensor.b",""},{2,"sensor.c",""}};
  PageEntityStateRuntime state(source,arena,transport,{3,3,16});
  assert(state.prepare(f.store)); state.activate_prepared(1);
  std::array<std::string,3> values;
  std::array<PageEntityStatus,3> statuses;
  statuses.fill(PageEntityStatus::UNKNOWN);
  uint32_t random = 981;
  for (int iteration=0;iteration<10000;++iteration) {
    random = random * 1664525u + 1013904223u;
    const size_t index = random % 3, length = (random >> 16) % 17;
    const std::string incoming(length, static_cast<char>('a' + index));
    size_t other_bytes=0;
    for (size_t i=0;i<3;++i) if (i!=index) other_bytes+=values[i].size();
    statuses[index] = length + other_bytes <= 16 ? PageEntityStatus::AVAILABLE : PageEntityStatus::EXHAUSTED;
    values[index] = statuses[index] == PageEntityStatus::AVAILABLE ? incoming : "";
    const auto &entry=transport.entries[index];
    const auto allocations=allocation_calls;
    entry.deliver(entry.owner,entry.revision,entry.channel,incoming);
    for (size_t i=0;i<3;++i) {
      const auto current=state.dependency({1,static_cast<uint16_t>(i)},0);
      assert(current.status==statuses[i]); assert(current.value==values[i]);
    }
    assert(allocation_calls==allocations);
    assert(state.memory().active_state_bytes<=16);
    if (iteration % 127 == 0) {
      ++transport.connection;
      assert(state.dependency({1,0},0).status==PageEntityStatus::UNKNOWN);
      for (auto &value:values) value.clear(); statuses.fill(PageEntityStatus::UNKNOWN);
    }
  }
}
}  // namespace
int main() {
  lifetime_and_state_budget(); preparation_and_replacements();
  page_store_prepares_resources_before_activation(); coordinator_deduplicates_generations_without_an_extra_cache();
  packed_state_matches_reference_without_allocations();
}
