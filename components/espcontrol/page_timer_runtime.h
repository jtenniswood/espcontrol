#pragma once
#include "page_entity_state_runtime.h"
namespace espcontrol::pages {
enum class PageTimerMode : uint8_t { UNAVAILABLE, IDLE, ACTIVE, PAUSED };
enum class PageTimerAction : uint8_t { NONE, START, CANCEL };
struct PageTimerSnapshot {
  PageTimerMode mode{PageTimerMode::UNAVAILABLE};
  int seconds{0};
  uint16_t confirmation_seconds{3};
  bool confirmation{false};
  uint32_t confirmation_revision{0};
};
struct PageTimerMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0}, sources{0}, cards{0},
      preparation_failures{0};
};
// Shared actual countdown anchors with per-card natural-finish/cancel history.
// Prepare before the graph. The application loop drives a single 250ms poll;
// this owner creates no timers, widgets, subscriptions or actions.
class PageTimerRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  using Now = uint32_t (*)();
  using Epoch = int64_t (*)();
  PageTimerRuntime(PageArenaAllocator&, PageEntitySubscriptions&, Now, Epoch, size_t max_sources = 32,
                   size_t max_cards = 512, size_t max_key_bytes = 2048);
  ~PageTimerRuntime() override;
  PageTimerRuntime(const PageTimerRuntime&) = delete;
  PageTimerRuntime& operator=(const PageTimerRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PageTimerSnapshot read(ConfigurationHandle) const;
  PageTimerAction action(ConfigurationHandle);
  void cancel_sent(ConfigurationHandle, bool accepted);
  uint64_t revision() const {
    return revision_;
  }
  uint32_t connection_generation() const {
    return subscriptions_.connection_generation();
  }
  uint32_t now_ms() const {
    return now_ ? now_() : 0;
  }
  PageTimerMemory memory() const;
  static bool supported(const PageStore&, CardId);

 private:
  struct Source {
    std::string_view entity;
    int duration{0}, remaining{0};
    int64_t finish_epoch{0};
    uint32_t anchor{0}, confirmation_revision{0};
    PageTimerMode mode{PageTimerMode::UNAVAILABLE};
    uint8_t known{0};
  };
  struct Card {
    CardId card{NO_CARD};
    uint16_t source{0}, confirmation_seconds{3};
    uint32_t finished_ms{0};
    bool confirmation{false}, cancel_requested{false};
  };
  struct Slot {
    void* arena{nullptr};
    size_t bytes{0}, sources{0}, cards{0};
    Source* source{nullptr};
    Card* card{nullptr};
    uint32_t generation{0};
    bool valid{false};
  };
  void clear(Slot&);
  int64_t remaining(const Source&) const;
  Card* find(ConfigurationHandle);
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  Now now_;
  Epoch epoch_;
  size_t max_sources_, max_cards_, max_key_bytes_, peak_{0}, failures_{0};
  uint32_t connection_, last_tick_{0};
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
