#include "page_confirmation_runtime.h"

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
    const char* attr;
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
  void release(void* owner) override {
    for (auto& e : entries)
      if (e.owner == owner) e.owner = nullptr;
  }
  void report(std::string_view entity, std::string_view value) {
    const auto before = allocations;
    for (size_t i = 0; i < used; ++i) {
      const auto e = entries[i];
      if (e.owner && entity == e.entity && !e.attr[0]) e.deliver(e.owner, e.revision, e.channel, value);
    }
    assert(before == allocations);
  }
};
struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> memory{};
  PageStore store;
  size_t size{0};
  explicit Document(const DeviceUiLimits& limits, uint32_t generation = 17) {
    PanelConfigWriter writer(bytes.data(), bytes.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {
        "switch.test;Toggle;Auto;Auto;;;;;confirm_on,confirm_off,confirm_message=Continue%3F",
        "cover.door;Open;Auto;Auto;open;;garage;;confirm_on",
        "script.test;Script;Auto;Auto;script.turn_on;;action;;confirm_on,script_fields=message%3D" +
            std::string(140, 's'),
        "cover.door;Garage;Auto;Auto;;;garage;;confirm_on,confirm_off",
        "button.test;Button;Auto;Auto;;;;;confirm_on,confirm_off",
        ";Rooms;;;indicator;;subpage;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child = "~B,1|,switch.test,Child,Auto,Auto,,,,confirm_on%2Cconfirm_off";
    assert(writer.append_subpage(6, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
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
  uint32_t binding{1};
  bool online{true}, busy{false}, service{true}, accepted{true};
  size_t sends{0}, closes{0};
  PageConfirmationAction action{PageConfirmationAction::NONE};
  std::string sent_entity, sent_options;
  std::function<void()> on_send;
  std::function<void(ParsedCfg&)> on_create;
  struct ModalPort : PageModalPort {
    Fixture& f;
    struct Context {
      ParsedCfg cfg;
      PageModalToken token;
      bool retired{false};
    };
    Context* context{nullptr};
    explicit ModalPort(Fixture& f) : f(f) {}
    bool valid_origin(PageBindingToken t) const override {
      return t.binding == f.binding && !t.back && t.configuration == f.doc.store.generation();
    }
    ControlModalKind kind(const PageStore& s, ConfigurationHandle h) const override {
      return page_confirmation_policy(s, h).enabled() ? ControlModalKind::SWITCH_CONFIRMATION
                                                      : ControlModalKind::NONE;
    }
    bool ready() const override {
      return !f.busy;
    }
    bool connected() const override {
      return f.online;
    }
    uint32_t connection_generation() const override {
      return f.transport.epoch;
    }
    void* create(const PageStore& s, ConfigurationHandle h, PageModalToken t) override {
      assert(!context);
      context = new Context;
      context->token = t;
      auto& p = context->cfg;
      const auto* row = s.card(h);
      assert(row);
      p.entity = s.field(*row, CardField::ENTITY);
      p.sensor = s.field(*row, CardField::SENSOR);
      p.type = s.field(*row, CardField::TYPE);
      p.unit = s.field(*row, CardField::UNIT);
      p.options = s.field(*row, CardField::OPTIONS);
      p.label = s.field(*row, CardField::LABEL);
      if (f.on_create) f.on_create(p);
      assert(f.confirm.plan(t).action != PageConfirmationAction::NONE);
      return context;
    }
    bool initialize(void* c, PageModalToken t) override {
      return f.confirm.bind(t, static_cast<Context*>(c)->cfg);
    }
    bool visible(void* c) const override {
      return !static_cast<Context*>(c)->retired;
    }
    void update(void*, PageModalToken) override {}
    ControlModalDismissPolicy dismiss_policy(void*) const override {
      return ControlModalDismissPolicy::DISMISS;
    }
    void retire(void* c, PageModalClose) override {
      auto& ctx = *static_cast<Context*>(c);
      ctx.retired = true;
      f.confirm.cancel(ctx.token);
    }
    void destroy(void* c) override {
      assert(c == context && context->retired);
      delete context;
      context = nullptr;
    }
  } modal_port{*this};
  PageModalRuntime modal;
  struct Port : PageConfirmationPort {
    Fixture& f;
    explicit Port(Fixture& f) : f(f) {}
    bool valid(PageBindingToken t) const override {
      return f.modal_port.valid_origin(t);
    }
    bool valid(PageModalToken t) const override {
      return f.modal.valid(t);
    }
    bool connected() const override {
      return f.online;
    }
    bool service_available() const override {
      return f.service;
    }
    bool ready() const override {
      return !f.busy;
    }
    uint64_t request(PageBindingToken t) override {
      return f.modal.request(t) ? f.modal.pending_request() : 0;
    }
    bool pending_request(uint64_t request) const override {
      return request && f.modal.pending_request() == request;
    }
    void cancel_request(uint64_t request) override {
      f.modal.cancel_request(request);
    }
    void close(PageModalToken token) override {
      ++f.closes;
      f.modal.notify_closed(token);
    }
    bool send(PageConfirmationAction action, const ParsedCfg& p) override {
      ++f.sends;
      f.action = action;
      f.sent_entity = p.entity;
      f.sent_options = p.options;
      const std::string saved_entity = p.entity, saved_options = p.options;
      if (f.on_send) f.on_send();
      assert(saved_entity == p.entity && saved_options == p.options);
      return f.accepted;
    }
  } port{*this};
  PageConfirmationRuntime confirm{graph, port};
  explicit Fixture(const DeviceUiLimits& limits, uint64_t opening = 1)
      : doc(limits), modal(modal_port, opening) {
    assert(graph.prepare(doc.store) && confirm.prepare(doc.store) && modal.prepare(doc.store));
    graph.activate_prepared(doc.store);
    confirm.activate_prepared(doc.store);
    modal.activate_prepared(doc.store);
    transport.report("switch.test", "off");
    transport.report("cover.door", "closed");
  }
  ~Fixture() {
    modal.retire_active();
    modal.loop();
    confirm.retire_active();
    graph.retire_active();
  }
  PageBindingToken origin(CardId card = 0, PageId page = 0) const {
    return {doc.store.generation(), binding, page, card, 0, false};
  }
  bool request(CardId card = 0, bool current = false, PageId page = 0) {
    const auto before = allocations;
    const bool accepted = confirm.request(origin(card, page), transport.epoch, current);
    assert(before == allocations);
    return accepted;
  }
  PageModalToken open(CardId card = 0, bool current = false, PageId page = 0) {
    assert(request(card, current, page));
    modal.loop();
    const auto token = modal.token();
    assert(modal.interactive(token));
    return token;
  }
  bool queue(PageModalToken t) {
    const auto before = allocations;
    const bool queued = confirm.queue(t, transport.epoch);
    assert(before == allocations);
    return queued;
  }
  void close() {
    modal.dismiss();
    modal.loop();
    assert(!modal_port.context);
  }
};
void policy_parity() {
  size_t cases = 0;
  for (const auto type : {"", "garage", "action", "light_switch", "fan_switch", "internal"})
    for (const auto entity : {"switch.test", "cover.test", "button.test", "button.", "input_button.test", ""})
      for (const auto mode : {"", "open", "close", "stop", "script.turn_on", "script.turn_off"})
        for (const auto options :
             {"", "confirm_on", "confirm_off", "confirm_on,confirm_off", "xconfirm_on,confirm_off=1",
              " confirm_on,confirm_off ", "confirm_on=1", "a,confirm_on,b"})
          for (bool current : {false, true}) {
            ParsedCfg p;
            p.type = type;
            p.entity = entity;
            p.sensor = mode;
            p.options = options;
            bool required = false;
            if (!p.entity.empty()) {
              if (p.type.empty() && !(p.entity.size() > 7 && p.entity.compare(0, 7, "button.") == 0))
                required = switch_confirmation_required(p, current);
              else if (p.type == "garage")
                required = garage_confirmation_required(
                    p, card_runtime_garage_command_mode(p.sensor) ? p.sensor == "open" : !current);
              else if (p.type == "action")
                required = action_script_confirmation_enabled(p);
            }
            const auto before = allocations;
            const auto policy = PageConfirmationPolicy::fields(type, entity, mode, options);
            assert((policy.choose(current) != PageConfirmationAction::NONE) == required &&
                   before == allocations);
            ++cases;
          }
  std::printf("Confirmation policy parity: %zu legacy cases\n", cases);
}
void basic(const DeviceUiLimits& profile) {
  Fixture f(profile);
  const auto registrations = f.transport.used;
  const auto arena = f.arena.live;
  for (int i = 0; i < 1000; ++i) {
    auto token = f.open();
    const auto before = f.sends;
    // New state cannot reverse the explicit direction chosen before the prompt.
    f.transport.report("switch.test", "on");
    assert(f.queue(token) && !f.queue(token));
    assert(f.confirm.loop() == PageConfirmationResult::SENT && f.sends == before + 1 &&
           f.action == PageConfirmationAction::TURN_ON);
    f.modal.loop();
    assert(!f.confirm.queue(token, f.transport.epoch));
    token = f.open(0, true);
    assert(f.queue(token));
    f.transport.report("switch.test", "off");
    assert(f.confirm.loop() == PageConfirmationResult::SENT && f.action == PageConfirmationAction::TURN_OFF);
    f.modal.loop();
    token = f.open(1);
    assert(f.queue(token));
    assert(f.confirm.loop() == PageConfirmationResult::SENT && f.action == PageConfirmationAction::OPEN);
    f.modal.loop();
    token = f.open(2);
    assert(f.queue(token));
    assert(f.confirm.loop() == PageConfirmationResult::SENT && f.action == PageConfirmationAction::SCRIPT);
    f.modal.loop();
    assert(f.sent_options.find(std::string(140, 's')) != std::string::npos);
    token = f.open(3, true);
    assert(f.queue(token));
    assert(f.confirm.loop() == PageConfirmationResult::SENT && f.action == PageConfirmationAction::CLOSE);
    f.modal.loop();
    assert(f.transport.used == registrations && f.arena.live == arena && !f.confirm.memory().contexts);
    f.transport.report("switch.test", "off");
  }
  assert(!f.request(4));
  const auto* child = f.doc.store.page(1);
  assert(child);
  auto token = f.open(child->first_card, false, 1);
  assert(f.queue(token));
  assert(f.confirm.loop() == PageConfirmationResult::SENT);
  f.modal.loop();
  std::printf("%s: 1,000 confirmation cycles across toggle, garage and script; stable owners/registrations\n",
              profile.device_profile);
}
void stale_and_cancel(const DeviceUiLimits& profile) {
  Fixture f(profile);
  auto token = f.open();
  assert(f.queue(token));
  const auto pressed = f.transport.epoch;
  ++f.transport.epoch;
  f.graph.invalidate_disconnected_state();
  assert(!f.confirm.queue(token, f.transport.epoch));
  assert(!f.confirm.queue(token, pressed) && f.confirm.loop() == PageConfirmationResult::CANCELLED &&
         f.sends == 0);
  f.modal.loop();
  assert(!f.request());  // Reconnect requires a fresh source report for a toggle.
  f.transport.report("switch.test", "off");
  assert(!f.confirm.request(f.origin(), pressed, false));
  token = f.open();
  assert(f.queue(token));
  f.close();
  assert(f.confirm.loop() == PageConfirmationResult::NONE && f.sends == 0);
  token = f.open();
  assert(f.queue(token));
  f.modal.display_takeover(false);
  f.modal.loop();
  assert(f.confirm.loop() == PageConfirmationResult::NONE && f.sends == 0);
  token = f.open();
  f.busy = true;
  assert(f.queue(token) && f.confirm.loop() == PageConfirmationResult::NONE && f.sends == 0);
  f.service = false;
  f.busy = false;
  assert(f.confirm.loop() == PageConfirmationResult::FAILED && f.sends == 0);
  f.modal.loop();
  f.service = true;
  token = f.open();
  assert(f.queue(token));
  f.transport.report("switch.test", "unavailable");
  assert(f.confirm.loop() == PageConfirmationResult::CANCELLED && f.sends == 0);
  f.modal.loop();
  f.transport.report("switch.test", "off");
  assert(f.request());
  ++f.binding;
  f.modal.loop();
  assert(!f.modal.token().opening);
  assert(f.confirm.loop() == PageConfirmationResult::NONE && !f.confirm.memory().pending_requests);
  f.confirm.cancel();
  assert(!f.confirm.memory().pending_requests);
  assert(f.request());
  f.confirm.cancel();
  f.modal.loop();
  assert(!f.modal.token().opening);
  token = f.open();
  auto bad = token;
  ++bad.opening;
  assert(!f.queue(bad));
  bad = token;
  bad.card = 1;
  assert(!f.queue(bad));
  bad = token;
  bad.kind = ControlModalKind::WIFI_QR;
  assert(!f.queue(bad));
  ParsedCfg wrong = f.modal_port.context->cfg;
  wrong.entity = "switch.other";
  assert(!f.confirm.bind(token, wrong));
  f.close();
  for (int field = 0; field < 5; ++field) {
    f.on_create = [field](ParsedCfg& c) {
      switch (field) {
        case 0:
          c.entity = "switch.other";
          break;
        case 1:
          c.options = "confirm_on";
          break;
        case 2:
          c.unit = "changed";
          break;
        case 3:
          c.sensor = "changed";
          break;
        default:
          c.type = "changed";
          break;
      }
    };
    assert(f.request());
    f.modal.loop();
    assert(!f.modal.token().opening && !f.confirm.memory().contexts);
  }
  f.on_create = {};
  f.online = false;
  assert(!f.request());
  f.online = true;
  f.accepted = false;
  token = f.open();
  assert(f.queue(token));
  assert(f.confirm.loop() == PageConfirmationResult::FAILED && f.sends == 1);
  f.modal.loop();
}
void reentrant_and_publication(const DeviceUiLimits& profile) {
  Fixture f(profile);
  auto token = f.open(2);
  assert(f.queue(token));
  PageModalToken newer;
  f.on_send = [&] {
    assert(f.confirm.loop() == PageConfirmationResult::NONE);
    f.close();
    newer = f.open(2);
    assert(f.queue(newer));
  };
  assert(f.confirm.loop() == PageConfirmationResult::SENT && f.modal.interactive(newer) &&
         f.confirm.pending(newer));
  f.on_send = {};
  assert(f.confirm.loop() == PageConfirmationResult::SENT);
  f.modal.loop();
  Document staged(profile, UINT32_MAX);
  alignas(std::max_align_t) std::array<uint8_t, 32768> committed_memory{};
  PageStore committed;
  assert(PageStore::stage_v1(staged.bytes.data(), staged.size, profile, 0, committed_memory.data(),
                             committed_memory.size(), &committed) == PageStoreStatus::OK);
  token = f.open();
  assert(f.queue(token));
  assert(f.confirm.prepare(staged.store));
  f.confirm.discard_prepared();
  assert(f.confirm.pending(token));
  assert(f.graph.prepare(staged.store) && f.modal.prepare(staged.store) && f.confirm.prepare(staged.store));
  f.busy = true;
  f.confirm.activate_prepared(committed);
  f.modal.activate_prepared(committed);
  f.graph.activate_prepared(committed);
  f.doc.memory.fill(0xDA);
  staged.memory.fill(0xDE);
  f.doc.store = committed;
  const auto sends = f.sends;
  assert(!f.queue(token));
  f.busy = false;
  f.modal.loop();
  assert(f.confirm.loop() == PageConfirmationResult::NONE && f.sends == sends);
  f.transport.report("switch.test", "off");
  token = f.open();
  assert(token.configuration == 0 && f.queue(token));
  f.on_send = [&] {
    f.confirm.retire_active();
    f.close();
    f.graph.retire_active();
  };
  assert(f.confirm.loop() == PageConfirmationResult::SENT && f.sends == sends + 1 &&
         !f.confirm.memory().contexts);
  f.on_send = {};
  Fixture last(profile, UINT64_MAX);
  token = last.open();
  assert(token.opening == UINT64_MAX && last.queue(token));
  assert(last.confirm.loop() == PageConfirmationResult::SENT);
  last.modal.loop();
  assert(!last.request());
}
}  // namespace
int main() {
  {
    Fixture f(DEVICE_UI_PROFILES[8]);const auto token=f.open();
    assert(f.queue(token));const auto sends=f.sends;
    f.confirm.cancel_queued();f.confirm.cancel_queued();
    assert(f.modal.interactive(token) && !f.confirm.pending(token) &&
      f.confirm.loop()==PageConfirmationResult::NONE && f.sends==sends);
    assert(f.queue(token) && f.confirm.loop()==PageConfirmationResult::SENT && f.sends==sends+1);
    f.modal.loop();
  }
  policy_parity();
  for (const auto& profile : DEVICE_UI_PROFILES) basic(profile);
  stale_and_cancel(DEVICE_UI_PROFILES[8]);
  reentrant_and_publication(DEVICE_UI_PROFILES[8]);
}
