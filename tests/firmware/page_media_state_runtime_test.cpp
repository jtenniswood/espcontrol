#include "page_media_state_runtime.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>

#include "page_card_dependencies.h"
#include "panel_config_document.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
static size_t cpp_allocations = 0;
void* operator new(size_t bytes) {
  ++cpp_allocations;
  if (void* p = std::malloc(bytes ? bytes : 1)) return p;
  std::abort();
}
void* operator new[](size_t bytes) {
  return ::operator new(bytes);
}
void operator delete(void* pointer) noexcept {
  std::free(pointer);
}
void operator delete[](void* pointer) noexcept {
  std::free(pointer);
}
void operator delete(void* p, std::size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
  std::free(p);
}
struct Arena : PageArenaAllocator {
  std::unordered_map<void*, size_t> live;
  bool fail = false;
  void* allocate(size_t bytes) override {
    if (fail) return nullptr;
    auto* p = std::malloc(bytes);
    assert(p);
    live[p] = bytes;
    return p;
  }
  void release(void* pointer, size_t bytes) override {
    assert(live.at(pointer) == bytes);
    live.erase(pointer);
    std::free(pointer);
  }
};
struct Transport : PageMediaTransport {
  bool online = true;
  uint32_t connection = 1;
  size_t requests = 0;
  std::function<void()> during;
  uint32_t now = 100;
  int64_t epoch = 1000;
  uint32_t now_ms() const override {
    return now;
  }
  int64_t epoch_seconds() const override {
    return epoch;
  }
  bool connected() const override {
    return online;
  }
  uint32_t connection_generation() const override {
    return connection;
  }
  void reannounce() override {
    ++requests;
    if (during) during();
  }
};
struct Fixture {
  explicit Fixture(uint32_t generation = 17, const std::string& entity = "media_player.room",
                   std::array<const char*, 3> modes = {"playlist", "playlist", "playlist"}) {
    const auto& limits = DEVICE_UI_PROFILES[8];
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    for (uint8_t slot = 1; slot <= 3; ++slot) {
      const auto text = entity + ";;;;" + modes[slot - 1] + ";;media;;playlist_content_id=abcd";
      assert(writer.append_button(slot, reinterpret_cast<const uint8_t*>(text.data()), text.size()) ==
             PanelConfigStatus::OK);
    }
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, generation, storage.data(), storage.size(),
                               &store) == PageStoreStatus::OK);
  }
  std::array<uint8_t, 8192> document{};
  size_t size = 0;
  alignas(std::max_align_t) std::array<uint8_t, 32768> storage{};
  PageStore store;
};
void model_lifecycle() {
  Arena arena;
  Transport transport;
  Fixture first, next(18), different(19, "media_player.other");
  {
    PageMediaStateRuntime runtime(arena, transport);
    assert(runtime.prepare(first.store));
    runtime.observed(17, "media_player.room", "", "playing", false);
    runtime.observed(17, "media_player.room", "media_content_id", "abcd", false);
    assert(!runtime.current({17, 0}, "media_player.room", "media_content_id"));
    const auto before = cpp_allocations;
    runtime.activate_prepared(first.store);
    assert(cpp_allocations == before && runtime.memory().sources == 1);
    assert(runtime.current({17, 0}, "media_player.room", "media_content_id") &&
           !runtime.current({18, 0}, "media_player.room", "media_content_id") &&
           !runtime.current({17, 99}, "media_player.room", "media_content_id"));
    runtime.loop();
    assert(transport.requests == 0);
    runtime.observed(17, "media_player.room", "", "idle", false);
    runtime.observed(17, "media_player.room", "", "playing", false);
    assert(!runtime.current({17, 0}, "media_player.room", "media_content_id"));
    runtime.observed(17, "media_player.room", "media_content_id", "abcd", true);
    assert(!runtime.current({17, 0}, "media_player.room", "media_content_id"));
    runtime.loop();
    runtime.loop();
    assert(transport.requests == 1);
    runtime.observed(17, "media_player.room", "media_content_id", "abcd", false);
    assert(runtime.current({17, 0}, "media_player.room", "media_content_id"));
    runtime.observed(17, "media_player.room", "", "PAUSED", false);
    assert(runtime.current({17, 0}, "media_player.room", "media_content_id"));
    // Repeated unusable states preserve genuinely new metadata received after the transition.
    runtime.observed(17, "media_player.room", "", "off", false);
    runtime.observed(17, "media_player.room", "media_content_id", "abcd", false);
    runtime.observed(17, "media_player.room", "", " OFF ", false);
    assert(runtime.current({17, 0}, "media_player.room", "media_content_id"));
    arena.fail = true;
    assert(!runtime.prepare(next.store));
    arena.fail = false;
    assert(runtime.current({17, 0}, "media_player.room", "media_content_id"));
    for (size_t i = 0; i < 1000; ++i) {
      const auto& fixture = i % 2 ? first : next;
      assert(runtime.prepare(fixture.store));
      const auto count = cpp_allocations;
      runtime.activate_prepared(fixture.store);
      assert(cpp_allocations == count && arena.live.size() == 1 &&
             runtime.current({fixture.store.generation(), 0}, "media_player.room", "media_content_id"));
    }
    runtime.observed(17, "media_player.room", "", "idle", false);
    assert(runtime.prepare(next.store));
    runtime.observed(18, "media_player.room", "", "idle", true);
    runtime.observed(18, "media_player.room", "media_content_id", "abcd", true);
    runtime.activate_prepared(next.store);
    assert(!runtime.current({18, 0}, "media_player.room", "media_content_id"));
    transport.online = false;
    ++transport.connection;
    assert(!runtime.current({18, 0}, "media_player.room", "media_content_id"));
    runtime.loop();
    transport.online = true;
    runtime.loop();
    runtime.observed(18, "media_player.room", "", "playing", false);
    runtime.observed(18, "media_player.room", "media_content_id", "abcd", false);
    assert(runtime.current({18, 0}, "media_player.room", "media_content_id"));
    assert(runtime.prepare(different.store));
    runtime.observed(19, "media_player.other", "", "playing", false);
    runtime.observed(19, "media_player.other", "media_content_id", "other", false);
    runtime.discard_prepared();
    assert(runtime.current({18, 0}, "media_player.room", "media_content_id"));
    runtime.observed(18, "media_player.room", "", "idle", false);
    runtime.observed(18, "media_player.room", "", "playing", false);
    transport.during = [&] {
      assert(runtime.prepare(different.store));
      const auto count = cpp_allocations;
      runtime.activate_prepared(different.store);
      assert(cpp_allocations == count);
      std::memset(next.storage.data(), 0xA5, next.storage.size());
      runtime.retire_active();
      runtime.loop();
    };
    runtime.loop();
    transport.during = {};
    assert(arena.live.empty());
    const auto ignored = runtime.memory().ignored;
    runtime.observed(18, "media_player.room", "media_content_id", "late", false);
    assert(runtime.memory().ignored == ignored + 1);
  }
  assert(arena.live.empty());
}
void only_declared_metadata_is_refreshed() {
  Arena arena;
  Transport transport;
  Fixture actions(30, "media_player.room", {"play_pause", "previous", "next"});
  Fixture mixed(31, "media_player.room", {"play_pause", "playlist", "next"});
  Fixture plain(32, "media_player.room", {"next", "previous", "play_pause"});
  PageMediaStateRuntime runtime(arena, transport);
  assert(runtime.prepare(actions.store));
  runtime.activate_prepared(actions.store);
  runtime.observed(30, "media_player.room", "", "playing", false);
  runtime.loop();
  assert(transport.requests == 0);
  runtime.observed(30, "media_player.room", "", "idle", false);
  runtime.observed(30, "media_player.room", "", "playing", false);
  runtime.loop();
  assert(transport.requests == 0);
  assert(runtime.prepare(mixed.store));
  runtime.activate_prepared(mixed.store);
  runtime.loop();
  assert(transport.requests == 1 && runtime.memory().sources == 1);
  runtime.observed(31, "media_player.room", "media_content_id", "abcd", false);
  assert(runtime.prepare(plain.store));
  runtime.activate_prepared(plain.store);
  runtime.loop();
  assert(transport.requests == 1);
  assert(!runtime.current({32, 0}, "media_player.room", "media_content_id"));
}
void playback_survives_navigation_and_replacement() {
  Arena arena;
  Transport transport;
  transport.now = 1000;
  transport.epoch = 1003;
  Fixture first(40, "media_player.room", {"position", "play_pause", "next"});
  Fixture next(41, "media_player.room", {"position", "play_pause", "previous"});
  Fixture actions(42, "media_player.room", {"play_pause", "previous", "next"});
  PageMediaStateRuntime runtime(arena, transport);
  PageMediaPlaybackSnapshot snapshot;
  assert(runtime.prepare(first.store));
  runtime.observed(40, "media_player.room", "", "playing", false);
  runtime.observed(40, "media_player.room", "media_duration", "300", false);
  runtime.observed(40, "media_player.room", "media_position", "10", false);
  runtime.observed(40, "media_player.room", "media_position_updated_at", "1970-01-01T00:16:40Z", false);
  assert(!runtime.playback({40, 0}, "media_player.room", snapshot));
  runtime.activate_prepared(first.store);
  assert(runtime.playback({40, 0}, "media_player.room", snapshot) && snapshot.available && snapshot.playing &&
         snapshot.seconds == 13 && snapshot.percent == 4);
  const auto revision = runtime.revision();
  transport.now = 2000;
  runtime.loop();
  assert(runtime.revision() == revision + 1);
  runtime.loop();
  assert(runtime.revision() == revision + 1 && transport.requests == 0);
  assert(runtime.playback({40, 0}, "media_player.room", snapshot) && snapshot.seconds == 14);
  runtime.observed(40, "media_player.room", "", "paused", false);
  transport.now = 10000;
  assert(runtime.playback({40, 0}, "media_player.room", snapshot) && snapshot.seconds == 14 &&
         !snapshot.playing);
  assert(runtime.prepare(next.store));
  runtime.observed(41, "media_player.room", "media_position", "10", true);
  const auto allocations = cpp_allocations;
  runtime.activate_prepared(next.store);
  assert(cpp_allocations == allocations);
  std::memset(first.storage.data(), 0xA5, first.storage.size());
  assert(runtime.playback({41, 0}, "media_player.room", snapshot) && snapshot.seconds == 14);
  runtime.observed(41, "media_player.room", "", "playing", false);
  transport.now = 11000;
  assert(runtime.playback({41, 0}, "media_player.room", snapshot) && snapshot.seconds == 15);
  const auto loop_allocations = cpp_allocations;
  for (size_t i = 0; i < 1000; ++i) {
    transport.now += 1000;
    runtime.loop();
    assert(runtime.playback({41, 0}, "media_player.room", snapshot) && snapshot.seconds <= 300);
  }
  assert(cpp_allocations == loop_allocations && transport.requests == 0);
  assert(runtime.prepare(actions.store));
  runtime.activate_prepared(actions.store);
  const auto quiet = runtime.revision();
  transport.now += 2000;
  runtime.loop();
  assert(runtime.revision() == quiet && !runtime.playback({42, 0}, "media_player.room", snapshot));
  transport.online = false;
  ++transport.connection;
  assert(!runtime.playback({42, 0}, "media_player.room", snapshot));
  runtime.loop();
}
void shared_cache_metadata_lifecycle() {
  struct Subscriptions : PageEntitySubscriptions {
    struct Binding {
      const char *entity, *attribute;
      void* owner;
      uint32_t revision, channel;
      Deliver deliver;
    };
    std::array<Binding, 64> entries{};
    size_t count = 0;
    Transport& transport;
    explicit Subscriptions(Transport& source) : transport(source) {}
    bool subscribe(const char* entity, const char* attribute, void* owner, uint32_t revision,
                   uint32_t channel, Deliver deliver) override {
      if (count == entries.size()) return false;
      entries[count++] = {entity, attribute, owner, revision, channel, deliver};
      return true;
    }
    void release(void* owner) override {
      size_t used = 0;
      for (size_t i = 0; i < count; ++i)
        if (entries[i].owner != owner) entries[used++] = entries[i];
      count = used;
    }
    uint32_t connection_generation() const override {
      return transport.connection;
    }
    void send(const char* attribute, std::string_view value) {
      for (size_t i = 0; i < count; ++i) {
        const auto binding = entries[i];
        if (std::strcmp(binding.attribute, attribute) == 0)
          binding.deliver(binding.owner, binding.revision, binding.channel, value);
      }
    }
  };
  Arena arena;
  Transport transport;
  Subscriptions subscriptions(transport);
  PageCardDependencies dependencies;
  Fixture first(70, "media_player.room", {"now_playing", "play_pause", "playlist"});
  Fixture replacement(71, "media_player.room", {"now_playing", "play_pause", "playlist"});
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 32, 8192}, &media);
  assert(media.prepare(first.store) && state.prepare(first.store));
  media.activate_prepared(first.store);
  state.activate_prepared(first.store);
  uint32_t generation = 70;
  const auto current = [&](const char* attribute) {
    return media.current({generation, 0}, "media_player.room", attribute);
  };
  subscriptions.send("", "playing");
  subscriptions.send("media_content_type", "music");
  subscriptions.send("media_content_id", "provider:track:1");
  subscriptions.send("media_title", "Old title");
  subscriptions.send("media_artist", "Artist");
  media.loop();
  assert(current("media_title") && current("media_artist"));
  const auto allocation_count = cpp_allocations, requests = transport.requests;
  // The observer compares the old packed cache before memmove overwrites it.
  for (size_t i = 0; i < 1000; ++i) {
    subscriptions.send("media_title", "Old title");
    media.loop();
  }
  assert(cpp_allocations == allocation_count && transport.requests == requests && current("media_artist"));
  subscriptions.send("media_title", "New title");
  assert(current("media_title") && !current("media_artist"));
  media.loop();
  assert(transport.requests == requests + 1);
  // Identical raw artist bytes still restore validity after invalidation.
  subscriptions.send("media_title", "New title");
  subscriptions.send("media_artist", "Artist");
  media.loop();
  assert(current("media_artist") && transport.requests == requests + 1);
  subscriptions.send("media_content_id", "provider:track:2");
  assert(!current("media_title") && !current("media_artist"));
  // Artist may arrive first in the refresh snapshot. The expected title must
  // not clear it again, even when the new item's title has the same bytes.
  subscriptions.send("media_artist", "Artist");
  subscriptions.send("media_title", "New title");
  assert(current("media_title") && current("media_artist"));
  media.loop();
  assert(transport.requests == requests + 2);
  subscriptions.send("media_content_id", "");
  subscriptions.send("media_content_id", "provider:track:3");
  assert(!current("media_title") && !current("media_artist"));
  // Replacement preserves invalidity independently of still-retained bytes.
  assert(media.prepare(replacement.store) && state.prepare(replacement.store));
  media.activate_prepared(replacement.store);
  state.activate_prepared(replacement.store);
  generation = 71;
  assert(!current("media_title") && !current("media_artist"));
  transport.now += media::MEDIA_TITLE_REFRESH_TIMEOUT_MS;
  subscriptions.send("media_artist", "Artist");
  subscriptions.send("media_title", "New title");
  assert(!current("media_artist"));  // Expired title refresh clears the old artist.
  subscriptions.send("media_artist", "Artist");
  subscriptions.send("media_content_type", "movie");
  assert(!current("media_artist"));
  subscriptions.send("media_artist", "Programme");
  subscriptions.send("", "idle");
  assert(!current("media_title") && !current("media_artist"));
  subscriptions.send("media_title", "Fresh after idle");
  subscriptions.send("", "idle");
  assert(current("media_title"));  // Repeated idle does not erase newer metadata.
  ++transport.connection;
  media.loop();
  assert(!current("media_title"));
  subscriptions.send("", "playing");
  subscriptions.send("media_title", "Fresh after reconnect");
  assert(current("media_title"));
  subscriptions.send("media_content_id", "provider:track:1");
  subscriptions.send("media_title", "New title");
  subscriptions.send("media_artist", "Artist");
  media.loop();
  const auto stable_allocations = cpp_allocations, channels = subscriptions.count;
  for (size_t item = 0; item < 1000; ++item) {
    subscriptions.send("media_content_id", item % 2 ? "provider:track:1" : "provider:track:2");
    subscriptions.send("media_artist", "Artist");
    subscriptions.send("media_title", "New title");
    media.loop();
    assert(current("media_title") && current("media_artist"));
    assert(cpp_allocations == stable_allocations && subscriptions.count == channels);
  }
  subscriptions.send("media_title", "Retire during metadata refresh");
  transport.during = [&] {
    state.retire_active();
    media.retire_active();
  };
  media.loop();
  transport.during = {};
  assert(arena.live.empty() && subscriptions.count == 0);
}

int main() {
  shared_cache_metadata_lifecycle();
  model_lifecycle();
  playback_survives_navigation_and_replacement();
  only_declared_metadata_is_refreshed();
}
