#include "page_local_sensor_runtime.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>

#include "panel_config_document.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
namespace {
struct Arena : PageArenaAllocator {
  void* allocate(size_t size) override {
    ++calls;
    if (remaining == 0) return nullptr;
    if (remaining > 0) --remaining;
    auto* p = std::malloc(size);
    assert(p);
    live[p] = size;
    return p;
  }
  void release(void* p, size_t size) override {
    assert(live.at(p) == size);
    live.erase(p);
    std::free(p);
  }
  size_t calls{0};
  int remaining{-1};
  std::unordered_map<void*, size_t> live;
};
struct Provider : PageLocalSensorProvider {
  struct Item {
    const char* name;
    bool text;
    float number;
    std::string value;
    uint64_t revision{1};
    bool watched{false};
  };
  std::array<Item, 4> values{{{"temp", false, 21.25f, ""},
                              {"status", true, 0, ""},
                              {"other", false, 44.5f, ""},
                              {"other_text", true, 0, "Other"}}};
  size_t capacity() const override {
    return values.size();
  }
  uint32_t resolve(std::string_view key, bool text) const override {
    for (size_t i = 0; i < values.size(); ++i)
      if (values[i].name == key && values[i].text == text) return i;
    return NO_LOCAL_SENSOR;
  }
  bool watch(uint32_t id) override {
    if (fail_watch) return false;
    values[id].watched = true;
    return true;
  }
  PageLocalSensorSample read(uint32_t id) const override {
    const auto& value = values[id];
    return {value.revision, value.number, value.value};
  }
  size_t callbacks() const override {
    size_t count = 0;
    for (const auto& item : values) count += item.watched;
    return count;
  }
  bool fail_watch{false};
};
struct Fixture {
  Fixture(uint32_t generation, const std::string& number = "temp", const std::string& text = "status",
          bool local = true) {
    const auto& limits = DEVICE_UI_PROFILES[8];
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        local ? number + ";Temperature;;;local;°C;sensor;2;" : ";;;;sensor.ha;;sensor;1;",
        local ? text + ";;;;local;;sensor;text;" : ";;;;sensor.other;;sensor;1;",
        local ? number + ";Again;;;local;;sensor;1;" : ";;;;;;;;"};
    for (size_t i = 0; i < 3; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, generation, storage.data(), storage.size(),
                               &store) == PageStoreStatus::OK);
  }
  std::array<uint8_t, 8192> document{};
  size_t size{0};
  alignas(std::max_align_t) std::array<uint8_t, 32768> storage{};
  PageStore store;
};
float number(PageLocalSensorRuntime& runtime, uint32_t generation, CardId card, std::string_view key) {
  const auto value = runtime.read({generation, card}, key, false);
  assert(value.status == PageEntityStatus::AVAILABLE && value.value.size() == sizeof(float));
  float result;
  std::memcpy(&result, value.value.data(), sizeof(result));
  return result;
}
void cycles() {
  Arena arena;
  Provider provider;
  Fixture first(17), second(18, "other", "other_text"), empty(19, "", "", false),
      missing(20, "missing", "missing_text");
  {
    PageLocalSensorRuntime runtime(arena, provider, {16, 8, 512});
    assert(runtime.prepare(first.store));
    provider.values[0].number = 21.5f;
    ++provider.values[0].revision;
    runtime.activate_prepared(first.store);
    assert(runtime.memory().live_bindings == 2 && provider.callbacks() == 2 &&
           runtime.memory().state.active_channels == 2);
    assert(number(runtime, 17, 0, "temp") == 21.5f);
    provider.values[0].number = 21.25f;
    ++provider.values[0].revision;
    runtime.loop();
    assert(number(runtime, 17, 0, "temp") == 21.25f && number(runtime, 17, 2, "temp") == 21.25f);
    assert(!runtime.install_manual_feed(false) && runtime.install_manual_feed(true));
    assert(page_local_sensor_feed().apply_number("temp", 12.75f));
    runtime.loop();
    assert(number(runtime, 17, 0, "temp") == 12.75f);
    provider.values[0].number = 21.25f;
    ++provider.values[0].revision;
    runtime.loop();
    assert(number(runtime, 17, 0, "temp") == 21.25f);
    {
      PageLocalSensorRuntime other(arena, provider, {16, 8, 512});
      assert(!other.install_manual_feed(true));
    }
    assert(runtime.prepare(empty.store));
    runtime.discard_prepared();
    assert(number(runtime, 17, 0, "temp") == 21.25f);
    assert(runtime.read({17, 1}, "status", true).status != PageEntityStatus::AVAILABLE);
    ++provider.values[1].revision;
    runtime.loop();
    assert(runtime.read({17, 1}, "status", true).status == PageEntityStatus::AVAILABLE &&
           runtime.read({17, 1}, "status", true).value.empty());
    assert(runtime.prepare(first.store));
    runtime.activate_prepared(first.store);
    assert(runtime.read({17, 1}, "status", true).status != PageEntityStatus::AVAILABLE);
    provider.values[1].value = "Raw text: ÉTÉ";
    ++provider.values[1].revision;
    runtime.loop();
    assert(runtime.read({17, 1}, "status", true).value == "Raw text: ÉTÉ");
    provider.values[0].number = std::numeric_limits<float>::quiet_NaN();
    ++provider.values[0].revision;
    runtime.loop();
    assert(std::isnan(number(runtime, 17, 0, "temp")));
    assert(runtime.prepare(second.store));
    assert(runtime.memory().live_bindings == 4);
    runtime.discard_prepared();
    assert(runtime.memory().live_bindings == 2 && number(runtime, 17, 2, "temp") != 44.5f);
    assert(runtime.prepare(second.store));
    runtime.activate_prepared(second.store);
    assert(number(runtime, 18, 0, "other") == 44.5f &&
           runtime.read({17, 0}, "temp", false).status != PageEntityStatus::AVAILABLE);
    const auto calls = arena.calls, live = arena.live.size();
    const auto revision = runtime.revision();
    for (int n = 0; n < 1000; ++n) runtime.loop();
    assert(arena.calls == calls && arena.live.size() == live && runtime.revision() == revision);
    for (int n = 0; n < 1000; ++n) {
      const auto& store = n % 2 ? first.store : second.store;
      assert(runtime.prepare(store));
      runtime.activate_prepared(store);
      runtime.loop();
      assert(runtime.memory().live_bindings == 2 && arena.live.size() == live && provider.callbacks() == 4);
    }
    assert(runtime.prepare(missing.store));
    runtime.activate_prepared(missing.store);
    assert(runtime.memory().live_bindings == 2);
    assert(runtime.read({20, 0}, "missing", false).status != PageEntityStatus::AVAILABLE);
    assert(!runtime.install_manual_feed(false) && runtime.install_manual_feed(true));
    assert(page_local_sensor_feed().apply_number("missing", 9.5f));
    assert(number(runtime, 20, 0, "missing") == 9.5f);
    assert(page_local_sensor_feed().apply_text("missing_text", "Computed raw text"));
    assert(runtime.read({20, 1}, "missing_text", true).value == "Computed raw text");
    assert(!page_local_sensor_feed().apply_number("missing_text", 1) &&
           !page_local_sensor_feed().apply_text("temp", "ignored"));
    assert(runtime.prepare(empty.store));
    runtime.activate_prepared(empty.store);
    assert(arena.live.empty() && runtime.memory().state.active_channels == 0);
    runtime.retire_active();
  }
  assert(arena.live.empty() && !page_local_sensor_feed().owner);
}
void failures() {
  Fixture first(17), second(18, "other", "other_text");
  for (int fail = 0; fail < 4; ++fail) {
    Arena arena;
    Provider provider;
    {
      PageLocalSensorRuntime runtime(arena, provider, {16, 8, 16});
      arena.remaining = fail;
      const bool prepared = runtime.prepare(first.store);
      arena.remaining = -1;
      if (prepared) {
        runtime.activate_prepared(first.store);
        runtime.retire_active();
      } else
        runtime.discard_prepared();
      assert(arena.live.empty());
    }
  }
  Arena arena;
  Provider provider;
  {
    PageLocalSensorRuntime runtime(arena, provider, {16, 8, 16});
    assert(runtime.prepare(first.store));
    runtime.activate_prepared(first.store);
    const auto live = arena.live.size();
    arena.remaining = 0;
    assert(!runtime.prepare(second.store));
    arena.remaining = -1;
    assert(arena.live.size() == live && number(runtime, 17, 0, "temp") == 21.25f);
    provider.fail_watch = true;
    assert(!runtime.prepare(second.store));
    provider.fail_watch = false;
    assert(arena.live.size() == live && runtime.memory().live_bindings == 2);
    provider.values[1].value = std::string(100, 'x');
    ++provider.values[1].revision;
    runtime.loop();
    assert(runtime.read({17, 1}, "status", true).status == PageEntityStatus::EXHAUSTED);
    provider.values[1].value = "ok";
    ++provider.values[1].revision;
    runtime.loop();
    assert(runtime.read({17, 1}, "status", true).value == "ok");
    runtime.retire_active();
  }
  assert(arena.live.empty());
}
}  // namespace
void channel_limit() {
  Arena arena;
  Provider provider;
  Fixture fixture(17);
  {
    PageLocalSensorRuntime runtime(arena, provider, {16, 8, 512}, 8);
    assert(runtime.text_capacity() == 8 && runtime.prepare(fixture.store));
    runtime.activate_prepared(fixture.store);
    provider.values[1].value = "ninebytes";
    ++provider.values[1].revision;
    runtime.loop();
    assert(runtime.read({17, 1}, "status", true).status == PageEntityStatus::EXHAUSTED);
    assert(number(runtime, 17, 0, "temp") == 21.25f);
    runtime.retire_active();
  }
  assert(arena.live.empty());
}
int main() {
  cycles();
  failures();
  channel_limit();
}
