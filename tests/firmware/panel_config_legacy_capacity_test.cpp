#include "panel_config_esphome_text.h"
#include <array>
#include <cassert>
#include <string>
#include <vector>

using namespace espcontrol::configuration;
namespace {
constexpr uint32_t golden_key(size_t index) {
  return (index == 0 ? 0x595bcf38u : index == 1 ? 0x595bc830u : 0x595bd7a8u) +
         static_cast<uint32_t>(index * 8192);
}
std::vector<uint8_t> blob(size_t maximum, const std::string &value) {
  assert(value.size() <= maximum);
  std::vector<uint8_t> bytes(maximum + 1);
  bytes[0] = static_cast<uint8_t>(value.size());
  std::copy(value.begin(), value.end(), bytes.begin() + 1);
  return bytes;
}
// Independent old-reader fixture: TemplateText's deployed length prefix and
// fixed allocation remain unchanged; missing keys retain the initial value.
std::string old_reader(esphome::Preferences &prefs, uint32_t key, size_t maximum,
                       const std::string &initial) {
  esphome::ESPPreferenceBackend backend;
  backend.key = key;
  backend.nvs_handle = prefs.nvs_handle;
  std::vector<uint8_t> bytes(maximum + 1);
  if (!backend.load(bytes.data(), bytes.size())) return initial;
  const size_t count = std::min(size_t{bytes[0]}, maximum);
  return {reinterpret_cast<const char *>(bytes.data() + 1), count};
}
struct Fixture {
  esphome::Preferences prefs;
  std::array<esphome::text::Text, 182> texts;
  std::array<EspHomePanelConfigTextValue, 182> values;
  PanelConfigTextBindings bindings;
  Fixture() {
    esphome::global_preferences = &prefs;
    bindings.set_device_profile("p4");
    for (size_t i = 0; i < texts.size(); ++i) {
      texts[i].preference_hash += static_cast<uint32_t>(i * 8192);
      texts[i].traits.maximum = i == 0 ? 120 : i == 1 ? 6 : 255;
      texts[i].traits.minimum = i == 1 ? 6 : 0;
      // Chrome deliberately has nonempty defaults in this fixture, exercising
      // explicit empty overrides rather than an inferred empty-default policy.
      values[i].bind(&texts[i], true, i >= 2);
    }
    bindings.set_button_order(&values[0]);
    bindings.set_button_on_color(&values[1]);
    for (size_t slot = 0; slot < 20; ++slot) {
      std::array<PanelConfigTextValue *, 8> chunks{};
      for (size_t chunk = 0; chunk < 8; ++chunk)
        chunks[chunk] = &values[22 + slot * 8 + chunk];
      bindings.set_button(static_cast<uint8_t>(slot + 1), &values[2 + slot], chunks);
    }
  }
  ~Fixture() { esphome::global_preferences = nullptr; }
};
std::vector<uint8_t> document(const std::string &profile = "p4",
                              const std::string &order = "1,2,3") {
  std::vector<uint8_t> bytes(4096);
  PanelConfigWriter writer(bytes.data(), bytes.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(profile.data()),
                                      profile.size()) == PanelConfigStatus::OK);
  for (uint8_t slot = 1; slot <= 20; ++slot) {
    const std::string row = "switch.fixture_" + std::to_string(slot) + ";New;Auto;Auto;;;;;";
    assert(writer.append_button(slot, reinterpret_cast<const uint8_t *>(row.data()), row.size()) ==
           PanelConfigStatus::OK);
  }
  assert(writer.append_setting(reinterpret_cast<const uint8_t *>("button_order"), 12,
                               reinterpret_cast<const uint8_t *>(order.data()), order.size()) ==
         PanelConfigStatus::OK);
  const std::string color = "112233";
  assert(writer.append_setting(reinterpret_cast<const uint8_t *>("button_on_color"), 15,
                               reinterpret_cast<const uint8_t *>(color.data()), color.size()) ==
         PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  bytes.resize(size);
  return bytes;
}
void distinct_keys_reclaim_before_replacement() {
  Fixture fixture;
  auto &prefs = fixture.prefs;
  constexpr uint32_t unrelated = 0xD00D1234;
  prefs.durable[unrelated] = {1, 2, 3, 4};
  // Model a partition with no room for a replacement blob until unwanted
  // fields are reclaimed. These are distinct keys, not repeated single-key
  // writes (which would miss the aggregate storage failure).
  for (size_t i = 0; i < 147; ++i) {
    prefs.durable[golden_key(i)] = blob(fixture.texts[i].traits.maximum, "old");
    fixture.texts[i].state = "old";
  }
  prefs.capacity_entries = prefs.used_entries();
  const auto previous = prefs.durable;
  assert(!fixture.values[2].set_value("new", 3));
  assert(prefs.durable == previous && prefs.pending.empty());
  const auto bytes = document();
  assert(fixture.bindings.publish_document(bytes.data(), bytes.size()));
  assert(prefs.durable == previous); // Publication itself never erases flash.
  assert(fixture.bindings.persist_document(bytes.data(), bytes.size()));
  assert(prefs.durable.at(unrelated) == std::vector<uint8_t>({1, 2, 3, 4}));
  for (size_t i = 0; i < 20; ++i) {
    assert(prefs.durable.at(golden_key(2 + i)).size() == 256);
    assert(old_reader(prefs, golden_key(2 + i), 255, "") == fixture.texts[2 + i].state);
  }
  for (size_t i = 22; i < 182; ++i) {
    assert(prefs.durable.count(golden_key(i)) == 0);
    assert(old_reader(prefs, golden_key(i), 255, "").empty());
  }
  assert(prefs.durable.size() == 23 && prefs.used_entries() < 250);
  assert(prefs.peak_queued_bytes <= 256);
  const auto first_write = std::find_if(prefs.operations.begin(), prefs.operations.end(),
      [](const auto &event) { return event.rfind("write:", 0) == 0; });
  assert(first_write != prefs.operations.end() && first_write != prefs.operations.begin());
  assert(std::all_of(prefs.operations.begin(), first_write,
      [](const auto &event) { return event.rfind("erase:", 0) == 0; }));
  const auto saves = prefs.saves;
  assert(fixture.bindings.persist_document(bytes.data(), bytes.size()));
  assert(prefs.saves == saves); // Identical values do not recreate empty keys.
}
void sparse_defaults_and_explicit_empty_overrides() {
  Fixture fixture;
  auto &prefs = fixture.prefs;
  auto &value = fixture.values[2];
  assert(value.set_value("", 0));
  assert(prefs.durable.empty() && prefs.saves == 0 && prefs.commits == 0);
  assert(old_reader(prefs, golden_key(2), 255, "").empty());
  assert(value.set_value("value", 5));
  assert(prefs.durable.at(golden_key(2)) == blob(255, "value"));
  assert(value.prepare_empty_value());
  assert(fixture.texts[2].state == "value"); // Preparation does not publish.
  assert(value.set_value("", 0) && fixture.texts[2].state.empty());
  assert(!prefs.durable.count(golden_key(2)));
  // A field whose initial value is nonempty needs an explicit empty blob so
  // the old SDK does not restore its nonempty factory value instead.
  auto &nonempty_default = fixture.values[0];
  assert(nonempty_default.set_value("", 0));
  assert(prefs.durable.at(golden_key(0)) == blob(120, ""));
  assert(old_reader(prefs, golden_key(0), 120, "factory").empty());
  esphome::text::Text custom;
  EspHomePanelConfigTextValue custom_value;
  custom_value.bind(&custom, false, true);
  esphome::global_preferences = nullptr;
  assert(custom_value.prepare_empty_value());
  assert(custom_value.set_value("", 0) && custom.controls == 1);
}
void erase_failures_are_reported_and_retryable() {
  Fixture fixture;
  auto &prefs = fixture.prefs;
  auto &value = fixture.values[2];
  const auto key = golden_key(2);
  prefs.durable[key] = blob(255, "old");
  fixture.texts[2].state = "old";
  prefs.fail_erase = true;
  assert(!value.prepare_empty_value() && prefs.durable.count(key));
  prefs.fail_erase = false;
  prefs.fail_commit = true;
  assert(!value.prepare_empty_value());
  assert(fixture.texts[2].state == "old");
  prefs.fail_commit = false;
  assert(value.set_value("", 0) && !prefs.durable.count(key));
  prefs.durable[key] = blob(255, "old");
  prefs.fail_sync = prefs.syncs + 1;
  assert(!value.prepare_empty_value());
  // Even a failed queue drain must not prevent reclaiming space. The pinned
  // SDK clears its failed queue; the caller still receives the failure.
  assert(!prefs.durable.count(key));
  assert(value.set_value("", 0));
  prefs.durable[key] = blob(255, "old");
  prefs.nvs_handle = 0;
  assert(!value.prepare_empty_value() && prefs.durable.count(key));
}
void invalid_projection_does_not_clear_preferences() {
  Fixture fixture;
  auto &prefs = fixture.prefs;
  prefs.durable[golden_key(22)] = blob(255, "preserve");
  const auto previous = prefs.durable;
  for (const auto &bytes : {document("wrong"), document("p4", std::string(121, 'x'))}) {
    assert(!fixture.bindings.persist_document(bytes.data(), bytes.size()));
    assert(prefs.durable == previous && prefs.erases == 0 && prefs.saves == 0);
  }
}
} // namespace
int main() {
  distinct_keys_reclaim_before_replacement();
  sparse_defaults_and_explicit_empty_overrides();
  erase_failures_are_reported_and_retryable();
  invalid_projection_does_not_clear_preferences();
}
