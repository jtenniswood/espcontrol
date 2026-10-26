#include "page_card_control_router.h"

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
#include "page_media_command_policy.h"
#include "page_runtime_resources_group.h"
#include "page_slider_command_policy.h"
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

uint32_t now() { return 100; }
int64_t epoch() { return 0; }
struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> memory{};
  PageStore store;
  explicit Document(const DeviceUiLimits& limits) {
    PanelConfigWriter w(bytes.data(), bytes.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {"light.room;Brightness;Auto;Auto;;;slider;;",
                                "input_number.room;Number;Auto;Auto;;;slider;;",
                                "light.room;Temperature;Auto;Auto;;2000-6500;light_temperature;;",
                                "timer.room;Timer;Auto;Auto;;;timer;;",
                                "media_player.room;Next;Auto;Auto;next;;media;;",
                                "media_player.room;Position;Auto;Auto;position;;media;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(w.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    size_t size = 0;
    assert(w.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, 0, memory.data(), memory.size(), &store) ==
           PageStoreStatus::OK);
  }
};
using Kind = PageCardCommandKind;
struct Port : PageCardCommandPort {
  PageCardControlPolicy& policy;
  bool online{true}, service{true}, covered{false}, accepted{true}, busy{false};
  uint32_t binding{1};
  size_t sends{0}, acknowledgements{0};
  std::array<Kind, 8> kinds{};
  size_t used{0};
  std::function<void(const PageCardCommandSnapshot&)> on_send;
  explicit Port(PageCardControlPolicy& p) : policy(p) {}
  bool ready() const override { return !busy; }
  bool valid(PageBindingToken t) const override { return !covered && t.binding == binding; }
  bool connected() const override { return online; }
  bool service_available() const override { return service; }
  void dispatch(const PageCardCommandSnapshot&) override { assert(false); }
  bool admit_control(const PageStore& s, PageBindingToken t, Kind k, PageCardCommandValue& v) override {
    return policy.admit(s, t, k, v);
  }
  bool valid_control(const PageStore& s, PageBindingToken t, Kind k, const PageCardCommandValue& v) override {
    return policy.valid(s, t, k, v);
  }
  void discard_control(PageBindingToken t, Kind k, const PageCardCommandValue& v) override {
    policy.discard(t, k, v);
  }
  void accepted_control(PageBindingToken t, Kind k, const PageCardCommandValue& v) override {
    ++acknowledgements;
    policy.accepted(t, k, v);
  }
  bool dispatch_control(const PageCardCommandSnapshot& s) override {
    ++sends;
    assert(used < kinds.size());
    kinds[used++] = s.kind;
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
  PageTimerRuntime timer{arena, transport, now, epoch};
  PageMediaStateRuntime media{arena, transport};
  PageEntityObserverSet<5> observers;
  PageEntityStateRuntime graph{deps, arena, transport, {256, 128, 8192}, &observers};
  PageSliderCommandPolicy sliders{graph, percent, number, temperature};
  PageTimerCommandPolicy timers{graph, timer};
  PageMediaCommandPolicy players{graph, media};
  PageCardControlRouter<8> router{{{{Kind::PERCENT, &sliders},
                                    {Kind::NUMBER, &sliders},
                                    {Kind::TEMPERATURE, &sliders},
                                    {Kind::SLIDER_TOGGLE, &sliders},
                                    {Kind::TIMER_START, &timers},
                                    {Kind::TIMER_CANCEL, &timers},
                                    {Kind::MEDIA_ACTION, &players},
                                    {Kind::MEDIA_SEEK, &players}}}};
  Port port{router};
  PageCardCommands commands{graph, port};
  PageRuntimeResourcesGroup<7> resources{
      {&commands, &percent, &number, &temperature, &timer, &media, &graph}};
  explicit Harness(const DeviceUiLimits& limits) : doc(limits) {
    assert(observers.add(percent) && observers.add(number) && observers.add(temperature) &&
           observers.add(timer) && observers.add(media));
    assert(router.ready() && resources.prepare(doc.store));
    resources.activate_prepared(doc.store);
    values();
  }
  PageBindingToken token(CardId card) const { return {0, port.binding, 0, card, card, false}; }
  void values() {
    transport.report("light.room", "on");
    transport.report("light.room", "128", "brightness");
    transport.report("light.room", "3200", "color_temp_kelvin");
    transport.report("input_number.room", "12");
    transport.report("input_number.room", "0", "min");
    transport.report("input_number.room", "20", "max");
    transport.report("input_number.room", "0.5", "step");
    transport.report("timer.room", "0:05:00", "duration");
    transport.report("timer.room", "0:00:01", "remaining");
    transport.report("timer.room", "active");
    transport.report("media_player.room", "playing");
    transport.report("media_player.room", "300", "media_duration");
    transport.report("media_player.room", "60", "media_position");
    transport.report("media_player.room", "track-a", "media_content_id");
  }
  PageMediaSeekProof seek_proof() const {
    PageMediaPlaybackSnapshot s;
    assert(media.playback({0, 5}, "media_player.room", s));
    return {transport.epoch, s.seek_revision};
  }
  void queue() {
    const auto before = allocations;
    assert(percent.preview({0, 0}, 70) && commands.request_percent(token(0), transport.epoch, 70));
    double value = 0;
    assert(number.begin({0, 1}) && number.finish({0, 1}, 31, value) && value == 15.5);
    assert(commands.request_number(token(1), transport.epoch, value, number.read({0, 1}).metadata));
    int kelvin = 0;
    assert(temperature.begin({0, 2}) && temperature.finish({0, 2}, 60, kelvin) && kelvin == 4700);
    assert(commands.request_temperature(token(2), transport.epoch, kelvin));
    assert(commands.request_timer(token(3), PageTimerAction::CANCEL,
                                  {transport.epoch, timer.read({0, 3}).confirmation_revision, false}));
    assert(commands.request_media(token(4), transport.epoch) &&
           commands.request_seek(token(5), seek_proof(), 80, 300));
    assert(allocations == before && commands.memory().pending == 6);
  }
};
void cycles(const DeviceUiLimits& limits) {
  Harness h(limits);
  const auto refs = h.transport.used, live = h.arena.live;
  for (int cycle = 0; cycle < 1000; ++cycle) {
    h.port.used = 0;
    h.queue();
    h.port.busy = true;
    assert(h.commands.loop() == PageCardCommandResult::NONE);
    h.port.busy = false;
    for (int i = 0; i < 6; ++i) assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
    const std::array<Kind, 6> expected = {Kind::PERCENT,      Kind::NUMBER,       Kind::TEMPERATURE,
                                          Kind::TIMER_CANCEL, Kind::MEDIA_ACTION, Kind::MEDIA_SEEK};
    for (size_t i = 0; i < expected.size(); ++i) assert(h.port.kinds[i] == expected[i]);
    h.transport.report("timer.room", "idle");
    assert(h.timer.read({0, 3}).seconds == 300);
    h.values();
    assert(h.commands.memory().pending == 0 && h.transport.used == refs && h.arena.live == live);
    ++h.port.binding;
  }
  h.queue();
  assert(h.commands.request_media(h.token(4), h.transport.epoch));
  assert(h.commands.request_seek(h.token(5), h.seek_proof(), 80, 300));
  assert(h.commands.memory().pending == 8 && !h.commands.request_media(h.token(4), h.transport.epoch));
  h.commands.cancel();
  assert(h.commands.memory().pending == 0);
  // A registered policy still owns validation: wrong-family routes fail closed.
  PageCardControlRouter<1> wrong{{{{Kind::NUMBER, &h.timers}}}};
  PageCardCommandValue value;
  value.number = 15.5;
  value.metadata = h.number.read({0, 1}).metadata;
  assert(wrong.ready() && !wrong.admit(h.doc.store, h.token(1), Kind::NUMBER, value));
  assert(!h.commands.request_percent(h.token(1), h.transport.epoch, 70));
  assert(!h.commands.request_media(h.token(3), h.transport.epoch));
  assert(!h.commands.request_timer(h.token(0), PageTimerAction::CANCEL, {h.transport.epoch, 0, true}));
}
void rollback_and_replacement() {
  Harness h(DEVICE_UI_PROFILES[8]);
  h.port.accepted = false;
  h.queue();
  for (int i = 0; i < 6; ++i) assert(h.commands.loop() == PageCardCommandResult::FAILED);
  assert(!h.percent.read({0, 0}).preview && h.percent.read({0, 0}).percent == 50);
  assert(h.number.read({0, 1}).position == 24 && h.temperature.read({0, 2}).display_kelvin == 3200);
  h.transport.report("timer.room", "idle");
  assert(h.timer.read({0, 3}).seconds == 0);
  h.values();
  h.port.used = 0;
  h.queue();
  h.port.covered = true;
  for (int i = 0; i < 6; ++i) assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.covered = false;
  assert(h.port.acknowledgements == 0);
  h.queue();
  ++h.transport.epoch;
  h.values();
  for (int i = 0; i < 6; ++i) assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.accepted = true;
  h.queue();
  h.port.on_send = [&](const PageCardCommandSnapshot&) { h.port.service = false; };
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  for (int i = 0; i < 5; ++i) assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.port.used == 1);
  h.port.service = true;
  h.port.on_send = {};
  Document next(DEVICE_UI_PROFILES[8]);
  h.port.on_send = [&](const PageCardCommandSnapshot& snapshot) {
    assert(h.resources.prepare(next.store));
    h.resources.activate_prepared(next.store);
    h.values();
    h.doc.memory.fill(0xdd);
    assert(snapshot.config.entity == "media_player.room" && snapshot.config.type == "media");
    assert(h.commands.request_media(h.token(4), h.transport.epoch) &&
           h.commands.loop() == PageCardCommandResult::NONE);
  };
  const auto ack = h.port.acknowledgements;
  assert(h.commands.request_media(h.token(4), h.transport.epoch) &&
         h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(h.port.acknowledgements == ack);
  h.port.on_send = {};
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  h.resources.retire_active();
}
struct Spy : PageCardControlPolicy {
  size_t admissions{0}, validations{0}, discards{0}, accepted_count{0};
  bool admit(const PageStore&, PageBindingToken, Kind, PageCardCommandValue&) override {
    ++admissions;
    return true;
  }
  bool valid(const PageStore&, PageBindingToken, Kind, const PageCardCommandValue&) override {
    ++validations;
    return true;
  }
  void discard(PageBindingToken, Kind, const PageCardCommandValue&) override { ++discards; }
  void accepted(PageBindingToken, Kind, const PageCardCommandValue&) override { ++accepted_count; }
};
void invalid_routes() {
  Spy spy;
  PageCardControlRouter<2> duplicate{{{{Kind::PERCENT, &spy}, {Kind::PERCENT, &spy}}}};
  PageCardControlRouter<1> empty{{{{Kind::NUMBER, nullptr}}}};
  PageCardControlRouter<1> basic{{{{Kind::PUSH, &spy}}}};
  PageCardControlRouter<1> partial{{{{Kind::NUMBER, &spy}}}};
  PageStore store;
  PageBindingToken origin;
  PageCardCommandValue value;
  for (PageCardControlPolicy* route :
       {static_cast<PageCardControlPolicy*>(&duplicate), static_cast<PageCardControlPolicy*>(&empty),
        static_cast<PageCardControlPolicy*>(&basic)}) {
    assert(!route->admit(store, origin, Kind::PERCENT, value) &&
           !route->valid(store, origin, Kind::PERCENT, value));
    route->discard(origin, Kind::PERCENT, value);
    route->accepted(origin, Kind::PERCENT, value);
  }
  assert(!duplicate.ready() && !empty.ready() && !basic.ready() && partial.ready());
  assert(!partial.admit(store, origin, Kind::PERCENT, value));
  assert(spy.admissions == 0 && spy.validations == 0 && spy.discards == 0 && spy.accepted_count == 0);
  const auto before = allocations;
  assert(partial.admit(store, origin, Kind::NUMBER, value) &&
         partial.valid(store, origin, Kind::NUMBER, value));
  partial.discard(origin, Kind::NUMBER, value);
  partial.accepted(origin, Kind::NUMBER, value);
  assert(allocations == before && spy.admissions == 1 && spy.validations == 1 && spy.discards == 1 &&
         spy.accepted_count == 1);
}
}  // namespace
int main() {
  invalid_routes();
  for (const auto& limits : DEVICE_UI_PROFILES) cycles(limits);
  rollback_and_replacement();
}
