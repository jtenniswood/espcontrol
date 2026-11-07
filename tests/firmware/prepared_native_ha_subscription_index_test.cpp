#include "prepared_native_ha_subscription_index.h"
#include "esphome/core/string_ref.h"

#include <cassert>
#include <functional>
#include <vector>

namespace {
using Text = esphome::StringRef;
using Row = NativeHaSubscriptionIndex<>::Row;
struct AllocationState {
  bool fail{false};
  size_t bytes{0}, peak{0}, attempts{0}, frees{0};
};
struct Allocator {
  AllocationState *state;
  Row *allocate(size_t count) {
    ++state->attempts;
    if (state->fail) return nullptr;
    auto *result = static_cast<Row *>(::operator new(count * sizeof(Row), std::nothrow));
    if (result) {
      state->bytes += count * sizeof(Row);
      state->peak = std::max(state->peak, state->bytes);
    }
    return result;
  }
  void deallocate(Row *rows, size_t count) {
    state->bytes -= count * sizeof(Row);
    ++state->frees;
    ::operator delete(rows);
  }
};
using Owner = PreparedNativeHaSubscriptionIndex<Allocator>;
struct Entry {
  const char *entity_id, *attribute;
  std::function<void(Text)> callback;
  bool once{false};
};
using Entries = std::vector<Entry>;
Entry entry(size_t &calls) {
  return {"sensor.same", "unit", [&](Text value) {
    assert(value.size() == 4 && std::memcmp(value.c_str(), "live", 4) == 0);
    ++calls;
  }};
}

void exhaustion_backoff_and_clock_rollover() {
  AllocationState allocations;
  allocations.fail = true;
  int source = 0;
  size_t calls = 0;
  Entries entries(4, entry(calls));
  {
    Owner owner(Allocator{&allocations}, 2, 16);
    assert(!owner.maintain(&source, entries, UINT32_MAX - 500));
    assert(owner.bytes() == 0 && owner.capacity() == 0 && owner.failures() == 1);
    assert(!owner.maintain(&source, entries, UINT32_MAX - 1));
    assert(!owner.maintain(&source, entries, 498));
    assert(allocations.attempts == 1);
    assert(!owner.maintain(&source, entries, 499));
    assert(allocations.attempts == 2 && owner.failures() == 2);
    allocations.fail = false;
    assert(!owner.maintain(&source, entries, 1000));
    assert(owner.maintain(&source, entries, 1499));
    assert(allocations.attempts == 3 && owner.bytes() == 32 && owner.current(&source, entries));
    assert(owner.dispatch(&source, entries, Text("sensor.same"), Text("unit"), Text("live")) == Owner::Result::COMPLETE);
    assert(calls == 4 && allocations.attempts == 3);
  }
  assert(allocations.bytes == 0);
}

void native_changes_and_exact_capacity_have_no_double_peak() {
  AllocationState allocations;
  int first_source = 0, second_source = 0;
  size_t calls = 0;
  Entries entries;
  entries.reserve(32);
  for (size_t i = 0; i < 4; ++i) entries.push_back(entry(calls));
  Owner owner(Allocator{&allocations}, 2, 16);
  assert(owner.maintain(&first_source, entries, 0));
  const auto *same_data = entries.data();
  entries[0].entity_id = "sensor.replaced";
  owner.changed();
  assert(!owner.current(&first_source, entries));
  assert(owner.dispatch(&first_source, entries, Text("sensor.same"), Text("unit"), Text("live")) == Owner::Result::FALLBACK);
  assert(owner.maintain(&first_source, entries, 1));
  assert(entries.data() == same_data && allocations.attempts == 1);
  assert(owner.dispatch(&first_source, entries, Text("sensor.same"), Text("unit"), Text("live")) == Owner::Result::COMPLETE);
  assert(calls == 3);
  assert(!owner.current(&second_source, entries));
  assert(owner.maintain(&second_source, entries, 2));
  assert(!owner.current(&first_source, entries) && allocations.attempts == 1);
  entries.push_back(entry(calls));
  assert(entries.data() == same_data && !owner.current(&second_source, entries));
  assert(owner.maintain(&second_source, entries, 3));
  assert(owner.bytes() == 40 && allocations.attempts == 2 && allocations.peak == 40);
  entries.resize(3);
  assert(owner.maintain(&second_source, entries, 4));
  assert(owner.bytes() == 24 && allocations.bytes == 24 && allocations.peak == 40);
  entries.reserve(64);
  assert(!owner.current(&second_source, entries));
  assert(owner.maintain(&second_source, entries, 5));
  assert(allocations.attempts == 3 && owner.rebuilds() == 6);
  entries.clear();
  assert(!owner.maintain(&second_source, entries, 6));
  assert(owner.bytes() == 0 && allocations.bytes == 0);
}

void disabled_and_invalid_sets_release_storage() {
  AllocationState allocations;
  int source = 0;
  size_t calls = 0;
  Entries entries(1, entry(calls));
  Owner owner(Allocator{&allocations}, 2, 16);
  assert(!owner.maintain(&source, entries, 0) && allocations.attempts == 0);
  entries.resize(17, entry(calls));
  assert(!owner.maintain(&source, entries, 1) && allocations.attempts == 0);
  entries.resize(4);
  assert(owner.maintain(&source, entries, 2));
  assert(!owner.maintain(nullptr, entries, 3));
  assert(owner.bytes() == 0 && allocations.bytes == 0);
  entries[0].entity_id = nullptr;
  assert(!owner.maintain(&source, entries, 4));
  assert(allocations.bytes == 0 && owner.failures() == 1);
  assert(owner.dispatch(&source, entries, Text("sensor.same"), Text("unit"), Text("live")) == Owner::Result::FALLBACK);
  assert(owner.reset() && owner.bytes() == 0 && calls == 0);
}

void delivery_guards_cover_stock_and_fast_paths() {
  AllocationState allocations;
  int source = 0;
  Owner owner(Allocator{&allocations}, 2, 16);
  Entries entries;
  size_t first = 0, second = 0;
  bool invalidate = true;
  entries.push_back({"sensor.same", "unit", [&](Text) {
    ++first;
    assert(owner.delivering());
    assert(!owner.reset() && !owner.maintain(&source, entries, 1000));
    if (invalidate) { owner.changed(); invalidate = false; }
  }});
  entries.push_back({"sensor.same", "unit", [&](Text) { ++second; }});
  // The native wrapper must retain this guard around the original SDK method
  // too, including before any index has been prepared.
  {
    auto stock_guard = owner.delivery();
    assert(owner.delivering());
    assert(!owner.maintain(&source, entries, 0) && !owner.reset());
    assert(allocations.attempts == 0);
  }
  assert(owner.maintain(&source, entries, 1));
  assert(owner.dispatch(&source, entries, Text("sensor.same"), Text("unit"), Text("live")) == Owner::Result::INTERRUPTED);
  assert(first == 1 && second == 0 && !owner.delivering() && owner.bytes() == 16);
  assert(owner.dispatch(&source, entries, Text("sensor.same"), Text("unit"), Text("live")) == Owner::Result::FALLBACK);
  assert(first == 1 && second == 0);
  assert(owner.maintain(&source, entries, 2));
  assert(owner.dispatch(&source, entries, Text("sensor.same"), Text("unit"), Text("live")) == Owner::Result::COMPLETE);
  assert(first == 2 && second == 1 && allocations.attempts == 1);
  assert(owner.reset() && allocations.bytes == 0);
}
}

int main() {
  exhaustion_backoff_and_clock_rollover();
  native_changes_and_exact_capacity_have_no_double_peak();
  disabled_and_invalid_sets_release_storage();
  delivery_guards_cover_stock_and_fast_paths();
}
