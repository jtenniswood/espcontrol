#pragma once

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
enum class PageFanStatus : uint8_t { UNAVAILABLE, ON, OFF, UNSUPPORTED, OSCILLATING, STILL };
struct PageFanStatusSnapshot {
  bool on{false}, showing{false};
  PageFanStatus status{PageFanStatus::UNAVAILABLE};
};
struct PageFanStatusMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0};
  size_t sources{0}, failures{0};
};
// Prepare before the HA graph, then fan out graph observations through
// PageEntityObserverSet. Hidden pages retain semantic history, never widgets.
class PageFanStatusRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  using Now = uint32_t (*)();
  PageFanStatusRuntime(PageArenaAllocator&, PageEntitySubscriptions&, Now, size_t max_sources = 32,
                       size_t max_key_bytes = 2048);
  ~PageFanStatusRuntime() override;
  PageFanStatusRuntime(const PageFanStatusRuntime&) = delete;
  PageFanStatusRuntime& operator=(const PageFanStatusRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PageFanStatusSnapshot read(ConfigurationHandle) const;
  uint64_t revision() const {
    return revision_;
  }
  PageFanStatusMemory memory() const;
  static const char* text(PageFanStatus);

 private:
  struct History {
    uint32_t changed_ms{0};
    PageFanStatus status{PageFanStatus::UNAVAILABLE};
    bool showing{false};
  };
  struct Source {
    std::string_view entity;
    History power, oscillation;
    bool available{false}, on{false}, oscillation_known{false}, oscillating{false};
  };
  struct Slot {
    void* arena{nullptr};
    size_t bytes{0}, count{0};
    Source* sources{nullptr};
    PageStore store;
  };
  void clear(Slot&);
  void update(Source&);
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  Now now_;
  size_t max_sources_, max_key_bytes_, peak_{0}, failures_{0};
  uint32_t connection_;
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
