#include "page_guest_wifi_runtime.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "page_card_dependencies.h"
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
void* operator new[](size_t n) {
  return ::operator new(n);
}
void operator delete(void* p) noexcept {
  std::free(p);
}
void operator delete[](void* p) noexcept {
  std::free(p);
}
void operator delete(void* p, size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, size_t) noexcept {
  std::free(p);
}
namespace {
uint32_t now = 100;
uint32_t clock_now() {
  return now;
}
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
  ~Arena() {
    assert(!live);
  }
};
struct Transport : PageEntitySubscriptions {
  struct Entry {
    const char* entity;
    const char* attribute;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::array<Entry, 128> entries{};
  size_t used{0};
  uint32_t epoch{1};
  uint32_t connection_generation() const override {
    return epoch;
  }
  bool subscribe(const char* e, const char* a, void* o, uint32_t r, uint32_t c, Deliver d) override {
    assert(used < entries.size());
    entries[used++] = {e, a, o, r, c, d};
    return true;
  }
  void release(void* o) override {
    for (auto& e : entries)
      if (e.owner == o) e.owner = nullptr;
  }
  void send(std::string_view value) {
    const auto before = allocations;
    size_t delivered = 0;
    for (size_t i = 0; i < used; ++i) {
      auto e = entries[i];
      if (e.owner && !std::strcmp(e.entity, "switch.guest_wifi") && !e.attribute[0]) {
        e.deliver(e.owner, e.revision, e.channel, value);
        ++delivered;
      }
    }
    assert(delivered && allocations == before);
  }
};
struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> memory{};
  PageStore store;
  size_t size{0};
  explicit Document(const DeviceUiLimits& limits, uint32_t generation = 17) {
    PanelConfigWriter w(bytes.data(), bytes.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {
        "switch.guest_wifi;Connect;Auto;Auto;;;wifi_qr;;ssid64=R3Vlc3Q,security=open,wifi_tabs=guest|qr",
        "switch.guest_wifi;Connect;Auto;Auto;;;wifi_qr;;ssid64=R3Vlc3Q,security=open,wifi_tabs=qr",
        ";Rooms;;;indicator;;subpage;;"};
    for (size_t i = 0; i < 3; ++i)
      assert(w.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child = "~B,1|wifi_qr,switch.guest_wifi,Connect,Auto,Auto,,,,wifi_tabs=guest";
    assert(w.append_subpage(3, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    assert(w.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, generation, memory.data(), memory.size(),
                               &store) == PageStoreStatus::OK);
  }
};
struct Fixture {
  Document doc;
  Arena arena;
  Transport transport;
  PageCardDependencies deps;
  PageEntityStateRuntime graph{deps, arena, transport, {256, 128, 32768}};
  struct ModalPort : PageModalPort {
    Fixture& h;
    std::string entity;
    PageModalToken opening;
    bool shown{false}, used{false};
    size_t creates{0}, destroys{0};
    explicit ModalPort(Fixture& f) : h(f) {}
    bool valid_origin(PageBindingToken t) const override {
      return t.binding != 0;
    }
    bool ready() const override {
      return true;
    }
    bool connected() const override {
      return true;
    }  // Local QR remains usable offline.
    uint32_t connection_generation() const override {
      return 0;
    }
    ControlModalKind kind(const PageStore& s, ConfigurationHandle c) const override {
      return s.card(c) && s.field(*s.card(c), CardField::TYPE) == "wifi_qr" ? ControlModalKind::WIFI_QR
                                                                            : ControlModalKind::NONE;
    }
    void* create(const PageStore& s, ConfigurationHandle c, PageModalToken t) override {
      assert(!used);
      used = true;
      ++creates;
      opening = t;
      entity = s.field(*s.card(c), CardField::ENTITY);
      return this;
    }
    bool initialize(void*, PageModalToken t) override {
      const auto count = allocations;
      const bool bound = h.guest.bind(t, entity);
      assert(count == allocations);
      shown = true;
      return bound;
    }
    bool visible(void* p) const override {
      return p == this && used && shown;
    }
    void update(void*, PageModalToken) override {}
    ControlModalDismissPolicy dismiss_policy(void*) const override {
      return ControlModalDismissPolicy::DISMISS;
    }
    void retire(void*, PageModalClose) override {
      shown = false;
      h.guest.unbind(opening);
    }
    void destroy(void*) override {
      assert(used && !shown);
      used = false;
      entity.clear();
      ++destroys;
    }
  } modal_port;
  PageModalRuntime modal;
  struct GuestPort : PageGuestWifiPort {
    Fixture& h;
    bool online{true}, services{true}, safe{true}, accept{true};
    size_t sends{0};
    bool target{false};
    std::function<void()> on_send;
    explicit GuestPort(Fixture& f) : h(f) {}
    bool valid(PageModalToken t) const override {
      return h.modal.valid(t);
    }
    bool connected() const override {
      return online;
    }
    bool service_available() const override {
      return services;
    }
    bool ready() const override {
      return safe;
    }
    bool send(std::string_view entity, bool on) override {
      ++sends;
      target = on;
      if (on_send) on_send();
      assert(entity == "switch.guest_wifi");
      return accept;
    }
  } guest_port;
  PageGuestWifiRuntime guest;
  explicit Fixture(const DeviceUiLimits& limits)
      : doc(limits),
        modal_port(*this),
        modal(modal_port),
        guest_port(*this),
        guest(graph, guest_port, clock_now) {
    now = 100;
    assert(graph.prepare(doc.store) && modal.prepare(doc.store));
    graph.activate_prepared(doc.store);
    modal.activate_prepared(doc.store);
  }
  ~Fixture() {
    modal.retire_active();
    modal.loop();
    guest.retire();
    graph.retire_active();
    assert(modal_port.creates == modal_port.destroys);
  }
  PageModalToken open(CardId card = 0) {
    assert(modal.request_card({doc.store.generation(), card}));
    modal.loop();
    return modal.token();
  }
  void close() {
    const auto before = allocations;
    modal.dismiss();
    assert(before == allocations);
    modal.loop();
  }
  void report(std::string_view value) {
    transport.send(value);
    guest.loop();
  }
};
void lifecycle(const DeviceUiLimits& limits) {
  Fixture f(limits);
  f.transport.send("off");
  auto token = f.open();
  auto state = f.guest.snapshot(token);
  assert(state.bound && state.known && !state.on && !state.pending);
  const auto channels = f.transport.used;
  for (size_t n = 0; n < 1000; ++n) {
    const auto count = allocations;
    assert(f.guest.queue(token, f.transport.epoch));
    assert(count == allocations);
    assert(!f.guest.queue(token, f.transport.epoch));
    assert(f.guest_port.sends == n && f.guest.loop() == PageGuestWifiDispatch::SENT);
    state = f.guest.snapshot(token);
    assert(state.pending && f.guest_port.target == !state.on);
    f.report(state.target_on ? "on" : "off");
    assert(!f.guest.snapshot(token).pending);
    f.close();
    assert(!f.guest.snapshot(token).bound);
    token = f.open();
    assert(token.opening);
  }
  assert(f.transport.used == channels);
  f.close();
  token = f.open(f.doc.store.page(1)->first_card);
  assert(token.opening && f.guest.snapshot(token).bound);
  f.close();
  f.open(1);
  assert(!f.modal.token().opening);  // No guest dependency on this static QR card.
}
void timeout_connection_and_guards() {
  Fixture f(DEVICE_UI_PROFILES[8]);
  auto token = f.open();
  assert(!f.guest.queue(token, 1));
  f.report("off");
  auto wrong = token;
  ++wrong.opening;
  assert(!f.guest.queue(wrong, 1));
  wrong = token;
  wrong.kind = ControlModalKind::IMAGE_CARD;
  assert(!f.guest.bind(wrong, "switch.guest_wifi"));
  assert(!f.guest.bind(token, "wifi.panel"));
  assert(f.guest.queue(token, 1));
  assert(f.guest.loop() == PageGuestWifiDispatch::SENT);
  f.report("off");
  assert(f.guest.snapshot(token).pending);
  now += 9999;
  f.guest.loop();
  assert(f.guest.snapshot(token).pending);
  ++now;
  f.guest.loop();
  assert(f.guest.snapshot(token).failed && !f.guest.snapshot(token).pending);
  f.report("on");
  assert(!f.guest.snapshot(token).failed);
  now = UINT32_MAX - 5000;
  assert(f.guest.queue(token, 1));
  f.guest.loop();
  now += 10000;
  f.guest.loop();
  assert(f.guest.snapshot(token).failed);
  assert(f.guest.queue(token, 1));
  f.guest_port.online = false;
  f.guest.loop();
  assert(f.modal.interactive(token) && f.guest.snapshot(token).bound && !f.guest.snapshot(token).known);
  const auto revision = f.guest.revision();
  for (int n = 0; n < 1000; ++n) f.guest.loop();
  assert(f.guest.revision() == revision);
  f.guest_port.online = true;
  ++f.transport.epoch;
  f.guest.loop();
  assert(!f.guest.snapshot(token).known);
  f.report("off");
  assert(!f.guest.queue(token, 1));
  assert(f.guest.queue(token, 2));
  f.guest_port.services = false;
  assert(f.guest.loop() == PageGuestWifiDispatch::FAILED && f.guest.snapshot(token).failed);
  f.guest_port.services = true;
  assert(f.guest.queue(token, 2));
  f.guest_port.accept = false;
  assert(f.guest.loop() == PageGuestWifiDispatch::FAILED && f.guest.snapshot(token).failed);
}
void deferred_and_exhausted() {
  Fixture f(DEVICE_UI_PROFILES[8]);
  f.transport.send("off");
  const auto token = f.open();
  assert(f.guest.queue(token, 1));
  f.guest_port.safe = false;
  assert(f.guest.loop() == PageGuestWifiDispatch::NONE && f.guest_port.sends == 0);
  f.transport.send("on");
  assert(f.guest.loop() == PageGuestWifiDispatch::CANCELLED && f.guest.snapshot(token).on);
  f.guest_port.safe = true;
  assert(f.guest.queue(token, 1));
  std::string overflow(32769, 'x');
  f.transport.send(overflow);
  assert(f.guest.loop() == PageGuestWifiDispatch::CANCELLED && !f.guest.snapshot(token).known &&
         f.guest_port.sends == 0);
  assert(!f.guest.queue(token, 1));
  f.report("off");
  assert(f.guest.queue(token, 1));
  f.guest_port.on_send = [&] {
    f.close();
    f.open();
    f.transport.send("on");
    assert(f.guest.loop() == PageGuestWifiDispatch::NONE);
  };
  f.guest_port.accept = false;
  assert(f.guest.loop() == PageGuestWifiDispatch::FAILED);
  f.guest_port.on_send = nullptr;
  f.guest.loop();
  assert(f.guest.snapshot(f.modal.token()).on && !f.guest.snapshot(f.modal.token()).failed);
}
void acknowledgement_during_failed_send() {
  Fixture f(DEVICE_UI_PROFILES[8]);
  f.transport.send("off");
  const auto token = f.open();
  assert(f.guest.queue(token, 1));
  f.guest_port.on_send = [&] { f.report("on"); };
  f.guest_port.accept = false;
  assert(f.guest.loop() == PageGuestWifiDispatch::FAILED);
  const auto state = f.guest.snapshot(token);
  assert(state.known && state.on && !state.pending && !state.failed && std::strcmp(state.status, "on") == 0);
  f.guest_port.on_send = nullptr;
}
void reentrant_same_opening_intent() {
  Fixture f(DEVICE_UI_PROFILES[8]);
  f.transport.send("off");
  const auto token = f.open();
  assert(f.guest.queue(token, 1));
  f.guest_port.on_send = [&] {
    f.transport.send("on");
    assert(f.guest.queue(token, 1));
  };
  f.guest_port.accept = false;
  assert(f.guest.loop() == PageGuestWifiDispatch::FAILED);
  f.guest_port.on_send = nullptr;
  assert(f.guest.snapshot(token).pending && !f.guest.snapshot(token).failed);
  f.guest_port.accept = true;
  assert(f.guest.loop() == PageGuestWifiDispatch::SENT && !f.guest_port.target);
  f.report("off");
  assert(!f.guest.snapshot(token).pending && !f.guest.snapshot(token).failed);
}
void final_intent_and_wait_timeout() {
  Fixture f(DEVICE_UI_PROFILES[8]);
  f.transport.send("off");
  const auto token = f.open();
  f.guest.retire();
  PageGuestWifiRuntime last(f.graph, f.guest_port, clock_now, UINT64_MAX);
  assert(last.bind(token, f.modal_port.entity) && last.queue(token, 1));
  assert(last.loop() == PageGuestWifiDispatch::SENT);
  f.transport.send("on");
  last.loop();
  assert(!last.queue(token, 1) && last.snapshot(token).on && !last.snapshot(token).pending);
  last.unbind(token);
  assert(f.guest.bind(token, f.modal_port.entity));
  f.guest_port.safe = false;
  assert(f.guest.queue(token, 1));
  now += 10000;
  assert(f.guest.loop() == PageGuestWifiDispatch::CANCELLED && f.guest.snapshot(token).failed &&
         f.guest.snapshot(token).known);
}
void replacement() {
  Fixture f(DEVICE_UI_PROFILES[8]);
  f.transport.send("off");
  auto token = f.open();
  assert(f.guest.queue(token, 1));
  Document staged(DEVICE_UI_PROFILES[8], UINT32_MAX);
  assert(f.graph.prepare(staged.store) && f.modal.prepare(staged.store));
  f.graph.discard_prepared();
  f.modal.discard_prepared();
  assert(f.guest.snapshot(token).pending);
  assert(f.graph.prepare(staged.store) && f.modal.prepare(staged.store));
  PageStore published;
  assert(PageStore::stage_v1(staged.bytes.data(), staged.size, DEVICE_UI_PROFILES[8], 0, staged.memory.data(),
                             staged.memory.size(), &published) == PageStoreStatus::OK);
  f.guest_port.on_send = [&] {
    const auto count = allocations;
    f.modal.activate_prepared(published);
    f.graph.activate_prepared(published);
    assert(count == allocations);
    std::memset(f.doc.memory.data(), 0xa5, f.doc.memory.size());
    f.modal.loop();
    assert(f.modal.request_card({0, 0}));
    f.modal.loop();
    f.transport.send("off");
  };
  f.guest_port.accept = false;
  assert(f.guest.loop() == PageGuestWifiDispatch::FAILED);
  f.guest_port.on_send = nullptr;
  f.guest.loop();
  assert(!f.guest.snapshot(token).bound && f.guest.snapshot(f.modal.token()).known &&
         !f.guest.snapshot(f.modal.token()).failed);
  f.modal.retire_active();
  f.modal.loop();
  f.graph.retire_active();
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) lifecycle(limits);
  timeout_connection_and_guards();
  deferred_and_exhausted();
  acknowledgement_during_failed_send();
  reentrant_same_opening_intent();
  final_intent_and_wait_timeout();
  replacement();
  std::puts(
      "guest Wi-Fi: 1000 demanded send/ack/reopen cycles per profile, offline QR lifetime, captured "
      "connection, deferred send and replacement passed");
}
