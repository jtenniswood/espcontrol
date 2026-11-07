#pragma once

#include "page_alarm_state_runtime.h"
namespace espcontrol::pages {
enum class PageAlarmFailureReason : uint8_t { NONE, CODE, ACTION };
struct PageAlarmFailure {
  bool valid{false};
  ConfigurationHandle origin;
  uint32_t connection{0};
  PageAlarmFailureReason reason{PageAlarmFailureReason::NONE};
};
struct PageAlarmAckMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0};
  size_t configured_cards{0}, pending{0}, failures_ready{0}, preparation_failures{0};
};
// Tracks only successfully submitted actions, never dispatches services or UI.
// Register source state first, acknowledgements second; due-report handling also tolerates reverse order.
// Retire this before source state. Use the same application millisecond clock for both owners.
class PageAlarmAckRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  using Now = PageAlarmStateRuntime::Now;
  PageAlarmAckRuntime(PageArenaAllocator&, PageEntitySubscriptions&, PageAlarmStateRuntime&, Now,
                      size_t max_cards = 4096);
  ~PageAlarmAckRuntime() override;
  PageAlarmAckRuntime(const PageAlarmAckRuntime&) = delete;
  PageAlarmAckRuntime& operator=(const PageAlarmAckRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  // Call only after successful dispatch and current intent validation. This is not authorization.
  // Full press/modal intent, live connection, current state/cache and service guards belong to the caller.
  bool accepted(ConfigurationHandle, uint32_t connection, alarm::Action, bool had_code);
  void loop();
  PageAlarmFailure take_failure();
  bool pending(ConfigurationHandle) const;
  PageAlarmAckMemory memory() const;

 private:
  enum Flag : uint8_t { IDLE = 0, WAITING = 1, HAD_CODE = 2, FAILED_CODE = 4, FAILED_ACTION = 8 };
  struct Pending {
    uint32_t started{0}, connection{0}, sequence{0};
    CardId card{NO_CARD};
    alarm::Action mode{alarm::Action::NONE};
    uint8_t flags{IDLE};
  };
  struct Slot {
    PageStore store;
    void* arena{nullptr};
    Pending* actions{nullptr};
    size_t count{0}, bytes{0};
  };
  void clear(Slot&);
  static void reset(Pending&);
  Pending* find(ConfigurationHandle);
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  PageAlarmStateRuntime& state_;
  Now now_;
  size_t max_cards_, peak_{0}, failures_{0};
  uint32_t connection_, sequence_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
