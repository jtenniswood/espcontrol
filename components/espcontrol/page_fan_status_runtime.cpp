#include "page_fan_status_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "ha_state_limits.h"
#include "page_fan_mode.h"

namespace espcontrol::pages {
namespace {
bool is_status_card(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  if (!card) return false;
  const auto type = store.field(*card, CardField::TYPE);
  return type == "fan_switch" || type == "fan_oscillate";
}
template <typename Emit>
bool keys(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id) {
      if (!is_status_card(store, id)) continue;
      const auto key = store.field(*store.card(id), CardField::ENTITY);
      bool duplicate = false;
      for (PageId before = 0; before <= p && !duplicate; ++before) {
        const auto* earlier = store.page(before);
        if (!earlier->navigable) continue;
        for (CardId previous = earlier->first_card;
             previous < earlier->first_card + earlier->card_count && previous < id; ++previous)
          if (is_status_card(store, previous) &&
              store.field(*store.card(previous), CardField::ENTITY) == key) {
            duplicate = true;
            break;
          }
      }
      if (!duplicate && !emit(key)) return false;
    }
  }
  return true;
}
bool state_on(std::string_view value) {
  for (const auto key : {"on", "true", "1", "home", "playing", "open", "opened", "opening", "closing",
                         "unlocked", "unlocking", "jammed"})
    if (fan_mode_equal(value, key)) return true;
  return false;
}
}  // namespace
PageFanStatusRuntime::PageFanStatusRuntime(PageArenaAllocator& allocator, PageEntitySubscriptions& subs,
                                           Now now, size_t sources, size_t names)
    : allocator_(allocator),
      subscriptions_(subs),
      now_(now),
      max_sources_(sources),
      max_key_bytes_(names),
      connection_(subs.connection_generation()) {}
PageFanStatusRuntime::~PageFanStatusRuntime() {
  discard_prepared();
  retire_active();
}
void PageFanStatusRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PageFanStatusRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t count = 0, names = 0;
  if (!store.source_document() || !now_ ||
      !keys(store,
            [&](std::string_view key) {
              if (count >= max_sources_ || names > max_key_bytes_ || key.size() >= max_key_bytes_ - names)
                return false;
              ++count;
              names += key.size() + 1;
              return true;
            }) ||
      count > (SIZE_MAX - names) / sizeof(Source)) {
    ++failures_;
    return false;
  }
  const size_t bytes = count * sizeof(Source) + names;
  auto* arena = bytes ? allocator_.allocate(bytes) : nullptr;
  if (bytes && !arena) {
    ++failures_;
    return false;
  }
  staged_.arena = arena;
  staged_.bytes = bytes;
  staged_.sources = static_cast<Source*>(arena);
  staged_.store = store;
  char* output = arena ? static_cast<char*>(arena) + count * sizeof(Source) : nullptr;
  keys(store, [&](std::string_view key) {
    auto& source = *new (staged_.sources + staged_.count++) Source{};
    if (!key.empty()) std::memcpy(output, key.data(), key.size());
    output[key.size()] = '\0';
    source.entity = {output, key.size()};
    output += key.size() + 1;
    return true;
  });
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PageFanStatusRuntime::discard_prepared() {
  clear(staged_);
}
void PageFanStatusRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.store.source_document() ||
      !page_prepared_generation_matches(staged_.store.generation(), store.generation()))
    return;
  clear(active_);
  active_ = staged_;
  active_.store = store;
  staged_ = {};
  ++revision_;
}
void PageFanStatusRuntime::retire_active() {
  if (active_.store.source_document()) ++revision_;
  clear(active_);
}
void PageFanStatusRuntime::update(Source& source) {
  const auto change = [&](History& history, PageFanStatus status) {
    if (history.status == status) return;
    history = {now_(), status, true};
    ++revision_;
  };
  change(source.power, !source.available ? PageFanStatus::UNAVAILABLE
                       : source.on       ? PageFanStatus::ON
                                         : PageFanStatus::OFF);
  change(source.oscillation, !source.available           ? PageFanStatus::UNAVAILABLE
                             : !source.oscillation_known ? PageFanStatus::UNSUPPORTED
                             : source.oscillating        ? PageFanStatus::OSCILLATING
                                                         : PageFanStatus::STILL);
}
void PageFanStatusRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                                    std::string_view value, bool) {
  loop();
  if (!attribute.empty() && attribute != "oscillating") return;
  for (auto* slot : {&active_, &staged_}) {
    if (slot->store.generation() != generation) continue;
    for (size_t i = 0; i < slot->count; ++i) {
      auto& source = slot->sources[i];
      if (source.entity != entity) continue;
      const auto before_on = source.on, before_oscillating = source.oscillating;
      // Match the legacy callback's bounded short-state normalization.
      value = value.substr(0, HA_SHORT_STATE_MAX_LEN);
      if (attribute.empty()) {
        source.available = !fan_mode_trim(value).empty() && !fan_mode_equal(value, "unavailable") &&
                           !fan_mode_equal(value, "unknown");
        source.on = source.available && state_on(value);
      } else {
        source.oscillation_known = true;
        if (fan_mode_equal(value, "true") || fan_mode_equal(value, "on") || fan_mode_equal(value, "yes") ||
            fan_mode_equal(value, "1"))
          source.oscillating = true;
        else if (fan_mode_equal(value, "false") || fan_mode_equal(value, "off") ||
                 fan_mode_equal(value, "no") || fan_mode_equal(value, "0"))
          source.oscillating = false;
        else
          source.oscillation_known = false;
      }
      if (before_on != source.on || before_oscillating != source.oscillating) ++revision_;
      update(source);
    }
  }
}
void PageFanStatusRuntime::loop() {
  if (!now_) return;
  const auto connection = subscriptions_.connection_generation();
  if (connection_ != connection) {
    connection_ = connection;
    for (auto* slot : {&active_, &staged_})
      for (size_t i = 0; i < slot->count; ++i) {
        auto& source = slot->sources[i];
        source.available = source.on = source.oscillation_known = source.oscillating = false;
        update(source);
      }
    ++revision_;
  }
  for (auto* slot : {&active_, &staged_})
    for (size_t i = 0; i < slot->count; ++i)
      for (auto* history : {&slot->sources[i].power, &slot->sources[i].oscillation})
        if (history->showing && static_cast<uint32_t>(now_() - history->changed_ms) >= 3000) {
          history->showing = false;
          ++revision_;
        }
}
PageFanStatusSnapshot PageFanStatusRuntime::read(ConfigurationHandle handle) const {
  if (handle.generation != active_.store.generation() || !is_status_card(active_.store, handle.index))
    return {};
  const auto& card = *active_.store.card(handle.index);
  const auto key = active_.store.field(card, CardField::ENTITY);
  const bool oscillation = active_.store.field(card, CardField::TYPE) == "fan_oscillate";
  for (size_t i = 0; i < active_.count; ++i) {
    const auto& source = active_.sources[i];
    if (source.entity != key) continue;
    const auto& history = oscillation ? source.oscillation : source.power;
    return {oscillation ? source.oscillating : source.on, history.showing, history.status};
  }
  return {};
}
PageFanStatusMemory PageFanStatusRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.count, failures_};
}
const char* PageFanStatusRuntime::text(PageFanStatus status) {
  switch (status) {
    case PageFanStatus::ON:
      return "On";
    case PageFanStatus::OFF:
      return "Off";
    case PageFanStatus::UNSUPPORTED:
      return "Unsupported";
    case PageFanStatus::OSCILLATING:
      return "Oscillating";
    case PageFanStatus::STILL:
      return "Still";
    default:
      return "Unavailable";
  }
}
}  // namespace espcontrol::pages
