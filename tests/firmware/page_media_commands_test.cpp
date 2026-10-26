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
  explicit Document(const DeviceUiLimits& limits) {
    PanelConfigWriter w(bytes.data(), bytes.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {"media_player.room;Play;Auto;Auto;play_pause;;media;;",
                                "media_player.room;Previous;Auto;Auto;previous;;media;;",
                                "media_player.room;Next;Auto;Auto;next;;media;;",
                                "media_player.room;Playlist;Auto;Auto;playlist;;media;;playlist_content_id="
                                "spotify%3Aplaylist%3Atest,playlist_player_source=Living%20Room",
                                "media_player.room;Position;Auto;Auto;position;;media;;",
                                ";Media;Auto;Auto;;;subpage;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(w.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child =
        "~B|M,media_player.room,Progress,Auto,Auto,now_playing,,progress,|M,media_player.room,Play,Auto,Auto,"
        "now_playing,,play_pause,|M,media_player.room,Controls,Auto,Auto,control_modal,,,|M,media_player."
        "room,Cover,Auto,Auto,now_playing,,play_pause,media_cover_art";
    assert(w.append_subpage(6, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    size_t size = 0;
    assert(w.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, 0, memory.data(), memory.size(), &store) ==
           PageStoreStatus::OK);
  }
};
struct Port : PageCardCommandPort {
  PageMediaCommandPolicy& policy;
  Transport& transport;
  bool busy{false}, service{true}, accepted{true}, covered{false};
  uint32_t binding{1};
  std::vector<PageCardCommandSnapshot> received;
  std::function<void(const PageCardCommandSnapshot&)> on_send;
  explicit Port(PageMediaCommandPolicy& p, Transport& t) : policy(p), transport(t) {}
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
  PageMediaStateRuntime media{arena, transport};
  PageEntityStateRuntime graph{deps, arena, transport, {256, 128, 8192}, &media};
  PageMediaCommandPolicy policy{graph, media};
  Port port{policy, transport};
  PageCardCommands commands{graph, port};
  PageRuntimeResourcesGroup<3> resources{{&commands, &media, &graph}};
  explicit Harness(const DeviceUiLimits& limits) : doc(limits) {
    assert(resources.prepare(doc.store));
    resources.activate_prepared(doc.store);
    values();
  }
  PageBindingToken token(CardId card = 0, PageId page = 0) const {
    return {0, port.binding, page, card, 0, false};
  }
  void values() {
    transport.report("media_player.room", "playing");
    transport.report("media_player.room", "300", "media_duration");
    transport.report("media_player.room", "60", "media_position");
    transport.report("media_player.room", "track-a", "media_content_id");
  }
  PageMediaSeekProof proof(CardId card = 4) const {
    PageMediaPlaybackSnapshot s;
    assert(media.playback({0, card}, "media_player.room", s));
    return {transport.epoch, s.seek_revision};
  }
  bool action(CardId card = 0, PageId page = 0) {
    const auto before = allocations;
    const bool accepted = commands.request_media(token(card, page), transport.epoch);
    assert(before == allocations);
    return accepted;
  }
  bool seek(CardId card = 4, PageId page = 0, uint8_t percent = 65, float duration = 300) {
    const auto before = allocations;
    const bool accepted = commands.request_seek(token(card, page), proof(card), percent, duration);
    assert(before == allocations);
    return accepted;
  }
};
void cycles(const DeviceUiLimits& limits) {
  Harness h(limits);
  const auto refs = h.transport.used, arenas = h.arena.live, reannouncements = h.transport.reannouncements;
  for (size_t cycle = 0; cycle < 1000; ++cycle) {
    h.port.received.clear();
    assert(h.action() && h.action(1) && h.action(2) && h.action(3) && h.seek());
    h.port.busy = true;
    assert(h.commands.loop() == PageCardCommandResult::NONE);
    h.port.busy = false;
    for (int i = 0; i < 5; ++i) assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
    assert(h.port.received.size() == 5 && h.port.received[1].config.sensor == "previous" &&
           h.port.received[2].config.sensor == "next");
    assert(h.port.received[4].value.integer == 65 && h.port.received[4].value.number == 300);
    assert(h.transport.used == refs && h.arena.live == arenas &&
           h.transport.reannouncements == reannouncements);
    ++h.port.binding;
  }
  const auto child = h.doc.store.page(1)->first_card;
  assert(h.seek(child, 1) && h.action(child + 1, 1));
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED &&
         h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(!h.action(child + 2, 1) && !h.action(child + 3, 1));  // panel-only variants
  assert(!h.action(5) && !h.seek(0) && !h.action(child));
}
void revocation() {
  Harness h(DEVICE_UI_PROFILES[8]);
  assert(!h.seek(4, 0, 255) && !h.seek(4, 0, 50, 299) &&
         !h.seek(4, 0, 50, std::numeric_limits<float>::quiet_NaN()));
  const auto proof = h.proof();
  assert(h.seek());
  h.transport.report("media_player.room", "track-b", "media_content_id");
  h.transport.report("media_player.room", "track-a", "media_content_id");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED && h.proof().revision != proof.revision);
  assert(!h.commands.request_seek(h.token(4), proof, 50, 300));
  const auto same = h.proof();
  assert(h.seek());
  h.transport.report("media_player.room", "track-a", "media_content_id");
  h.transport.report("media_player.room", "61", "media_position");
  h.transport.report("media_player.room", "playing");
  assert(h.proof().revision == same.revision && h.commands.loop() == PageCardCommandResult::DISPATCHED);
  assert(h.seek());
  h.transport.report("media_player.room", "301", "media_duration");
  h.transport.report("media_player.room", "300", "media_duration");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.seek());
  h.transport.report("media_player.room", "unavailable");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.action(1));  // Transport buttons remain allowed without playback metadata.
  h.commands.cancel();
  h.values();
  assert(h.seek());
  h.port.service = false;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED && !h.action());
  h.port.service = true;
  assert(h.seek());
  h.transport.online = false;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.transport.online = true;
  assert(h.seek());
  ++h.transport.epoch;
  h.values();
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.seek());
  ++h.port.binding;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  assert(h.seek());
  h.port.covered = true;
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED);
  h.port.covered = false;
  for (int i = 0; i < 8; ++i) assert(h.action());
  assert(!h.seek());
  h.commands.cancel();
  h.port.accepted = false;
  assert(h.seek() && h.commands.loop() == PageCardCommandResult::FAILED);
  h.port.accepted = true;
  assert(h.seek());
  const std::string large(8193, 'x');
  h.transport.report("media_player.room", large, "media_content_id");
  assert(h.commands.loop() == PageCardCommandResult::CANCELLED && !h.seek());
  h.values();
  Document next(DEVICE_UI_PROFILES[8]);
  h.port.on_send = [&](const PageCardCommandSnapshot& s) {
    assert(h.resources.prepare(next.store));
    h.resources.activate_prepared(next.store);
    h.values();
    h.doc.memory.fill(0xdd);
    assert(s.config.entity == "media_player.room" && s.config.type == "media" &&
           s.config.options.find("spotify") != std::string::npos);
    assert(h.action() && h.commands.loop() == PageCardCommandResult::NONE);
  };
  assert(h.action(3) && h.commands.loop() == PageCardCommandResult::DISPATCHED);
  h.port.on_send = {};
  assert(h.commands.loop() == PageCardCommandResult::DISPATCHED);
  h.resources.retire_active();
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) cycles(limits);
  revocation();
}
