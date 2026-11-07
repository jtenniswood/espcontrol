#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "page_card_dependencies.h"
#include "page_entity_observer_set.h"
#include "page_runtime_resources_group.h"
#include "page_slider_command_policy.h"
#include "panel_config_document.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
size_t allocations = 0;
void* operator new(size_t n) {
  ++allocations;
  if (auto* p = std::malloc(n ? n : 1)) return p;
  std::abort();
}
void* operator new[](size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }
void operator delete[](void* p, size_t) noexcept { std::free(p); }
namespace {
struct Arena : PageArenaAllocator {
  size_t live{0};
  void* allocate(size_t n) override {
    ++live;
    return std::malloc(n);
  }
  void release(void* p, size_t) override {
    assert(live);
    --live;
    std::free(p);
  }
  ~Arena() { assert(!live); }
};
struct Transport : PageEntitySubscriptions {
  struct Entry {
    const char* entity;
    const char* attr;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::array<Entry, 128> entries{};
  size_t used{0};
  uint32_t epoch{1};
  uint32_t connection_generation() const override { return epoch; }
  bool subscribe(const char* e, const char* a, void* o, uint32_t r, uint32_t c, Deliver d) override {
    assert(used < entries.size());
    entries[used++] = {e, a, o, r, c, d};
    return true;
  }
  void release(void* owner) override {
    for (auto& e : entries)
      if (e.owner == owner) e.owner = nullptr;
  }
  void report(std::string_view entity, std::string_view value, std::string_view attribute = {}) {
    const auto before = allocations;
    for (size_t i = 0; i < used; ++i) {
      const auto e = entries[i];
      if (e.owner && entity == e.entity && attribute == e.attr)
        e.deliver(e.owner, e.revision, e.channel, value);
    }
    assert(before == allocations);
  }
};

struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> memory{};
  PageStore store;
  explicit Document(const DeviceUiLimits& limits, uint32_t generation = 0) {
    PanelConfigWriter writer(bytes.data(), bytes.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {"light.bulb;Light;Auto;Auto;;;slider;;",
                                "fan.speed;Fan;Auto;Auto;;;slider;;",
                                "cover.shade;Shade;Auto;Auto;tilt;;cover;;",
                                "input_number.level;Number;Auto;Auto;;;slider;;",
                                "light.temp;Temperature;Auto;Auto;;2000-6500;light_temperature;;",
                                ";Rooms;Home;Auto;;;subpage;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child =
        "~B|L,light.bulb,Light,Auto,Auto,,,,|L,input_number.level,Number,Auto,Auto,,,,|N,light.temp,"
        "Temperature,Auto,Auto,,2000-6500,,";
    assert(writer.append_subpage(6, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    size_t size = 0;
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, generation, memory.data(), memory.size(),
                               &store) == PageStoreStatus::OK);
  }
};
struct Port : PageCardCommandPort {
  PageSliderCommandPolicy& policy;
  bool busy{false}, online{true}, service{true}, accepted{true}, covered{false};
  uint32_t binding{1};
  std::vector<PageCardCommandSnapshot> received;
  std::function<void(const PageCardCommandSnapshot&)> on_send;
  explicit Port(PageSliderCommandPolicy& policy) : policy(policy) {}
  bool ready() const override { return !busy; }
  bool valid(PageBindingToken t) const override { return !covered && t.binding == binding; }
  bool connected() const override { return online; }
  bool service_available() const override { return service; }
  void dispatch(const PageCardCommandSnapshot&) override { assert(false); }
  bool admit_control(const PageStore& s, PageBindingToken t, PageCardCommandKind k,
                     PageCardCommandValue& v) override {
    return policy.admit(s, t, k, v);
  }
  bool valid_control(const PageStore& s, PageBindingToken t, PageCardCommandKind k,
                     const PageCardCommandValue& v) override {
    return policy.valid(s, t, k, v);
  }
  void discard_control(PageBindingToken t, PageCardCommandKind k, const PageCardCommandValue& v) override {
    policy.discard(t, k, v);
  }
  bool dispatch_control(const PageCardCommandSnapshot& s) override {
    received.push_back(s);
    if (on_send) on_send(s);
    return accepted;
  }
};
struct Harness {
  Document doc;
  Arena arena;
  Transport transport;
  PageCardDependencies deps;
  PagePercentSliderRuntime percent{arena, transport};
  PageNumberSliderRuntime number{arena, transport};
  PageLightTemperatureRuntime temperature{arena, transport};
  PageEntityObserverSet<3> observers;
  PageEntityStateRuntime graph{deps, arena, transport, {256, 128, 8192}, &observers};
  PageSliderCommandPolicy policy{graph, percent, number, temperature};
  Port port{policy};
  PageCardCommands commands{graph, port};
  PageRuntimeResourcesGroup<5> resources{{&commands, &percent, &number, &temperature, &graph}};
  explicit Harness(const DeviceUiLimits& limits) : doc(limits) {
    assert(observers.add(percent) && observers.add(number) && observers.add(temperature));
    assert(resources.prepare(doc.store));
    resources.activate_prepared(doc.store);
    values();
  }
  PageBindingToken token(CardId card, PageId page = 0) const {
    return {doc.store.generation(), port.binding, page, card, 0, false};
  }
  void values() {
    for (auto entity : {"light.bulb", "fan.speed", "cover.shade", "light.temp"})
      transport.report(entity, "on");
    transport.report("light.bulb", "128", "brightness");
    transport.report("fan.speed", "42", "percentage");
    transport.report("cover.shade", "31", "current_tilt_position");
    transport.report("input_number.level", "12");
    transport.report("input_number.level", "0", "min");
    transport.report("input_number.level", "20", "max");
    transport.report("input_number.level", "0.5", "step");
    transport.report("light.temp", "3200", "color_temp_kelvin");
  }
  bool queue_percent(CardId id, int target, PageId page = 0) {
    const auto before = allocations;
    assert(percent.preview({doc.store.generation(), id}, static_cast<uint8_t>(target)));
    const bool result = commands.request_percent(token(id, page), transport.epoch, target);
    assert(allocations == before);
    return result;
  }
  bool queue_number(CardId id = 3, PageId page = 0) {
    const auto before = allocations;
    const ConfigurationHandle h{doc.store.generation(), id};
    double value = 0;
    assert(number.begin(h) && number.finish(h, 21, value));
    assert(value == 10.5);
    const bool result =
        commands.request_number(token(id, page), transport.epoch, value, number.read(h).metadata);
    assert(allocations == before);
    return result;
  }
  bool queue_temperature(CardId id = 4, PageId page = 0) {
    const auto before = allocations;
    const ConfigurationHandle h{doc.store.generation(), id};
    int kelvin = 0;
    assert(temperature.begin(h) && temperature.finish(h, 60, kelvin) && kelvin == 4700);
    const bool result = commands.request_temperature(token(id, page), transport.epoch, kelvin);
    assert(allocations == before);
    return result;
  }
};
void cycles(const DeviceUiLimits& limits) {
  Harness h(limits);
  const auto refs = h.transport.used, arenas = h.arena.live;
  for (int cycle = 0; cycle < 1000; ++cycle) {
    h.port.received.clear();
    assert(h.queue_percent(0, 57) && h.queue_percent(1, 58) && h.queue_percent(2, 59));
    assert(h.queue_number() && h.queue_temperature());
    assert(h.commands.request_slider_toggle(h.token(0), h.transport.epoch));
    h.port.busy = true;
    assert(h.commands.loop() == PageCardCommandResult::NONE);
    h.port.busy = false;
    for (int i = 0; i < 6; ++i) assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
    assert(h.port.received.size() == 6 && h.commands.memory().pending == 0);
    assert(h.port.received[2].config.sensor == "tilt" && h.port.received[2].value.integer == 59);
    assert(h.port.received[3].value.number == 10.5 && h.port.received[4].value.integer == 4700);
    assert(h.transport.used == refs && h.arena.live == arenas);
    ++h.port.binding;
  }
  const auto child = h.doc.store.page(1)->first_card;
  assert(h.queue_percent(child, 61, 1) && h.queue_number(child + 1, 1) && h.queue_temperature(child + 2, 1));
  for (int i = 0; i < 3; ++i) assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(!h.commands.request_percent(h.token(0), h.transport.epoch, 101));
  assert(!h.commands.request_number(h.token(3), h.transport.epoch, std::numeric_limits<double>::infinity(),
                                    {0, 20, 0.5}));
  assert(!h.commands.request_number(h.token(3), h.transport.epoch, 10.3, {0, 20, 0.5}));
  assert(!h.commands.request_number(h.token(3), h.transport.epoch, 10.5, {0, 20, 1}));
  assert(!h.commands.request_temperature(h.token(4), h.transport.epoch, 999));
  assert(!h.commands.request_percent(h.token(3), h.transport.epoch, 50));
  assert(!h.commands.request_slider_toggle(h.token(3), h.transport.epoch));
  assert(!h.commands.request_slider_toggle(h.token(4), h.transport.epoch));
  assert(!h.commands.request_percent({0, h.port.binding, 1, 0, 0, false}, h.transport.epoch, 50));
  assert(!h.commands.request(h.token(0), h.transport.epoch));
}
void rollback_and_revocation() {
  Harness h(DEVICE_UI_PROFILES[8]);
  // Admission can fail after the UI finished a preview (e.g. service unavailable).
  h.port.service = false;
  assert(!h.queue_percent(0, 70) && !h.percent.read({0, 0}).preview);
  assert(!h.queue_number() && h.number.read({0, 3}).position == 24);
  assert(!h.queue_temperature() && h.temperature.read({0, 4}).display_kelvin == 3200);
  h.port.service = true;
  h.port.accepted = false;
  assert(h.queue_percent(0, 70));
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(!h.percent.read({0, 0}).preview && h.percent.read({0, 0}).percent == 50);
  assert(h.queue_number());
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.number.read({0, 3}).position == 24);
  assert(h.queue_temperature());
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.temperature.read({0, 4}).display_kelvin == 3200);
  // Primary-state-only reports rewrite the widget but leave the optimistic
  // Kelvin target intact, so they must not disable rollback after a failed send.
  assert(h.queue_temperature());
  h.transport.report("light.temp", "on");
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.temperature.read({0, 4}).display_kelvin == 3200);
  assert(h.queue_temperature());
  h.transport.report("light.temp", "off");
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.temperature.read({0, 4}).percent == 0);
  h.transport.report("light.temp", "on");
  assert(h.temperature.read({0, 4}).fill_kelvin == 3200);
  assert(h.queue_temperature());
  h.transport.report("light.temp", "4300", "color_temp_kelvin");
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.temperature.read({0, 4}).fill_kelvin == 4300);
  h.values();
  // An older failed send cannot erase a newer queued edit or a newer drag.
  assert(h.queue_percent(0, 20) && h.queue_percent(0, 70));
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.percent.read({0, 0}).preview && h.percent.read({0, 0}).percent == 70);
  assert(h.commands.loop() == PageCardCommandResult::FAILED && !h.percent.read({0, 0}).preview);
  assert(h.queue_number());
  assert(h.number.begin({0, 3}) && h.number.drag({0, 3}, 30));
  assert(h.commands.loop() == PageCardCommandResult::FAILED && h.number.read({0, 3}).gesture);
  h.number.cancel({0, 3});
  assert(h.queue_temperature());
  assert(h.temperature.begin({0, 4}) && h.temperature.drag({0, 4}, 80));
  assert(h.commands.loop() == PageCardCommandResult::FAILED && h.temperature.read({0, 4}).gesture);
  h.temperature.cancel({0, 4});
  h.values();
  h.port.accepted = true;
  for (int i = 0; i < 8; ++i) assert(h.commands.request_slider_toggle(h.token(0), h.transport.epoch));
  assert(!h.queue_percent(0, 80) && !h.percent.read({0, 0}).preview);
  h.commands.cancel();
  assert(h.queue_number());
  h.transport.report("input_number.level", "1", "step");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.values();
  assert(h.queue_percent(0, 80));
  h.transport.report("light.bulb", "unavailable");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.values();
  assert(h.queue_temperature());
  ++h.transport.epoch;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.values();
  assert(h.queue_number());
  ++h.port.binding;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.queue_percent(0, 60));
  h.port.covered = true;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.covered = false;
  assert(h.queue_percent(0, 60) && h.queue_number() && h.queue_temperature());
  assert(h.resources.prepare(h.doc.store));
  h.resources.activate_prepared(h.doc.store);
  assert(!h.commands.memory().pending);
  h.values();
  // Same-generation replacement during a failed native send cannot roll back
  // a newly queued edit, even when owner data/revision happen to match.
  Document next(DEVICE_UI_PROFILES[8]);
  h.port.on_send = [&](const PageCardCommandSnapshot& snapshot) {
    assert(snapshot.value.integer == 40 && h.commands.loop() == PageCardCommandResult::NONE);
    assert(h.resources.prepare(next.store));
    h.resources.activate_prepared(next.store);
    h.values();
    h.doc.memory.fill(0xdd);
    assert(snapshot.config.entity == "light.bulb" && snapshot.config.type == "slider");
    assert(h.queue_percent(0, 70));
    h.port.accepted = false;
  };
  assert(h.queue_percent(0, 40));
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.percent.read({0, 0}).preview && h.percent.read({0, 0}).percent == 70);
  h.port.on_send = {};
  h.port.accepted = true;
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  h.resources.retire_active();
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) cycles(limits);
  rollback_and_revocation();
}
