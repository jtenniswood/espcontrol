#pragma once

#include "page_alarm_policy.h"
#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
enum class PageAlarmSourceKind : uint8_t { NONE, CONTROL, ACTION };
struct PageAlarmSnapshot {
  bool valid{false}, reported{false}, available{false}, command_available{false}, icon_reported{false};
  // Active is the panel state; action-card checked state uses alarm::matches with its prepared mode.
  bool status_showing{false}, status_known{false}, active{false}, triggered{false};
  // Status preserves history bytes. Display only the prefix before its first NUL, as the legacy label API.
  std::string_view entity, state, arm_mode, status;
  int remaining{-1}, progress{-1};
};
struct PageAlarmTakeover {
  bool valid{false};
  ConfigurationHandle origin;
  std::string_view entity;
  bool triggered{false};
  int remaining{-1}, progress{-1};
};
struct PageAlarmMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0};
  size_t active_sources{0}, staged_sources{0}, preparation_failures{0};
};
// No widgets, subscriptions, callbacks into UI, service dispatch or LVGL timers.
// Put before the graph in the application resource/observer group; loop even with no visible alarm.
class PageAlarmStateRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  using Now = uint32_t (*)();
  // Return the input key or immutable firmware-lifetime text; never allocate or retain a temporary.
  using Translate = const char* (*)(const char*);
  PageAlarmStateRuntime(PageArenaAllocator&, PageEntitySubscriptions&, Now, size_t max_sources = 64,
                        size_t max_key_bytes = 4096, Translate = nullptr);
  ~PageAlarmStateRuntime() override;
  PageAlarmStateRuntime(const PageAlarmStateRuntime&) = delete;
  PageAlarmStateRuntime& operator=(const PageAlarmStateRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PageAlarmSnapshot read(ConfigurationHandle) const;
  PageAlarmTakeover takeover() const;
  uint64_t revision() const {
    return revision_;
  }
  // Fresh eligible reports that would refresh the legacy takeover. Countdown
  // ticks/caption expiry and control-only arm-mode reports do not reopen UI.
  uint64_t takeover_events() const {
    return takeover_events_;
  }
  PageAlarmMemory memory() const;
  static PageAlarmSourceKind classify(const PageStore&, CardId);
  static std::string_view effective_entity(const PageStore&, CardId);
  static std::string_view effective_options(const PageStore&, CardId);

 private:
  struct Source {
    std::string_view entity;
    const char* literal{nullptr};
    char state[33]{}, arm_mode[33]{}, caption[97]{};
    uint8_t state_size{0}, mode_size{0}, caption_size{0};
    PageAlarmSourceKind kind{PageAlarmSourceKind::NONE};
    CardId origin{NO_CARD};
    int seconds{-1}, total{-1}, last_remaining{-1};
    uint32_t started{0}, changed{0};
    bool reported{false}, mode_reported{false}, available{false}, fresh{false}, invalidated{false},
        inherited{false};
    bool status_known{false}, showing{false}, revert{false};
  };
  struct Slot {
    PageStore store;
    void* arena{nullptr};
    size_t bytes{0}, count{0};
    Source* sources{nullptr};
    uint16_t* card_source{nullptr};
    uint16_t selected{UINT16_MAX};
  };
  static std::string_view state(const Source& s) {
    return {s.state, s.state_size};
  }
  static std::string_view mode(const Source& s) {
    return {s.arm_mode, s.mode_size};
  }
  static std::string_view caption(const Source& s) {
    return s.literal ? std::string_view(s.literal) : std::string_view(s.caption, s.caption_size);
  }
  bool eligible(const Source&) const;
  void clear(Slot&);
  void update(Source&, std::string_view, std::string_view, bool);
  void status(Source&);
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  Now now_;
  size_t max_sources_, max_key_bytes_, peak_{0}, failures_{0};
  Translate translate_;
  uint32_t connection_;
  uint64_t revision_{0}, takeover_events_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
