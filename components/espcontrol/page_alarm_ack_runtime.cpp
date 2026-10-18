#include "page_alarm_ack_runtime.h"

#include <algorithm>
#include <new>

namespace espcontrol::pages {
namespace {
template <typename Emit>
bool cards(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id)
      if (PageAlarmStateRuntime::classify(store, id) != PageAlarmSourceKind::NONE && !emit(id)) return false;
  }
  return true;
}
}  // namespace
PageAlarmAckRuntime::PageAlarmAckRuntime(PageArenaAllocator& allocator,
                                         PageEntitySubscriptions& subscriptions, PageAlarmStateRuntime& state,
                                         Now now, size_t cards)
    : allocator_(allocator),
      subscriptions_(subscriptions),
      state_(state),
      now_(now),
      max_cards_(cards),
      connection_(subscriptions.connection_generation()) {}
PageAlarmAckRuntime::~PageAlarmAckRuntime() {
  discard_prepared();
  retire_active();
}
void PageAlarmAckRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
void PageAlarmAckRuntime::reset(Pending& action) {
  const auto card = action.card;
  action = {};
  action.card = card;
}
bool PageAlarmAckRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t count = 0;
  if (!now_ || !store.source_document() || !cards(store, [&](CardId) {
        if (count >= max_cards_ || count >= SIZE_MAX / sizeof(Pending)) return false;
        ++count;
        return true;
      })) {
    ++failures_;
    return false;
  }
  const size_t bytes = count * sizeof(Pending);
  auto* arena = bytes ? allocator_.allocate(bytes) : nullptr;
  if (bytes && !arena) {
    ++failures_;
    return false;
  }
  staged_.store = store;
  staged_.arena = arena;
  staged_.actions = static_cast<Pending*>(arena);
  staged_.bytes = bytes;
  cards(store, [&](CardId id) {
    auto& action = *new (staged_.actions + staged_.count++) Pending{};
    action.card = id;
    return true;
  });
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PageAlarmAckRuntime::discard_prepared() {
  clear(staged_);
}
void PageAlarmAckRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.store.source_document() ||
      !page_prepared_generation_matches(staged_.store.generation(), store.generation()))
    return;
  clear(active_);
  active_ = staged_;
  active_.store = store;
  staged_ = {};
}
void PageAlarmAckRuntime::retire_active() {
  clear(active_);
}
PageAlarmAckRuntime::Pending* PageAlarmAckRuntime::find(ConfigurationHandle handle) {
  if (!active_.store.source_document() || handle.generation != active_.store.generation()) return nullptr;
  for (size_t i = 0; i < active_.count; ++i)
    if (active_.actions[i].card == handle.index) return active_.actions + i;
  return nullptr;
}
bool PageAlarmAckRuntime::accepted(ConfigurationHandle origin, uint32_t connection, alarm::Action mode,
                                   bool had_code) {
  loop();
  auto* action = find(origin);
  if (!action || !now_ || (mode < alarm::Action::AWAY || mode > alarm::Action::DISARM) ||
      connection != subscriptions_.connection_generation() || !state_.read(origin).valid)
    return false;
  reset(*action);
  action->mode = mode;
  action->started = now_();
  action->connection = connection;
  action->sequence = ++sequence_;
  action->flags = static_cast<uint8_t>(WAITING | (had_code ? HAD_CODE : IDLE));
  return true;
}
void PageAlarmAckRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                                   std::string_view value, bool copied) {
  if (copied || (!attribute.empty() && attribute != "arm_mode")) return;
  // A due LVGL timer could be cancelled by a report before its callback ran.
  // Merge this report before polling deadlines, even if observer registration order changes.
  if (active_.store.source_document() && active_.store.generation() == generation) {
    const auto connection = subscriptions_.connection_generation();
    for (size_t i = 0; i < active_.count; ++i) {
      auto& action = active_.actions[i];
      if (!(action.flags & WAITING) || action.connection != connection) continue;
      const auto status = state_.read({generation, action.card});
      if (!status.valid || status.entity != entity) continue;
      const auto state = attribute.empty() ? value.substr(0, 32) : status.state;
      const auto arm_mode = attribute == "arm_mode" ? value.substr(0, 32) : status.arm_mode;
      if (alarm::progressed(action.mode, state, arm_mode)) reset(action);
    }
  }
  loop();
}
void PageAlarmAckRuntime::loop() {
  if (!now_) return;
  state_.loop();
  const auto connection = subscriptions_.connection_generation();
  if (connection != connection_) {
    connection_ = connection;
    for (size_t i = 0; i < active_.count; ++i) reset(active_.actions[i]);
  }
  const uint32_t current_tick = now_();
  for (size_t i = 0; i < active_.count; ++i) {
    auto& action = active_.actions[i];
    if (!(action.flags & WAITING) || static_cast<uint32_t>(current_tick - action.started) < 3500) continue;
    const auto status = state_.read({active_.store.generation(), action.card});
    if (action.connection != connection || !status.valid ||
        alarm::progressed(action.mode, status.state, status.arm_mode)) {
      reset(action);
      continue;
    }
    action.mode = alarm::Action::NONE;
    action.flags = (action.flags & HAD_CODE) ? FAILED_CODE : FAILED_ACTION;
  }
}
PageAlarmFailure PageAlarmAckRuntime::take_failure() {
  loop();
  const uint32_t current_tick = now_ ? now_() : 0;
  Pending* oldest = nullptr;
  for (size_t i = 0; i < active_.count; ++i) {
    auto& action = active_.actions[i];
    if (!(action.flags & (FAILED_CODE | FAILED_ACTION))) continue;
    if (!oldest ||
        static_cast<uint32_t>(current_tick - action.started) >
            static_cast<uint32_t>(current_tick - oldest->started) ||
        (action.started == oldest->started && static_cast<uint32_t>(sequence_ - action.sequence) >
                                                  static_cast<uint32_t>(sequence_ - oldest->sequence)))
      oldest = &action;
  }
  if (!oldest) return {};
  const PageAlarmFailure result{
      true,
      {active_.store.generation(), oldest->card},
      oldest->connection,
      (oldest->flags & FAILED_CODE) ? PageAlarmFailureReason::CODE : PageAlarmFailureReason::ACTION};
  reset(*oldest);
  return result;
}
bool PageAlarmAckRuntime::pending(ConfigurationHandle handle) const {
  if (!active_.store.source_document() || active_.store.generation() != handle.generation) return false;
  for (size_t i = 0; i < active_.count; ++i)
    if (active_.actions[i].card == handle.index) return (active_.actions[i].flags & WAITING) != 0;
  return false;
}
PageAlarmAckMemory PageAlarmAckRuntime::memory() const {
  size_t pending = 0, ready = 0;
  for (size_t i = 0; i < active_.count; ++i) {
    pending += (active_.actions[i].flags & WAITING) != 0;
    ready += (active_.actions[i].flags & (FAILED_CODE | FAILED_ACTION)) != 0;
  }
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.count, pending, ready, failures_};
}
}  // namespace espcontrol::pages
