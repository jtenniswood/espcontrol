#pragma once

#include <algorithm>
#include <cstddef>

#include "page_store_runtime.h"

namespace espcontrol::pages {

struct PageArenaMemory {
  size_t reserved_bytes{0};
  size_t peak_reserved_bytes{0};
  size_t live_allocations{0};
  size_t allocation_calls{0};
  size_t allocation_failures{0};
  size_t accounting_errors{0};
};

// Main-loop accounting for requested scene/model bytes. The backing allocator
// must outlive this owner; all matching releases must pass through this
// wrapper. This excludes allocator overhead, the enclosing device object, LVGL
// widgets, image download/decode buffers and platform framebuffers. It never
// keeps a per-allocation registry or changes the backing allocator's admission
// policy.
class PageCountedArena final : public PageArenaAllocator {
public:
  explicit PageCountedArena(PageArenaAllocator &backing) : backing_(backing) {}
  PageCountedArena(const PageCountedArena &) = delete;
  PageCountedArena &operator=(const PageCountedArena &) = delete;

  void *allocate(size_t bytes) override {
    ++memory_.allocation_calls;
    void *allocation = backing_.allocate(bytes);
    if (allocation != nullptr) {
      memory_.reserved_bytes += bytes;
      ++memory_.live_allocations;
      memory_.peak_reserved_bytes =
          std::max(memory_.peak_reserved_bytes, memory_.reserved_bytes);
    } else if (bytes != 0) {
      ++memory_.allocation_failures;
    }
    return allocation;
  }

  void release(void *allocation, size_t bytes) override {
    if (allocation == nullptr)
      return;
    if (memory_.live_allocations == 0 || bytes > memory_.reserved_bytes) {
      // Report broken accounting rather than wrapping a size counter or
      // changing the real allocation's lifetime. Such snapshots are invalid.
      ++memory_.accounting_errors;
    } else {
      --memory_.live_allocations;
      memory_.reserved_bytes -= bytes;
    }
    backing_.release(allocation, bytes);
  }

  PageArenaMemory memory() const { return memory_; }
  size_t allocation_failure_count() const override {
    return memory_.allocation_failures;
  }

private:
  PageArenaAllocator &backing_;
  PageArenaMemory memory_;
};

} // namespace espcontrol::pages
