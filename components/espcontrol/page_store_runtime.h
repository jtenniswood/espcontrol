#pragma once

#include "configuration_service.h"
#include "page_store.h"
#include "page_runtime_resources.h"

namespace espcontrol::pages {

// Device wiring supplies fallible, aligned arena storage (PSRAM on ESP32).
// No fallback to scarce internal RAM is implied by this interface.
class PageArenaAllocator {
 public:
  virtual ~PageArenaAllocator() = default;
  virtual void *allocate(size_t bytes) = 0;
  virtual void release(void *arena, size_t bytes) = 0;
  // Shared scene allocators can also observe downstream resource preparation.
  // Implementations without counters still classify direct arena failures.
  virtual size_t allocation_failure_count() const { return 0; }
};

struct PageRuntimeMemory {
  size_t active_reserved_bytes{0};
  size_t staged_reserved_bytes{0};
  size_t peak_reserved_bytes{0};
  size_t preparation_scratch_bytes{0};
  size_t peak_preparation_scratch_bytes{0};
  size_t preparation_failures{0};
};

// Owns immutable page definitions across a prepared configuration replacement.
// The capacity is a hard staging ceiling; retained arenas contain only the
// bytes used by that generation. The allocator must outlive this owner.
class PageStoreRuntime final : public configuration::ConfigurationRuntimeAdapter {
 public:
  PageStoreRuntime(const DeviceUiLimits &limits, PageArenaAllocator &allocator,
                   size_t arena_capacity, PageRuntimeResources *resources = nullptr)
      : limits_(limits), allocator_(allocator), arena_capacity_(arena_capacity), resources_(resources) {}
  ~PageStoreRuntime() override;
  PageStoreRuntime(const PageStoreRuntime &) = delete;
  PageStoreRuntime &operator=(const PageStoreRuntime &) = delete;

  bool prepare(uint16_t version, const uint8_t *document, size_t size) override;
  bool preparation_allocation_failed() const override {
    return preparation_allocation_failed_;
  }
  void discard_prepared() override;
  bool activate_prepared(uint32_t generation, uint16_t version,
                         const uint8_t *document, size_t size) override;
  // A generation must come from a successful durable commit. Calling the old
  // apply-only entry point must not manufacture a replacement generation.
  bool apply(uint16_t, const uint8_t *, size_t) override { return false; }

  const PageStore &active() const { return active_; }
  const PageRuntimeMemory &memory() const { return memory_; }
  PageStoreStatus last_stage_status() const { return last_stage_status_; }

 private:
  const DeviceUiLimits &limits_;
  PageArenaAllocator &allocator_;
  size_t arena_capacity_;
  PageRuntimeResources *resources_;
  void *active_arena_{nullptr};
  void *staged_arena_{nullptr};
  PageStore active_;
  PageStore staged_;
  PageRuntimeMemory memory_;
  PageStoreStatus last_stage_status_{PageStoreStatus::OK};
  bool preparation_allocation_failed_{false};
};

}  // namespace espcontrol::pages
