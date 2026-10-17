#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "button_grid_subpage_parser.h"
#include "page_store.h"
#include "panel_config_document.h"
#include "page_store_goldens.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;

namespace {

struct Document {
  std::array<uint8_t, 40 * 1024> buffer{};
  PanelConfigWriter writer{buffer.data(), buffer.size()};
  size_t size{0};

  explicit Document(const DeviceUiLimits &limits) {
    assert(writer.begin() == PanelConfigStatus::OK);
    const std::string profile = limits.device_profile;
    assert(writer.append_device_profile(
      reinterpret_cast<const uint8_t *>(profile.data()), profile.size()) == PanelConfigStatus::OK);
  }
  void button(uint8_t slot, const std::string &value) {
    assert(writer.append_button(slot, reinterpret_cast<const uint8_t *>(value.data()), value.size()) == PanelConfigStatus::OK);
  }
  void subpage(uint8_t slot, const std::string &value) {
    assert(writer.append_subpage(slot, reinterpret_cast<const uint8_t *>(value.data()), value.size()) == PanelConfigStatus::OK);
  }
  void setting(const std::string &key, const std::string &value) {
    assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()), key.size(),
      reinterpret_cast<const uint8_t *>(value.data()), value.size()) == PanelConfigStatus::OK);
  }
  void finish() { assert(writer.finish(&size) == PanelConfigStatus::OK); }
};

using Arena = std::array<uint32_t, 40 * 1024>;

void equal_fields(const PageStore &store, const CardDefinition &card, const ParsedCfg &expected) {
  const std::array<std::string, 9> fields = {expected.entity, expected.label,
    expected.icon, expected.icon_on, expected.sensor, expected.unit, expected.type,
    expected.precision, expected.options};
  for (size_t i = 0; i < fields.size(); ++i)
    assert(store.field(card, static_cast<CardField>(i)) == fields[i]);
}

void profile_corpus(const DeviceUiLimits &limits) {
  Arena arena{};
  PageStore store;
  Document empty(limits);
  empty.finish();
  assert(PageStore::stage_v1(empty.buffer.data(), empty.size, limits, 1,
      arena.data(), sizeof(arena), &store) == PageStoreStatus::OK);
  assert(store.page_count() == 1);
  assert(store.card_count() == limits.slot_count);
  assert(store.page(0)->navigable);
  assert(store.page(0)->parent == NO_PAGE);
  for (CardId i = 0; i < limits.slot_count; ++i) {
    assert(store.card(i)->slot == i + 1);
    equal_fields(store, *store.card(i), parse_cfg(""));
  }

  // The maximum existing profile: all root cards are folders and every child
  // slot is populated. Use a separate arena so the active view remains intact.
  Arena staged_arena{};
  Document maximum(limits);
  const std::string root = ";Room;Home;Auto;indicator;;subpage;;";
  std::string child = "~B=Parent%3A%20Home:FF8C00";
  for (size_t i = 1; i <= limits.cards_per_subpage; ++i)
    child += "|s,sensor.temperature,Temp%20%C2%B0C,Auto,Auto,,%C2%B0C,1,";
  for (size_t i = 1; i <= limits.slot_count; ++i) {
    maximum.button(i, root);
    maximum.subpage(i, child);
  }
  maximum.setting("button_order", "2w,1,,3");
  maximum.setting("unrecognised_future_setting", "preserve me");
  maximum.finish();
  assert(PageStore::stage_v1(maximum.buffer.data(), maximum.size, limits, 2,
      staged_arena.data(), sizeof(staged_arena), &store) == PageStoreStatus::OK);
  assert(store.page_count() == 1 + limits.max_first_level_subpages);
  assert(store.card_count() == limits.max_cards());
  assert(store.string(store.page(0)->order) == "2w,1,,3");
  assert(store.string(store.page(1)->order) == "B=Parent%3A%20Home:FF8C00");
  for (CardId i = 0; i < limits.slot_count; ++i) {
    const auto *card = store.card(i);
    equal_fields(store, *card, parse_cfg(root));
    assert(card->target == i + 1);
    const auto *page = store.page(card->target);
    assert(page->parent == 0 && page->owner == i && page->navigable);
    assert(page->card_count == limits.cards_per_subpage);
    // Interning shares repeated fields across the entire configuration.
    assert(card->fields[1] == store.card(0)->fields[1]);
    assert(store.field(*store.card(page->first_card), CardField::LABEL) == "Temp °C");
    assert(store.field(*store.card(page->first_card), CardField::UNIT) == "°C");
  }
  assert(store.source_size() == maximum.size);
  assert(std::memcmp(store.source_document(), maximum.buffer.data(), maximum.size) == 0);
  assert(store.memory().arena_bytes < sizeof(staged_arena));
  assert(store.memory().string_count < 20);
  assert(store.card(ConfigurationHandle{1, 0}) == nullptr);
  assert(store.card(ConfigurationHandle{2, 0}) != nullptr);
  assert(store.page(ConfigurationHandle{1, 0}) == nullptr);
  assert(store.page(ConfigurationHandle{2, 0}) != nullptr);
  assert(store.card(NO_CARD) == nullptr && store.page(NO_PAGE) == nullptr);
  assert(store.string(UINT32_MAX).empty());
  assert(store.string(1).empty());

  const auto before = store.memory().arena_bytes;
  Arena small{};
  assert(PageStore::stage_v1(empty.buffer.data(), empty.size, limits, 3,
      small.data(), 1, &store) == PageStoreStatus::BUFFER_TOO_SMALL);
  assert(store.generation() == 2 && store.memory().arena_bytes == before);
  // No in-place rewrite of a live view, overlapping input or unaligned arena.
  assert(PageStore::stage_v1(empty.buffer.data(), empty.size, limits, 3,
      staged_arena.data(), sizeof(staged_arena), &store) == PageStoreStatus::INVALID_ARGUMENT);
  assert(PageStore::stage_v1(empty.buffer.data(), empty.size, limits, 3,
      empty.buffer.data(), empty.buffer.size(), &store) == PageStoreStatus::INVALID_ARGUMENT);
  assert(PageStore::stage_v1(empty.buffer.data(), empty.size, limits, 3,
      reinterpret_cast<uint8_t *>(small.data()) + 1, sizeof(small) - 1,
      &store) == PageStoreStatus::INVALID_ARGUMENT);
  std::cout << limits.device_profile << ": pages=" << store.page_count()
            << " cards=" << store.card_count()
            << " arena=" << store.memory().arena_bytes
            << " strings=" << store.memory().string_bytes << '\n';
}

void compatibility_and_failures() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  Document doc(limits);
  const std::string source = "light.kitchen;Kitchen;Lightbulb;Auto;;;light_switch;;";
  doc.button(1, source);
  // A saved subpage behind a non-folder and a slot outside the current profile
  // must survive without becoming reachable navigation targets.
  doc.subpage(1, "B,,1w|light.one:One:Lightbulb:Auto:::::");
  doc.button(20, ";Hidden;Auto;Auto;;;subpage;;");
  doc.subpage(20, "B,1|sensor.one:Hidden:Auto:Auto::°C:sensor:1:");
  doc.finish();
  Arena arena{};
  PageStore store;
  assert(PageStore::stage_v1(doc.buffer.data(), doc.size, limits, 1,
      arena.data(), sizeof(arena), &store) == PageStoreStatus::OK);
  assert(store.page_count() == 3 && store.page(0)->card_count == 20);
  assert(!store.page(1)->navigable && !store.page(2)->navigable);
  assert(store.card(0)->target == NO_PAGE && store.card(19)->target == NO_PAGE);
  equal_fields(store, *store.card(0), parse_cfg(source));
  const auto children = parse_subpage_config("B,,1w|light.one:One:Lightbulb:Auto:::::");
  equal_fields(store, *store.card(store.page(1)->first_card), parsed_cfg_from_subpage_btn(children[0]));
  assert(store.string(store.page(1)->order) == "B,,1w");
  std::fill(doc.buffer.begin(), doc.buffer.end(), 0);
  assert(store.field(*store.card(0), CardField::LABEL) == "Kitchen");
  assert(store.source_document()[0] == 'E');

  Arena other{};
  Document valid(limits);
  valid.finish();
  assert(PageStore::stage_v1(valid.buffer.data(), valid.size, DEVICE_UI_PROFILES[0], 2,
      other.data(), sizeof(other), &store) == PageStoreStatus::PROFILE_MISMATCH);
  assert(store.generation() == 1);
  valid.buffer[0] = 0;
  assert(PageStore::stage_v1(valid.buffer.data(), valid.size, limits, 2,
      other.data(), sizeof(other), &store) == PageStoreStatus::INVALID_DOCUMENT);
  assert(store.generation() == 1);
}

void card_type_corpus() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  for (size_t i = 0; i < limits.card_type_count; ++i) {
    const auto &type = limits.card_types[i];
    const std::string root = "light.test;Example;Auto;Auto;;;" + std::string(type.type) + ";;";
    Document doc(limits);
    doc.button(1, root);
    doc.button(2, ";Folder;Auto;Auto;;;subpage;;");
    const std::string child = "B,1|light.test:Example:Auto:Auto:::" + std::string(type.type) + "::";
    if (type.allow_in_subpage) doc.subpage(2, child);
    doc.finish();
    Arena arena{};
    PageStore store;
    assert(PageStore::stage_v1(doc.buffer.data(), doc.size, limits, 1,
        arena.data(), sizeof(arena), &store) == PageStoreStatus::OK);
    equal_fields(store, *store.card(0), parse_cfg(root));
    if (type.allow_in_subpage) {
      const auto expected = parse_subpage_config(child);
      const auto *page = store.page(store.card(1)->target);
      assert(page->card_count == 1);
      equal_fields(store, *store.card(page->first_card), parsed_cfg_from_subpage_btn(expected[0]));
    }
  }
}

void byte_limits_and_exhaustion() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  Document doc(limits);
  std::string label;
  for (size_t i = 0; i < 1200; ++i) label += "é";
  const std::string source = "sensor.test;" + label + ";Auto;Auto;;°C;sensor;1;";
  doc.button(1, source);
  doc.finish();
  Arena arena{};
  PageStore store;
  assert(PageStore::stage_v1(doc.buffer.data(), doc.size, limits, 1,
      arena.data(), sizeof(arena), &store) == PageStoreStatus::OK);
  assert(store.field(*store.card(0), CardField::LABEL) == label);
  const size_t needed = store.memory().arena_bytes;
  Arena second{};
  // Exhaust the string pool after records/source were successfully staged.
  assert(PageStore::stage_v1(doc.buffer.data(), doc.size, limits, 2,
      second.data(), needed - 1, &store) == PageStoreStatus::BUFFER_TOO_SMALL);
  assert(store.generation() == 1);
  assert(store.field(*store.card(0), CardField::LABEL) == label);
  assert(PageStore::stage_v1(doc.buffer.data(), doc.size, limits, 2,
      second.data(), needed, &store) == PageStoreStatus::OK);
  assert(store.memory().arena_bytes == needed);
  assert(store.field(*store.card(0), CardField::LABEL) == label);
}

void authored_golden_corpus() {
  for (const auto &limits : DEVICE_UI_PROFILES) {
    for (const auto &fixture : PAGE_STORE_GOLDENS) {
      Document doc(limits);
      doc.button(1, fixture.input);
      doc.finish();
      Arena arena{};
      PageStore store;
      assert(PageStore::stage_v1(doc.buffer.data(), doc.size, limits, 1,
          arena.data(), sizeof(arena), &store) == PageStoreStatus::OK);
      for (size_t i = 0; i < fixture.expected.size(); ++i)
        assert(store.field(*store.card(0), static_cast<CardField>(i)) == fixture.expected[i]);
    }
  }
}

}  // namespace

int main() {
  for (const auto &limits : DEVICE_UI_PROFILES) profile_corpus(limits);
  compatibility_and_failures();
  card_type_corpus();
  byte_limits_and_exhaustion();
  authored_golden_corpus();
}
