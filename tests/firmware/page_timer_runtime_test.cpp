#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "timer_state_policy.h"
int64_t epoch_now = 0;
time_t controlled_time(time_t* output) {
  const auto value = static_cast<time_t>(epoch_now);
  if (output) *output = value;
  return value;
}
namespace legacy_timer {
namespace esphome {
using StringRef = std::string;
uint32_t now_ms = 100;
uint32_t millis() {
  return now_ms;
}
}  // namespace esphome
struct lv_obj_t {
  std::string text;
  bool checked = false;
};
struct lv_timer_t {
  void* data;
};
struct lv_font_t {};
constexpr int LV_OBJ_FLAG_HIDDEN = 1, LV_PART_MAIN = 0;
int timers = 0;
lv_timer_t* lv_timer_create(void (*)(lv_timer_t*), uint32_t, void* p) {
  ++timers;
  return new lv_timer_t{p};
}
void lv_timer_del(lv_timer_t* t) {
  --timers;
  delete t;
}
void lv_timer_set_repeat_count(lv_timer_t*, int) {}
void* lv_timer_get_user_data(lv_timer_t* t) {
  return t->data;
}
void lv_label_set_display_text(lv_obj_t* o, const char* s) {
  o->text = s;
}
const char* lv_label_get_text(lv_obj_t* o) {
  return o->text.c_str();
}
void lv_obj_add_state(lv_obj_t* o, int) {
  o->checked = true;
}
void lv_obj_clear_state(lv_obj_t* o, int) {
  o->checked = false;
}
int checked_state_updates = 0;
void set_card_checked_state(lv_obj_t* o, bool active) {
  ++checked_state_updates;
  o->checked = active;
}
void lv_obj_add_flag(lv_obj_t*, int) {}
void lv_obj_clear_flag(lv_obj_t*, int) {}
void lv_obj_set_style_text_font(lv_obj_t*, const lv_font_t*, int) {}
const char* espcontrol_i18n(const char* s) {
  return s;
}
using Callback = std::function<void(esphome::StringRef)>;
std::map<std::string, Callback> callbacks;
void* released = nullptr;
struct HaCallbackOwnerScope {
  explicit HaCallbackOwnerScope(void*) {}
};
void ha_release_callbacks_for_owner(void* p) {
  released = p;
  callbacks.clear();
}
void ha_subscribe_state(const std::string&, Callback cb) {
  callbacks["state"] = cb;
}
void ha_subscribe_attribute(const std::string&, const std::string& attr, Callback cb) {
  callbacks[attr] = cb;
}
bool connected = true;
bool ha_api_state_connected() {
  return connected;
}
std::string action;
bool ha_send_entity_action(const std::string&, const char* s) {
  action = s;
  return connected;
}
struct BtnSlot {
  lv_obj_t *icon_lbl, *sensor_container, *sensor_lbl, *unit_lbl, *text_lbl;
};
struct ParsedCfg {
  std::string label;
};
#define time controlled_time
#include "button_grid_timer.h"
#undef time
}  // namespace legacy_timer
using namespace legacy_timer;
#include "page_card_dependencies.h"
#include "page_runtime_resources_group.h"
#include "page_store_runtime.h"
#include "page_timer_runtime.h"
#include "panel_config_document.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
uint32_t now_tick() {
  return esphome::now_ms;
}
int64_t now_epoch() {
  return epoch_now;
}

static size_t cpp_allocations = 0;
void* operator new(size_t bytes) {
  ++cpp_allocations;
  if (void* pointer = std::malloc(bytes ? bytes : 1)) return pointer;
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
void operator delete(void* pointer, std::size_t) noexcept {
  std::free(pointer);
}
void operator delete[](void* pointer, std::size_t) noexcept {
  std::free(pointer);
}
namespace {
struct Arena : PageArenaAllocator {
  std::unordered_map<void*, size_t> live;
  bool fail{false};
  size_t calls{0};
  void* allocate(size_t bytes) override {
    ++calls;
    if (fail) return nullptr;
    auto* pointer = std::malloc(bytes);
    assert(pointer);
    live[pointer] = bytes;
    return pointer;
  }
  void release(void* pointer, size_t bytes) override {
    assert(live.at(pointer) == bytes);
    live.erase(pointer);
    std::free(pointer);
  }
};
struct Subscriptions : PageEntitySubscriptions {
  bool subscribe(const char*, const char*, void*, uint32_t, uint32_t, Deliver) override {
    std::abort();
  }
  void release(void*) override {
    std::abort();
  }
  uint32_t connection_generation() const override {
    return connection;
  }
  uint32_t connection{1};
};
struct Fixture {
  Fixture(uint32_t generation, const std::string& entity = "timer.room", const std::string& type = "timer",
          const std::string& mode = "3", const DeviceUiLimits& limits = DEVICE_UI_PROFILES[8]) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const auto card = entity + ";Room;Auto;Auto;confirm;" + mode + ";" + type + ";;";
    for (size_t i = 0; i < 2; ++i)
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
void parity() {
  esphome::now_ms = 100;
  epoch_now = 0;
  Fixture fixture(17);
  Arena arena;
  Subscriptions subscriptions;
  PageTimerRuntime runtime(arena, subscriptions, now_tick, now_epoch);
  assert(runtime.prepare(fixture.store));
  runtime.activate_prepared(fixture.store);
  lv_obj_t button, value, label;
  TimerCardCtx legacy;
  legacy.entity_id = "timer.room";
  legacy.btn = &button;
  legacy.value_lbl = &value;
  legacy.text_lbl = &label;
  subscribe_timer_card(&legacy);
  auto report = [&](const char* attribute, const char* text) {
    callbacks[attribute[0] ? attribute : "state"](text);
    runtime.observed(17, "timer.room", attribute, text, false);
  };
  uint32_t seed = 1234;
  for (int i = 0; i < 20000; ++i) {
    seed = seed * 1664525 + 1013904223;
    switch (seed % 8) {
      case 0: {
        const char* states[] = {"active", "paused", "idle", "unavailable", "", "ACTIVE"};
        report("", states[(seed >> 8) % 6]);
        break;
      }
      case 1: {
        const char* values[] = {"0:05:00", "1 day, 0:00:00", "0:00:01", "invalid"};
        report("duration", values[(seed >> 8) % 4]);
        break;
      }
      case 2: {
        const char* values[] = {"0:05:00", "0:00:01", "0:00:00", "1:04:03", "2 days, 1:02:03"};
        report("remaining", values[(seed >> 8) % 5]);
        break;
      }
      case 3:
        esphome::now_ms += ((seed >> 8) % 5000);
        if (epoch_now) epoch_now += ((seed >> 8) % 5);
        break;
      case 4: {
        const bool accepted = (seed >> 8) % 2;
        legacy.cancel_requested = legacy.state == "active" && accepted;
        runtime.cancel_sent({17, 0}, accepted);
        break;
      }
      case 5:
        epoch_now = timer_state::parse_finish("2026-05-08T18:34:00Z");
        report("finishes_at", "2026-05-08T18:34:20Z");
        break;
      case 6:
        epoch_now = 0;
        report("finishes_at", "");
        break;
      case 7:
        report("remaining", "0:00:01");
        break;
    }
    runtime.loop();
    timer_card_refresh(&legacy);
    const auto snapshot = runtime.read({17, 0});
    char text[16];
    if (snapshot.mode == PageTimerMode::UNAVAILABLE)
      std::snprintf(text, sizeof(text), "--:--");
    else
      timer_state::format_seconds(snapshot.seconds, text, sizeof(text));
    assert(value.text == text);
    const auto expected = legacy.state == "active"                             ? PageTimerAction::CANCEL
                          : legacy.state == "idle" || legacy.state == "paused" ? PageTimerAction::START
                                                                               : PageTimerAction::NONE;
    assert(runtime.action({17, 0}) == expected);
    assert(snapshot.confirmation && snapshot.confirmation_seconds == 3);
  }
}
void lifecycle() {
  esphome::now_ms = 100;
  epoch_now = 0;
  Arena arena;
  Subscriptions subscriptions;
  Fixture first(17), second(18, "timer.other"), empty(19, "sensor.room", "sensor");
  {
    PageTimerRuntime runtime(arena, subscriptions, now_tick, now_epoch);
    assert(runtime.prepare(first.store));
    runtime.activate_prepared(first.store);
    auto report = [&](const char* attribute, const char* value) {
      runtime.observed(17, "timer.room", attribute, value, false);
    };
    report("duration", "0:05:00");
    report("remaining", "0:00:01");
    report("", "active");
    runtime.cancel_sent({17, 0}, true);
    report("", "idle");
    assert(runtime.read({17, 0}).seconds == 300 && runtime.read({17, 1}).seconds == 0);
    esphome::now_ms += 4999;
    runtime.loop();
    assert(runtime.read({17, 1}).seconds == 0);
    ++esphome::now_ms;
    runtime.loop();
    assert(runtime.read({17, 1}).seconds == 300);
    report("remaining", "0:05:00");
    report("", "active");
    const auto revision = runtime.revision();
    esphome::now_ms += 249;
    runtime.loop();
    // The loop shares a single cadence; allow the already accumulated interval above.
    const auto after = runtime.revision();
    runtime.loop();
    assert(runtime.revision() == after && after >= revision);
    esphome::now_ms += 1000;
    assert(runtime.read({17, 0}).seconds == 299);
    const auto allocations = cpp_allocations, calls = arena.calls;
    for (int i = 0; i < 1000; ++i) {
      report("remaining", "0:05:00");
      report("duration", "0:10:00");
      report("finishes_at", "");
      esphome::now_ms += 250;
      runtime.loop();
      (void)runtime.read({17, 0});
      runtime.cancel_sent({17, 0}, true);
    }
    assert(allocations == cpp_allocations && calls == arena.calls);
    assert(runtime.prepare(second.store));
    runtime.discard_prepared();
    assert(runtime.read({17, 0}).mode == PageTimerMode::ACTIVE);
    arena.fail = true;
    assert(!runtime.prepare(second.store));
    arena.fail = false;
    assert(runtime.read({17, 0}).mode == PageTimerMode::ACTIVE && arena.live.size() == 1);
    ++subscriptions.connection;
    runtime.loop();
    assert(runtime.read({17, 0}).mode == PageTimerMode::UNAVAILABLE);
    for (int i = 0; i < 1000; ++i) {
      const auto& store = i % 2 ? first.store : second.store;
      assert(runtime.prepare(store));
      runtime.activate_prepared(store);
      assert(runtime.memory().sources == 1 && runtime.memory().cards == 2 && arena.live.size() == 1);
    }
    assert(runtime.prepare(empty.store));
    runtime.activate_prepared(empty.store);
    assert(arena.live.empty());
    assert(runtime.action({17, 0}) == PageTimerAction::NONE);
  }
  assert(arena.live.empty());
  for (int limit = 0; limit < 3; ++limit) {
    PageTimerRuntime runtime(arena, subscriptions, now_tick, now_epoch, limit == 0 ? 0 : 32,
                             limit == 1 ? 0 : 512, limit == 2 ? 0 : 2048);
    assert(!runtime.prepare(first.store) && arena.live.empty());
  }
  {
    PageTimerRuntime runtime(arena, subscriptions, nullptr, now_epoch);
    assert(!runtime.prepare(first.store));
  }
  for (const auto& limits : DEVICE_UI_PROFILES) {
    Fixture fixture(17, "timer.room", "timer", "99", limits);
    PageTimerRuntime runtime(arena, subscriptions, now_tick, now_epoch);
    assert(runtime.prepare(fixture.store));
    runtime.activate_prepared(fixture.store);
    assert(runtime.read({17, 0}).confirmation_seconds == 30);
  }
  assert(arena.live.empty());
}
void graph_snapshots() {
  struct Transport : PageEntitySubscriptions {
    struct Binding {
      const char* attribute;
      void* owner;
      uint32_t revision, channel;
      Deliver deliver;
    };
    std::vector<Binding> bindings;
    uint32_t connection{1};
    bool subscribe(const char*, const char* attribute, void* owner, uint32_t revision, uint32_t channel,
                   Deliver deliver) override {
      bindings.push_back({attribute, owner, revision, channel, deliver});
      return true;
    }
    void release(void* owner) override {
      bindings.erase(std::remove_if(bindings.begin(), bindings.end(),
                                    [&](const auto& binding) { return binding.owner == owner; }),
                     bindings.end());
    }
    uint32_t connection_generation() const override {
      return connection;
    }
    void send(const char* attribute, const char* value) {
      for (const auto& binding : bindings)
        if (std::strcmp(binding.attribute, attribute) == 0)
          binding.deliver(binding.owner, binding.revision, binding.channel, value);
    }
  } subscriptions;
  esphome::now_ms = 100;
  epoch_now = 0;
  Fixture fixture(17), empty(18, "sensor.room", "sensor");
  Arena arena;
  PageCardDependencies dependencies;
  {
    PageTimerRuntime runtime(arena, subscriptions, now_tick, now_epoch);
    PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 2048}, &runtime);
    PageRuntimeResourcesGroup<2> resources({&runtime, &state});
    PageStoreRuntime store(DEVICE_UI_PROFILES[8], arena, 32768, &resources);
    const auto prepare = [&]() {
      return store.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size);
    };
    const auto activate = [&](uint32_t generation) {
      return store.activate_prepared(generation, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                     fixture.size);
    };
    assert(prepare() && activate(0));
    subscriptions.send("duration", "0:05:00");
    subscriptions.send("remaining", "0:05:00");
    subscriptions.send("", "active");
    esphome::now_ms += 10000;
    assert(runtime.read({0, 0}).seconds == 290);
    assert(prepare());
    store.discard_prepared();
    assert(runtime.read({0, 0}).seconds == 290);
    esphome::now_ms += 10000;
    assert(prepare() && activate(2));
    assert(runtime.read({2, 0}).seconds == 280 &&
           !runtime.read({0, 0}).confirmation);  // A copied remaining value must retain its actual anchor.
    subscriptions.send("remaining", "0:05:00");
    assert(runtime.read({2, 0}).seconds == 300);  // A live repeat resets it.
    ++subscriptions.connection;
    runtime.loop();
    assert(runtime.action({2, 0}) == PageTimerAction::NONE);
    subscriptions.send("remaining", "0:00:20");
    subscriptions.send("", "active");
    assert(runtime.read({2, 0}).seconds == 20);
    assert(store.prepare(PANEL_CONFIG_DOCUMENT_VERSION, empty.document.data(), empty.size));
    assert(store.activate_prepared(3, PANEL_CONFIG_DOCUMENT_VERSION, empty.document.data(), empty.size));
    assert(runtime.memory().active_bytes == 0 && runtime.action({2, 0}) == PageTimerAction::NONE);
  }
  assert(arena.live.empty() && subscriptions.bindings.empty());
}
void wall_clock_and_wrap() {
  Arena arena;
  Subscriptions subscriptions;
  Fixture fixture(17);
  PageTimerRuntime runtime(arena, subscriptions, now_tick, now_epoch);
  assert(runtime.prepare(fixture.store));
  runtime.activate_prepared(fixture.store);
  esphome::now_ms = UINT32_MAX - 5;
  epoch_now = 0;
  runtime.observed(17, "timer.room", "remaining", "0:00:10", false);
  runtime.observed(17, "timer.room", "", "active", false);
  esphome::now_ms += 1500;
  assert(runtime.read({17, 0}).seconds == 9);
  epoch_now = timer_state::parse_finish("2026-05-08T18:34:00Z");
  runtime.observed(17, "timer.room", "finishes_at", "2026-05-08T18:34:20Z", false);
  assert(runtime.read({17, 0}).seconds == 20);
  epoch_now += 7;
  assert(runtime.read({17, 0}).seconds == 13);
  runtime.observed(17, "timer.room", "finishes_at", "9999-01-01T00:00:00Z", false);
  assert(runtime.read({17, 0}).seconds == INT32_MAX);
}
}  // namespace
int main() {
  parity();
  lifecycle();
  graph_snapshots();
  wall_clock_and_wrap();
}
