#pragma once

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
struct PageCalendarState {
  bool date_valid{false};
  int day{0}, month{0};
  bool time_valid{false};
  int hour{0}, minute{0};
  bool use_12h{false};
};
struct PageCalendarMemory {
  size_t fixed_bytes{0}, active_sources{0}, staged_sources{0}, preparation_failures{0};
};
// One ordered date/time state per configuration, independent of calendar widgets.
// Prepare before the HA graph; the application installs central producer events.
class PageCalendarRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  using Publish = void (*)(void*, const PageCalendarState&);
  explicit PageCalendarRuntime(PageCalendarState initial = {}) : base_(initial) {}
  ~PageCalendarRuntime() override;
  PageCalendarRuntime(const PageCalendarRuntime&) = delete;
  PageCalendarRuntime& operator=(const PageCalendarRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void date(bool, int, int);
  void time(bool, int, int, int, int, bool);
  bool seed(PageCalendarState);
  // Copy scalars synchronously; do not allocate, retain the argument or replace configuration here.
  bool attach_publisher(void*, Publish);
  void detach_publisher(void*);
  bool has_publisher() const {
    return publish_ != nullptr;
  }
  PageCalendarState read(ConfigurationHandle) const;
  PageCalendarState state() const {
    return active_.store.source_document() ? active_.state : base_;
  }
  uint64_t revision() const {
    return revision_;
  }
  PageCalendarMemory memory() const {
    return {sizeof(*this), active_.sources, staged_.sources, failures_};
  }
  static bool supported(const PageStore&, CardId);

 private:
  struct Slot {
    PageStore store;
    PageCalendarState state;
    size_t sources{0};
  };
  static bool equal(const PageCalendarState&, const PageCalendarState&);
  static bool source(const PageStore&, std::string_view);
  static size_t sources(const PageStore&);
  static void apply_date(PageCalendarState&, bool, int, int);
  static void apply_time(PageCalendarState&, bool, int, int, int, int, bool);
  void publish();
  PageCalendarState base_;
  Slot active_, staged_;
  void* publisher_{nullptr};
  Publish publish_{nullptr};
  uint64_t revision_{0};
  size_t failures_{0};
};
}  // namespace espcontrol::pages
