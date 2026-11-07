#include "page_cover_art_runtime.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "page_card_dependencies.h"
#include "page_entity_observer_set.h"
#include "panel_config_document.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
static size_t allocations = 0;
void* operator new(size_t size) {
  ++allocations;
  if (void* p = std::malloc(size ? size : 1)) return p;
  std::abort();
}
void* operator new[](size_t size) {
  return ::operator new(size);
}
void operator delete(void* p) noexcept {
  std::free(p);
}
void operator delete[](void* p) noexcept {
  std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
  std::free(p);
}
namespace {
struct Arena final : PageArenaAllocator {
  size_t live{0};
  void* allocate(size_t bytes) override {
    ++live;
    return std::malloc(bytes);
  }
  void release(void* p, size_t) override {
    --live;
    std::free(p);
  }
  ~Arena() {
    assert(live == 0);
  }
};
struct Subscriptions final : PageEntitySubscriptions {
  struct Entry {
    const char *entity, *attribute;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::array<Entry, 128> entries{};
  size_t used{0};
  uint32_t generation{1};
  bool subscribe(const char* entity, const char* attribute, void* owner, uint32_t revision, uint32_t channel,
                 Deliver deliver) override {
    assert(used < entries.size());
    entries[used++] = {entity, attribute, owner, revision, channel, deliver};
    return true;
  }
  void release(void* owner) override {
    for (auto& entry : entries)
      if (entry.owner == owner) entry.owner = nullptr;
  }
  uint32_t connection_generation() const override {
    return generation;
  }
  void send(const char* entity, const char* attribute, std::string_view value) {
    for (size_t i = 0; i < used; ++i) {
      const auto entry = entries[i];
      if (entry.owner && std::strcmp(entity, entry.entity) == 0 &&
          std::strcmp(attribute, entry.attribute) == 0)
        entry.deliver(entry.owner, entry.revision, entry.channel, value);
    }
  }
};
struct Transport final : PageMediaTransport {
  Subscriptions& subscriptions;
  explicit Transport(Subscriptions& value) : subscriptions(value) {}
  uint32_t now{100};
  bool online{true};
  bool connected() const override {
    return online;
  }
  uint32_t connection_generation() const override {
    return subscriptions.generation;
  }
  uint32_t now_ms() const override {
    return now;
  }
  int64_t epoch_seconds() const override {
    return 1000;
  }
  void reannounce() override {}
};
struct Origin final : PageImageOrigin {
  Transport& transport;
  explicit Origin(Transport& value) : transport(value) {}
  uint32_t configuration{17};
  bool valid(PageBindingToken token) const override {
    return token.configuration == configuration && token.binding;
  }
  bool connected() const override {
    return transport.online;
  }
  uint32_t connection_generation() const override {
    return transport.connection_generation();
  }
};
struct Environment final : PageCoverArtEnvironment {
  std::string base{"https://ha"};
  uint32_t generation{1};
  bool allowed{true};
  std::string_view base_url() const override {
    return base;
  }
  uint32_t endpoint_generation() const override {
    return generation;
  }
  bool request_allowed(uint16_t width, uint16_t height) const override {
    assert(width == 200 && height == 100);
    return allowed;
  }
};
struct Requests final : PageImageRequests {
  struct Delivery {
    void* owner{nullptr};
    Receive receive{nullptr};
    size_t slot{0};
    uint64_t request{0};
    void reply(bool success = true) const {
      if (receive) receive(owner, slot, request, success, false);
    }
  };
  std::array<Delivery, 6> deliveries{};
  std::string url;
  size_t starts{0}, cancels{0}, releases{0};
  bool accept{true};
  std::function<void()> during;
  bool start(size_t slot, uint64_t request, std::string_view source, PageImageOptions options, void* owner,
             Receive receive) override {
    assert(options.cover && options.retain_completed);
    url = source;
    ++starts;
    deliveries[slot] = {owner, receive, slot, request};
    if (during) during();
    return accept;
  }
  void cancel(size_t slot) override {
    ++cancels;
    deliveries[slot].reply();
  }
  void release_surface(size_t) override {
    ++releases;
  }
};
template <size_t Slots>
struct Fixture {
  std::array<uint8_t, 8192> document{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> storage{};
  PageStore store;
  Arena arena;
  Subscriptions subscriptions;
  Transport transport{subscriptions};
  PageCardDependencies dependencies;
  PageMediaStateRuntime media{arena, transport};
  PageEntityObserverSet<2> observers;
  PageEntityStateRuntime state{dependencies, arena, subscriptions, {256, 128, 32768}, &observers};
  Requests requests;
  Origin origin{transport};
  PageImageRuntime<Slots> images{requests, origin};
  Environment environment;
  PageCoverArtRuntime<Slots> cover{state, media, images, environment};
  explicit Fixture(const DeviceUiLimits& limits) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string config =
        "media_player.primary;;;;cover_art;;media;;cover_art_secondary_entity=media_player.secondary";
    for (uint8_t slot = 1; slot <= 3; ++slot)
      assert(writer.append_button(slot, reinterpret_cast<const uint8_t*>(config.data()), config.size()) ==
             PanelConfigStatus::OK);
    size_t bytes = 0;
    assert(writer.finish(&bytes) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), bytes, limits, 17, storage.data(), storage.size(), &store) ==
           PageStoreStatus::OK);
    assert(observers.add(media) && observers.add(cover));
    assert(media.prepare(store) && state.prepare(store));
    media.activate_prepared(store);
    state.activate_prepared(store);
  }
  ~Fixture() {
    observers.remove(cover);
    observers.remove(media);
  }
  PageBindingToken token(uint32_t binding = 1, CardId card = 0, uint16_t shell = 0) const {
    return {17, binding, 0, card, shell, false};
  }
  void send(const char* attribute, std::string_view value, const char* entity = "media_player.primary") {
    subscriptions.send(entity, attribute, value);
  }
  void tick(uint32_t advance = 0) {
    transport.now += advance;
    media.loop();
    images.loop();
    cover.loop();
  }
  void preload() {
    send("", "playing");
    send("media_content_id", "track:1");
    send("media_title", "Title");
    send("media_artist", "Artist");
    send("entity_picture", "https://remote/art");
    send("entity_picture_local", "/local/art");
  }
  size_t bind(PageBindingToken token) {
    return cover.bind(token, {200, 100, 0, true, true});
  }
};
template <size_t Slots>
void lifecycle(const DeviceUiLimits& limits) {
  Fixture<Slots> f(limits);
  f.preload();
  const auto token = f.token();
  const auto slot = f.bind(token);
  assert(slot == 0 && f.requests.starts == 0);
  f.tick();
  f.tick(74);
  assert(f.requests.starts == 0);
  f.tick(1);
  assert(f.requests.starts == 1 && f.requests.url == "https://ha/local/art");
  f.requests.deliveries[slot].reply();
  const auto initial = f.cover.snapshot(token);
  assert(initial.ready && initial.asset);
  f.send("media_title", "Another title");
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 1);  // Nonempty content ID owns the refresh signature.
  f.send("media_content_id", "track:2");
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 2 && f.cover.snapshot(token).ready && f.cover.snapshot(token).pending);
  f.requests.deliveries[slot].reply(false);
  assert(f.cover.snapshot(token).failed && f.cover.snapshot(token).asset == initial.asset);
  f.tick();
  f.tick(1999);
  assert(f.requests.starts == 2);
  f.tick(1);
  assert(f.requests.starts == 3 && f.cover.snapshot(token).ready);
  f.requests.deliveries[slot].reply();
  assert(f.cover.snapshot(token).asset != initial.asset);
  f.send("entity_picture", "https://remote/new");
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 4 && f.requests.url == "https://remote/new");
  const auto old = f.requests.deliveries[slot];
  old.reply();
  f.send("entity_picture", "None");
  assert(!f.cover.snapshot(token).ready);  // Authoritative clear also guards draw callbacks.
  f.tick();
  f.tick(75);
  assert(!f.cover.snapshot(token).ready && f.requests.starts == 4);
  assert(f.requests.releases == 0);
  f.tick();
  assert(f.requests.releases == 1);
  f.send("entity_picture_local", "/local/alone");
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 4);
  f.tick();
  const auto cleared_revision = f.cover.revision();
  const auto cleared_releases = f.requests.releases;
  for (size_t i = 0; i < 1000; ++i) f.tick(100);
  assert(f.cover.revision() == cleared_revision && f.requests.releases == cleared_releases);
  f.environment.allowed = false;
  f.send("entity_picture", "https://remote/return");
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 4);
  f.environment.allowed = true;
  f.tick();
  assert(f.requests.starts == 5);
  old.reply();
  assert(f.cover.snapshot(token).pending);
  f.requests.deliveries[slot].reply();
  assert(f.cover.snapshot(token).ready);
  ++f.environment.generation;
  assert(!f.cover.snapshot(token).ready);
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 6);
  f.requests.deliveries[slot].reply();
  f.transport.online = false;
  f.tick();
  assert(!f.cover.snapshot(token).ready);
  ++f.subscriptions.generation;
  f.transport.online = true;
  f.tick();
  assert(!f.cover.snapshot(token).ready);
  f.preload();
  f.tick();
  f.tick(75);
  assert(f.cover.snapshot(token).pending);
  f.requests.deliveries[slot].reply();
  const auto starts = f.requests.starts;
  f.send("", "playing", "media_player.secondary");
  f.send("media_title", "TV programme", "media_player.secondary");
  f.send("entity_picture", "https://tv/art", "media_player.secondary");
  f.send("entity_picture_local", "None", "media_player.secondary");
  f.send("source", "TV");
  assert(!f.cover.snapshot(token).ready);
  f.tick();
  f.tick(75);
  assert(f.requests.starts == starts + 1 && f.requests.url == "https://tv/art");
  f.requests.deliveries[slot].reply();
  assert(f.cover.snapshot(token).ready);
  const auto retired = f.requests.deliveries[slot];
  f.cover.unbind(token);
  retired.reply();
  assert(!f.cover.snapshot(token).ready);
  const auto count = allocations;
  for (uint32_t i = 2; i <= 1001; ++i) {
    const auto next = f.token(i, 0, static_cast<uint16_t>(limits.slot_count - 1));
    assert(f.bind(next) == slot);
    f.cover.unbind(next);
  }
  assert(allocations == count);  // No strings, subscriptions, widgets or jobs on navigation.
  f.tick();
}
void retries_and_missing_pair() {
  Fixture<2> f(DEVICE_UI_PROFILES[8]);
  f.send("", "playing");
  f.send("media_title", "Title");
  f.send("entity_picture", "https://remote/art");
  const auto token = f.token();
  assert(f.bind(token) == 0);
  f.transport.now = UINT32_MAX - 100;  // Debounce and retry clocks survive wrap.
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 0);  // Companion still unknown.
  f.tick(299);
  assert(f.requests.starts == 0);
  f.tick(1);
  assert(f.requests.starts == 1 && f.requests.url == "https://remote/art");
  f.requests.deliveries[0].reply(false);
  // Exactly ten startup retries. Subsequent loops do not spin or allocate jobs.
  for (size_t retry = 0; retry < 10; ++retry) {
    f.tick();
    f.tick(2000);
    assert(f.requests.starts == retry + 2);
    f.requests.deliveries[0].reply(false);
  }
  f.tick();
  f.tick(10000);
  assert(f.requests.starts == 11);
  f.send("entity_picture", "https://remote/recovered");
  f.tick();
  f.tick(375);
  assert(f.requests.starts == 12);
  f.requests.deliveries[0].reply();
  assert(f.cover.snapshot(token).ready);
  // Rapid metadata notifications coalesce into a single settled transfer.
  const auto before = allocations;
  for (size_t i = 0; i < 1000; ++i) f.send("media_title", i % 2 ? "A" : "B");
  assert(allocations == before);
  f.tick();
  f.tick(375);
  assert(f.requests.starts == 13);
  f.requests.deliveries[0].reply();
  // Invalid configuration releases its lease without waiting for a late reply.
  ++f.origin.configuration;
  f.tick();
  assert(!f.cover.snapshot(token).ready && f.images.vacant(0));
}
void limits_and_auth() {
  Fixture<2> f(DEVICE_UI_PROFILES[8]);
  f.preload();
  assert(f.bind(f.token()) == 0 && f.bind(f.token(2, 1, 3)) == 1);
  assert(f.bind(f.token(3, 2, 2)) == 2);  // Exhaustion: ordinary fallback, no third image owner.
  f.send("entity_picture", "/api/image_proxy/image.cover");
  f.send("entity_picture_local", "None");
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 0);
  f.send("access_token", "example-token");
  f.tick();
  f.tick(75);
  assert(f.requests.starts == 2 &&
         f.requests.url == "https://ha/api/image_proxy/image.cover?token=example-token");
  const auto late = f.requests.deliveries[0];
  f.requests.during = [&] { f.cover.retire(); };
  f.send("media_content_id", "next");
  f.tick();
  f.tick(75);
  late.reply();
  assert(!f.cover.snapshot(f.token()).ready);
}
}  // namespace

void explicit_cover_refresh() {
  Fixture<2> f(DEVICE_UI_PROFILES[8]);f.preload();assert(f.bind(f.token())==0);f.tick();f.tick(75);
  f.requests.deliveries[0].reply();f.tick();
  const auto starts=f.requests.starts,count=allocations;
  for(int i=0;i<4;++i)f.cover.refresh();
  assert(f.requests.starts==starts&&allocations==count);
  f.tick();assert(f.requests.starts==starts);f.tick(75);assert(f.requests.starts==starts+1);
  f.requests.deliveries[0].reply();f.tick();
  f.cover.retire();f.cover.refresh();f.tick();assert(f.requests.starts==starts+1);
}

int main() {
  explicit_cover_refresh();
  for (const auto& limits : DEVICE_UI_PROFILES) {
    lifecycle<2>(limits);
    lifecycle<6>(limits);
  }
  retries_and_missing_pair();
  limits_and_auth();
}
