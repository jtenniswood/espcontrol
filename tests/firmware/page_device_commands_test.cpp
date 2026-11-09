#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "page_card_dependencies.h"
#include "page_device_command_policy.h"
#include "page_entity_observer_set.h"
#include "page_runtime_resources_group.h"
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
struct Transport : PageEntitySubscriptions, PageMediaTransport {
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
  bool online{true};
  bool connected() const override { return online; }
  uint32_t now_ms() const override { return 100; }
  int64_t epoch_seconds() const override { return 0; }
  void reannounce() override { ++reannouncements; }
  size_t reannouncements{0};
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
  Document(const DeviceUiLimits& limits, std::string_view row) {
    PanelConfigWriter w(bytes.data(), bytes.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    assert(w.append_button(1, reinterpret_cast<const uint8_t*>(row.data()), row.size()) ==
           PanelConfigStatus::OK);
    assert(w.append_button(32, reinterpret_cast<const uint8_t*>(row.data()), row.size()) ==
           PanelConfigStatus::OK);
    const std::string folder = ";Child;Auto;Auto;;;subpage;;";
    assert(w.append_button(6, reinterpret_cast<const uint8_t*>(folder.data()), folder.size()) ==
           PanelConfigStatus::OK);
    const auto config = parse_cfg(std::string(row));
    const std::string child = "~B,1|" + config.type + "," + config.entity + "," + config.label + "," +
                              config.icon + "," + config.icon_on + "," + config.sensor + "," + config.unit +
                              "," + config.precision + "," + config.options;
    assert(w.append_subpage(6, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    assert(w.append_setting(reinterpret_cast<const uint8_t*>("button_order"), 12,
                            reinterpret_cast<const uint8_t*>("1,6"), 3) == PanelConfigStatus::OK);
    size_t size = 0;
    assert(w.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, 0, memory.data(), memory.size(), &store) ==
           PageStoreStatus::OK);
  }
};
struct Port : PageCardCommandPort {
  PageDeviceCommandPolicy& policy;
  Transport& transport;
  bool busy{false}, service{true}, accepted{true}, covered{false};
  uint32_t binding{1};
  std::vector<PageCardCommandSnapshot> received;
  size_t acknowledgements{0};
  std::function<void()> on_send;
  Port(PageDeviceCommandPolicy& p, Transport& t) : policy(p), transport(t) {}
  bool ready() const override { return !busy; }
  bool valid(PageBindingToken t) const override { return !covered && t.binding == binding; }
  bool connected() const override { return transport.online; }
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
  void accepted_control(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) override {
    ++acknowledgements;
  }
  bool dispatch_control(const PageCardCommandSnapshot& s) override {
    received.push_back(s);
    if (on_send) on_send();
    return accepted;
  }
};
uint32_t now() { return 100; }
struct Harness {
  Document doc;
  Arena arena;
  Transport transport;
  PageCardDependencies deps;
  PageFanStatusRuntime fan{arena, transport, now};
  PageAccessStateRuntime access{arena, transport, now};
  PageEntityObserverSet<2> observers;
  PageEntityStateRuntime graph{deps, arena, transport, {256, 128, 8192}, &observers};
  PageDeviceCommandPolicy policy{graph, fan, access};
  Port port{policy, transport};
  PageCardCommands commands{graph, port};
  PageRuntimeResourcesGroup<4> resources{{&commands, &fan, &access, &graph}};
  Harness(const DeviceUiLimits& limits, std::string_view row) : doc(limits, row) {
    assert(observers.add(fan) && observers.add(access));
    assert(resources.prepare(doc.store));
    resources.activate_prepared(doc.store);
  }
  PageBindingToken token(bool child = false) const {
    return {0,
            port.binding,
            static_cast<PageId>(child ? 1 : 0),
            child ? doc.store.page(1)->first_card : static_cast<CardId>(0),
            0,
            false};
  }
  std::string_view entity() const { return doc.store.field(*doc.store.card(0), CardField::ENTITY); }
  void report(std::string_view state, std::string_view attr = {}) { transport.report(entity(), state, attr); }
  bool request(bool child = false) {
    const auto before = allocations;
    bool ok = commands.request_device(token(child), transport.epoch);
    assert(before == allocations);
    return ok;
  }
};
using A = PageDeviceAction;
struct Case {
  const char* row;
  const char* state;
  const char* attribute;
  const char* value;
  A action;
};
const Case cases[] = {
    {"fan.room;Fan;Auto;Auto;;;fan_switch;;", "off", "", "", A::FAN_ON},
    {"fan.room;Fan;Auto;Auto;;;fan_switch;;", "on", "", "", A::FAN_OFF},
    {"fan.room;Fan;Auto;Auto;;;fan_oscillate;;", "on", "oscillating", " YES ", A::OSCILLATE_OFF},
    {"fan.room;Fan;Auto;Auto;;;fan_oscillate;;", "off", "oscillating", "false", A::OSCILLATE_ON},
    {"fan.room;Fan;Auto;Auto;;;fan_direction;;", "on", "direction", " REVERSE ", A::FORWARD},
    {"fan.room;Fan;Auto;Auto;;;fan_direction;;", "off", "direction", "forward", A::REVERSE},
    {"cover.room;Cover;Auto;Auto;toggle;;cover;;", "unavailable", "", "", A::TOGGLE},
    {"cover.room;Open;Auto;Auto;open;;cover;;", "", "supported_features", "1", A::COVER},
    {"cover.room;Tilt;Auto;Auto;close;;cover;;", "", "supported_features", "32", A::COVER_TILT},
    {"cover.room;Stop;Auto;Auto;stop;;cover;;", "", "supported_features", "8", A::COVER},
    {"cover.room;Position;Auto;Auto;set_position;75;cover;;", "", "supported_features", "128", A::COVER_TILT},
    {"cover.room;Garage;Auto;Auto;;;garage;;", "closed", "", "", A::TOGGLE},
    {"cover.room;Garage;Auto;Auto;open;;garage;;", "", "", "", A::COVER},
    {"cover.room;Gate;Auto;Auto;stop;;gate;;", "open", "", "", A::COVER},
    {"lock.room;Lock;Auto;Auto;;;lock;;", "locked", "", "", A::UNLOCK},
    {"lock.room;Lock;Auto;Auto;;;lock;;", "unlocked", "", "", A::LOCK},
    {"lock.room;Lock;Auto;Auto;unlock;;lock;;", "", "", "", A::UNLOCK},
    {"vacuum.room;Vacuum;Auto;Auto;start_stop;;vacuum;;", "cleaning", "", "", A::VACUUM_STOP},
    {"vacuum.room;Vacuum;Auto;Auto;start_stop;;vacuum;;", "idle", "", "", A::VACUUM_START},
    {"vacuum.room;Vacuum;Auto;Auto;start_dock;;vacuum;;", "cleaning", "", "", A::VACUUM_DOCK},
    {"vacuum.room;Vacuum;Auto;Auto;pause_resume;;vacuum;;", "cleaning", "", "", A::VACUUM_PAUSE},
    {"vacuum.room;Vacuum;Auto;Auto;pause_resume;;vacuum;;", "idle", "", "", A::VACUUM_START_PAUSE},
    {"vacuum.room;Vacuum;Auto;Auto;clean_area;kitchen;vacuum;;", "", "", "", A::VACUUM_AREA},
    {"vacuum.room;Vacuum;Auto;Auto;clean_spot;;vacuum;;", "", "", "", A::VACUUM_SPOT},
    {"vacuum.room;Vacuum;Auto;Auto;locate;;vacuum;;", "", "", "", A::VACUUM_LOCATE},
    {"lawn_mower.room;Mower;Auto;Auto;pause_resume;;lawn_mower;;", "mowing", "", "", A::MOWER_PAUSE},
    {"lawn_mower.room;Mower;Auto;Auto;dock;;lawn_mower;;", "paused", "", "", A::MOWER_DOCK},
    {"lawn_mower.room;Mower;Auto;Auto;;;lawn_mower;;", "paused", "", "", A::MOWER_START},
};
void cycles(const DeviceUiLimits& limits) {
  for (const auto& c : cases) {
    Harness h(limits, c.row);
    if (*c.state) h.report(c.state);
    if (*c.attribute) h.report(c.value, c.attribute);
    const auto arenas = h.arena.live, refs = h.transport.used;
    for (size_t i = 0; i < 1000; ++i) {
      assert(h.request(i % 2));
      h.port.busy = true;
      assert(h.commands.loop() == PageCardCommandResult::NONE);
      h.port.busy = false;
      assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
      const auto& sent = h.port.received.back();
      assert(sent.kind == PageCardCommandKind::DEVICE_ACTION &&
             sent.value.integer == static_cast<int>(c.action));
      assert(h.arena.live == arenas && h.transport.used == refs);
      h.port.received.clear();
      ++h.port.binding;
    }
    const PageBindingToken dormant{0, h.port.binding, 0, 31, 0, false};
    PageCardCommandValue value;
    assert(!h.policy.admit(h.doc.store, dormant, PageCardCommandKind::DEVICE_ACTION, value));
    assert(!h.commands.request_device(dormant, h.transport.epoch));
    assert(h.commands.memory().fixed_bytes == sizeof(PageCardCommands));
  }
}
void revocation() {
  Harness h(DEVICE_UI_PROFILES[8], cases[0].row);
  h.report("off");
  assert(h.request());
  h.report("on");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.request());
  h.port.service = false;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.service = true;
  assert(h.request());
  h.port.covered = true;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.covered = false;
  assert(h.request());
  ++h.port.binding;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  const auto epoch = h.transport.epoch;
  assert(h.request());
  ++h.transport.epoch;
  (void)h.graph.connection_generation();
  h.fan.loop();
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.report("off");
  assert(!h.commands.request_device(h.token(), epoch));
  h.port.accepted = false;
  const auto acks = h.port.acknowledgements;
  assert(h.request());
  assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(h.port.acknowledgements == acks);
  h.port.accepted = true;
  for (size_t i = 0; i < 8; ++i) assert(h.request());
  assert(!h.request());
  h.commands.cancel();
  assert(h.commands.memory().pending == 0);
  assert(h.request());
  h.port.on_send = [&] {
    assert(h.commands.loop() == PageCardCommandResult::NONE);
    h.commands.retire_active();
    assert(h.commands.prepare(h.doc.store));
    h.commands.activate_prepared(h.doc.store);
  };
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED && h.port.acknowledgements == acks);
  h.port.on_send = {};
  assert(h.request());
  assert(h.resources.prepare(h.doc.store));
  h.resources.activate_prepared(h.doc.store);
  assert(h.commands.memory().pending == 0);
  PageCardCommandValue value;
  assert(!h.policy.admit(h.doc.store, h.token(), PageCardCommandKind::MEDIA_ACTION, value));
  value.integer = 999;
  assert(!h.policy.valid(h.doc.store, h.token(), PageCardCommandKind::DEVICE_ACTION, value));
}
void gates() {
  for (const auto* row :
       {"fan.room;Fan;Auto;Auto;;;fan_direction;;", "fan.room;Fan;Auto;Auto;;;fan_oscillate;;"}) {
    Harness h(DEVICE_UI_PROFILES[8], row);
    h.report("on");
    assert(!h.request());
    h.report("unknown", "direction");
    h.report("unknown", "oscillating");
    assert(!h.request());
  }
  for (const auto* row :
       {"cover.room;Open;Auto;Auto;open;;garage;;confirm_on",
        "cover.room;Garage;Auto;Auto;;;garage;;confirm_on,confirm_off",
        "vacuum.room;Status;Auto;Auto;status;;vacuum;;", "vacuum.room;Area;Auto;Auto;clean_area;;vacuum;;",
        "media_player.room;Play;Auto;Auto;play_pause;;media;;", "cover.room;Modal;Auto;Auto;;;cover;;"}) {
    Harness h(DEVICE_UI_PROFILES[8], row);
    h.report("closed");
    assert(!h.request());
  }
  Harness cover(DEVICE_UI_PROFILES[8], cases[7].row);
  cover.report("16", "supported_features");
  assert(cover.request());
  cover.report("1", "supported_features");
  assert(cover.commands.loop() == PageCardCommandResult::CANCELLED);
  cover.report("0", "supported_features");
  assert(!cover.request());
  Harness gate(DEVICE_UI_PROFILES[8], "cover.room;Gate;Auto;Auto;;;gate;;");
  gate.report("unavailable");
  assert(!gate.request());
  Harness mower(DEVICE_UI_PROFILES[8], cases[25].row);
  mower.report("unknown");
  assert(!mower.request());
  Harness vacuum(DEVICE_UI_PROFILES[8], cases[17].row);
  vacuum.report("cleaning");
  assert(vacuum.request());
  vacuum.report("idle");
  assert(vacuum.commands.loop() == PageCardCommandResult::CANCELLED);
  Harness confirm(DEVICE_UI_PROFILES[8], "cover.room;Garage;Auto;Auto;;;garage;;confirm_on");
  confirm.report("open");
  assert(confirm.request());
  confirm.report("closed");
  assert(confirm.commands.loop() == PageCardCommandResult::CANCELLED && !confirm.request());
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) cycles(limits);
  revocation();
  gates();
  std::printf("all 9 profiles: %zu command modes x 1000 root/child cycles; queue=%zu policy=%zu bytes\n",
              sizeof(cases) / sizeof(cases[0]), sizeof(PageCardCommands), sizeof(PageDeviceCommandPolicy));
}
