#pragma once

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
struct PageCardDependencyPolicy {
#ifdef ESPCONTROL_LOW_HEAP_MEDIA_CONTROL
  bool low_heap_media_control{true};
#else
  bool low_heap_media_control{false};
#endif
};

// Declares state needed by stored, navigable pages without creating widgets.
// Forecast responses, local sensors, images and transient modal discovery have
// separate owners; this source covers their static HA state/attribute inputs.
class PageCardDependencies final : public PageDependencySource {
 public:
  explicit PageCardDependencies(PageCardDependencyPolicy policy = {}) : policy_(policy) {}
  bool enumerate(const PageStore &store, PageDependencySink &sink) const override;
 private:
  PageCardDependencyPolicy policy_;
};
}  // namespace espcontrol::pages
