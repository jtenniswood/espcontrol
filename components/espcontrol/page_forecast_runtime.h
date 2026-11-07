#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "page_runtime_resources.h"
#include "page_store_runtime.h"
#include "weather_daily_forecast.h"
namespace espcontrol::pages {
struct PageForecastToken {
  uint32_t configuration{0}, connection{0}, request{0};
  uint8_t source{0};
};
class PageForecastRequests {
 public:
  using Receive = void (*)(void*, PageForecastToken, std::string_view, bool);
  virtual ~PageForecastRequests() = default;
  // All calls and deliveries run on the application loop. Cancellation may
  // synchronously deliver failure; a transport never retains the entity view.
  // The connection generation advances across every disconnect, even if loop
  // did not observe the disconnected state. The owner outlives cancellations.
  virtual bool connected() const = 0;
  virtual uint32_t connection_generation() const = 0;
  virtual uint32_t now_ms() const = 0;
  virtual bool start(void*, PageForecastToken, std::string_view, Receive) = 0;
  virtual void cancel(void*, PageForecastToken) = 0;
};
struct PageForecastSnapshot {
  weather::Forecast forecast;
  bool loading{true}, stale{false};
};
struct PageForecastMemory {
  size_t active_bytes{0}, staged_bytes{0}, retired_bytes{0}, peak_bytes{0}, sources{0}, pending{0};
  size_t requests{0}, completed{0}, failures{0}, stale_responses{0}, cancellations{0}, timeouts{0};
};
// One compact source per distinct configured entity, in external arena storage.
// Query copies a snapshot: no caller can retain pointers into a retired model.
class PageForecastRuntime final : public PageRuntimeResources {
 public:
  static constexpr uint32_t READY_DELAY_MS = 10000, TIMEOUT_MS = 60000, RETRY_MS = 300000,
                            REFRESH_MS = 3600000;
  PageForecastRuntime(PageArenaAllocator& allocator, PageForecastRequests& requests)
      : allocator_(allocator), requests_(requests) {}
  ~PageForecastRuntime() override;
  PageForecastRuntime(const PageForecastRuntime&) = delete;
  PageForecastRuntime& operator=(const PageForecastRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void loop();
  void refresh();
  bool snapshot(ConfigurationHandle card, std::string_view entity, PageForecastSnapshot& output) const;
  uint64_t revision() const {
    return revision_;
  }
  PageForecastMemory memory() const;

 private:
  struct Source {
    std::string_view entity;
    weather::Forecast forecast;
    uint32_t due{0};
    bool loading{true}, stale{false}, wanted{true};
  };
  struct Slot {
    void* arena{nullptr};
    Source* sources{nullptr};
    size_t bytes{0}, count{0};
    uint32_t generation{0};
    uint8_t* cards{nullptr};
    size_t card_count{0};
  };
  static void receive(void*, PageForecastToken, std::string_view, bool);
  void accept(PageForecastToken, std::string_view, bool);
  bool current(PageForecastToken) const;
  void fail(PageForecastToken, bool timeout = false);
  void invalidate_pending();
  void synchronize();
  void collect();
  void release(int);
  PageArenaAllocator& allocator_;
  PageForecastRequests& requests_;
  Slot slots_[2];
  int active_{-1}, staged_{-1};
  bool retired_[2]{};
  PageForecastToken pending_{}, cancel_{};
  uint32_t started_{0}, next_request_{1}, connection_{0}, ready_at_{0};
  uint64_t revision_{0};
  size_t depth_{0};
  bool connected_{false}, preparing_{false};
  PageForecastMemory memory_;
};
}  // namespace espcontrol::pages
