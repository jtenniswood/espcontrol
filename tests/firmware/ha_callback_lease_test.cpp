#include "ha_callback_lease.h"

#include <cassert>
#include <functional>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace {
struct AllocationState {
  size_t allocations = 0;
  size_t deallocations = 0;
  bool reject_construction = false;
};

template <typename T> struct StatefulAllocator {
  using value_type = T;
  AllocationState *state;
  explicit StatefulAllocator(AllocationState *state) : state(state) {}
  template <typename U>
  StatefulAllocator(const StatefulAllocator<U> &other) : state(other.state) {}
  T *allocate(size_t count) {
    ++state->allocations;
    return std::allocator<T>{}.allocate(count);
  }
  void deallocate(T *pointer, size_t count) {
    ++state->deallocations;
    std::allocator<T>{}.deallocate(pointer, count);
  }
  template <typename U, typename... Args>
  void construct(U *pointer, Args &&...args) {
    if (state->reject_construction)
      throw std::bad_alloc();
    ::new (static_cast<void *>(pointer)) U(std::forward<Args>(args)...);
  }
};

using Callback = std::function<void()>;
using Lease = HaCallbackLease<Callback, StatefulAllocator<Callback>>;
static_assert(sizeof(Lease) == sizeof(void *),
              "Snapshots need only one block pointer");

void copies_moves_and_reentrant_release_keep_the_callback_alive() {
  AllocationState state;
  std::weak_ptr<int> lifetime;
  size_t calls = 0;
  {
    Lease owner;
    auto captured = std::make_shared<int>(42);
    lifetime = captured;
    owner = Lease::create(
        [&, captured] {
          owner = {};
          assert(*captured == 42 && !lifetime.expired());
          ++calls;
        },
        StatefulAllocator<Callback>(&state));
    captured.reset();
    const auto allocation_calls = state.allocations;
    Lease snapshot = owner;
    auto *self = &snapshot;
    snapshot = *self;
    snapshot = std::move(*self);
    assert(snapshot == owner && state.allocations == allocation_calls);
    Lease moved = std::move(snapshot);
    assert(!snapshot && moved == owner);
    (*moved)();
    assert(!owner && calls == 1 && !lifetime.expired());
    snapshot = moved;
    moved = {};
    assert(snapshot && !lifetime.expired());
    snapshot = {};
    assert(lifetime.expired());
  }
  assert(state.allocations == 1 && state.deallocations == 1);
}

void independent_callbacks_and_cancelled_snapshots_are_released_once() {
  AllocationState state;
  {
    std::vector<Lease> owners;
    for (size_t i = 0; i < 7600; ++i)
      owners.push_back(
          Lease::create([] {}, StatefulAllocator<Callback>(&state)));
    auto snapshots = owners;
    assert(owners[0] != owners[1]);
    for (auto &owner : owners)
      *owner = nullptr;
    owners.clear();
    assert(state.deallocations == 0);
    for (const auto &snapshot : snapshots)
      assert(snapshot && !*snapshot);
  }
  assert(state.allocations == 7600 && state.deallocations == 7600);
}

void failed_construction_returns_storage_to_the_same_allocator() {
  AllocationState state;
  state.reject_construction = true;
  bool threw = false;
  try {
    auto lease = Lease::create([] {}, StatefulAllocator<Callback>(&state));
    (void)lease;
  } catch (const std::bad_alloc &) {
    threw = true;
  }
  assert(threw && state.allocations == 1 && state.deallocations == 1);
}
} // namespace

int main() {
  copies_moves_and_reentrant_release_keep_the_callback_alive();
  independent_callbacks_and_cancelled_snapshots_are_released_once();
  failed_construction_returns_storage_to_the_same_allocator();
}
