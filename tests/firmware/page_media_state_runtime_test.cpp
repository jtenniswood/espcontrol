#include "page_media_state_runtime.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>

#include "cover_art.h"
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
  size_t calls = 0, fail_at = SIZE_MAX;
  void* allocate(size_t bytes) override {
    if (fail || ++calls == fail_at) return nullptr;
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
struct Subscriptions : PageEntitySubscriptions {
  struct Binding {
    const char *entity, *attribute;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::array<Binding, 128> entries{};
  size_t count = 0;
  Transport& transport;
  explicit Subscriptions(Transport& source) : transport(source) {}
  bool subscribe(const char* entity, const char* attribute, void* owner, uint32_t revision, uint32_t channel,
                 Deliver deliver) override {
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
    send("", attribute, value);
  }
  void send(const char* entity, const char* attribute, std::string_view value) {
    for (size_t i = 0; i < count; ++i) {
      const auto binding = entries[i];
      if ((!*entity || std::strcmp(binding.entity, entity) == 0) &&
          std::strcmp(binding.attribute, attribute) == 0)
        binding.deliver(binding.owner, binding.revision, binding.channel, value);
    }
  }
};

struct Fixture {
  explicit Fixture(uint32_t generation = 17, const std::string& entity = "media_player.room",
                   std::array<const char*, 3> modes = {"playlist", "playlist", "playlist"},
                   std::array<const char*, 3> options = {"playlist_content_id=abcd",
                                                         "playlist_content_id=abcd",
                                                         "playlist_content_id=abcd"},
                   std::array<const char*, 3> entities = {nullptr, nullptr, nullptr}) {
    const auto& limits = DEVICE_UI_PROFILES[8];
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    for (uint8_t slot = 1; slot <= 3; ++slot) {
      const auto text = std::string(entities[slot - 1] ? entities[slot - 1] : entity.c_str()) + ";;;;" +
                        modes[slot - 1] + ";;media;;" + options[slot - 1];
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

void external_inputs_are_configuration_owned() {
  Arena arena;
  Transport transport;
  Fixture plain(90, "media_player.room", {"now_playing", "position", "play_pause"});
  Fixture cover(91, "media_player.room", {"cover_art", "position", "play_pause"});
  Fixture next(92, "media_player.room", {"cover_art", "position", "play_pause"});
  PageMediaStateRuntime media(arena, transport);
  assert(media.prepare(plain.store));
  media.activate_prepared(plain.store);
  const auto observe = [&](uint32_t generation, const char* attr, const char* value, bool copied = false) {
    media.observed(generation, "media_player.room", attr, value, copied);
  };
  observe(90, "", "playing");
  observe(90, "media_title", "Title");
  observe(90, "media_artist", "Artist");
  observe(90, "media_duration", "300");
  observe(90, "media_position", "10");
  observe(90, "source", "TV");
  PageMediaPlaybackSnapshot playback;
  assert(media.external_input({90, 0}, "media_player.room") &&
         media.current({90, 0}, "media_player.room", "media_title"));
  assert(media.playback({90, 0}, "media_player.room", playback) && playback.duration == 300);
  assert(media.prepare(cover.store));
  media.activate_prepared(cover.store);
  observe(91, "entity_picture_local", "/image");
  observe(91, "source", "HDMI 1");
  assert(!media.current({91, 0}, "media_player.room", "media_title") &&
         !media.current({91, 0}, "media_player.room", "entity_picture_local"));
  assert(media.playback({91, 0}, "media_player.room", playback) && !playback.has_duration &&
         !playback.has_position);
  assert(media.prepare(next.store));
  observe(92, "", "playing", true);
  observe(92, "source", "Spotify", true);
  media.activate_prepared(next.store);
  observe(92, "media_title", "Programme");
  assert(media.external_input({92, 0},
                              "media_player.room"));  // Copied state did not reset the live snapshot flags.
  observe(92, "", "playing");
  observe(92, "media_title", "Music");
  assert(!media.external_input({92, 0}, "media_player.room"));
  observe(92, "media_content_id", "x-sonos-htastream:device:spdif");
  observe(92, "source", "Spotify");
  assert(media.external_input({92, 0}, "media_player.room"));
  ++transport.connection;
  assert(!media.external_input({92, 0}, "media_player.room"));
  media.loop();
  assert(!media.external_input({92, 0}, "media_player.room"));
}
void secondary_routes_and_picture_freshness() {
  Arena arena;
  Transport transport;
  Fixture first(100, "media_player.primary", {"cover_art", "play_pause", "now_playing"},
                {"cover_art_secondary_entity=media_player.secondary", "", ""},
                {nullptr, "media_player.other", "media_player.secondary"});
  Fixture next(101, "media_player.primary", {"cover_art", "play_pause", "now_playing"},
               {"cover_art_secondary_entity=media_player.secondary", "", ""},
               {nullptr, "media_player.other", "media_player.secondary"});
  PageMediaStateRuntime media(arena, transport);
  assert(media.prepare(first.store));
  media.activate_prepared(first.store);
  assert(media.memory().sources == 3 && arena.live.size() == 1);
  assert(media.memory().peak_preparation_bytes > 0);
  const ConfigurationHandle cover{100, 0}, other{100, 1}, plain{100, 2};
  PageMediaRouteSnapshot route;
  PageMediaSourceSnapshot state;
  PageMediaPlaybackSnapshot playback;
  auto observe = [&](const char* entity, const char* attribute, const char* value) {
    media.observed(100, entity, attribute, value, false);
  };
  assert(media.cover_route(cover, route) && route.entity == "media_player.primary" &&
         !route.state.state_known);
  assert(!media.cover_route(plain, route) && route.entity.empty());
  observe("media_player.primary", "", "playing");
  observe("media_player.primary", "source", "TV");
  assert(media.cover_route(cover, route) && route.external_fallback && !route.secondary_selected);
  assert(!media.source_snapshot(other, "media_player.primary", state));
  assert(!media.current(plain, "media_player.primary", "source"));
  observe("media_player.secondary", "entity_picture", "/before-state");
  observe("media_player.secondary", "", "playing");
  assert(media.source_snapshot(cover, "media_player.secondary", state) && !state.has_content &&
         !state.has_artwork);
  assert(media.cover_route(cover, route) && route.external_fallback);
  observe("media_player.secondary", "media_content_type", "movie");
  assert(media.cover_route(cover, route) && !route.secondary_selected);  // Type alone is not content.
  observe("media_player.secondary", "media_title", "Programme");
  observe("media_player.secondary", "media_artist", "Station");
  observe("media_player.secondary", "media_duration", "200");
  observe("media_player.secondary", "media_position", "10");
  assert(media.cover_route(cover, route) && route.secondary_selected && !route.external_fallback &&
         route.entity == "media_player.secondary" && route.secondary == "media_player.secondary");
  assert(media.playback(cover, route.entity, playback) && playback.duration == 200);
  assert(!media.playback(other, route.entity, playback));
  observe("media_player.secondary", "source", "HDMI 1");
  // Secondary inputs retain programme metadata/progress, matching the current
  // route. Only a primary cover source invalidates them on its input change.
  assert(media.current(cover, "media_player.secondary", "media_title"));
  assert(media.playback(cover, "media_player.secondary", playback) && playback.duration == 200);
  assert(media.cover_route(cover, route) && route.secondary_selected && route.suppress_artwork);
  observe("media_player.secondary", "", "idle");
  observe("media_player.secondary", "entity_picture_local", "/new-but-idle");
  assert(media.cover_route(cover, route) && route.external_fallback);
  observe("media_player.secondary", "", "playing");
  assert(media.cover_route(cover, route) && !route.secondary_selected);  // No resurrection of idle artwork.
  observe("media_player.secondary", "entity_picture_local", "None");
  assert(media.cover_route(cover, route) && !route.secondary_selected);
  observe("media_player.secondary", "entity_picture_local", "/fresh-local");
  assert(media.cover_route(cover, route) && route.secondary_selected && route.suppress_artwork);
  // Local pictures do not clear an omitted source; remote pictures do.
  observe("media_player.secondary", "entity_picture", "/fresh-remote");
  assert(media.cover_route(cover, route) && route.secondary_selected && !route.suppress_artwork);
  assert(media.effective_source(cover, "media_player.secondary", "HDMI 1").empty());
  assert(media.effective_source(other, "media_player.secondary", "HDMI 1").empty());
  observe("media_player.secondary", "entity_picture_local", "None");
  observe("media_player.secondary", "entity_picture", "unknown");
  assert(media.cover_route(cover, route) && !route.secondary_selected);
  observe("media_player.secondary", "entity_picture", "&#78;one");
  assert(media.cover_route(cover, route) && route.secondary_selected);  // URL bytes are not HTML-decoded.
  observe("media_player.primary", "media_content_id", "x-sonos-htastream:device:spdif");
  observe("media_player.primary", "source", "Spotify");
  assert(media.effective_source(cover, "media_player.primary", "Spotify") == "TV");
  media.loop();
  const auto stable = cpp_allocations;
  for (int i = 0; i < 1000; ++i) {
    observe("media_player.secondary", "", i % 2 ? "playing" : "paused");
    observe("media_player.secondary", "entity_picture", "/fresh");
    assert(media.cover_route(cover, route) && route.secondary_selected);
    assert(media.playback(cover, route.entity, playback));
    assert(media.source_snapshot(plain, route.entity, state));
    media.loop();
  }
  assert(cpp_allocations == stable);

  // Final arena allocation failure drains scratch and leaves active links intact.
  arena.fail_at = arena.calls + 2;
  assert(!media.prepare(next.store));
  arena.fail_at = SIZE_MAX;
  assert(arena.live.size() == 1 && media.cover_route(cover, route) && route.secondary_selected);
  assert(media.prepare(next.store));
  const auto activation_allocations = cpp_allocations;
  media.activate_prepared(next.store);
  assert(cpp_allocations == activation_allocations);
  std::memset(first.storage.data(), 0xA5, first.storage.size());
  assert(!media.cover_route(cover, route) && route.entity.empty());
  assert(media.cover_route({101, 0}, route) && route.secondary_selected &&
         route.entity == "media_player.secondary");  // Names live in the new media arena.
  ++transport.connection;
  assert(!media.cover_route({101, 0}, route));
  media.loop();
  assert(media.cover_route({101, 0}, route) && !route.secondary_selected && !route.state.state_known);
  assert(!media.current({101, 1}, "media_player.secondary", "entity_picture"));
}
void duplicate_and_legacy_cover_routes() {
  Arena arena;
  Transport transport;
  Fixture fixture(110, "media_player.primary", {"now_playing", "cover_art", "now_playing"},
                  {"media_cover_art,cover_art_secondary_entity=media_player.secondary",
                   "cover_art_secondary_entity=media_player.primary", ""});
  PageMediaStateRuntime media(arena, transport);
  assert(media.prepare(fixture.store));
  media.activate_prepared(fixture.store);
  assert(media.memory().sources == 2);
  PageMediaRouteSnapshot route;
  media.observed(110, "media_player.primary", "", "playing", false);
  media.observed(110, "media_player.primary", "source", "TV", false);
  media.observed(110, "media_player.secondary", "", "playing", false);
  media.observed(110, "media_player.secondary", "media_content_id", "programme", false);
  assert(media.cover_route({110, 0}, route) && route.secondary_selected);
  assert(media.cover_route({110, 1}, route) && !route.secondary_selected && route.secondary.empty() &&
         route.external_fallback);
  assert(!media.cover_route({110, 2}, route));
}
void routing_policy_parity() {
  Arena arena;
  Transport transport;
  Fixture fixture(120, "media_player.primary", {"cover_art", "play_pause", "next"},
                  {"cover_art_secondary_entity=media_player.secondary", "", ""});
  PageMediaStateRuntime media(arena, transport);
  const char* states[]{nullptr, "",        "unknown", "unavailable", "off",
                       "idle",  "playing", "paused",  " BUFFERING "};
  const char* attributes[]{nullptr,          "media_title",          "media_artist",      "media_content_id",
                           "entity_picture", "entity_picture_local", "media_content_type"};
  for (const bool external : {false, true}) {
    for (const char* secondary_state : states) {
      for (const char* attribute : attributes) {
        media.retire_active();
        assert(media.prepare(fixture.store));
        media.activate_prepared(fixture.store);
        media.observed(120, "media_player.primary", "", "playing", false);
        media.observed(120, "media_player.primary", "source", external ? "TV" : "Spotify", false);
        if (secondary_state) media.observed(120, "media_player.secondary", "", secondary_state, false);
        if (attribute) media.observed(120, "media_player.secondary", attribute, "content", false);
        const bool known = secondary_state != nullptr;
        const std::string state = known ? cover_art::normalized_media_source(secondary_state) : "";
        const bool available = known && !state.empty() && state != "unknown" && state != "unavailable";
        const bool usable = cover_art::media_entity_state_usable(state);
        const bool picture = attribute && (std::strcmp(attribute, "entity_picture") == 0 ||
                                           std::strcmp(attribute, "entity_picture_local") == 0);
        const bool content =
            attribute && std::strcmp(attribute, "media_content_type") != 0 &&
            (!picture || cover_art::media_artwork_content_current(known, available, state, true));
        const bool has_content = cover_art::media_entity_content_available(known, available, content);
        const bool selected =
            cover_art::use_secondary_media_entity(external, true, available && usable, has_content);
        PageMediaRouteSnapshot route;
        assert(media.cover_route({120, 0}, route));
        assert(route.secondary_selected == selected && route.external_fallback == (external && !selected));
        assert(route.entity == (selected ? "media_player.secondary" : "media_player.primary"));
        PageMediaSourceSnapshot snapshot;
        assert(media.source_snapshot({120, 0}, "media_player.secondary", snapshot));
        assert(snapshot.state_known == known && snapshot.available == available &&
               snapshot.usable == usable && snapshot.has_content == has_content);
      }
    }
  }
  for (const char* primary_state : states) {
    media.retire_active();
    assert(media.prepare(fixture.store));
    media.activate_prepared(fixture.store);
    if (primary_state) media.observed(120, "media_player.primary", "", primary_state, false);
    const bool known = primary_state != nullptr;
    const std::string state = known ? cover_art::normalized_media_source(primary_state) : "";
    const bool available = known && !state.empty() && state != "unknown" && state != "unavailable";
    PageMediaRouteSnapshot route;
    assert(media.cover_route({120, 0}, route));
    assert(route.clear_artwork == cover_art::media_card_artwork_should_clear(known, available, state, false));
    assert(route.idle_placeholder ==
           cover_art::media_cover_art_idle_placeholder_visible(known, available, state, false, false));
  }
}
void secondary_routes_use_the_shared_graph() {
  Arena arena;
  Transport transport;
  Subscriptions subscriptions(transport);
  PageCardDependencies dependencies({true});
  Fixture first(130, "media_player.primary", {"cover_art", "play_pause", "now_playing"},
                {"cover_art_secondary_entity=media_player.secondary", "", ""},
                {nullptr, "media_player.other", "media_player.secondary"});
  Fixture next(131, "media_player.primary", {"cover_art", "play_pause", "now_playing"},
               {"cover_art_secondary_entity=media_player.secondary", "", ""},
               {nullptr, "media_player.other", "media_player.secondary"});
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime graph(dependencies, arena, subscriptions, {256, 128, 32768}, &media);
  assert(media.prepare(first.store) && graph.prepare(first.store));
  media.activate_prepared(first.store);
  graph.activate_prepared(first.store);
  subscriptions.send("media_player.primary", "", "playing");
  subscriptions.send("media_player.primary", "source", "TV");
  subscriptions.send("media_player.secondary", "", "playing");
  subscriptions.send("media_player.secondary", "media_title", "Programme");
  subscriptions.send("media_player.secondary", "media_artist", "Station");
  subscriptions.send("media_player.secondary", "media_duration", "200");
  subscriptions.send("media_player.secondary", "media_position", "20");
  PageMediaRouteSnapshot route;
  PageMediaPlaybackSnapshot playback;
  assert(media.cover_route({130, 0}, route) && route.secondary_selected);
  assert(media.playback({130, 0}, route.entity, playback) && playback.duration == 200 &&
         playback.seconds == 20);
  const auto channels = subscriptions.count;
  media.loop();
  const auto stable = cpp_allocations;
  for (int i = 0; i < 1000; ++i) {
    subscriptions.send("media_player.primary", "source", i % 2 ? "TV" : "Spotify");
    subscriptions.send("media_player.secondary", "media_title", "Programme");
    media.loop();
    assert(media.cover_route({130, 0}, route) && route.secondary_selected == (i % 2 != 0));
    assert(subscriptions.count == channels);
  }
  assert(cpp_allocations == stable);
  // A current remote picture clears stale input semantics without changing the
  // raw source cache. The renderer must use the model's effective source.
  subscriptions.send("media_player.primary", "", "playing");
  subscriptions.send("media_player.primary", "entity_picture", "/fresh-primary");
  assert(media.effective_source({130, 0}, "media_player.primary", "TV").empty());
  assert(media.cover_route({130, 0}, route) && !route.secondary_selected);
  subscriptions.send("media_player.primary", "source", "TV");
  Subscriptions::Binding late{};
  for (size_t i = 0; i < channels; ++i)
    if (std::strcmp(subscriptions.entries[i].entity, "media_player.secondary") == 0 &&
        std::strcmp(subscriptions.entries[i].attribute, "media_title") == 0)
      late = subscriptions.entries[i];
  assert(late.deliver != nullptr);
  assert(media.prepare(next.store) && graph.prepare(next.store));
  media.activate_prepared(next.store);
  graph.activate_prepared(next.store);
  std::memset(first.storage.data(), 0xA5, first.storage.size());
  late.deliver(late.owner, late.revision, late.channel, "Obsolete title");
  assert(media.cover_route({131, 0}, route) && route.secondary_selected);
  assert(media.current({131, 0}, route.entity, "media_title"));
  bool title_checked = false;
  for (size_t i = 0; i < graph.dependency_count({131, 0}); ++i) {
    const auto value = graph.dependency({131, 0}, i);
    if (value.entity == route.entity && value.attribute == "media_title") {
      assert(value.value == "Programme");
      title_checked = true;
    }
  }
  assert(title_checked);
  assert(!media.current({131, 1}, route.entity, "media_title"));
  assert(subscriptions.count == channels);
  ++transport.connection;
  media.loop();
  graph.revision();
  assert(media.cover_route({131, 0}, route) && !route.secondary_selected);
  subscriptions.send("media_player.primary", "", "playing");
  subscriptions.send("media_player.primary", "source", "TV");
  subscriptions.send("media_player.secondary", "", "playing");
  assert(media.cover_route({131, 0}, route) && !route.secondary_selected);
  subscriptions.send("media_player.secondary", "entity_picture_local", "/fresh-after-reconnect");
  assert(media.cover_route({131, 0}, route) && route.secondary_selected);
  graph.retire_active();
  media.retire_active();
  assert(subscriptions.count == 0 && arena.live.empty());
}
void removed_secondary_cannot_be_read_through_another_card() {
  Arena arena;
  Transport transport;
  Fixture first(140, "media_player.primary", {"cover_art", "play_pause", "now_playing"},
                {"cover_art_secondary_entity=media_player.secondary", "", ""},
                {nullptr, nullptr, "media_player.secondary"});
  Fixture next(141, "media_player.primary", {"cover_art", "play_pause", "now_playing"}, {"", "", ""},
               {nullptr, nullptr, "media_player.secondary"});
  PageMediaStateRuntime media(arena, transport);
  assert(media.prepare(first.store));
  media.activate_prepared(first.store);
  media.observed(140, "media_player.primary", "", "playing", false);
  media.observed(140, "media_player.primary", "source", "TV", false);
  media.observed(140, "media_player.secondary", "", "playing", false);
  media.observed(140, "media_player.secondary", "media_title", "Programme", false);
  PageMediaRouteSnapshot route;
  assert(media.cover_route({140, 0}, route) && route.secondary_selected);
  assert(media.prepare(next.store));
  media.activate_prepared(next.store);
  std::memset(first.storage.data(), 0xA5, first.storage.size());
  assert(media.memory().sources == 2);
  assert(media.cover_route({141, 0}, route) && route.secondary.empty() && route.external_fallback);
  assert(!media.current({141, 0}, "media_player.secondary", "media_title"));
  assert(media.current({141, 2}, "media_player.secondary", "media_title"));
}
void reused_sources_refresh_new_roles() {
  Arena arena;
  Transport transport;
  Fixture playlist(150), details(151, "media_player.room", {"now_playing", "playlist", "playlist"}),
      progress(152, "media_player.room", {"position", "play_pause", "next"});
  PageMediaStateRuntime media(arena, transport);
  assert(media.prepare(playlist.store));
  media.activate_prepared(playlist.store);
  media.observed(150, "media_player.room", "", "playing", false);
  media.observed(150, "media_player.room", "media_content_id", "cached-id", false);
  // Old data from a previous progress role cannot survive an interval during
  // which this source had no progress responsibility/subscriptions.
  media.observed(150, "media_player.room", "media_duration", "42", false);
  media.observed(150, "media_player.room", "media_position", "10", false);
  media.loop();
  assert(transport.requests == 0);
  assert(media.prepare(details.store));
  assert(transport.requests == 0);  // Preparation cannot request network work.
  media.activate_prepared(details.store);
  assert(transport.requests == 0);
  media.loop();
  assert(transport.requests == 1);  // Content ID cannot satisfy newly required title/artist.
  media.loop();
  assert(transport.requests == 1);
  media.observed(151, "media_player.room", "", "idle", false);
  assert(media.prepare(progress.store));
  media.activate_prepared(progress.store);
  PageMediaPlaybackSnapshot playback;
  assert(media.playback({152, 0}, "media_player.room", playback) && !playback.has_duration &&
         !playback.has_position);
  media.loop();
  assert(transport.requests == 1);  // Keep the new-role request pending while idle.
  media.observed(152, "media_player.room", "", "playing", false);
  media.loop();
  assert(transport.requests == 2);
  media.loop();
  assert(transport.requests == 2);
  Fixture before(153, "media_player.primary", {"play_pause", "play_pause", "now_playing"}, {"", "", ""},
                 {nullptr, nullptr, "media_player.secondary"});
  Fixture after(154, "media_player.primary", {"cover_art", "play_pause", "now_playing"},
                {"cover_art_secondary_entity=media_player.secondary", "", ""},
                {nullptr, nullptr, "media_player.secondary"});
  media.retire_active();
  assert(media.prepare(before.store));
  media.activate_prepared(before.store);
  media.observed(153, "media_player.primary", "", "playing", false);
  media.observed(153, "media_player.secondary", "", "playing", false);
  media.observed(153, "media_player.secondary", "media_content_id", "cached-id", false);
  media.loop();
  const auto previous_requests = transport.requests;
  assert(media.prepare(after.store));
  media.activate_prepared(after.store);
  media.loop();
  assert(transport.requests == previous_requests + 1);  // Both role upgrades consolidate into one snapshot.
  media.loop();
  assert(transport.requests == previous_requests + 1);
}
int main() {
  reused_sources_refresh_new_roles();
  removed_secondary_cannot_be_read_through_another_card();
  secondary_routes_use_the_shared_graph();
  routing_policy_parity();
  secondary_routes_and_picture_freshness();
  duplicate_and_legacy_cover_routes();
  external_inputs_are_configuration_owned();
  shared_cache_metadata_lifecycle();
  model_lifecycle();
  playback_survives_navigation_and_replacement();
  only_declared_metadata_is_refreshed();
}
