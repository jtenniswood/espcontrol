#pragma once

#include "number_slider_policy.h"
#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
struct PageNumberSliderSnapshot {
  number_slider::Metadata metadata;
  uint16_t position{0}, positions{100};
  uint8_t fill_percent{0};
  bool available{false}, ready{false}, dragging{false}, gesture{false};
};
struct PageNumberSliderMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0};
  size_t sources{0}, cards{0}, preparation_failures{0};
};
// Configuration-owned numeric ranges, last state and per-card thumb history.
// Prepare before the graph and fan out observations via PageEntityObserverSet.
// This owner never formats text, holds widgets or dispatches actions.
class PageNumberSliderRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  PageNumberSliderRuntime(PageArenaAllocator&, PageEntitySubscriptions&, size_t max_sources = 32,
                          size_t max_cards = 512, size_t max_key_bytes = 2048);
  ~PageNumberSliderRuntime() override;
  PageNumberSliderRuntime(const PageNumberSliderRuntime&) = delete;
  PageNumberSliderRuntime& operator=(const PageNumberSliderRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PageNumberSliderSnapshot read(ConfigurationHandle) const;
  bool begin(ConfigurationHandle);
  bool drag(ConfigurationHandle, int position);
  bool finish(ConfigurationHandle, int position, double& value);
  void cancel(ConfigurationHandle);
  void discard_finished(ConfigurationHandle, double expected, const number_slider::Metadata&);
  uint64_t revision() const {
    return revision_;
  }
  uint32_t connection_generation() const {
    return subscriptions_.connection_generation();
  }
  PageNumberSliderMemory memory() const;
  static bool supported(const PageStore&, CardId);

 private:
  struct Source {
    std::string_view entity;
    number_slider::Metadata metadata;
    double state{0};
    uint8_t known{0};
  };
  struct Card {
    CardId card{NO_CARD};
    uint16_t source{0}, position{0}, positions{100};
    uint8_t fill{0};
    bool dragging{false}, gesture{false};
  };
  struct Slot {
    void* arena{nullptr};
    size_t bytes{0}, sources{0}, cards{0};
    Source* source{nullptr};
    Card* card{nullptr};
    uint32_t generation{0};
    bool valid{false};
  };
  static bool ready(const Source&);
  static void set_position(Card&, int position);
  static void apply_state(Card&, const Source&);
  Card* find(ConfigurationHandle);
  void clear(Slot&);
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  size_t max_sources_, max_cards_, max_key_bytes_, peak_{0}, failures_{0};
  uint32_t connection_;
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
