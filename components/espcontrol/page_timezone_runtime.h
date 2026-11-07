#pragma once

#include <array>
#include <ctime>

#include "page_store_runtime.h"

namespace espcontrol::pages {
struct PageTimezoneSelection {
  std::string_view id;
  const char* posix{nullptr};
};
struct PageTimezoneResolver {
  void* context{nullptr};
  // Return views into the input option or firmware ROM. Do not allocate or retain the input.
  PageTimezoneSelection (*resolve)(void*, std::string_view, time_t){nullptr};
};
struct PageTimezoneSnapshot {
  bool valid{false}, use_12h{false};
  int hour{0}, minute{0};
  // Borrowed until the next central producer update or configuration replacement.
  std::string_view zone_id;
};
struct PageTimezoneMemory {
  size_t fixed_bytes{0}, active_cards{0}, staged_cards{0}, preparation_failures{0}, rejected_updates{0};
};
// The clock producer is global. Configuration owns only borrowed document views,
// never a timezone string, hidden widget, HA subscription or per-card timer.
class PageTimezoneRuntime final : public PageRuntimeResources {
 public:
  static constexpr size_t ACTIVE_OPTION_CAPACITY = 128;
  explicit PageTimezoneRuntime(PageTimezoneResolver resolver) : resolver_(resolver) {}
  ~PageTimezoneRuntime() override;
  PageTimezoneRuntime(const PageTimezoneRuntime&) = delete;
  PageTimezoneRuntime& operator=(const PageTimezoneRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  bool update(bool valid, time_t epoch, std::string_view active_option, bool use_12h);
  bool seed(bool valid, time_t epoch, std::string_view active_option, bool use_12h);
  PageTimezoneSnapshot read(ConfigurationHandle) const;
  uint64_t revision() const {
    return revision_;
  }
  PageTimezoneMemory memory() const;
  static bool supported(const PageStore&, CardId);

 private:
  struct Slot {
    PageStore store;
    size_t cards{0};
  };
  PageTimezoneResolver resolver_;
  Slot active_, staged_;
  std::array<char, ACTIVE_OPTION_CAPACITY> option_{};
  size_t option_size_{0}, failures_{0}, rejected_{0};
  bool valid_{false}, use_12h_{false};
  time_t epoch_{0};
  uint64_t revision_{0};
};
}  // namespace espcontrol::pages
