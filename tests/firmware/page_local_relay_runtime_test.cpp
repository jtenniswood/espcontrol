#include "page_local_relay_runtime.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

#include "panel_config_document.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
namespace {
struct Arena : PageArenaAllocator {
  void* allocate(size_t bytes) override {
    ++calls;
    if (fail) return nullptr;
    auto* p = std::malloc(bytes);
    assert(p);
    live[p] = bytes;
    return p;
  }
  void release(void* p, size_t bytes) override {
    assert(live.at(p) == bytes);
    live.erase(p);
    std::free(p);
  }
  bool fail{false};
  size_t calls{0};
  std::unordered_map<void*, size_t> live;
};
struct Provider : PageLocalRelayProvider {
  PageLocalRelaySnapshot read(std::string_view key) const override {
    return key == "relay_one"   ? PageLocalRelaySnapshot{first, "Hall Relay"}
           : key == "relay_two" ? PageLocalRelaySnapshot{second, "Other Relay"}
                                : PageLocalRelaySnapshot{};
  }
  bool first{false}, second{true};
};
struct Fixture {
  Fixture(uint32_t generation, const char* key = "relay_one", bool push = false, bool empty = false) {
    const auto& limits = DEVICE_UI_PROFILES[8];
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string card =
        empty ? ";;;;sensor.room;;sensor;1;"
              : std::string(key) + ";;Auto;Auto;" + (push ? "push" : "switch") + ";;internal;;";
    for (int i = 0; i < 2; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) ==
             PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, generation, storage.data(), storage.size(),
                               &store) == PageStoreStatus::OK);
  }
  std::array<uint8_t, 8192> document{};
  size_t size{0};
  alignas(std::max_align_t) std::array<uint8_t, 32768> storage{};
  PageStore store;
};
void lifetime() {
  Arena arena;
  Provider provider;
  Fixture first(17), second(18, "relay_two"), push(19, "relay_one", true), empty(20, "", false, true),
      missing(21, "unregistered");
  {
    PageLocalRelayRuntime runtime(arena, provider, 2, 64);
    assert(!runtime.install(false) && runtime.install(true));
    assert(runtime.prepare(first.store));
    provider.first = true;
    runtime.activate_prepared(first.store);
    assert(runtime.on(17, "relay_one") && runtime.memory().active_sources == 1 &&
           runtime.registered_label("relay_one") == "Hall Relay");
    assert(!runtime.on(18, "relay_one") && !runtime.on(17, "unknown"));
    const auto calls = arena.calls;
    const auto revision = runtime.revision();
    for (int n = 0; n < 1000; ++n) page_local_relay_feed().notify("relay_one", true);
    assert(arena.calls == calls && runtime.revision() == revision);
    page_local_relay_feed().notify("relay_one", false);
    assert(!runtime.on(17, "relay_one"));
    assert(runtime.prepare(second.store));
    page_local_relay_feed().notify("relay_one", true);
    runtime.discard_prepared();
    assert(runtime.on(17, "relay_one") && arena.live.size() == 1);
    assert(runtime.prepare(second.store));
    runtime.activate_prepared(second.store);
    assert(runtime.on(18, "relay_two") && !runtime.on(17, "relay_one"));
    for (int n = 0; n < 1000; ++n) {
      const auto& store = n % 2 ? first.store : second.store;
      assert(runtime.prepare(store));
      runtime.activate_prepared(store);
      assert(runtime.memory().active_sources == 1 && arena.live.size() == 1);
    }
    assert(runtime.prepare(missing.store));
    runtime.activate_prepared(missing.store);
    page_local_relay_feed().notify("unregistered", true);
    assert(runtime.on(21, "unregistered"));
    assert(runtime.prepare(push.store));
    runtime.activate_prepared(push.store);
    assert(runtime.memory().active_sources == 0 && arena.live.empty());
    const auto before = runtime.revision();
    page_local_relay_feed().notify("relay_one", true);
    assert(runtime.revision() == before);
    assert(runtime.prepare(empty.store));
    runtime.activate_prepared(empty.store);
    assert(arena.live.empty());
    {
      PageLocalRelayRuntime other(arena, provider);
      assert(!other.install(true));
    }
    runtime.retire_active();
  }
  assert(arena.live.empty() && !page_local_relay_feed().owner);
  page_local_relay_feed().notify("relay_one", true);
}
void failures() {
  Arena arena;
  Provider provider;
  Fixture first(17), second(18, "relay_two");
  {
    PageLocalRelayRuntime runtime(arena, provider, 1, 16);
    assert(runtime.prepare(first.store));
    runtime.activate_prepared(first.store);
    arena.fail = true;
    assert(!runtime.prepare(second.store));
    arena.fail = false;
    assert(arena.live.size() == 1 && runtime.memory().active_sources == 1);
    assert(runtime.prepare(second.store));
    runtime.discard_prepared();
    assert(arena.live.size() == 1);
    runtime.retire_active();
  }
  for (const auto& limits : {std::pair<size_t, size_t>{0, 64}, {1, 3}}) {
    PageLocalRelayRuntime runtime(arena, provider, limits.first, limits.second);
    assert(!runtime.prepare(first.store));
    assert(arena.live.empty());
  }
}
}  // namespace
int main() {
  lifetime();
  failures();
}
