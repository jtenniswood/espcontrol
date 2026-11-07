#include <cstdlib>
#include <cstdio>
#include <limits>
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "external_memory_allocator.h"
#include "ha_read_coordinator.h"

namespace {

void require_at(bool condition, int line) {
  if (!condition) { std::fprintf(stderr, "allocator check failed at line %d\n", line); std::abort(); }
}
#define require(condition) require_at((condition), __LINE__)

void reset_allocator_state() {
  fake_esphome_allocator::external_available = true;
  fake_esphome_allocator::internal_available = true;
  require(fake_esphome_allocator::external_pointers.empty());
  fake_esphome_allocator::last_allocation_flags = 0;
  EspControlExternalAllocatorStats::external_bytes = 0;
  EspControlExternalAllocatorStats::internal_bytes = 0;
  EspControlExternalAllocatorStats::external_allocations = 0;
  EspControlExternalAllocatorStats::internal_fallbacks = 0;
  EspControlExternalAllocatorStats::failed_allocations = 0;
}

void external_memory_is_preferred() {
  reset_allocator_state();
  EspControlExternalAllocator<int> allocator;
  int *pointer = allocator.allocate(4);
  require(pointer != nullptr);
  require(fake_esphome_allocator::last_allocation_flags ==
          (esphome::RAMAllocator<int>::ALLOC_EXTERNAL |
           esphome::RAMAllocator<int>::ALLOC_INTERNAL));
  require(EspControlExternalAllocatorStats::external_bytes == 4 * sizeof(int));
  require(EspControlExternalAllocatorStats::external_allocations == 1);
  require(EspControlExternalAllocatorStats::internal_bytes == 0);
  allocator.deallocate(pointer, 4);
  require(EspControlExternalAllocatorStats::external_bytes == 0);
}

void internal_memory_is_the_fallback() {
  reset_allocator_state();
  fake_esphome_allocator::external_available = false;
  EspControlExternalAllocator<int> allocator;
  int *pointer = allocator.allocate(3);
  require(pointer != nullptr);
  require(EspControlExternalAllocatorStats::external_bytes == 0);
  require(EspControlExternalAllocatorStats::internal_bytes == 3 * sizeof(int));
  require(EspControlExternalAllocatorStats::internal_fallbacks == 1);
  allocator.deallocate(pointer, 3);
  require(EspControlExternalAllocatorStats::internal_bytes == 0);
}

void overlapping_allocations_keep_their_regions() {
  for (bool reverse : {false, true}) {
    reset_allocator_state();
    EspControlExternalAllocator<int> allocator;
    int *first = allocator.allocate(2);
    fake_esphome_allocator::external_available = false;
    int *fallback = allocator.allocate(3);
    fake_esphome_allocator::external_available = true;
    int *second = allocator.allocate(4);
    require(EspControlExternalAllocatorStats::external_bytes == 6 * sizeof(int));
    require(EspControlExternalAllocatorStats::internal_bytes == 3 * sizeof(int));
    require(EspControlExternalAllocatorStats::internal_fallbacks == 1);
    allocator.deallocate(reverse ? second : first, reverse ? 4 : 2);
    require(EspControlExternalAllocatorStats::external_bytes ==
            (reverse ? 2 : 4) * sizeof(int));
    require(EspControlExternalAllocatorStats::internal_bytes == 3 * sizeof(int));
    allocator.deallocate(fallback, 3);
    require(EspControlExternalAllocatorStats::internal_bytes == 0);
    allocator.deallocate(reverse ? first : second, reverse ? 2 : 4);
    require(EspControlExternalAllocatorStats::external_bytes == 0);
    require(fake_esphome_allocator::external_pointers.empty());
  }
}

struct BorrowedState {
  BorrowedState(const char *bytes, size_t length) : bytes(bytes), length(length) {}
  const char *c_str() const { return bytes; }
  size_t size() const { return length; }
  const char *bytes;
  size_t length;
};
struct StringStorageTransport {
  using State = BorrowedState;
  using Callback = std::function<void(State)>;
  static constexpr bool RECLAIMABLE_SUBSCRIPTIONS = true;
  bool available() const { return true; }
  bool state_connected() const { return true; }
  uint32_t subscribe(const std::string &, const std::string &, Callback callback) {
    callbacks.push_back(std::move(callback)); return callbacks.size();
  }
  void unsubscribe(uint32_t token) { callbacks.at(token-1) = {}; }
  bool request(std::string_view entity, std::string_view attribute) {
    requested_entity.assign(entity.data(), entity.size());
    requested_attribute.assign(attribute.data(), attribute.size());
    return true;
  }
  // Reject an accidental conversion back to default-allocator strings.
  bool request(const std::string &, const std::string &) = delete;
  std::vector<Callback> callbacks;
  std::string requested_entity, requested_attribute;
};
struct StringStorageHeap {
  bool available(const char *, size_t, size_t) const { return true; }
};
struct MissingSnapshot {
  const char *data() const { return nullptr; }
  size_t size() const { return 0; }
  bool current() const { return false; }
};
void coordinator_strings_follow_region_and_snapshot_lifetimes(bool external) {
  reset_allocator_state();
  fake_esphome_allocator::external_available = external;
  const std::string entity = "sensor." + std::string(96, 'e');
  const std::string attribute(80, 'a'), value(512, 'v');
  using Coordinator = HaReadCoordinator<StringStorageTransport, StringStorageHeap,
                                        EspControlExternalAllocator<std::byte>>;
  {
    Coordinator coordinator; int owner = 0, replay_owner = 0;
    require(coordinator.subscribe(entity, attribute, [](BorrowedState) {}, 1, &owner, true));
    bool inspected = false;
    require(coordinator.replay_owner_missing(&owner,
        [&](std::string_view stored_entity, std::string_view stored_attribute) {
          require(stored_entity == entity && stored_attribute == attribute);
          require((fake_esphome_allocator::external_pointers.count(stored_entity.data()) != 0) == external);
          require((fake_esphome_allocator::external_pointers.count(stored_attribute.data()) != 0) == external);
          inspected = true;
          return std::optional<MissingSnapshot>{};
        }) == 0);
    require(inspected);
    const size_t names_bytes = external ? EspControlExternalAllocatorStats::external_bytes
                                       : EspControlExternalAllocatorStats::internal_bytes;
    coordinator.transport().callbacks[0](BorrowedState(value.data(), value.size()));
    const size_t retained_bytes = external ? EspControlExternalAllocatorStats::external_bytes
                                          : EspControlExternalAllocatorStats::internal_bytes;
    require(retained_bytes >= names_bytes + 2 * (value.size() + 1));
    bool replayed = false;
    require(coordinator.subscribe(entity, attribute, [&](BorrowedState state) {
      require(state.size() == value.size() && std::memcmp(state.c_str(), value.data(), value.size()) == 0);
      require(state.c_str()[state.size()] == '\0');
      if (!replayed) require((fake_esphome_allocator::external_pointers.count(state.c_str()) != 0) == external);
      replayed = true;
    }, 2, &replay_owner, true));
    require(replayed);
    bool read = false;
    require(coordinator.read_retained(entity, attribute, [&](BorrowedState state) {
      require((fake_esphome_allocator::external_pointers.count(state.c_str()) != 0) == external);
      coordinator.invalidate_retained_state();
      require((fake_esphome_allocator::external_pointers.count(state.c_str()) != 0) == external);
      require(state.size() == value.size() && std::memcmp(state.c_str(), value.data(), value.size()) == 0);
      require(state.c_str()[state.size()] == '\0');
      read = true;
    }, true, 0, 0));
    require(read);
    // Views need not be terminated at their declared end. The transport sees
    // exact lengths, including the scheduled path from allocated channel names.
    const auto entity_with_suffix = entity + "ignored", attribute_with_suffix = attribute + "ignored";
    require(coordinator.request_fresh(std::string_view(entity_with_suffix.data(), entity.size()),
                                      std::string_view(attribute_with_suffix.data(), attribute.size())));
    require(coordinator.transport().requested_entity == entity && coordinator.transport().requested_attribute == attribute);
    coordinator.transport().callbacks[0](BorrowedState(value.data(), value.size()));
    require(coordinator.schedule_fresh(entity, attribute, 1, 100));
    coordinator.flush_fresh(200, 0, 0);
    require(coordinator.transport().requested_entity == entity && coordinator.transport().requested_attribute == attribute);
    coordinator.release_owner(&owner); coordinator.release_owner(&replay_owner);
    require(coordinator.reclaim_unused_channels() == 1);
    require(coordinator.subscription_channel_count() == 0);
  }
  require(fake_esphome_allocator::external_pointers.empty());
  require(EspControlExternalAllocatorStats::external_bytes == 0);
  require(EspControlExternalAllocatorStats::internal_bytes == 0);
  require(external ? EspControlExternalAllocatorStats::internal_fallbacks == 0
                   : EspControlExternalAllocatorStats::internal_fallbacks != 0);
}

template<typename Callback>
void require_abort(Callback callback) {
  pid_t child = fork();
  require(child >= 0);
  if (child == 0) {
    callback();
    _exit(EXIT_SUCCESS);
  }
  int status = 0;
  require(waitpid(child, &status, 0) == child);
  require(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}

void exhaustion_uses_the_explicit_fatal_path() {
  require_abort([] {
    reset_allocator_state();
    fake_esphome_allocator::external_available = false;
    fake_esphome_allocator::internal_available = false;
    EspControlExternalAllocator<int> allocator;
    (void) allocator.allocate(1);
  });
}

void allocation_overflow_uses_the_explicit_fatal_path() {
  require_abort([] {
    reset_allocator_state();
    EspControlExternalAllocator<int> allocator;
    (void) allocator.allocate(allocator.max_size() + 1);
  });
}

}  // namespace

int main() {
  external_memory_is_preferred();
  internal_memory_is_the_fallback();
  overlapping_allocations_keep_their_regions();
  coordinator_strings_follow_region_and_snapshot_lifetimes(true);
  coordinator_strings_follow_region_and_snapshot_lifetimes(false);
  exhaustion_uses_the_explicit_fatal_path();
  allocation_overflow_uses_the_explicit_fatal_path();
  return EXIT_SUCCESS;
}
