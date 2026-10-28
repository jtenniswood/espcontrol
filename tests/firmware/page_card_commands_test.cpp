#include "page_card_commands.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

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
  std::string entity = "switch." + std::string(160, 's');
  Document(const DeviceUiLimits& limits, uint32_t generation = 0, bool confirmation = false,
           bool cover = false, bool retained = false) {
    if (cover) entity = "cover.door";
    PanelConfigWriter writer(bytes.data(), bytes.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {
        entity + ";Toggle;Auto;Auto;;;;;" + (confirmation ? "confirm_on" : ""),
        ";Rooms;Home;Auto;;;subpage;;",
        "script.test;Action;Auto;Auto;script.turn_on;;action;;script_fields=message%3D" +
            std::string(180, 'x'),
        "button.test;Button;Auto;Auto;;;;;",
        "relay;Relay;Auto;Auto;;;internal;;",
        ";;Gesture Tap;Auto;;;push;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    if (retained) {
      assert(writer.append_button(31, reinterpret_cast<const uint8_t*>(rows[0].data()), rows[0].size()) ==
             PanelConfigStatus::OK);
      assert(writer.append_button(32, reinterpret_cast<const uint8_t*>(rows[1].data()), rows[1].size()) ==
             PanelConfigStatus::OK);
    }
    const std::string children =
        "~B|P,,Push,,,,,,|A,local_key,Local,,,local,,,|WH,https://"
        "example.invalid,Hook,,,GET,,webhook,|SS,,,,,,,screensaver,|SL,,,,,,,screen_lock,";
    assert(writer.append_subpage(2, reinterpret_cast<const uint8_t*>(children.data()), children.size()) ==
           PanelConfigStatus::OK);
    if (retained)
      assert(writer.append_subpage(32, reinterpret_cast<const uint8_t*>(children.data()), children.size()) ==
             PanelConfigStatus::OK);
    size_t size = 0;
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, generation, memory.data(), memory.size(),
                               &store) == PageStoreStatus::OK);
  }
};
struct Port : PageCardCommandPort {
  uint32_t binding{1};
  bool busy{false}, online{true}, covered{false}, service{true};
  size_t dispatched{0};
  std::vector<PageCardCommandSnapshot> received;
  std::function<void(const PageCardCommandSnapshot&)> on_dispatch;
  bool ready() const override { return !busy; }
  bool valid(PageBindingToken t) const override { return t.binding == binding && !covered; }
  bool connected() const override { return online; }
  bool service_available() const override { return service; }
  void dispatch(const PageCardCommandSnapshot& s) override {
    ++dispatched;
    received.push_back(s);
    if (on_dispatch) on_dispatch(s);
  }
};
struct Harness {
  Document doc;
  Arena arena;
  Transport transport;
  PageCardDependencies deps;
  PageEntityStateRuntime graph{deps, arena, transport, {128, 128, 8192}};
  Port port;
  PageCardCommands commands{graph, port};
  explicit Harness(const DeviceUiLimits& limits, bool confirmation = false, bool cover = false, bool retained = false)
      : doc(limits, 0, confirmation, cover, retained) {
    assert(graph.prepare(doc.store) && commands.prepare(doc.store));
    graph.activate_prepared(doc.store);
    commands.activate_prepared(doc.store);
    transport.report(doc.entity, "on");
  }
  PageBindingToken token(CardId card, PageId page = 0) const {
    return {0, port.binding, page, card, 0, false};
  }
  bool request(CardId card, PageId page = 0) {
    const auto before = allocations;
    const bool accepted = commands.request(token(card, page), transport.epoch);
    assert(allocations == before);
    return accepted;
  }
};
void retained_definitions_do_not_dispatch(const DeviceUiLimits& limits) {
  Harness h(limits, false, false, true);
  assert(h.doc.store.page(0)->card_count == 32);
  assert(h.request(0));
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(h.port.received.size() == 1);
  assert(!h.request(30));  // Same entity and current binding as the working root card.
  const auto* page = h.doc.store.page(2);
  assert(page && !page->navigable && page->card_count);
  assert(!h.request(page->first_card, 2));
  assert(h.commands.memory().pending == 0 && h.port.received.size() == 1);
}
void cycles(const DeviceUiLimits& limits) {
  Harness h(limits);
  const auto subscriptions = h.transport.used, arenas = h.arena.live;
  for (size_t cycle = 0; cycle < 1000; ++cycle) {
    h.port.received.clear();
    for (CardId card : {0, 2, 3, 4, 5}) assert(h.request(card));
    assert(h.commands.memory().pending == 5);
    const auto before = h.port.dispatched;
    h.port.busy = true;
    assert(h.commands.loop() == PageCardCommandResult::NONE);
    assert(h.port.dispatched == before);
    h.port.busy = false;
    for (size_t i = 0; i < 5; ++i) assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
    assert(h.port.received.size() == 5 && h.commands.memory().pending == 0);
    const PageCardCommandKind expected[] = {PageCardCommandKind::TOGGLE, PageCardCommandKind::ACTION,
                                            PageCardCommandKind::BUTTON, PageCardCommandKind::RELAY,
                                            PageCardCommandKind::PUSH};
    for (size_t i = 0; i < 5; ++i) assert(h.port.received[i].kind == expected[i]);
    assert(h.port.received[0].config.entity == h.doc.entity);
    assert(h.port.received[1].config.options.size() > 180);
    assert(h.port.received[4].root && h.port.received[4].slot == 6);
    ++h.port.binding;
    assert(h.transport.used == subscriptions && h.arena.live == arenas);
  }
  h.port.online = false;
  h.port.service = false;
  ++h.transport.epoch;
  assert(!h.request(0) && h.request(4));
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  // Classify actual decoded child records; all local actions remain independent of HA.
  const auto* child = h.doc.store.page(1);
  assert(child && child->navigable);
  bool saw_local = false, saw_webhook = false, saw_screensaver = false;
  for (CardId id = child->first_card; id < child->first_card + child->card_count; ++id) {
    const auto kind = PageCardCommands::classify(h.doc.store, *h.doc.store.card(id));
    if (PageCardCommands::local(kind)) {
      assert(h.request(id, 1));
      assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
      const auto& s = h.port.received.back();
      assert(!s.root);
      saw_local |= kind == PageCardCommandKind::LOCAL;
      saw_webhook |= kind == PageCardCommandKind::WEBHOOK;
      saw_screensaver |= kind == PageCardCommandKind::SCREENSAVER;
    }
  }
  assert(saw_local && saw_webhook && saw_screensaver);
  h.port.online = h.port.service = true;
  for (size_t i = 0; i < 8; ++i) assert(h.request(5));
  assert(!h.request(5) && h.commands.memory().pending == 8);
  h.commands.cancel();
  assert(h.commands.memory().pending == 0);
  assert(h.request(0));
  ++h.transport.epoch;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.request(5));
  ++h.port.binding;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.request(5));
  h.port.covered = true;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.covered = false;
  assert(h.request(5));
  assert(h.commands.prepare(h.doc.store));
  h.commands.activate_prepared(h.doc.store);
  assert(h.commands.memory().pending == 0);  // Same numeric generation also revokes input.
  assert(!h.commands.request({0, h.port.binding, 0, 0, 0, true}, h.transport.epoch));
  assert(!h.commands.request({0, h.port.binding, 1, 0, 0, false}, h.transport.epoch));
  assert(!h.request(1));  // Folders are navigation, never service actions.
}
void confirmation_and_reentrant_replacement() {
  Harness h(DEVICE_UI_PROFILES[8], true);
  for (const auto state : {"on", "true", "1", "home", "playing", "open", "opened", "opening", "closing",
                           "unlocked", "unlocking", "jammed", " ON "}) {
    h.transport.report(h.doc.entity, state);
    assert(h.request(0));
    assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
    assert(h.port.received.back().kind == PageCardCommandKind::TURN_OFF);
  }
  h.transport.report(h.doc.entity, "off");
  assert(!h.request(0));  // ON requires a prompt.
  h.transport.report(h.doc.entity, "on");
  assert(h.request(0));
  h.transport.report(h.doc.entity, "off");
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(h.port.received.back().kind ==
         PageCardCommandKind::TURN_OFF);  // Never reverse the chosen direction.
  h.transport.report(h.doc.entity, "on");
  assert(h.request(0));
  h.transport.report(h.doc.entity, "unavailable");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  Document next(DEVICE_UI_PROFILES[8], 17);
  const auto old_options = std::string(h.doc.store.field(*h.doc.store.card(2), CardField::OPTIONS));
  h.port.on_dispatch = [&](const PageCardCommandSnapshot& snapshot) {
    assert(h.commands.loop() == PageCardCommandResult::NONE);
    assert(h.commands.prepare(next.store) && h.graph.prepare(next.store));
    h.commands.activate_prepared(next.store);
    h.graph.activate_prepared(next.store);
    h.doc.memory.fill(0xdd);
    assert(snapshot.config.options == old_options && snapshot.config.entity == "script.test");
  };
  assert(h.request(2));
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  Harness cover(DEVICE_UI_PROFILES[8], true, true);
  cover.transport.report(cover.doc.entity, "open");
  assert(cover.request(0));
  assert(cover.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(cover.port.received.back().kind == PageCardCommandKind::CLOSE);
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) {
    cycles(limits);
    retained_definitions_do_not_dispatch(limits);
  }
  confirmation_and_reentrant_replacement();
}
