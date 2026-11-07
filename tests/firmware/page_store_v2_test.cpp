#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "page_store_runtime.h"
#include "page_store_goldens.h"
#include "panel_config_import.h"
#include "nested_page_fixture.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
using K = PanelConfigV2Kind;
using Row = PanelConfigV2Record;
using namespace espcontrol::pages::test;
using Bytes = std::vector<uint8_t>;
using Arena = std::vector<uint64_t>;

namespace {
size_t cases = 0;
Bytes make_v1(const DeviceUiLimits &limits, const std::string &root,
              const std::string &children = "", bool maximum = false) {
  Bytes output(128 * 1024);
  PanelConfigWriter writer(output.data(), output.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),
      std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  for (size_t slot = 1; slot <= (maximum ? limits.slot_count : size_t{1}); ++slot) {
    assert(writer.append_button(slot, reinterpret_cast<const uint8_t *>(root.data()), root.size()) == PanelConfigStatus::OK);
    if (!children.empty()) assert(writer.append_subpage(slot,
      reinterpret_cast<const uint8_t *>(children.data()), children.size()) == PanelConfigStatus::OK);
  }
  const std::string order = "1w,,B=Parent%3A%20Home:FF8000";
  assert(writer.append_setting(reinterpret_cast<const uint8_t *>("button_order"), 12,
      reinterpret_cast<const uint8_t *>(order.data()), order.size()) == PanelConfigStatus::OK);
  const std::string opaque = std::string("future\0data", 11) + " 🌡️";
  assert(writer.append_setting(reinterpret_cast<const uint8_t *>("opaque"), 6,
      reinterpret_cast<const uint8_t *>(opaque.data()), opaque.size()) == PanelConfigStatus::OK);
  size_t size = 0; assert(writer.finish(&size) == PanelConfigStatus::OK); output.resize(size); return output;
}
Bytes imported(const Bytes &source) {
  PanelConfigImportBounds bounds;
  assert(panel_config_v1_to_v2_bounds(source.data(), source.size(), &bounds) == PanelConfigImportStatus::OK);
  Arena scratch((bounds.workspace_bytes + 7) / 8);
  Bytes output(bounds.output_bytes); size_t size = 0;
  assert(panel_config_v1_to_v2(source.data(), source.size(), output.data(), output.size(),
      scratch.data(), scratch.size() * 8, &size) == PanelConfigImportStatus::OK);
  output.resize(size); return output;
}
void equal_store(const PageStore &left, const PageStore &right) {
  assert(left.generation() == right.generation() && left.page_count() == right.page_count() && left.card_count() == right.card_count());
  for (PageId i = 0; i < left.page_count(); ++i) {
    const auto &a = *left.page(i), &b = *right.page(i);
    assert(a.parent == b.parent && a.owner == b.owner && a.first_card == b.first_card &&
      a.card_count == b.card_count && a.runtime_card_count == b.runtime_card_count && a.navigable == b.navigable);
    assert(left.string(a.order) == right.string(b.order));
  }
  for (CardId i = 0; i < left.card_count(); ++i) {
    const auto &a = *left.card(i), &b = *right.card(i);
    assert(a.slot == b.slot && a.target == b.target);
    assert(bool(left.runtime_card(i)) == bool(right.runtime_card(i)));
    for (size_t f = 0; f < a.fields.size(); ++f)
      assert(left.field(a, static_cast<CardField>(f)) == right.field(b, static_cast<CardField>(f)));
  }
  assert(left.memory().record_bytes == right.memory().record_bytes &&
    left.memory().string_count == right.memory().string_count && left.memory().string_bytes == right.memory().string_bytes);
}
PageStoreStatus stage(const Bytes &source, const DeviceUiLimits &limits, Arena &arena,
                     Arena &scratch, PageStore &output) {
  size_t needed = 0;
  const auto sized = PageStore::v2_workspace_bytes(source.data(), source.size(), &needed);
  if (sized != PageStoreStatus::OK) return sized;
  scratch.resize((needed + 7) / 8);
  return PageStore::stage_v2(source.data(), source.size(), limits, 7, arena.data(),
      arena.size() * 8, scratch.data(), needed, &output);
}
void parity(const Bytes &source, const DeviceUiLimits &limits) {
  const auto v2 = imported(source);
  Arena first(128 * 1024 / 8), second(first.size()), scratch;
  PageStore old, current;
  assert(PageStore::stage_v1(source.data(), source.size(), limits, 7, first.data(), first.size() * 8, &old) == PageStoreStatus::OK);
  assert(stage(v2, limits, second, scratch, current) == PageStoreStatus::OK);
  equal_store(old, current);
  assert(old.source_version() == 1 && current.source_version() == 2);
  assert(current.source_size() == v2.size() && std::memcmp(current.source_document(), v2.data(), v2.size()) == 0);
  std::fill(scratch.begin(), scratch.end(), UINT64_MAX);
  equal_store(old, current); // No retained dictionary, grouping or input pointers.
  ++cases;
}
void nested_depth_two() {
  for(const auto& profile:DEVICE_UI_PROFILES) {
    auto limits=profile;limits.max_navigation_depth=2;
    auto records=authored_nested(limits);auto source=wire(records);
    auto old_limits=limits;old_limits.max_navigation_depth=1;
    Arena arena(16384),other(16384),scratch;PageStore store;
    assert(stage(source,old_limits,other,scratch,store)==PageStoreStatus::INVALID_DOCUMENT);
    assert(stage(source,limits,arena,scratch,store)==PageStoreStatus::OK);
    assert(store.page_count()==3 && store.card_count()==static_cast<size_t>(limits.slot_count)+2);
    const CardId folder=limits.slot_count,leaf=folder+1;
    assert(store.page(1)->parent==0 && store.page(1)->owner==1);
    assert(store.card(1)->target==1 && store.card(folder)->target==2);
    assert(store.page(2)->parent==1 && store.page(2)->owner==folder);
    assert(store.page(2)->first_card==leaf && store.page(2)->card_count==1 && store.page(2)->navigable);
    assert(store.string(store.page(2)->order)=="B=Back%20Up:112233,1");
    assert(store.field(*store.card(leaf),CardField::LABEL)=="Nested Leaf");
    const auto original=source;
    std::fill(source.begin(),source.end(),0);std::fill(scratch.begin(),scratch.end(),UINT64_MAX);
    assert(store.field(*store.card(leaf),CardField::LABEL)=="Nested Leaf");
    assert(std::memcmp(store.source_document(),original.data(),original.size())==0);
    // A retained root outside the active profile keeps its complete subtree,
    // while acquiring no active navigation targets or state dependencies.
    records[3].second=32;const auto inactive=wire(records);PageStore dormant;
    assert(stage(inactive,limits,other,scratch,dormant)==PageStoreStatus::OK);
    assert(dormant.card_count()==34 && !dormant.page(1)->navigable && !dormant.page(2)->navigable);
    assert(!dormant.runtime_card(32) && !dormant.runtime_card(33));
    assert(dormant.field(*dormant.card(33),CardField::LABEL)=="Nested Leaf");
    ++cases;
  }
}
void corpus() {
  for (const auto &limits : DEVICE_UI_PROFILES) {
    parity(make_v1(limits, ""), limits);
    std::string children = "~B=Parent%3A%20Home:FF8000";
    for (size_t i = 0; i < limits.cards_per_subpage; ++i)
      children += "|S,sensor.temperature,温度%20🌡️,Auto,Auto,,°C,1,";
    parity(make_v1(limits, ";Folder;Auto;Auto;;;subpage;;", children, true), limits);
    for (size_t i = 0; i < limits.card_type_count; ++i) {
      const auto &type = limits.card_types[i];
      const std::string card = "light.test;Label;Auto;Auto;sensor.other;°C;" + std::string(type.type) + ";1;card_on_color=FF8000";
      parity(make_v1(limits, card), limits);
      if (type.allow_in_subpage && std::strcmp(type.type, "subpage") != 0) parity(make_v1(limits, ";Folder;;;;;subpage",
          "B,,1w|light.test:Label:Auto:Auto:sensor.other:°C:" + std::string(type.type) + ":1:card_on_color=FF8000"), limits);
    }
    for (const auto &golden : PAGE_STORE_GOLDENS) parity(make_v1(limits, golden.input), limits);
    // Retained inactive subpages and empty/missing imported records survive.
    parity(make_v1(limits, "light.test;Not a folder", "B,1|sensor.one:Hidden:Auto:Auto:::sensor"), limits);
    const auto direct = wire(authored(limits)); Arena arena(16384), scratch; PageStore store;
    assert(stage(direct, limits, arena, scratch, store) == PageStoreStatus::OK);
    assert(store.page_count() == 2 && store.card(1)->target == 1 && store.page(1)->owner == 1);
    const auto *child = store.card(store.page(1)->first_card);
    assert(store.field(*child, CardField::LABEL) == "温度 🌡️" && store.field(*child, CardField::ICON) == "Auto");
    assert(store.string(store.page(1)->order) == "B,1w"); ++cases;
  }
}

class Allocator final : public PageArenaAllocator {
 public:
  void *allocate(size_t bytes) override {
    if (++attempts == fail_at || refuse) return nullptr;
    void *value = std::malloc(bytes); assert(value); allocations[value] = bytes;
    live += bytes; peak = std::max(peak, live); return value;
  }
  void release(void *value, size_t bytes) override {
    assert(allocations.at(value) == bytes); live -= bytes; allocations.erase(value); std::free(value);
  }
  size_t attempts{0}, fail_at{0}, live{0}, peak{0}; bool refuse{false};
  std::unordered_map<void *, size_t> allocations;
};
class Resources final : public PageRuntimeResources {
 public:
  bool prepare(const PageStore &store) override {
    assert(owner->memory().preparation_scratch_bytes == 0);
    prepared = store; return !fail;
  }
  void discard_prepared() override { prepared = {}; }
  void activate_prepared(const PageStore &store) override { assert(prepared.source_document() == store.source_document()); prepared = {}; ++activations; }
  void retire_active() override {}
  PageStoreRuntime *owner{nullptr}; PageStore prepared; bool fail{false}; size_t activations{0};
};
void replacement_and_failures() {
  const auto &limits = DEVICE_UI_PROFILES[8];
  const auto original = make_v1(limits, "light.one;Original");
  const auto replacement = imported(make_v1(limits, "light.two;Replacement"));
  for (size_t failure = 1; failure <= 3; ++failure) {
    Allocator allocator; Resources resources;
    {
      PageStoreRuntime runtime(limits, allocator, 128 * 1024, &resources); resources.owner = &runtime;
      assert(runtime.prepare(1, original.data(), original.size()));
      assert(runtime.activate_prepared(1, 1, original.data(), original.size()));
      const size_t retained = allocator.live;
      allocator.fail_at = allocator.attempts + failure;
      assert(!runtime.prepare(2, replacement.data(), replacement.size()));
      assert(runtime.preparation_allocation_failed());
      assert(runtime.active().generation() == 1 && allocator.live == retained && allocator.allocations.size() == 1);
      assert(runtime.memory().preparation_scratch_bytes == 0 && runtime.memory().staged_reserved_bytes == 0);
      allocator.fail_at = 0; resources.fail = true;
      assert(!runtime.prepare(2, replacement.data(), replacement.size()));
      assert(runtime.last_stage_status() == PageStoreStatus::RESOURCE_UNAVAILABLE && allocator.live == retained);
      assert(!runtime.preparation_allocation_failed());
      resources.fail = false;
      assert(runtime.prepare(2, replacement.data(), replacement.size()));
      assert(!runtime.preparation_allocation_failed());
      assert(runtime.memory().preparation_scratch_bytes == 0 && allocator.allocations.size() == 2);
      assert(!runtime.activate_prepared(2, 1, replacement.data(), replacement.size()));
      auto wrong = replacement; wrong.back() ^= 1;
      assert(!runtime.activate_prepared(2, 2, wrong.data(), wrong.size()));
      allocator.refuse = true;
      assert(runtime.activate_prepared(2, 2, replacement.data(), replacement.size()));
      assert(allocator.allocations.size() == 1 && runtime.active().source_version() == 2);
      assert(runtime.active().field(*runtime.active().card(0), CardField::LABEL) == "Replacement");
      assert(runtime.memory().peak_reserved_bytes == allocator.peak && runtime.memory().peak_preparation_scratch_bytes > 0);
      allocator.refuse = false;
      assert(runtime.prepare(1, original.data(), original.size()));
      assert(runtime.activate_prepared(3, 1, original.data(), original.size()));
      assert(runtime.active().source_version() == 1 && resources.activations == 3);
    }
    assert(allocator.allocations.empty() && allocator.live == 0); ++cases;
  }
}

void legacy_child_folders_need_explicit_targets() {
  const auto &limits=DEVICE_UI_PROFILES[8];
  const auto source=make_v1(limits,";Root;;;;;subpage","~B,1|G,,Unlinked");
  Allocator allocator;PageStoreRuntime runtime(limits,allocator,128*1024);
  assert(!runtime.prepare_write(1,source.data(),source.size()));
  assert(runtime.last_stage_status()==PageStoreStatus::CAPACITY_EXCEEDED);
  assert(!runtime.active().page_count() && allocator.live==0);
}

void retained_off_grid_subpage_cards() {
  for (const auto &limits : DEVICE_UI_PROFILES) {
    std::string children = "B,1";
    for (size_t slot = 1; slot <= limits.cards_per_subpage + size_t{2}; ++slot)
      children += "|:Stored:Auto:Auto:sensor.retained_" + std::to_string(slot) + "::sensor";
    const auto legacy = make_v1(limits, ";Folder;;;;;subpage", children);
    std::string overfull_order = "B";
    for (size_t slot = 1; slot <= limits.slot_count; ++slot)
      overfull_order += "," + std::to_string(slot);
    const auto overfull = make_v1(limits, ";Folder;;;;;subpage",
      overfull_order + children.substr(children.find('|')));
    for (uint16_t version : {1, 2}) {
      const auto document = version == 1 ? legacy : imported(legacy);
      Allocator allocator;
      {
        PageStoreRuntime runtime(limits, allocator, 128 * 1024);
        assert(runtime.prepare_write(version, document.data(), document.size()));
        assert(runtime.activate_prepared(1, version, document.data(), document.size()));
        const auto &store = runtime.active();
        const auto *page = store.page(1);
        assert(page->card_count == limits.cards_per_subpage + size_t{2});
        assert(page->runtime_card_count == 1);
        assert(store.runtime_card(page->first_card));
        assert(!store.runtime_card(page->first_card + 1));
        assert(!store.runtime_card(page->first_card + limits.cards_per_subpage));
        const auto *hidden = store.card(page->first_card + limits.cards_per_subpage);
        assert(store.field(*hidden, CardField::SENSOR) ==
          "sensor.retained_" + std::to_string(limits.cards_per_subpage + 1));
        assert(store.source_size() == document.size() &&
          std::memcmp(store.source_document(), document.data(), document.size()) == 0);
        const auto rejected = version == 1 ? overfull : imported(overfull);
        const auto retained = allocator.live;
        assert(!runtime.prepare_write(version, rejected.data(), rejected.size()));
        assert(runtime.preparation_capacity_exceeded());
        assert(store.generation() == 1 && allocator.live == retained &&
          std::memcmp(store.source_document(), document.data(), document.size()) == 0);
      }
      assert(allocator.live == 0);
    }
    auto records = authored_nested(limits);
    records[4].second = limits.slot_count; // Stored folder beyond the Back-limited grid.
    for (uint16_t slot = 1; slot < limits.slot_count; ++slot) {
      auto card = records[4];
      card.id = 62000 + slot; card.second = slot; card.fields[6] = 75;
      records.push_back(card);
    }
    const auto document = wire(records);
    Arena arena(16384), scratch; PageStore store;
    assert(stage(document, limits, arena, scratch, store) == PageStoreStatus::OK);
    assert(store.page(1)->navigable && !store.page(2)->navigable);
    const CardId hidden_folder = store.page(1)->first_card + limits.slot_count - 1;
    assert(!store.runtime_card(hidden_folder) && store.card(hidden_folder)->target == NO_PAGE);
    assert(!store.runtime_card(store.page(2)->first_card));
    assert(store.field(*store.card(store.page(2)->first_card), CardField::LABEL) == "Nested Leaf");
    // Moving that retained folder into the order must activate its complete
    // subtree even though its original slot exceeds the Back-limited prefix.
    const std::string reordered_order = "B," + std::to_string(limits.slot_count);
    for (auto &record : records)
      if (record.kind == K::STRING && record.id == 72)
        record.text = reordered_order;
    const auto reordered = wire(records);
    Arena reordered_arena(arena.size()); PageStore visible_store;
    assert(stage(reordered, limits, reordered_arena, scratch, visible_store) == PageStoreStatus::OK);
    assert(visible_store.page(1)->runtime_card_count == 1 && visible_store.page(2)->navigable);
    assert(visible_store.runtime_card(hidden_folder) && visible_store.card(hidden_folder)->target == 2);
    assert(visible_store.runtime_card(visible_store.page(2)->first_card));
    ++cases;
  }
}

void nested_write_admission() {
  for (const auto &profile : DEVICE_UI_PROFILES) {
    auto limits = profile;
    limits.max_navigation_depth = 2;
    const auto source = wire(authored_nested(limits));
    Allocator allocator;
    {
      PageStoreRuntime runtime(limits, allocator, 128 * 1024);
      assert(runtime.prepare_write(2, source.data(), source.size()));
      assert(runtime.activate_prepared(1, 2, source.data(), source.size()));
      assert(runtime.active().page_depth(0) == 0 && runtime.active().page_depth(1) == 1 &&
             runtime.active().page_depth(2) == 2 && runtime.active().page_depth(NO_PAGE) == NO_PAGE);
      const auto *active = runtime.active().source_document();
      const auto retained = allocator.live;
      // One first-level folder can own a nested folder; it is not two root folders.
      limits.max_first_level_subpages = 1;
      assert(runtime.prepare_write(2, source.data(), source.size()));
      runtime.discard_prepared();
      limits.cards_per_subpage = 0;
      assert(!runtime.prepare_write(2, source.data(), source.size()));
      assert(runtime.last_stage_status() == PageStoreStatus::CAPACITY_EXCEEDED);
      assert(runtime.active().source_document() == active && allocator.live == retained);
    }
    assert(allocator.live == 0);
    ++cases;
  }
}

void hostile_and_bounds() {
  const auto &limits = DEVICE_UI_PROFILES[8]; const auto source = wire(authored(limits));
  Arena arena(16384), other(arena.size()), scratch; PageStore store;
  assert(stage(source, limits, arena, scratch, store) == PageStoreStatus::OK);
  const auto active_source = store.source_document(); const auto needed = store.memory().arena_bytes;
  assert(PageStore::stage_v2(source.data(), source.size(), limits, 8, other.data(), needed - 1,
      scratch.data(), scratch.size()*8, &store) == PageStoreStatus::BUFFER_TOO_SMALL);
  assert(PageStore::stage_v2(source.data(), source.size(), limits, 8, arena.data(), arena.size()*8,
      scratch.data(), scratch.size()*8, &store) == PageStoreStatus::INVALID_ARGUMENT);
  assert(PageStore::stage_v2(source.data(), source.size(), limits, 8, other.data(), other.size()*8,
      const_cast<uint8_t *>(active_source), store.source_size(), &store) == PageStoreStatus::INVALID_ARGUMENT);
  assert(PageStore::stage_v2(source.data(), source.size(), limits, 8, other.data(), other.size()*8,
      scratch.data(), 1, &store) == PageStoreStatus::BUFFER_TOO_SMALL);
  size_t workspace_size = 0;
  assert(PageStore::v2_workspace_bytes(source.data(), source.size(), &workspace_size) == PageStoreStatus::OK);
  assert(PageStore::stage_v2(source.data(), source.size(), limits, 8, other.data(), other.size()*8,
      scratch.data(), workspace_size - 1, &store) == PageStoreStatus::BUFFER_TOO_SMALL);
  assert(PageStore::stage_v2(source.data(), source.size(), limits, 8, other.data(), other.size()*8,
      reinterpret_cast<uint8_t *>(scratch.data()) + 1, workspace_size, &store) == PageStoreStatus::INVALID_ARGUMENT);
  assert(PageStore::stage_v2(source.data(), source.size(), limits, 8, other.data(), other.size()*8,
      other.data(), other.size()*8, &store) == PageStoreStatus::INVALID_ARGUMENT);
  assert(store.source_document() == active_source && store.generation() == 7);
  for (size_t scenario = 0; scenario < 5; ++scenario) {
    auto records = authored(limits);
    if (scenario == 0) { records[2].id = 63001; records[2].first = 63000; records[2].second = 62000;
      Row first; first.kind=K::PAGE; first.id=63000; first.second=61000; records.push_back(first);
      records[4].first=63000; records[5].first=63000;
      records[4].fields[6]=73; } // Nested folder without its required NAV record.
    if (scenario == 1) records[4].second = 2; // Sparse children cannot round-trip to v1.
    if (scenario == 2) records[3].second = 33;
    if (scenario == 3) { records[4].flags=PANEL_CONFIG_V2_RAW_SOURCE; records[4].raw_source=74; }
    if (scenario == 4) records[0].text = "other-profile";
    const auto invalid = wire(records);
    const auto result = stage(invalid, limits, other, scratch, store);
    assert(result == (scenario == 4 ? PageStoreStatus::PROFILE_MISMATCH : PageStoreStatus::INVALID_DOCUMENT));
    assert(store.source_document() == active_source && store.generation() == 7); ++cases;
  }
  auto corrupt = source; corrupt[0] = 0;
  assert(stage(corrupt, limits, other, scratch, store) == PageStoreStatus::INVALID_DOCUMENT);
}

void dormant_and_owned_source() {
  for (const auto &limits : DEVICE_UI_PROFILES) {
    Bytes source(8192); PanelConfigWriter writer(source.data(), source.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),
      std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string folder=";Dormant;;;;;subpage";
    const std::string children="B,1|light.hidden:Hidden:Auto:Auto:::light_switch";
    assert(writer.append_button(32,reinterpret_cast<const uint8_t *>(folder.data()),folder.size())==PanelConfigStatus::OK);
    assert(writer.append_subpage(32,reinterpret_cast<const uint8_t *>(children.data()),children.size())==PanelConfigStatus::OK);
    assert(writer.append_button(1,reinterpret_cast<const uint8_t *>(""),0)==PanelConfigStatus::OK);
    size_t size=0;assert(writer.finish(&size)==PanelConfigStatus::OK);source.resize(size);
    parity(source,limits);
    auto v2=imported(source);const auto saved=v2;Arena arena(16384),scratch;PageStore store;
    assert(stage(v2,limits,arena,scratch,store)==PageStoreStatus::OK);
    assert(store.card_count()==33 && store.page(0)->card_count==32 && store.page(0)->runtime_card_count==limits.slot_count);
    assert(!store.page(1)->navigable && !store.runtime_card(31) && !store.runtime_card(32));
    std::fill(v2.begin(),v2.end(),0);std::fill(scratch.begin(),scratch.end(),UINT64_MAX);
    assert(store.field(*store.card(31),CardField::LABEL)=="Dormant");
    assert(store.field(*store.card(32),CardField::LABEL)=="Hidden");
    assert(std::memcmp(store.source_document(),saved.data(),saved.size())==0);
  }
}
} // namespace
int main() {
  nested_depth_two(); nested_write_admission(); legacy_child_folders_need_explicit_targets();
  retained_off_grid_subpage_cards();
  corpus(); replacement_and_failures(); hostile_and_bounds(); dormant_and_owned_source();
  std::cout << "PASS direct v2 PageStore parity/normalization/identities/admission/atomic replacement " << cases << " cases\n";
}
