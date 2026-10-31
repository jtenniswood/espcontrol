#include "native_scene_test_support.h"

struct Arena : PageArenaAllocator {
  size_t live{0};
  bool fail{false};
  void* allocate(size_t n) override {
    if (fail) return nullptr;
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
struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> memory{};
  PageStore store;
  explicit Document(uint32_t generation, bool dormant = false) {
    const auto& limits = DEVICE_UI_PROFILES[8];
    PanelConfigWriter writer(bytes.data(), bytes.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {"fan.room;Fan;Auto;Auto;;;fan_switch;;",
                                "fan.room;Speed;Auto;Auto;;;fan_speed;;",
                                "light.room;Temperature;Auto;Auto;;2000-6500;light_temperature;color;",
                                "number.room;Number;Auto;Auto;;;slider;;",
                                "sensor.date;Calendar;Auto;Auto;;;calendar;datetime;",
                                "local_value;Local;Auto;Auto;local;;sensor;1;",
                                "relay;Relay;Auto;Auto;;;internal;;",
                                "alarm_control_panel.home;Alarm;Auto;Auto;;;alarm;;",
                                ";Children;Auto;Auto;;;subpage;;"};
    for (size_t i = 0; i < 9; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    if (dormant) {
      const std::string retained[] = {
        "fan.room;Dormant;Auto;Auto;;;fan_switch;;",
        "fan.room;Dormant;Auto;Auto;;;fan_speed;;",
        "light.room;Dormant;Auto;Auto;;2000-6500;light_temperature;color;",
        "number.room;Dormant;Auto;Auto;;;slider;;",
        "sensor.date;Dormant;Auto;Auto;;;calendar;datetime;",
        "local_value;Dormant;Auto;Auto;local;;sensor;1;",
        "relay;Dormant;Auto;Auto;;;internal;;",
        "alarm_control_panel.home;Dormant;Auto;Auto;;;alarm;;",
        "media_player.room;Dormant;Auto;Auto;control_modal;;media;;",
        "weather.dormant;Dormant;Auto;Auto;;;weather_forecast;;",
        "UTC;Dormant;Auto;Auto;;;timezone;;",
        "cover.dormant;Dormant;Auto;Auto;;;garage;;",
        "lock.dormant;Dormant;Auto;Auto;;;lock;;",
        "cover.dormant;Dormant;Auto;Auto;toggle;;cover;;",
        "timer.dormant;Dormant;Auto;Auto;;;timer;;",
        "fan.dormant;Dormant;Auto;Auto;;;fan_switch;;",
        "number.dormant;Dormant;Auto;Auto;;;slider;;",
        "missing_local;Dormant;Auto;Auto;local;;sensor;1;",
        "relay_dormant;Dormant;Auto;Auto;;;internal;;",
        "media_player.dormant;Dormant;Auto;Auto;play_pause;;media;;",
        "sensor.dormant;Dormant;Auto;Auto;;;calendar;;",
        "alarm_control_panel.dormant;Dormant;Auto;Auto;;;alarm;;",
        "future.dormant;Opaque;Auto;Auto;;;future_card;;future=x"};
      static_assert(std::size(retained) == 23);
      for (size_t i = 0; i < std::size(retained); ++i)
        assert(writer.append_button(i + 10, reinterpret_cast<const uint8_t*>(retained[i].data()), retained[i].size()) == PanelConfigStatus::OK);
    }
    const std::string children =
        "~B,1,2|fan_switch,fan.room,Hidden fan,Auto,Auto,,,,|M,media_player.room,Hidden "
        "media,Auto,Auto,play_pause,,,";
    assert(writer.append_subpage(9, reinterpret_cast<const uint8_t*>(children.data()), children.size()) ==
           PanelConfigStatus::OK);
    size_t size = 0;
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, generation, memory.data(), memory.size(),
                               &store) == PageStoreStatus::OK);
  }
};
PageNativeStateBudget budget() { return {128, 16384, {1024, 512, 65536}, {128, 128, 8192}}; }
void observations() {
  Arena arena;
  Document old(17), next(18);
  {
    PageNativeSceneState state(arena, budget(), page_timezone_resolver(), {}, nullptr);
    assert(!state.ready() && !state.prepare(old.store) && !state.initialize(false));
    calendar_count = 1;
    assert(!state.initialize(true));
    calendar_count = 0;
    assert(state.initialize(true) && state.initialize(true));
    assert(calendar_state_events().owner && timezone_state_events().owner);
    assert(state.prepare(old.store));
    state.activate_prepared(old.store);
    const auto registrations = coordinator.entries.size();
    assert(registrations && page_local_sensor_feed().owner == &state.local &&
           page_local_relay_feed().owner == &state.relays);
    coordinator.report("fan.room", "", "on");
    coordinator.report("fan.room", "percentage", "65");
    coordinator.report("light.room", "color_temp_kelvin", "4200");
    coordinator.report("light.room", "", "on");
    coordinator.report("number.room", "min", "0");
    coordinator.report("number.room", "max", "100");
    coordinator.report("number.room", "step", "0.5");
    coordinator.report("number.room", "", "12.5");
    coordinator.report("sensor.date", "", "2026-10-26 18:30:00");
    state.loop();
    assert(state.fans.read({17, 0}).on && state.percent.read({17, 1}).percent == 65);
    assert(state.temperature.read({17, 2}).fill_kelvin == 4200);
    assert(state.number.read({17, 3}).ready && state.number.read({17, 3}).metadata.step == 0.5 &&
           state.number.read({17, 3}).position == 25);
    assert(state.calendar.read({17, 4}).day == 26);
    assert(state.local.read({17, 5}, "local_value", false).status == PageEntityStatus::AVAILABLE);
    assert(page_local_sensor_feed().apply_number("local_value", 27));
    const auto sample = state.local.read({17, 5}, "local_value", false);
    float local_value = 0;
    assert(sample.value.size() == sizeof(local_value));
    std::memcpy(&local_value, sample.value.data(), sizeof(local_value));
    assert(local_value == 27);
    page_local_relay_feed().notify("relay", true);
    assert(state.relays.on(17, "relay"));
    const auto* child = old.store.page(1);
    assert(child && state.fans.read({17, child->first_card}).on);
    arena.fail = true;
    assert(!state.prepare(next.store));
    arena.fail = false;
    assert(state.percent.read({17, 1}).percent == 65 && coordinator.entries.size() == registrations);
    assert(state.prepare(next.store));
    state.discard_prepared();
    assert(coordinator.entries.size() == registrations);
    assert(state.prepare(next.store));
    state.activate_prepared(next.store);
    old.memory.fill(0xDA);
    assert(state.percent.read({18, 1}).percent == 65 && coordinator.entries.size() == registrations);
    ++connection;
    state.loop();
    assert(state.graph.read_attribute({18, 0}, "fan.room", {}).status != PageEntityStatus::AVAILABLE);
    coordinator.report("fan.room", "", "off");
    assert(!state.fans.read({18, 0}).on);
    state.stop();
    state.stop();
    assert(!state.ready() && !state.initialize(true) && !state.prepare(next.store));
    assert(!calendar_state_events().owner && !timezone_state_events().owner &&
           !page_local_sensor_feed().owner && !page_local_relay_feed().owner);
  }
  assert(coordinator.entries.empty());
}
void dormant_families() {
  Arena arena; Document baseline(101), retained(102, true);
  assert(retained.store.page(0)->card_count == 32 && retained.store.page(0)->runtime_card_count == 9);
  assert(retained.store.field(*retained.store.card(31), CardField::TYPE) == "future_card");
  assert(!retained.store.runtime_card(31));
  auto registrations = []() {
    std::vector<std::string> keys;
    for (const auto& entry : coordinator.entries) keys.push_back(entry.entity + "/" + entry.attribute);
    std::sort(keys.begin(), keys.end()); return keys;
  };
  {
    PageNativeSceneState state(arena, budget(), page_timezone_resolver(), {}, nullptr);
    assert(state.initialize(true) && state.prepare(baseline.store)); state.activate_prepared(baseline.store);
    const auto counts = [&]() {
      return std::array<size_t, 13>{state.fans.memory().sources, state.percent.memory().sources,
        state.temperature.memory().sources, state.number.memory().sources, state.calendar.memory().active_sources,
        state.local.memory().live_bindings, state.relays.memory().active_sources, state.alarms.memory().active_sources,
        state.media.memory().sources, state.forecasts.memory().sources, state.timezones.memory().active_cards,
        state.timers.memory().sources, state.access.memory().histories};
    };
    const auto before = counts();
    const auto keys = registrations();
    coordinator.report("fan.room", "", "on");
    assert(state.prepare(retained.store)); state.activate_prepared(retained.store);
    baseline.memory.fill(0xDA);
    assert(counts() == before && registrations() == keys);
    const auto child = retained.store.page(1)->first_card;
    assert(state.fans.read({102, 0}).on && state.fans.read({102, child}).on);
    assert(!state.fans.read({102, 9}).on && !state.fans.read({101, 9}).on);
    assert(state.percent.read({102, 10}).kind == PagePercentSliderKind::NONE);
    assert(!state.number.read({102, 12}).ready && !state.calendar.read({102, 13}).date_valid);
    assert(state.local.read({102, 14}, "local_value", false).status != PageEntityStatus::AVAILABLE);
    state.stop();
  }
  assert(coordinator.entries.empty() && !page_local_sensor_feed().owner && !calendar_state_events().owner);
}
int main() {
  observations();
  dormant_families();
  for (int cycle = 0; cycle < 1000; ++cycle) {
    Arena arena;
    Document document(17);
    {
      PageNativeSceneState state(arena, budget(), page_timezone_resolver(), {}, nullptr);
      assert(state.initialize(true) && state.prepare(document.store));
      state.activate_prepared(document.store);
      state.loop();
      state.stop();
    }
    assert(coordinator.entries.empty() && !page_local_sensor_feed().owner && !calendar_state_events().owner);
  }
}
