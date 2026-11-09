#include "native_ha_subscription_index.h"
#include "esphome/core/string_ref.h"

#include <cassert>
#include <cstdlib>
#include <functional>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <vector>

namespace {
bool watch_allocations = false;
size_t allocations = 0;
}
void *operator new(size_t size) {
  if (watch_allocations) ++allocations;
  if (auto *p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void *operator new[](size_t size) { return ::operator new(size); }
void operator delete[](void *p) noexcept { ::operator delete(p); }
#ifdef __cpp_sized_deallocation
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
#endif

namespace {
using Text = esphome::StringRef;
struct Entry {
  const char *entity_id, *attribute;
  std::function<void(Text)> callback;
  bool once;
  std::unique_ptr<std::string> entity_id_dynamic_storage, attribute_dynamic_storage;
};
using Entries = std::vector<Entry>;
using Index = NativeHaSubscriptionIndex<>;
Entry entry(std::string entity, const char *attribute, std::function<void(Text)> callback, bool once = false) {
  Entry result{};
  result.entity_id_dynamic_storage = std::make_unique<std::string>(std::move(entity));
  result.entity_id = result.entity_id_dynamic_storage->c_str();
  if (attribute) {
    result.attribute_dynamic_storage = std::make_unique<std::string>(attribute);
    result.attribute = result.attribute_dynamic_storage->c_str();
  }
  result.callback = std::move(callback);
  result.once = once;
  return result;
}
// Independent expression of the pinned SDK's loop: exact complete text,
// nullptr means empty attribute, and every match retains native order.
void stock(const Entries &entries, Text entity, Text attribute, Text state) {
  if (entity.size() == 0) return;
  for (const auto &value : entries) {
    if (std::string_view(entity.c_str(), entity.size()) != std::string_view(value.entity_id)) continue;
    if (value.attribute ? std::string_view(attribute.c_str(), attribute.size()) != std::string_view(value.attribute)
                        : attribute.size() != 0) continue;
    value.callback(state);
  }
}
struct Collisions {
  uint32_t operator()(const char *, size_t, const char *, size_t) const { return 17; }
};

template<typename Policy>
void differential_matching_and_order() {
  using Lookup = NativeHaSubscriptionIndex<Policy>;
  Entries entries;
  entries.reserve(1000);
  std::vector<size_t> calls;
  calls.reserve(1000);
  const std::string state("value\0bytes", 11);
  std::vector<std::string> entities{"sensor.same", "", "sensor.short", "sensor." + std::string(400, 'x'), "sensor.\xC3\xA9"};
  std::vector<std::string> attributes{"", "unit", "temperature", std::string(300, 'a')};
  for (size_t i = 0; i < 1000; ++i) {
    const auto &entity = entities[(i * 13) % entities.size()];
    const char *attribute = i % 7 == 0 ? nullptr : attributes[(i * 17) % attributes.size()].c_str();
    entries.push_back(entry(entity, attribute, [&, i](Text value) {
      assert(value.size() == state.size() && std::memcmp(value.c_str(), state.data(), state.size()) == 0);
      calls.push_back(i);
    }, i % 2 == 0));
  }
  std::vector<typename Lookup::Row> rows(entries.size());
  Lookup lookup;
  uint64_t epoch = 1;
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  assert(lookup.bytes() == entries.size() * 8);
  entities.push_back("missing.entity");
  entities.push_back(std::string("sensor.same\0suffix", 18));
  attributes.push_back("missing");
  attributes.push_back(std::string("unit\0suffix", 11));
  for (const auto &entity : entities) for (const auto &attribute : attributes) {
    calls.clear();
    stock(entries, Text(entity), Text(attribute), Text(state));
    const auto expected = calls;
    calls.clear();
    allocations = 0;
    watch_allocations = true;
    const auto result = lookup.dispatch(entries, epoch, Text(entity), Text(attribute), Text(state));
    watch_allocations = false;
    assert(result == Lookup::Result::COMPLETE && calls == expected && allocations == 0);
  }
  for (const auto &value : entries) {
    assert(value.entity_id == value.entity_id_dynamic_storage->c_str());
    assert(!value.attribute || value.attribute == value.attribute_dynamic_storage->c_str());
  }
  assert(lookup.reset() && lookup.bytes() == 0);
}

void source_freshness_and_rejected_preparation() {
  size_t calls = 0;
  Entries entries;
  entries.reserve(8);
  entries.push_back(entry("sensor.old", nullptr, [&](Text) { ++calls; }));
  std::vector<Index::Row> rows(8);
  Index lookup;
  uint64_t epoch = 1;
  Text entity("sensor.old"), attribute(""), state("live");
  assert(lookup.dispatch(entries, epoch, entity, attribute, state) == Index::Result::FALLBACK);
  assert(!lookup.prepare(entries, epoch, nullptr, 8));
  assert(!lookup.prepare(entries, epoch, rows.data(), 0));
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  const auto *same_data = entries.data();
  entries.push_back(entry("sensor.foreign", "unit", [](Text) {}, true));
  assert(entries.data() == same_data);
  assert(lookup.dispatch(entries, epoch, entity, attribute, state) == Index::Result::FALLBACK && calls == 0);
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  entries[0] = entry("sensor.replaced", nullptr, [](Text) {});
  ++epoch;  // Removal/addition can preserve both native size and data address.
  assert(entries.data() == same_data);
  assert(lookup.dispatch(entries, epoch, entity, attribute, state) == Index::Result::FALLBACK);
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  entries.reserve(32);
  assert(entries.data() != same_data);
  assert(lookup.dispatch(entries, epoch, entity, attribute, state) == Index::Result::FALLBACK);
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  assert(!lookup.prepare(entries, epoch, rows.data(), 1));
  assert(lookup.dispatch(entries, epoch, entity, attribute, state) == Index::Result::FALLBACK);
  Entries invalid;
  invalid.push_back(Entry{});
  assert(!lookup.prepare(invalid, epoch, rows.data(), rows.size()));
  Entries empty;
  assert(lookup.prepare(empty, epoch, nullptr, 0));
  assert(lookup.dispatch(empty, epoch, entity, attribute, state) == Index::Result::COMPLETE);
}

void reentry_cannot_replace_storage_during_dispatch() {
  Entries entries;
  std::vector<Index::Row> rows(2);
  Index lookup;
  uint64_t epoch = 1;
  bool nested = false;
  std::vector<int> calls;
  calls.reserve(4);
  Text entity("sensor.same"), attribute("unit"), state("live");
  entries.push_back(entry("sensor.same", "unit", [&](Text) {
    calls.push_back(0);
    assert(lookup.dispatching());
    assert(!lookup.reset() && !lookup.prepare(entries, epoch, rows.data(), rows.size()));
    if (!nested) {
      nested = true;
      assert(lookup.dispatch(entries, epoch, entity, attribute, state) == Index::Result::COMPLETE);
    }
  }));
  entries.push_back(entry("sensor.same", "unit", [&](Text) { calls.push_back(1); }));
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  assert(lookup.dispatch(entries, epoch, entity, attribute, state) == Index::Result::COMPLETE);
  assert((calls == std::vector<int>{0, 0, 1, 1}));
  assert(!lookup.dispatching() && lookup.reset());
}

void invalidation_interrupts_without_double_delivery() {
  Entries entries;
  std::vector<Index::Row> rows(2);
  Index lookup;
  uint64_t epoch = 1;
  size_t first = 0, second = 0;
  entries.push_back(entry("sensor.same", "unit", [&](Text) { ++first; ++epoch; }));
  entries.push_back(entry("sensor.same", "unit", [&](Text) { ++second; }));
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  assert(lookup.dispatch(entries, epoch, Text("sensor.same"), Text("unit"), Text("live")) == Index::Result::INTERRUPTED);
  assert(first == 1 && second == 0 && !lookup.dispatching());
  assert(lookup.dispatch(entries, epoch, Text("sensor.same"), Text("unit"), Text("live")) == Index::Result::FALLBACK);
  assert(first == 1 && second == 0);
}

void maximum_workload_and_borrowed_storage_reuse() {
  Entries entries;
  entries.reserve(7600);
  std::vector<Index::Row> rows(7600);
  Index lookup;
  uint64_t epoch = 1;
  size_t calls = 0;
  for (size_t i = 0; i < 7600; ++i)
    entries.push_back(entry("sensor.maximum_" + std::to_string(i), "unit", [&](Text) { ++calls; }));
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  allocations = 0;
  watch_allocations = true;
  assert(lookup.dispatch(entries, epoch, Text("sensor.maximum_7599"), Text("unit"), Text("live")) == Index::Result::COMPLETE);
  assert(lookup.dispatch(entries, epoch, Text("sensor.missing"), Text("unit"), Text("live")) == Index::Result::COMPLETE);
  watch_allocations = false;
  assert(calls == 1 && allocations == 0 && lookup.bytes() == 60800);
  assert(lookup.reset());
  entries.clear();
  entries.push_back(entry("sensor.replacement", nullptr, [&](Text) { ++calls; }));
  ++epoch;
  assert(lookup.prepare(entries, epoch, rows.data(), rows.size()));
  assert(lookup.bytes() == 8);
  assert(lookup.dispatch(entries, epoch, Text("sensor.replacement"), Text(""), Text("live")) == Index::Result::COMPLETE);
  assert(calls == 2);
}
}

int main() {
  differential_matching_and_order<NativeHaSubscriptionHash>();
  differential_matching_and_order<Collisions>();
  source_freshness_and_rejected_preparation();
  reentry_cannot_replace_storage_during_dispatch();
  invalidation_interrupts_without_double_delivery();
  maximum_workload_and_borrowed_storage_reuse();
}
