#include "page_forecast_runtime.h"

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
void* operator new(size_t size) {
  ++cpp_allocations;
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
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
namespace {
struct Arena : PageArenaAllocator {
  std::unordered_map<void*, size_t> live;
  size_t allocations = 0;
  bool fail = false;
  void* allocate(size_t size) override {
    ++allocations;
    if (fail) return nullptr;
    void* p = std::malloc(size);
    assert(p);
    live[p] = size;
    return p;
  }
  void release(void* p, size_t size) override {
    assert(live.at(p) == size);
    live.erase(p);
    std::free(p);
  }
};
struct Transport : PageForecastRequests {
  struct Delivery {
    void* owner = nullptr;
    PageForecastToken token;
    Receive receive = nullptr;
  };
  Delivery delivery;
  std::string entity;
  bool active = false, online = true, accepted = true, cancel_delivers = true;
  uint32_t clock = 100, connection = 1;
  size_t starts = 0, cancels = 0;
  std::function<void(std::string_view)> on_start;
  const char* synchronous = nullptr;
  bool connected() const override {
    return online;
  }
  uint32_t connection_generation() const override {
    return connection;
  }
  uint32_t now_ms() const override {
    return clock;
  }
  bool start(void* owner, PageForecastToken token, std::string_view value, Receive receive) override {
    assert(!active);
    ++starts;
    delivery = {owner, token, receive};
    entity = value;
    active = accepted;
    if (on_start) on_start(value);
    if (synchronous && active) reply(synchronous);
    return accepted;
  }
  void cancel(void* owner, PageForecastToken token) override {
    ++cancels;
    if (active && delivery.owner == owner && delivery.token.request == token.request) {
      const auto old = take();
      if (cancel_delivers) old.receive(old.owner, old.token, {}, false);
    }
  }
  Delivery take() {
    assert(active);
    active = false;
    return delivery;
  }
  void reply(std::string_view payload, bool success = true) {
    const auto old = take();
    old.receive(old.owner, old.token, payload, success);
  }
  void advance(uint32_t ms) {
    clock += ms;
  }
};
const DeviceUiLimits& large_profile() {
  for (const auto& limits : DEVICE_UI_PROFILES)
    if (limits.slot_count == 20) return limits;
  std::abort();
}
struct Fixture {
  Fixture(size_t count = 2, uint32_t generation = 17, bool duplicate = true,
          const std::string& prefix = "weather.room") {
    const auto& limits = large_profile();
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    for (size_t i = 0; i < count; ++i) {
      const std::string text = prefix + std::to_string(duplicate ? 0 : i) + ";;;;;;weather;daily;";
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(text.data()), text.size()) ==
             PanelConfigStatus::OK);
    }
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, generation, storage.data(), storage.size(),
                               &store) == PageStoreStatus::OK);
  }
  std::array<uint8_t, 8192> document{};
  size_t size = 0;
  alignas(std::max_align_t) std::array<uint8_t, 65536> storage{};
  PageStore store;
};
constexpr const char* PAYLOAD =
    "WF1|2026-10-17|2026-10-18|°C|mm|km/"
    "h\n2026-10-17|sunny|20|10|0|0|1|50\n2026-10-18|rainy|18|9|20|1|2|75\n";
void lifecycle() {
  Fixture fixture;
  Arena arena;
  Transport transport;
  {
    PageForecastRuntime runtime(arena, transport);
    assert(runtime.prepare(fixture.store));
    assert(transport.starts == 0);
    const auto before = cpp_allocations;
    runtime.activate_prepared(fixture.store);
    assert(cpp_allocations == before && runtime.memory().sources == 1 && arena.live.size() == 1);
    runtime.loop();
    assert(transport.starts == 0);
    transport.advance(PageForecastRuntime::READY_DELAY_MS);
    runtime.loop();
    assert(transport.starts == 1 && runtime.memory().pending == 1);
    PageForecastSnapshot snapshot;
    assert(runtime.snapshot({17, 0}, "weather.room0", snapshot) && snapshot.loading);
    assert(!runtime.snapshot({17, 2}, "weather.room0", snapshot) &&
           !runtime.snapshot({18, 0}, "weather.room0", snapshot));
    transport.reply(PAYLOAD);
    assert(runtime.snapshot({17, 1}, "weather.room0", snapshot) && snapshot.forecast.count == 2 &&
           !snapshot.loading && !snapshot.stale);
    const auto reserved = arena.allocations;
    const auto query_allocations = cpp_allocations;
    for (int i = 0; i < 1000; ++i) {
      runtime.loop();
      assert(runtime.snapshot({17, 0}, "weather.room0", snapshot));
    }
    assert(cpp_allocations == query_allocations && transport.starts == 1 && arena.allocations == reserved);
    for (int i = 0; i < 1000; ++i) {
      runtime.refresh();
      runtime.loop();
      assert(transport.active && runtime.memory().pending == 1);
      transport.reply(PAYLOAD);
      runtime.loop();
      assert(runtime.memory().pending == 0 && runtime.memory().completed == size_t(i + 2) &&
             arena.allocations == reserved && arena.live.size() == 1);
    }
    runtime.refresh();
    runtime.loop();
    const auto old = transport.take();
    transport.online = false;
    ++transport.connection;
    const auto stale_before = cpp_allocations;
    old.receive(old.owner, old.token, PAYLOAD, true);
    assert(cpp_allocations == stale_before && runtime.memory().stale_responses == 1);
    runtime.loop();
    assert(runtime.snapshot({17, 0}, "weather.room0", snapshot) && snapshot.stale && !snapshot.loading);
    transport.online = true;
    ++transport.connection;
    runtime.loop();
    transport.advance(PageForecastRuntime::READY_DELAY_MS);
    runtime.loop();
    assert(transport.active);
    transport.reply(PAYLOAD);
    assert(runtime.snapshot({17, 0}, "weather.room0", snapshot) && !snapshot.stale);
    runtime.refresh();
    runtime.loop();
    transport.reply("bad");
    assert(runtime.snapshot({17, 0}, "weather.room0", snapshot) && snapshot.stale &&
           snapshot.forecast.count == 2);
    const auto requests = transport.starts;
    runtime.loop();
    assert(transport.starts == requests);
    transport.advance(PageForecastRuntime::RETRY_MS);
    runtime.loop();
    assert(transport.active);
    transport.reply(PAYLOAD);
    runtime.refresh();
    runtime.loop();
    transport.advance(PageForecastRuntime::TIMEOUT_MS);
    runtime.loop();
    assert(runtime.memory().timeouts == 1);
    runtime.loop();
    assert(!transport.active);
    transport.advance(PageForecastRuntime::RETRY_MS);
    transport.synchronous = PAYLOAD;
    runtime.loop();
    assert(runtime.memory().pending == 0);
    transport.synchronous = nullptr;
    runtime.retire_active();
    runtime.loop();
    assert(arena.live.empty());
  }
  assert(!transport.active && arena.live.empty());
}
void preparation() {
  Fixture first, next(2, 18), maximum(8, 19, false), over(9, 20, false), too_many_cards(17, 23), empty(0, 21),
      invalid(1, 22, false, "sensor.room");
  Arena arena;
  Transport transport;
  {
    PageForecastRuntime runtime(arena, transport);
    assert(runtime.prepare(first.store));
    runtime.activate_prepared(first.store);
    transport.advance(PageForecastRuntime::READY_DELAY_MS);
    runtime.loop();
    transport.reply(PAYLOAD);
    PageForecastSnapshot snapshot;
    arena.fail = true;
    assert(!runtime.prepare(next.store));
    arena.fail = false;
    assert(runtime.snapshot({17, 0}, "weather.room0", snapshot) && snapshot.forecast.count == 2);
    assert(runtime.prepare(over.store));
    runtime.activate_prepared(over.store);
    assert(runtime.memory().sources == 8);
    assert(runtime.snapshot({20, 7}, "weather.room7", snapshot) &&
           !runtime.snapshot({20, 8}, "weather.room8", snapshot));
    assert(runtime.prepare(too_many_cards.store));
    runtime.activate_prepared(too_many_cards.store);
    assert(runtime.memory().sources == 1 && runtime.snapshot({23, 15}, "weather.room0", snapshot) &&
           !runtime.snapshot({23, 16}, "weather.room0", snapshot));
    assert(runtime.prepare(maximum.store));
    runtime.discard_prepared();
    assert(runtime.memory().sources == 1 && arena.live.size() == 1);
    for (int i = 0; i < 1000; ++i) {
      const auto& fixture = i % 2 ? first : next;
      assert(runtime.prepare(fixture.store));
      const auto before = cpp_allocations;
      runtime.activate_prepared(fixture.store);
      assert(cpp_allocations == before && arena.live.size() == 1);
      assert(runtime.snapshot({fixture.store.generation(), 0}, "weather.room0", snapshot) &&
             snapshot.forecast.count == 2);
    }
    assert(runtime.prepare(invalid.store));
    runtime.activate_prepared(invalid.store);
    assert(runtime.memory().sources == 0 && runtime.memory().active_bytes == 0);
    assert(runtime.prepare(empty.store));
    runtime.activate_prepared(empty.store);
    assert(runtime.memory().active_bytes == 0 && arena.live.empty());
  }
}
void replacement_during_start() {
  Fixture first, next(2, 18, false, "weather.after");
  Arena arena;
  Transport transport;
  {
    PageForecastRuntime runtime(arena, transport);
    assert(runtime.prepare(first.store));
    runtime.activate_prepared(first.store);
    transport.advance(PageForecastRuntime::READY_DELAY_MS);
    Transport::Delivery late;
    transport.on_start = [&](std::string_view entity) {
      late = transport.delivery;
      assert(runtime.prepare(next.store));
      const auto before = cpp_allocations;
      runtime.activate_prepared(next.store);
      assert(cpp_allocations == before && runtime.memory().retired_bytes > 0);
      std::memset(first.storage.data(), 0xA5, first.storage.size());
      runtime.loop();
      assert(entity == "weather.room0" && runtime.memory().retired_bytes > 0);
    };
    runtime.loop();
    transport.on_start = {};
    assert(runtime.memory().retired_bytes == 0 && arena.live.size() == 1);
    const auto before = cpp_allocations;
    late.receive(late.owner, late.token, PAYLOAD, true);
    assert(cpp_allocations == before && runtime.memory().stale_responses == 1);
    runtime.loop();
    assert(!transport.active);
    transport.advance(PageForecastRuntime::READY_DELAY_MS);
    runtime.loop();
    assert(transport.active && transport.entity == "weather.after0");
    transport.reply(PAYLOAD);
    runtime.refresh();
    transport.on_start = [&](std::string_view) {
      Fixture last(0, 19);
      assert(runtime.prepare(last.store));
      runtime.activate_prepared(last.store);
      runtime.retire_active();
      assert(runtime.memory().retired_bytes > 0);
    };
    runtime.loop();
    transport.on_start = {};
    runtime.loop();
    assert(arena.live.empty());
  }
  assert(!transport.active && arena.live.empty());
}
void wrap_and_send_failure() {
  Fixture fixture;
  Arena arena;
  Transport transport;
  transport.clock = UINT32_MAX - 5000;
  {
    PageForecastRuntime runtime(arena, transport);
    assert(runtime.prepare(fixture.store));
    runtime.activate_prepared(fixture.store);
    transport.advance(PageForecastRuntime::READY_DELAY_MS - 1);
    runtime.loop();
    assert(!transport.active);
    transport.advance(1);
    transport.accepted = false;
    runtime.loop();
    assert(runtime.memory().failures == 1 && runtime.memory().pending == 0);
    transport.accepted = true;
    transport.advance(PageForecastRuntime::RETRY_MS);
    runtime.loop();
    transport.reply(std::string(weather::MAX_PAYLOAD_BYTES + 1, 'x'));
    assert(runtime.memory().failures == 2);
    runtime.retire_active();
    runtime.loop();
  }
  assert(arena.live.empty());
}
void no_hidden_setup_allocation() {
  struct StaticArena : PageArenaAllocator {
    void* live = nullptr;
    size_t bytes = 0;
    void* allocate(size_t size) override {
      assert(!live);
      bytes = size;
      return live = std::malloc(size);
    }
    void release(void* p, size_t size) override {
      assert(p == live && size == bytes);
      std::free(p);
      live = nullptr;
    }
  } arena;
  const std::string prefix = "weather." + std::string(180, 'a');
  Fixture fixture(1, 17, false, prefix);
  Transport transport;
  {
    PageForecastRuntime runtime(arena, transport);
    const auto before = cpp_allocations;
    assert(runtime.prepare(fixture.store));
    assert(cpp_allocations == before);
    runtime.activate_prepared(fixture.store);
    assert(cpp_allocations == before);
    runtime.retire_active();
    assert(cpp_allocations == before && !arena.live);
  }
}

}  // namespace
int main() {
  no_hidden_setup_allocation();
  lifecycle();
  preparation();
  replacement_during_start();
  wrap_and_send_failure();
}
