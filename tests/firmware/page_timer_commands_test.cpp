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
#include "page_timer_command_policy.h"
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

static uint32_t tick = 100;
uint32_t now() { return tick; }
int64_t epoch() { return 0; }
struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> memory{};
  PageStore store;
  explicit Document(const DeviceUiLimits& limits) {
    PanelConfigWriter writer(bytes.data(), bytes.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {"timer.room;Confirmed;Auto;Auto;confirm;1;timer;;",
                                "timer.room;Immediate;Auto;Auto;;;timer;;", ";Timers;Auto;Auto;;;subpage;;"};
    for (size_t i = 0; i < 3; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child = "~B|TM,timer.room,Child,Auto,Auto,,,,";
    assert(writer.append_subpage(3, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    size_t size = 0;
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, 0, memory.data(), memory.size(), &store) ==
           PageStoreStatus::OK);
  }
};
struct Port : PageCardCommandPort {
  PageTimerCommandPolicy& policy;
  bool busy{false}, online{true}, service{true}, accepted{true}, covered{false};
  uint32_t binding{1};
  size_t sends{0}, acknowledgements{0};
  PageCardCommandKind last{PageCardCommandKind::NONE};
  std::function<void(const PageCardCommandSnapshot&)> on_send;
  explicit Port(PageTimerCommandPolicy& p) : policy(p) {}
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
  void accepted_control(PageBindingToken t, PageCardCommandKind k, const PageCardCommandValue& v) override {
    ++acknowledgements;
    policy.accepted(t, k, v);
  }
  bool dispatch_control(const PageCardCommandSnapshot& s) override {
    ++sends;
    last = s.kind;
    if (on_send) on_send(s);
    return accepted;
  }
};
struct Harness {
  Document doc;
  Arena arena;
  Transport transport;
  PageCardDependencies deps;
  PageTimerRuntime timer{arena, transport, now, epoch};
  PageEntityStateRuntime graph{deps, arena, transport, {256, 128, 8192}, &timer};
  PageTimerCommandPolicy policy{graph, timer};
  Port port{policy};
  PageCardCommands commands{graph, port};
  PageRuntimeResourcesGroup<3> resources{{&commands, &timer, &graph}};
  explicit Harness(const DeviceUiLimits& limits) : doc(limits) {
    tick = 100;
    assert(resources.prepare(doc.store));
    resources.activate_prepared(doc.store);
    values();
    assert(timer.read({0, 0}).confirmation && !timer.read({0, 1}).confirmation);
  }
  PageBindingToken token(CardId card = 0, PageId page = 0) const {
    return {0, port.binding, page, card, 0, false};
  }
  void values() {
    transport.report("timer.room", "0:05:00", "duration");
    transport.report("timer.room", "0:00:01", "remaining");
    transport.report("timer.room", "active");
  }
  PageTimerCommandProof proof(CardId card = 0, bool confirmed = true) const {
    return {transport.epoch, timer.read({0, card}).confirmation_revision, confirmed};
  }
  bool queue(PageTimerAction action = PageTimerAction::CANCEL, CardId card = 0, PageId page = 0,
             bool confirmed = true) {
    const auto before = allocations;
    const bool result = commands.request_timer(token(card, page), action, proof(card, confirmed));
    assert(before == allocations);
    return result;
  }
};
void cycles(const DeviceUiLimits& limits) {
  Harness h(limits);
  const auto refs = h.transport.used, live = h.arena.live;
  for (int i = 0; i < 1000; ++i) {
    h.transport.report("timer.room", "idle");
    assert(h.queue(PageTimerAction::START));
    assert(h.commands.loop() == PageCardCommandResult::DISPATCHED &&
           h.port.last == PageCardCommandKind::TIMER_START);
    h.transport.report("timer.room", "active");
    assert(!h.queue(PageTimerAction::CANCEL, 0, 0, false));
    assert(h.queue());
    h.port.busy = true;
    assert(h.commands.loop() == PageCardCommandResult::NONE);
    h.port.busy = false;
    assert(h.commands.loop() == PageCardCommandResult::DISPATCHED &&
           h.port.last == PageCardCommandKind::TIMER_CANCEL);
    h.transport.report("timer.room", "idle");
    assert(h.timer.read({0, 0}).seconds == 300 && h.timer.read({0, 1}).seconds == 0);
    h.transport.report("timer.room", "active");
    assert(h.transport.used == refs && h.arena.live == live && h.commands.memory().pending == 0);
    ++h.port.binding;
  }
  const auto child = h.doc.store.page(1)->first_card;
  assert(h.queue(PageTimerAction::CANCEL, child, 1, false));
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(!h.queue(PageTimerAction::NONE));
  assert(!h.commands.request_timer(h.token(2), PageTimerAction::CANCEL, h.proof()));
  assert(!h.commands.request_timer(h.token(child), PageTimerAction::CANCEL, h.proof(child)));
}
void acknowledgement_and_revalidation() {
  Harness h(DEVICE_UI_PROFILES[8]);
  // Queue acceptance must not suppress the natural-finish grace period.
  assert(h.queue());
  h.transport.report("timer.room", "idle");
  assert(h.timer.read({0, 0}).seconds == 0);
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED && h.port.sends == 0);
  h.values();
  h.port.accepted = false;
  assert(h.queue() && h.commands.loop() == PageCardCommandResult::FAILED);
  h.transport.report("timer.room", "idle");
  assert(h.timer.read({0, 0}).seconds == 0);
  h.values();
  h.port.accepted = true;
  assert(h.queue() && h.commands.loop() == PageCardCommandResult::DISPATCHED);
  h.port.accepted = false;
  assert(h.queue() && h.commands.loop() == PageCardCommandResult::FAILED);
  h.transport.report("timer.room", "idle");
  assert(h.timer.read({0, 0}).seconds == 300);  // Later failure cannot undo an accepted cancel.
  h.values();
  h.port.accepted = true;
  auto stale = h.proof();
  h.transport.report("timer.room", "idle");
  h.transport.report("timer.room", "active");
  assert(!h.commands.request_timer(h.token(), PageTimerAction::CANCEL, stale));
  assert(h.queue());
  h.transport.report("timer.room", "paused");
  h.transport.report("timer.room", "active");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.queue());
  h.port.service = false;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED && !h.queue());
  h.port.service = true;
  assert(h.queue());
  h.port.online = false;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.online = true;
  assert(h.queue());
  h.port.covered = true;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.covered = false;
  assert(h.queue());
  ++h.port.binding;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.queue());
  ++h.transport.epoch;
  h.values();
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  for (int i = 0; i < 8; ++i) assert(h.queue());
  assert(!h.queue());
  h.commands.cancel();
  // A report delivered synchronously by a helper must not mark a newer active phase cancelled.
  const auto acknowledged = h.port.acknowledgements;
  h.port.on_send = [&](const PageCardCommandSnapshot&) {
    h.transport.report("timer.room", "idle");
    h.transport.report("timer.room", "active");
  };
  assert(h.queue() && h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(h.port.acknowledgements == acknowledged + 1);
  h.transport.report("timer.room", "idle");
  assert(h.timer.read({0, 0}).seconds == 0);
  h.port.on_send = {};
  h.values();
  // Reconnect during dispatch also prevents a success from acknowledging the new source.
  h.port.on_send = [&](const PageCardCommandSnapshot&) {
    ++h.transport.epoch;
    h.values();
  };
  const auto before = h.port.acknowledgements;
  assert(h.queue() && h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(h.port.acknowledgements == before);
  h.transport.report("timer.room", "idle");
  assert(h.timer.read({0, 0}).seconds == 0);
  h.port.on_send = {};
  h.values();
  // Same-generation replacement during send must not acknowledge a fresh timer history.
  Document next(DEVICE_UI_PROFILES[8]);
  h.port.on_send = [&](const PageCardCommandSnapshot& snapshot) {
    assert(h.resources.prepare(next.store));
    h.resources.activate_prepared(next.store);
    h.values();
    h.doc.memory.fill(0xdd);
    assert(snapshot.config.entity == "timer.room" && snapshot.config.type == "timer");
    assert(h.commands.loop() == PageCardCommandResult::NONE);
  };
  const auto ack = h.port.acknowledgements;
  assert(h.queue() && h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(h.port.acknowledgements == ack);
  h.transport.report("timer.room", "idle");
  assert(h.timer.read({0, 0}).seconds == 0);
  h.resources.retire_active();
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) cycles(limits);
  acknowledgement_and_revalidation();
}
