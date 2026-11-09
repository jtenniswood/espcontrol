#include "page_alarm_presentation.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "page_card_dependencies.h"
#include "page_entity_observer_set.h"
#include "page_runtime_resources_group.h"
#include "panel_config_document.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
static size_t allocations = 0;
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

uint32_t tick = 100;
uint32_t now() {
  return tick;
}
namespace {
struct Arena : PageArenaAllocator {
  size_t live{0}, calls{0};
  bool fail{false};
  void* allocate(size_t n) override {
    ++calls;
    if (fail) return nullptr;
    ++live;
    return std::malloc(n);
  }
  void release(void* p, size_t) override {
    assert(live);
    --live;
    std::free(p);
  }
};
struct Transport : PageEntitySubscriptions {
  struct Binding {
    const char* entity;
    const char* attribute;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::vector<Binding> bindings;
  uint32_t connection{1};
  uint32_t connection_generation() const override {
    return connection;
  }
  bool subscribe(const char* entity, const char* attribute, void* owner, uint32_t revision, uint32_t channel,
                 Deliver deliver) override {
    bindings.push_back({entity, attribute, owner, revision, channel, deliver});
    return true;
  }
  void release(void* owner) override {
    bindings.erase(
        std::remove_if(bindings.begin(), bindings.end(), [&](const auto& b) { return b.owner == owner; }),
        bindings.end());
  }
  void send(const char* entity, const char* attribute, std::string_view value) {
    for (const auto& b : bindings)
      if (std::strcmp(entity, b.entity) == 0 && std::strcmp(attribute, b.attribute) == 0)
        b.deliver(b.owner, b.revision, b.channel, value);
  }
};
struct Fixture {
  std::array<uint8_t, 16384> document{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> storage{};
  size_t size{0};
  PageStore store;
  Fixture(uint32_t generation, const char* entity = "alarm_control_panel.home",
          const DeviceUiLimits& limits = DEVICE_UI_PROFILES[8], bool empty = false,
          const std::string& options = "", const std::string& action_mode = "away") {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        std::string(entity) + ";;Auto;Auto;;;alarm;;" + options,
        std::string(entity) + ";;Auto;Auto;" + action_mode + ";;alarm_action;;" + options,
        std::string(entity) + ";Alarms;Auto;Auto;indicator;;subpage;;subpage_kind=alarm"};
    for (size_t i = 0; i < 3; ++i) {
      const auto card = empty ? std::string(";;;;sensor.empty;;sensor;1;") : cards[i];
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) ==
             PanelConfigStatus::OK);
    }
    const std::string child = empty ? "~B,1|S,,Value,Auto,Auto,sensor.empty,,1,"
                                    : "~B,1,2|Y,,,Auto,Auto,,,,|AA,,,Auto,Auto,home,,,";
    assert(writer.append_subpage(3, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, generation, storage.data(), storage.size(),
                               &store) == PageStoreStatus::OK);
  }
};

void only_control(Fixture& f, const DeviceUiLimits& limits) {
  PanelConfigWriter writer(f.document.data(), f.document.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                      std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  const char* card = "alarm_control_panel.home;;Auto;Auto;;;alarm;;";
  assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(card), std::strlen(card)) ==
         PanelConfigStatus::OK);
  assert(writer.finish(&f.size) == PanelConfigStatus::OK);
}
struct ModalPort : PageModalPort {
  PageAlarmStateRuntime& state;
  Transport& transport;
  PageModalToken context;
  bool live{false}, shown{false}, busy{false}, connected_value{true}, fail_create{false}, pin{false};
  size_t creates{0}, destroys{0};
  ModalPort(PageAlarmStateRuntime& s, Transport& t) : state(s), transport(t) {}
  bool valid_origin(PageBindingToken) const override {
    return true;
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle handle) const override {
    const auto source = PageAlarmStateRuntime::classify(store, handle.index);
    return source == PageAlarmSourceKind::CONTROL  ? ControlModalKind::ALARM_CONTROL
           : source == PageAlarmSourceKind::ACTION ? ControlModalKind::ALARM_PIN
                                                   : ControlModalKind::NONE;
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle handle,
                        ControlModalKind requested) const override {
    if (requested == ControlModalKind::ALARM_CONTROL) {
      const auto selected = state.takeover();
      if (selected.valid && selected.origin.generation == handle.generation &&
          selected.origin.index == handle.index)
        return requested;
    }
    return PageModalPort::kind(store, handle, requested);
  }
  bool ready() const override {
    return !busy;
  }
  bool connected() const override {
    return connected_value;
  }
  uint32_t connection_generation() const override {
    return transport.connection;
  }
  void* create(const PageStore&, ConfigurationHandle, PageModalToken token) override {
    assert(!live);
    ++creates;
    if (fail_create) return nullptr;
    live = true;
    context = token;
    return &context;
  }
  bool initialize(void*, PageModalToken) override {
    shown = true;
    pin = false;
    return true;
  }
  bool visible(void*) const override {
    return shown;
  }
  void update(void*, PageModalToken) override {}
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return pin ? ControlModalDismissPolicy::DISMISS
               : ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER;
  }
  uint64_t source_revision(void*) const override {
    return state.takeover_events();
  }
  void retire(void*, PageModalClose) override {
    shown = false;
  }
  void destroy(void*) override {
    assert(live && !shown);
    live = false;
    ++destroys;
  }
};
struct PresentationPort : PageAlarmPresentationPort {
  ModalPort& modal;
  Transport& transport;
  bool safe{true}, critical_value{false};
  size_t starts{0}, ends{0}, failures{0};
  PageAlarmFailure last;
  std::function<void()> during_failure;
  PresentationPort(ModalPort& m, Transport& t) : modal(m), transport(t) {}
  bool ready() const override {
    return safe;
  }
  bool connected() const override {
    return modal.connected_value;
  }
  uint32_t connection_generation() const override {
    return transport.connection;
  }
  void critical(bool value) override {
    assert(safe && value != critical_value);
    critical_value = value;
    if (value)
      ++starts;
    else
      ++ends;
  }
  void failure(PageAlarmFailure event) override {
    assert(safe);
    ++failures;
    last = event;
    if (during_failure) during_failure();
  }
};
struct Harness {
  Arena arena;
  Transport transport;
  PageCardDependencies deps;
  PageAlarmStateRuntime state{arena, transport, now};
  PageAlarmAckRuntime ack{arena, transport, state, now};
  PageEntityObserverSet<2> observers;
  PageEntityStateRuntime graph{deps, arena, transport, {128, 128, 8192}, &observers};
  ModalPort port{state, transport};
  PageModalRuntime modal{port};
  PresentationPort display{port, transport};
  PageAlarmPresentation presentation{state, ack, modal, display};
  PageRuntimeResourcesGroup<5> resources{{&presentation, &modal, &ack, &state, &graph}};
  PageStoreRuntime publication;
  Harness(const DeviceUiLimits& limits) : publication(limits, arena, 32768, &resources) {
    assert(observers.add(state) && observers.add(ack));
  }
  void publish(Fixture& f, uint32_t gen = 0) {
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, f.document.data(), f.size));
    assert(publication.activate_prepared(gen, PANEL_CONFIG_DOCUMENT_VERSION, f.document.data(), f.size));
  }
  void report(std::string_view value) {
    transport.send("alarm_control_panel.home", "", value);
  }
  void frame() {
    presentation.loop();
    modal.loop();
    presentation.loop();
  }
};
void closing_and_report_events(const DeviceUiLimits& limits) {
  Fixture f(17, "alarm_control_panel.home", limits);
  only_control(f, limits);
  Harness h(limits);
  h.publish(f);
  h.report("arming");
  h.transport.send("alarm_control_panel.home", "delay", "60");
  h.frame();
  assert(h.display.critical_value && h.modal.token().card == 0 && h.port.creates == 1);
  h.modal.dismiss();
  h.modal.loop();
  h.presentation.loop();
  assert(!h.display.critical_value && h.presentation.memory().suppressed);
  const auto events = h.state.takeover_events();
  for (int i = 0; i < 10; ++i) {
    tick += 1000;
    h.frame();
  }
  assert(h.port.creates == 1 && h.state.takeover_events() == events);
  h.transport.send("alarm_control_panel.home", "arm_mode", "home");
  h.frame();
  assert(h.port.creates == 1 && !h.display.critical_value);
  h.report("arming");
  h.frame();
  assert(h.port.creates == 2 && h.display.critical_value);
  h.report("disarmed");
  h.frame();
  assert(!h.port.live && !h.display.critical_value);
  // A user can close the auto-opened panel before presentation observes it.
  h.report("triggered");
  h.presentation.loop();
  const auto pending = h.modal.pending_request();
  h.modal.loop();
  assert(h.modal.token().opening == pending);
  h.modal.dismiss();
  h.modal.loop();
  h.presentation.loop();
  assert(!h.display.critical_value && h.presentation.memory().suppressed);
  h.report("triggered");
  h.frame();
  assert(h.display.critical_value && h.port.live);
}
void input_report_order(const DeviceUiLimits& limits) {
  Fixture f(17, "alarm_control_panel.home", limits);
  only_control(f, limits);
  Harness h(limits);
  h.publish(f);
  h.report("triggered");
  h.frame();
  h.report("triggered");
  h.modal.dismiss();
  h.modal.loop();
  h.presentation.loop();
  assert(!h.display.critical_value && h.presentation.memory().suppressed);
  const auto attempts = h.port.creates;
  h.frame();
  assert(h.port.creates == attempts);
  h.report("triggered");
  h.frame();
  h.modal.dismiss();
  h.modal.loop();
  h.report("triggered");
  h.frame();
  assert(h.display.critical_value && h.port.live);
  h.port.pin = true;
  h.modal.dismiss();
  h.modal.loop();
  h.presentation.loop();
  assert(h.display.critical_value && !h.port.live);
  const auto pin_attempts = h.port.creates;
  h.frame();
  assert(h.port.creates == pin_attempts);
  h.report("triggered");
  h.frame();
  assert(h.port.live && h.display.critical_value);
}
void pending_manual_and_expiry(const DeviceUiLimits& limits) {
  Fixture f(17, "alarm_control_panel.home", limits);
  only_control(f, limits);
  Harness h(limits);
  h.publish(f);
  h.report("disarmed");
  assert(h.modal.request_card({0, 0}));
  h.modal.loop();
  auto manual = h.modal.token();
  h.report("triggered");
  h.frame();
  assert(h.modal.token().opening == manual.opening && h.display.critical_value);
  h.report("disarmed");
  h.frame();
  assert(h.modal.valid(manual) && !h.display.critical_value);
  h.modal.dismiss();
  h.modal.loop();
  h.report("triggered");
  h.presentation.loop();
  assert(h.modal.pending_request());
  h.report("disarmed");
  h.presentation.loop();
  h.modal.loop();
  assert(!h.port.live);
  h.report("triggered");
  h.presentation.loop();
  const auto old = h.modal.pending_request();
  assert(h.modal.request_card({0, 0}));
  const auto newer = h.modal.pending_request();
  assert(newer != old);
  h.report("disarmed");
  h.presentation.loop();
  assert(h.modal.pending_request() == newer);
  h.modal.loop();
  assert(h.port.live);
  h.modal.dismiss();
  h.modal.loop();
  // Expiry before presentation observes an opening still closes the auto panel.
  h.report("triggered");
  h.presentation.loop();
  h.modal.loop();
  h.report("disarmed");
  h.presentation.loop();
  h.modal.loop();
  assert(!h.port.live);
  h.report("arming");
  h.transport.send("alarm_control_panel.home", "delay", "1");
  h.frame();
  h.port.pin = true;
  const auto pin = h.modal.token();
  tick += 1000;
  h.frame();
  assert(h.modal.valid(pin) && !h.display.critical_value);
  h.modal.dismiss();
  h.modal.loop();
}
void hidden_reconnect_and_failed_setup(const DeviceUiLimits& limits) {
  Fixture f(17, "alarm_control_panel.home", limits);
  Harness h(limits);
  h.publish(f);
  h.report("triggered");
  h.frame();
  const auto selected = h.state.takeover();
  assert(selected.valid && selected.origin.index > 2 && h.modal.token().card == selected.origin.index &&
         h.modal.token().kind == ControlModalKind::ALARM_CONTROL);
  ++h.transport.connection;
  h.port.connected_value = false;
  h.frame();
  assert(!h.port.live && h.display.critical_value);
  h.port.connected_value = true;
  h.frame();
  assert(h.port.live && h.display.critical_value);
  h.modal.dismiss();
  h.modal.loop();
  h.presentation.loop();
  assert(!h.display.critical_value);
  h.transport.send("alarm_control_panel.home", "arm_mode", "away");
  h.frame();
  assert(h.display.critical_value && h.port.live);  // Action reports refresh critical routing.
  h.report("disarmed");
  h.frame();
  h.port.fail_create = true;
  h.report("triggered");
  h.frame();
  const auto attempts = h.port.creates;
  for (int n = 0; n < 1000; ++n) h.frame();
  assert(h.port.creates == attempts && h.display.critical_value);
  h.port.fail_create = false;
  h.report("triggered");
  h.frame();
  assert(h.port.live);
}
void feedback_and_replacement(const DeviceUiLimits& limits) {
  Fixture f(17, "alarm_control_panel.home", limits), next(18, "alarm_control_panel.home", limits);
  Harness h(limits);
  h.publish(f);
  h.report("disarmed");
  assert(h.ack.accepted({0, 0}, h.transport.connection, alarm::Action::AWAY, true));
  tick += 3500;
  h.display.safe = false;
  h.presentation.loop();
  assert(h.display.failures == 0);
  h.display.safe = true;
  h.presentation.loop();
  assert(h.display.failures == 1 && h.display.last.reason == PageAlarmFailureReason::CODE);
  assert(h.ack.accepted({0, 0}, h.transport.connection, alarm::Action::AWAY, false));
  tick += 3500;
  h.ack.loop();
  ++h.transport.connection;
  h.frame();
  assert(h.display.failures == 1);
  h.report("disarmed");
  assert(h.ack.accepted({0, 0}, h.transport.connection, alarm::Action::AWAY, false));
  tick += 3500;
  h.display.during_failure = [&]() {
    h.publish(next, 1);
    h.presentation.loop();
  };
  h.presentation.loop();
  assert(h.display.failures == 2);
  h.display.during_failure = {};
  h.report("triggered");
  h.presentation.loop();
  assert(h.modal.pending_request());
  const auto effects = h.display.starts + h.display.ends;
  h.publish(f, 2);
  assert(h.display.starts + h.display.ends == effects);
  h.frame();
  assert(h.modal.token().configuration == 2 && h.display.critical_value);
  Fixture empty(17, "alarm_control_panel.home", limits, true);
  h.publish(empty, 3);
  assert(h.display.critical_value);
  h.frame();
  assert(!h.display.critical_value && !h.port.live);
}
void bounded_cycles(const DeviceUiLimits& limits) {
  Fixture f(17, "alarm_control_panel.home", limits);
  only_control(f, limits);
  Harness h(limits);
  h.publish(f);
  h.report("disarmed");
  const auto before = allocations, live = h.arena.live, channels = h.transport.bindings.size();
  for (int n = 0; n < 1000; ++n) {
    h.report("triggered");
    h.frame();
    assert(h.port.live && h.display.critical_value);
    h.modal.dismiss();
    h.modal.loop();
    h.presentation.loop();
    assert(!h.display.critical_value && h.arena.live == live && h.transport.bindings.size() == channels &&
           allocations == before);
  }
  assert(h.port.creates == h.port.destroys && h.display.starts == 1000 && h.display.ends == 1000);
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) {
    closing_and_report_events(limits);
    pending_manual_and_expiry(limits);
    input_report_order(limits);
    hidden_reconnect_and_failed_setup(limits);
    feedback_and_replacement(limits);
    bounded_cycles(limits);
  }
  std::puts(
      "alarm presentation: nine profiles, hidden critical routing, close/PIN/report semantics and 1000 "
      "allocation-free cycles passed");
}
