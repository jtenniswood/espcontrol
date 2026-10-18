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
#include "page_entity_observer_set.h"
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
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
namespace {
struct Arena final : PageArenaAllocator {
  void *allocate(size_t bytes) override {
    if (fail || remaining == 0) return nullptr;
    if (remaining > 0) --remaining;
    ++calls;
    auto *p = std::malloc(bytes); assert(p); live[p] = bytes; return p;
  }
  void release(void *p, size_t bytes) override { assert(live.at(p) == bytes); live.erase(p); std::free(p); }
  bool fail{false}; int remaining{-1}; size_t calls{0}; std::unordered_map<void *, size_t> live;
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
void interleaved_dependencies_keep_ordinals_and_fail_atomically() {
  Fixture f;Arena arena;Subscriptions transport;Source source;
  source.items={{0,"sensor.old",""}};
  PageEntityStateRuntime state(source,arena,transport,{12,8,128});
  assert(state.prepare(f.store));state.activate_prepared(17);transport.send("sensor.old","","old");
  source.items={{2,"sensor.two",""},{0,"sensor.zero","first"},{2,"sensor.two","second"},
                {1,"sensor.one",""},{0,"sensor.zero","second"},{0,"sensor.zero","first"}};
  arena.remaining=1; // Staged arena succeeds; temporary stable-grouping scratch fails.
  assert(!state.prepare(f.store));arena.remaining=-1;
  assert(arena.live.size()==1&&transport.entries.size()==1);
  assert(state.dependency({17,0},0).value=="old"&&state.dependency_count({17,1})==0);
  const auto before=arena.calls;
  assert(state.prepare(f.store));assert(arena.calls==before+2&&arena.live.size()==2);
  state.activate_prepared(18);assert(arena.live.size()==1&&transport.entries.size()==5);
  assert(state.dependency_count({18,0})==2&&state.dependency_count({18,1})==1&&state.dependency_count({18,2})==2);
  assert(state.dependency({18,0},0).attribute=="first"&&state.dependency({18,0},1).attribute=="second");
  assert(state.dependency({18,1},0).entity=="sensor.one");
  assert(state.dependency({18,2},0).attribute.empty()&&state.dependency({18,2},1).attribute=="second");
  assert(state.dependency_count({17,0})==0&&state.dependency_count({18,UINT16_MAX})==0);
  assert(state.dependency({18,0},SIZE_MAX).status==PageEntityStatus::STALE);
  assert(state.dependency({18,UINT16_MAX},0).status==PageEntityStatus::STALE);
  transport.send("sensor.zero","first","aa");transport.send("sensor.zero","second","bb");transport.send("sensor.two","","cc");
  const auto allocations=allocation_calls;
  for(size_t i=0;i<1000;++i) {
    const auto &entry=transport.entries[1]; // sensor.zero / first, original channel order.
    entry.deliver(entry.owner,entry.revision,entry.channel,i%2?"11":"22");
    assert(state.dependency({18,0},0).value==(i%2?"11":"22"));
    assert(state.dependency({18,0},1).value=="bb"&&state.dependency({18,2},0).value=="cc");
  }
  assert(allocation_calls==allocations);
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
      for (auto &value:values) value.clear();
      statuses.fill(PageEntityStatus::UNKNOWN);
    }
  }
}
}  // namespace
void observer_distinguishes_copied_state_from_transport_delivery() {
  Fixture fixture;Arena arena;Source source;source.items={{0,"sensor.shared",""}};
  HaReadCoordinator<Transport,Heap> coordinator;CoordinatorPageEntitySubscriptions adapter(coordinator,16);
  bool legacy_delivered=false;assert(coordinator.subscribe("sensor.shared","",[&](const std::string &){legacy_delivered=true;},1,nullptr,true));
  coordinator.transport().entries[0].callback("legacy retained");assert(legacy_delivered&&coordinator.retained_channel_count()==1);
  struct Observer:PageEntityObserver {
    size_t copied=0,live=0;uint32_t generation=0;char text[64]{},previous[64]{};
    void observed(uint32_t config,std::string_view entity,std::string_view attribute,std::string_view value,bool seed)override {
      assert(entity=="sensor.shared"&&attribute.empty()&&value.size()<sizeof(text));generation=config;
      std::memcpy(text,value.data(),value.size());text[value.size()]='\0';if(seed)++copied;else ++live;
    }
    void observed_update(uint32_t config,std::string_view entity,std::string_view attribute,
                         std::string_view value,bool seed,std::string_view old)override {
      assert(old.size()<sizeof(previous));
      if(!old.empty())std::memcpy(previous,old.data(),old.size());
      previous[old.size()]='\0';
      observed(config,entity,attribute,value,seed);
    }
  } observer;
  {
    PageEntityStateRuntime state(source,arena,adapter,{8,4,128},&observer);
    assert(state.prepare(fixture.store));state.activate_prepared(17);
    assert(observer.live==0&&observer.copied==0&&state.dependency({17,0},0).status==PageEntityStatus::UNKNOWN);
    coordinator.transport().entries[0].callback("fresh");assert(observer.live==1&&observer.generation==17&&std::strcmp(observer.text,"fresh")==0&&observer.previous[0]==0);
    assert(state.prepare(fixture.store));assert(observer.copied==1&&observer.live==1);
    state.activate_prepared(18);coordinator.transport().entries[0].callback("fresh again");assert(observer.live==2&&observer.generation==18&&std::strcmp(observer.previous,"fresh")==0);
    coordinator.transport().entries[0].callback("fresh again");
    assert(observer.live==3&&std::strcmp(observer.previous,"fresh again")==0);
  }
  coordinator.reset_subscriptions(1);coordinator.reclaim_unused_channels();assert(arena.live.empty()&&coordinator.transport().entries.empty());
}

void shared_observers_see_borrowed_previous_values() {
  struct Observer final : PageEntityObserver {
    size_t calls = 0;
    int order = 0;
    int* phase = nullptr;
    PageEntityObserverSet<2>* set = nullptr;
    PageEntityObserver* remove_next = nullptr;
    PageEntityObserver* add_during_dispatch = nullptr;
    char previous[64]{};
    void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override {
      ++calls;
    }
    void observed_update(uint32_t generation, std::string_view entity, std::string_view attribute,
                         std::string_view, bool copied, std::string_view old) override {
      assert(generation == 17 && entity == "media_player.room" && attribute == "entity_picture" && !copied);
      if (order == 1)
        *phase = 1;
      else {
        assert(*phase == 1);
        *phase = 2;
      }
      assert(old.size() < sizeof(previous));
      if (!old.empty()) std::memcpy(previous, old.data(), old.size());
      previous[old.size()] = '\0';
      ++calls;
      if (remove_next) set->remove(*remove_next);
      if (add_during_dispatch) assert(!set->add(*add_during_dispatch));
    }
  };
  Fixture fixture;
  Arena arena;
  Subscriptions transport;
  Source source;
  source.items = {{0, "media_player.room", "entity_picture"},
                  {1, "media_player.room", "entity_picture_local"}};
  PageEntityObserverSet<2> observers;
  Observer first, second, extra;
  int phase = 0;
  first.order = 1;
  second.order = 2;
  first.phase = second.phase = &phase;
  first.set = second.set = &observers;
  assert(observers.add(first) && observers.add(second) && !observers.add(first) && !observers.add(extra));
  PageEntityStateRuntime state(source, arena, transport, {8, 4, 256}, &observers);
  assert(state.prepare(fixture.store));
  state.activate_prepared(17);
  const auto delivery = transport.entries[0];
  delivery.deliver(delivery.owner, delivery.revision, delivery.channel, "first");
  assert(first.calls == 1 && second.calls == 1 && first.previous[0] == 0 && second.previous[0] == 0);
  const auto allocations = allocation_calls;
  for (size_t i = 0; i < 1000; ++i) {
    delivery.deliver(delivery.owner, delivery.revision, delivery.channel, "same URL");
    const auto current = state.read_attribute({17, 0}, "media_player.room", "entity_picture");
    assert(current.status == PageEntityStatus::AVAILABLE && current.value == "same URL");
    assert(state.read_attribute({17, 1}, "media_player.room", "entity_picture").status ==
           PageEntityStatus::STALE);
    assert(state.read_attribute({18, 0}, "media_player.room", "entity_picture").status ==
           PageEntityStatus::STALE);
  }
  assert(allocation_calls == allocations && first.calls == 1001 && second.calls == 1001);
  assert(std::strcmp(first.previous, "same URL") == 0 && std::strcmp(second.previous, "same URL") == 0);
  first.remove_next = &second;
  first.add_during_dispatch = &extra;
  delivery.deliver(delivery.owner, delivery.revision, delivery.channel, "last");
  assert(first.calls == 1002 && second.calls == 1001 && extra.calls == 0);
  first.remove_next = nullptr;
  first.add_during_dispatch = nullptr;
  assert(observers.add(second));
  ++transport.connection;
  assert(state.read_attribute({17, 0}, "media_player.room", "entity_picture").status ==
         PageEntityStatus::UNKNOWN);
  observers.remove(first);
  observers.remove(second);
}

int main() {
  shared_observers_see_borrowed_previous_values();
  observer_distinguishes_copied_state_from_transport_delivery();
  lifetime_and_state_budget(); preparation_and_replacements();
  page_store_prepares_resources_before_activation(); coordinator_deduplicates_generations_without_an_extra_cache();
  interleaved_dependencies_keep_ordinals_and_fail_atomically();
  packed_state_matches_reference_without_allocations();
}
