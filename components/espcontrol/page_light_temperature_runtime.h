#pragma once
#include "page_entity_state_runtime.h"
namespace espcontrol::pages {
struct PageLightTemperatureSnapshot {
  int minimum{2000}, maximum{6500}, fill_kelvin{4250}, display_kelvin{2000};
  uint8_t percent{0};
  bool available{false}, dragging{false}, gesture{false}, color{false};
  uint32_t write_revision{0};
};
struct PageLightTemperatureMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0};
  size_t sources{0}, cards{0}, preparation_failures{0};
};
// Ordered HA history and per-card drag previews, independent of widgets.
// Prepare before the state graph and fan out observations through its observer set.
class PageLightTemperatureRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  PageLightTemperatureRuntime(PageArenaAllocator&, PageEntitySubscriptions&, size_t max_sources = 32,
                              size_t max_cards = 512, size_t max_key_bytes = 2048);
  ~PageLightTemperatureRuntime() override;
  PageLightTemperatureRuntime(const PageLightTemperatureRuntime&) = delete;
  PageLightTemperatureRuntime& operator=(const PageLightTemperatureRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PageLightTemperatureSnapshot read(ConfigurationHandle) const;
  bool begin(ConfigurationHandle);
  bool drag(ConfigurationHandle, int percent);
  bool finish(ConfigurationHandle, int percent, int& kelvin);
  void cancel(ConfigurationHandle);
  void discard_finished(ConfigurationHandle, int expected, uint32_t write_revision, PageEntityStateRuntime&);
  uint64_t revision() const {
    return revision_;
  }
  uint32_t connection_generation() const {
    return subscriptions_.connection_generation();
  }
  PageLightTemperatureMemory memory() const;
  static bool supported(const PageStore&, CardId);

 private:
  struct Source {
    std::string_view entity;
    bool live_primary{false}, live_kelvin{false};
  };
  struct Card {
    CardId card{NO_CARD};
    uint16_t source{0};
    int minimum{2000}, maximum{6500}, last_kelvin{2000}, fill_kelvin{4250}, display_kelvin{2000};
    uint32_t write_revision{0};
    uint8_t percent{0};
    bool available{true}, on{false}, known{false}, has_kelvin{false}, dragging{false}, gesture{false},
        color{false};
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
  static void apply_kelvin(Card&);
  Card* find(ConfigurationHandle);
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  size_t max_sources_, max_cards_, max_key_bytes_, peak_{0}, failures_{0};
  uint32_t connection_;
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
