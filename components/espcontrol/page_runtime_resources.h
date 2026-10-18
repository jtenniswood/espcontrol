#pragma once

#include <cstdint>
#include "page_store.h"

namespace espcontrol::pages {

// PageStoreRuntime stages privately before durable publication assigns the real
// generation. Resources must promote this sentinel, not compare it as a live ID.
// Zero is valid for initial loads; track preparation presence separately.
inline bool page_prepared_generation_matches(uint32_t prepared, uint32_t committed) {
  return prepared == committed || prepared == UINT32_MAX;
}

// Resources that must be reserved before durable publication. The owner must
// outlive PageStoreRuntime. Activation and retirement cannot allocate or fail.
// Copy the immutable PageStore view when needed; its argument object itself is
// temporary. Its backing arena remains valid until the matching retirement.
// Preparation may use UINT32_MAX; activation supplies the committed generation.
class PageRuntimeResources {
 public:
  virtual ~PageRuntimeResources() = default;
  virtual bool prepare(const PageStore &store) = 0;
  virtual void discard_prepared() = 0;
  virtual void activate_prepared(const PageStore &active) = 0;
  virtual void retire_active() = 0;
};
}  // namespace espcontrol::pages
