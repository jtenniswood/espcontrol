#pragma once

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
enum class PagePercentSliderKind : uint8_t { NONE, LIGHT, FAN, COVER_POSITION, COVER_TILT };
struct PagePercentSliderSnapshot {
  PagePercentSliderKind kind{PagePercentSliderKind::NONE};
  uint8_t percent{0};
  bool available{false}, icon_on{false}, preview{false}, fill_known{false};
  uint32_t write_revision{0};
};
struct PagePercentSliderMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0};
  size_t sources{0}, cards{0}, preparation_failures{0};
};
// Ordered report semantics, independent of widgets. Prepare before the HA graph
// and fan out observations through PageEntityObserverSet. Previews are per-card;
// a shared remote report reconciles every consumer, including hidden pages.
class PagePercentSliderRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  PagePercentSliderRuntime(PageArenaAllocator&, PageEntitySubscriptions&, size_t max_sources = 32,
                           size_t max_cards = 512, size_t max_key_bytes = 2048);
  ~PagePercentSliderRuntime() override;
  PagePercentSliderRuntime(const PagePercentSliderRuntime&) = delete;
  PagePercentSliderRuntime& operator=(const PagePercentSliderRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PagePercentSliderSnapshot read(ConfigurationHandle) const;
  bool preview(ConfigurationHandle, uint8_t percent);
  void discard_preview(ConfigurationHandle, uint8_t expected, uint32_t write_revision);
  uint32_t connection_generation() const {
    return subscriptions_.connection_generation();
  }
  uint64_t revision() const {
    return revision_;
  }
  PagePercentSliderMemory memory() const;
  static PagePercentSliderKind classify(const PageStore&, CardId);
  static const char* attribute(PagePercentSliderKind);

 private:
  struct Source {
    std::string_view entity;
    uint32_t write_revision{0};
    PagePercentSliderKind kind{PagePercentSliderKind::NONE};
    uint8_t percent{0}, seed_level{0};
    bool available{true}, icon_on{false};
    bool seed_primary{false}, seed_off{false}, seed_level_known{false};
    bool live_primary{false}, live_fill{false}, live_icon{false};
  };
  struct Card {
    CardId card{NO_CARD};
    uint16_t source{0};
    uint8_t percent{0};
    bool preview{false};
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
  void clear_previews(Slot&, size_t source);
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  size_t max_sources_, max_cards_, max_key_bytes_, peak_{0}, failures_{0};
  uint32_t connection_;
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
