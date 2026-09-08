#include <cstdlib>
#include <limits>

#include <sys/wait.h>
#include <unistd.h>

#include "external_memory_allocator.h"

namespace {

void require(bool condition) {
  if (!condition) std::abort();
}

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
  exhaustion_uses_the_explicit_fatal_path();
  allocation_overflow_uses_the_explicit_fatal_path();
  return EXIT_SUCCESS;
}
