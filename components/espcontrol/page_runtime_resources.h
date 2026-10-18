#pragma once

#include <cstdint>
#include "page_store.h"

namespace espcontrol::pages {

// Resources that must be reserved before durable publication. The owner must
// outlive PageStoreRuntime. Activation and retirement cannot allocate or fail.
// Copy the immutable PageStore view when needed; its argument object itself is
// temporary. Its backing arena remains valid until the matching retirement.
class PageRuntimeResources {
 public:
  virtual ~PageRuntimeResources() = default;
  virtual bool prepare(const PageStore &store) = 0;
  virtual void discard_prepared() = 0;
  virtual void activate_prepared(const PageStore &active) = 0;
  virtual void retire_active() = 0;
};
}  // namespace espcontrol::pages
