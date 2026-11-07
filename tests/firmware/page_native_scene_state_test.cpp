#include "native_scene_test_support.h"
#include "nested_page_fixture.h"
#include <cstdio>

struct Arena : PageArenaAllocator {
  size_t live{0}, bytes{0};
  bool fail{false};
  void* allocate(size_t n) override {
    if (fail) return nullptr;
    auto* result = std::malloc(n);
    assert(result);
    ++live;
    bytes += n;
    return result;
  }
  void release(void* p, size_t n) override {
    assert(live && bytes >= n);
    --live;
    bytes -= n;
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
void maximum_first_level_climate_fits_metadata_ceiling() {
  for (const auto& limits : DEVICE_UI_PROFILES) {
    std::vector<uint8_t> bytes(128 * 1024);
    std::vector<uint64_t> storage(limits.page_store_bytes / sizeof(uint64_t));
    PanelConfigWriter writer(bytes.data(), bytes.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                       std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string folder = ";Folder;Auto;Auto;;;subpage;;indicator";
    for (unsigned parent = 1; parent <= limits.max_first_level_subpages; ++parent) {
      assert(writer.append_button(parent, reinterpret_cast<const uint8_t*>(folder.data()), folder.size()) == PanelConfigStatus::OK);
      std::string children = "~B";
      for (unsigned slot = 1; slot <= limits.cards_per_subpage; ++slot)
        children += "," + std::to_string(slot);
      for (unsigned slot = 1; slot <= limits.cards_per_subpage; ++slot)
        children += "|HC,climate.maximum_" + std::to_string(parent) + "_" + std::to_string(slot) +
                    ",Climate,Auto,Auto,,,,";
      assert(children.size() <= limits.legacy_subpage_byte_ceiling());
      assert(writer.append_subpage(parent, reinterpret_cast<const uint8_t*>(children.data()), children.size()) == PanelConfigStatus::OK);
    }
    size_t size = 0;
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    PageStore store;
    assert(PageStore::stage_v1(bytes.data(), size, limits, 17, storage.data(),
                              storage.size() * sizeof(uint64_t), &store) == PageStoreStatus::OK);
    Arena arena;
    PageNativeSceneState state(arena, page_native_state_budget(limits), page_timezone_resolver(), {}, nullptr);
    assert(state.initialize(true) && state.prepare(store));
    const auto required = state.metadata_memory().staged_metadata_bytes;
    std::fprintf(stderr, "%s first-level climate metadata: %zu / %zu bytes\n", limits.device_profile,
                 required, limits.state_metadata_bytes);
    assert(required <= limits.state_metadata_bytes);
    state.discard_prepared();
    state.stop();
    assert(!arena.live && !arena.bytes && coordinator.entries.empty());
  }
}
void unchanged_dashboards_keep_their_memory() {
  for (const auto& limits : DEVICE_UI_PROFILES) {
    test::NativeGraphFixture document(limits, test::authored(limits), 17);
    auto flat_limits = limits;
    flat_limits.max_navigation_depth = 1;
    const auto before = page_native_state_budget(flat_limits);
    const auto after = page_native_state_budget(limits);
    assert(before.cards < after.cards && before.keys == after.keys);
    assert(before.ha.dependencies == after.ha.dependencies && before.ha.channels == after.ha.channels);
    assert(before.ha.state_bytes == after.ha.state_bytes && before.metadata_bytes == after.metadata_bytes);
    size_t flat_bytes = 0;
    for (const auto& selected : {before, after}) {
      Arena arena;
      PageNativeSceneState state(arena, selected, page_timezone_resolver(), {}, nullptr);
      assert(state.initialize(true) && state.prepare(document.store));
      state.activate_prepared(document.store);
      if (!flat_bytes) flat_bytes = arena.bytes;
      assert(arena.bytes == flat_bytes);
      assert(state.metadata_memory().active_metadata_bytes <= limits.state_metadata_bytes);
      state.stop();
      state.retire_active();
      assert(!arena.live && !arena.bytes && coordinator.entries.empty());
    }
  }
}
void additional_nested_capacity() {
  for (const auto& limits : DEVICE_UI_PROFILES) {
    test::NativeGraphFixture document(limits, test::capacity_nested(limits), 17);
    Arena arena;
    const auto production = page_native_state_budget(limits);
    PageNativeSceneState state(arena, production, page_timezone_resolver(), {}, nullptr);
    assert(state.initialize(true) && state.prepare(document.store));
    state.activate_prepared(document.store);
    const auto count = state.number.memory().cards;
    assert(count > size_t{limits.slot_count} * limits.slot_count);
    coordinator.report("number.shared", "min", "0");
    coordinator.report("number.shared", "max", "100");
    coordinator.report("number.shared", "step", "1");
    coordinator.report("number.shared", "", "41");
    state.loop();
    const auto last = static_cast<CardId>(document.store.card_count() - 1);
    assert(state.number.read({17, last}).ready && state.number.read({17, last}).position == 41);
    state.stop();
    state.retire_active();
    assert(!arena.live && coordinator.entries.empty());
  }
}
void metadata_admission_is_atomic() {
  const auto& limits = DEVICE_UI_PROFILES[8];
  Document original(17);
  auto records = test::capacity_nested(limits);
  test::Row key; key.kind = test::K::STRING; key.id = 5; key.text = "local_value";
  test::Row type; type.kind = test::K::STRING; type.id = 6; type.text = "local_sensor";
  test::Row entity; entity.kind = test::K::ENTITY; entity.id = 2; entity.first = 5;
  records.insert(records.end(), {key, type, entity});
  for (auto& row : records) {
    if (row.kind != test::K::CARD || row.fields[6] != 2) continue;
    row.fields[6] = 6; row.fields[0] = 2;
    break;
  }
  test::NativeGraphFixture next(limits, std::move(records), 18);
  size_t required = 0;
  {
    Arena arena;
    PageNativeSceneState state(arena, page_native_state_budget(limits), page_timezone_resolver(), {}, nullptr);
    assert(state.initialize(true) && state.prepare(next.store));
    required = state.metadata_memory().staged_metadata_bytes;
    assert(required > 0 && required < limits.state_metadata_bytes);
    state.discard_prepared();
    assert(state.metadata_memory().staged_metadata_bytes == 0 && !arena.live);
    state.stop();
  }
  for (const auto limit : {required - 1, required}) {
    Arena arena;
    auto selected_budget = page_native_state_budget(limits);
    selected_budget.metadata_bytes = limit;
    PageNativeSceneState state(arena, selected_budget, page_timezone_resolver(), {}, nullptr);
    assert(state.initialize(true) && state.prepare(original.store));
    state.activate_prepared(original.store);
    const auto active_bytes = state.metadata_memory().active_metadata_bytes;
    const auto allocations = arena.live;
    const auto registrations = coordinator.entries.size();
    assert(active_bytes < limit);
    const bool prepared = state.prepare_write(next.store);
    assert(prepared == (limit == required));
    assert(state.preparation_capacity_exceeded() == !prepared);
    if (prepared) {
      assert(state.metadata_memory().staged_metadata_bytes == required);
      state.activate_prepared(next.store);
      assert(state.metadata_memory().active_metadata_bytes == required);
    } else {
      assert(state.metadata_memory().staged_metadata_bytes == 0);
      assert(state.metadata_memory().active_metadata_bytes == active_bytes);
      assert(arena.live == allocations && coordinator.entries.size() == registrations);
      coordinator.report("fan.room", "", "on");
      state.loop();
      assert(state.fans.read({17, 0}).on);
    }
    state.stop();
    state.retire_active();
    assert(!arena.live && coordinator.entries.empty());
  }
}
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
void activation_requests_current_values_after_shared_channel_binding() {
  Arena arena;
  std::array<uint8_t, 1024> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 8192> memory{};
  const auto &limits = DEVICE_UI_PROFILES[8];
  PanelConfigWriter writer(bytes.data(), bytes.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),
                                      std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  const std::string card = ";Shared;Auto;Auto;sensor.shared;;sensor;1;";
  assert(writer.append_button(1, reinterpret_cast<const uint8_t *>(card.data()), card.size()) ==
         PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  PageStore store;
  assert(PageStore::stage_v1(bytes.data(), size, limits, 17, memory.data(), memory.size(), &store) ==
         PageStoreStatus::OK);
  // A clock-bar consumer received its value before the native graph existed.
  // The graph deliberately refuses synchronous retained replay while staging.
  int clock_owner = 0;
  std::string clock_value;
  assert(coordinator.subscribe("sensor.shared", "", [&](const auto &v) { clock_value = v; },
                               1, &clock_owner, false, false, false));
  coordinator.report("sensor.shared", "", "41");
  {
    PageNativeSceneState state(arena, budget(), page_timezone_resolver(), {}, nullptr);
    assert(state.initialize(true) && state.prepare(store));
    const auto registrations = coordinator.entries.size();
    const auto before = subscription_announcements;
    assert(clock_value == "41");
    state.activate_prepared(store);
    assert(state.graph.read_attribute({17, 0}, "sensor.shared", "").status == PageEntityStatus::UNKNOWN);
    assert(subscription_announcements == before);
    // Model a fresh HA response using the already installed subscriptions.
    subscription_announcement_reply = [&] { coordinator.report("sensor.shared", "", "42"); };
    busy = true;
    state.loop();
    assert(subscription_announcements == before);
    busy = false;
    state.loop();
    assert(subscription_announcements == before + 1);
    assert(state.graph.read_attribute({17, 0}, "sensor.shared", "").value == "42");
    assert(clock_value == "42" && coordinator.entries.size() == registrations);
    for (int i = 0; i < 20; ++i) state.loop();
    assert(subscription_announcements == before + 1);
    // Failed/discarded staging does not request values for an uncommitted graph.
    arena.fail = true;
    assert(!state.prepare(store));
    arena.fail = false;
    state.discard_prepared();
    state.loop();
    assert(subscription_announcements == before + 1);
    assert(state.prepare(store));
    state.activate_prepared(store);
    state.retire_active();
    state.loop();
    assert(subscription_announcements == before + 1);
    subscription_announcement_reply = {};
    state.stop();
  }
  coordinator.release_owner(&clock_owner);
  assert(coordinator.entries.empty());
}
int main() {
  maximum_first_level_climate_fits_metadata_ceiling();
  unchanged_dashboards_keep_their_memory();
  additional_nested_capacity();
  metadata_admission_is_atomic();
  activation_requests_current_values_after_shared_channel_binding();
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
