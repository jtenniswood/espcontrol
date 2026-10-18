#include "page_media_state_runtime.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>

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
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
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
  explicit Fixture(uint32_t generation = 17, const std::string& entity = "media_player.room") {
    const auto& limits = DEVICE_UI_PROFILES[8];
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    for (uint8_t slot = 1; slot <= 3; ++slot) {
      const auto text = entity + ";;;;playlist;;media;;playlist_content_id=abcd";
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
int main() {
  model_lifecycle();
}
