#include "page_calendar_runtime.h"

#include <cctype>

namespace espcontrol::pages {
namespace {
std::string_view key(const PageStore& store, CardId id) {
  const auto field = store.field(*store.card(id), CardField::ENTITY);
  return field.empty() ? std::string_view("sensor.date") : field;
}
bool parse(std::string_view value, int& day, int& month) {
  value = value.substr(0, 16);
  if (value.size() < 10) return false;
  const auto digit = [&](size_t i) { return std::isdigit(static_cast<unsigned char>(value[i])); };
  const auto valid = [](int day, int month) { return day >= 1 && day <= 31 && month >= 1 && month <= 12; };
  if (digit(0) && digit(1) && digit(2) && digit(3) && value[4] == '-' && digit(5) && digit(6) &&
      value[7] == '-' && digit(8) && digit(9)) {
    const int d = (value[8] - '0') * 10 + value[9] - '0', m = (value[5] - '0') * 10 + value[6] - '0';
    if (valid(d, m)) {
      day = d;
      month = m;
      return true;
    }
  }
  if (digit(0) && digit(1) && (value[2] == '/' || value[2] == '-') && digit(3) && digit(4) &&
      (value[5] == '/' || value[5] == '-') && digit(6) && digit(7) && digit(8) && digit(9)) {
    const int d = (value[0] - '0') * 10 + value[1] - '0', m = (value[3] - '0') * 10 + value[4] - '0';
    if (valid(d, m)) {
      day = d;
      month = m;
      return true;
    }
  }
  return false;
}
}  // namespace
PageCalendarRuntime::~PageCalendarRuntime() {
  discard_prepared();
  retire_active();
}
bool PageCalendarRuntime::supported(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  return card && store.field(*card, CardField::TYPE) == "calendar";
}
bool PageCalendarRuntime::equal(const PageCalendarState& a, const PageCalendarState& b) {
  return a.date_valid == b.date_valid && a.day == b.day && a.month == b.month &&
         a.time_valid == b.time_valid && a.hour == b.hour && a.minute == b.minute && a.use_12h == b.use_12h;
}
bool PageCalendarRuntime::source(const PageStore& store, std::string_view entity) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id)
      if (supported(store, id) && key(store, id) == entity) return true;
  }
  return false;
}
size_t PageCalendarRuntime::sources(const PageStore& store) {
  size_t count = 0;
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id) {
      if (!supported(store, id)) continue;
      bool duplicate = false;
      for (PageId before = 0; before <= p && !duplicate; ++before) {
        const auto* earlier = store.page(before);
        if (!earlier->navigable) continue;
        for (CardId previous = earlier->first_card;
             previous < earlier->first_card + earlier->card_count && previous < id; ++previous)
          if (supported(store, previous) && key(store, previous) == key(store, id)) {
            duplicate = true;
            break;
          }
      }
      if (!duplicate) ++count;
    }
  }
  return count;
}
bool PageCalendarRuntime::prepare(const PageStore& store) {
  discard_prepared();
  if (!store.source_document()) {
    ++failures_;
    return false;
  }
  staged_ = {store, state(), sources(store)};
  return true;
}
void PageCalendarRuntime::discard_prepared() {
  staged_ = {};
}
void PageCalendarRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.store.source_document() ||
      !page_prepared_generation_matches(staged_.store.generation(), store.generation()))
    return;
  active_ = staged_;
  active_.store = store;
  staged_ = {};
  ++revision_;
  publish();
}
void PageCalendarRuntime::retire_active() {
  if (active_.store.source_document()) {
    base_ = active_.state;
    active_ = {};
    ++revision_;
    publish();
  }
}
void PageCalendarRuntime::apply_date(PageCalendarState& state, bool valid, int day, int month) {
  state.date_valid = valid && day >= 1 && day <= 31 && month >= 1 && month <= 12;
  state.day = state.date_valid ? day : 0;
  state.month = state.date_valid ? month : 0;
  if (!state.date_valid && !state.time_valid) {
    state.hour = state.minute = 0;
    state.use_12h = false;
  }
}
void PageCalendarRuntime::apply_time(PageCalendarState& state, bool valid, int day, int month, int hour,
                                     int minute, bool use_12h) {
  state.time_valid = valid && day >= 1 && day <= 31 && month >= 1 && month <= 12 && hour >= 0 && hour <= 23 &&
                     minute >= 0 && minute <= 59;
  state.hour = state.time_valid ? hour : 0;
  state.minute = state.time_valid ? minute : 0;
  state.use_12h = state.time_valid ? use_12h : false;
  if (state.time_valid) {
    state.date_valid = true;
    state.day = day;
    state.month = month;
  }
}
void PageCalendarRuntime::date(bool valid, int day, int month) {
  const auto before = state();
  apply_date(base_, valid, day, month);
  for (auto* slot : {&active_, &staged_})
    if (slot->store.source_document()) {
      const auto previous = slot->state;
      apply_date(slot->state, valid, day, month);
      if (!equal(previous, slot->state)) ++revision_;
    }
  if (!equal(before, state())) {
    ++revision_;
    publish();
  }
}
void PageCalendarRuntime::time(bool valid, int day, int month, int hour, int minute, bool use_12h) {
  const auto before = state();
  apply_time(base_, valid, day, month, hour, minute, use_12h);
  for (auto* slot : {&active_, &staged_})
    if (slot->store.source_document()) {
      const auto previous = slot->state;
      apply_time(slot->state, valid, day, month, hour, minute, use_12h);
      if (!equal(previous, slot->state)) ++revision_;
    }
  if (!equal(before, state())) {
    ++revision_;
    publish();
  }
}
void PageCalendarRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                                   std::string_view value, bool) {
  if (!attribute.empty()) return;
  int day, month;
  if (!parse(value, day, month)) return;
  for (auto* slot : {&active_, &staged_})
    if (slot->store.source_document() && slot->store.generation() == generation &&
        source(slot->store, entity)) {
      const auto previous = slot->state;
      apply_date(slot->state, true, day, month);
      if (!equal(previous, slot->state)) {
        ++revision_;
        if (slot == &active_) publish();
      }
    }
}
bool PageCalendarRuntime::seed(PageCalendarState initial) {
  if (active_.store.source_document() || staged_.store.source_document()) return false;
  if (!equal(base_, initial)) {
    base_ = initial;
    ++revision_;
  }
  return true;
}
bool PageCalendarRuntime::attach_publisher(void* owner, Publish function) {
  if (!owner || !function || publish_) return false;
  publisher_ = owner;
  publish_ = function;
  return true;
}
void PageCalendarRuntime::detach_publisher(void* owner) {
  if (publisher_ == owner) {
    publisher_ = nullptr;
    publish_ = nullptr;
  }
}
void PageCalendarRuntime::publish() {
  if (publish_) publish_(publisher_, state());
}
PageCalendarState PageCalendarRuntime::read(ConfigurationHandle handle) const {
  return active_.store.source_document() && handle.generation == active_.store.generation() &&
                 supported(active_.store, handle.index)
             ? active_.state
             : PageCalendarState{};
}
}  // namespace espcontrol::pages
