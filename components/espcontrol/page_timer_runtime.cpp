#include "page_timer_runtime.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

#include "timer_state_policy.h"
namespace espcontrol::pages {
namespace {
constexpr uint8_t PRIMARY = 1, DURATION = 2, REMAINING = 4, FINISH = 8;
uint16_t confirmation_timeout(std::string_view value) {
  char buffer[65];
  if (value.size() >= sizeof(buffer)) return 3;
  if (!value.empty()) std::memcpy(buffer, value.data(), value.size());
  buffer[value.size()] = '\0';
  const auto parsed = std::strtoll(buffer, nullptr, 10);
  return static_cast<uint16_t>(parsed < 1 ? 3 : std::min<long long>(30, parsed));
}
PageTimerMode mode(std::string_view value) {
  return value == "active"   ? PageTimerMode::ACTIVE
         : value == "idle"   ? PageTimerMode::IDLE
         : value == "paused" ? PageTimerMode::PAUSED
                             : PageTimerMode::UNAVAILABLE;
}
template <typename Emit>
bool cards(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id)
      if (PageTimerRuntime::supported(store, id) &&
          !emit(id, store.field(*store.card(id), CardField::ENTITY)))
        return false;
  }
  return true;
}
}  // namespace
bool PageTimerRuntime::supported(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  if (!card || store.field(*card, CardField::TYPE) != "timer") return false;
  const auto entity = store.field(*card, CardField::ENTITY);
  return entity.size() > 6 && entity.substr(0, 6) == "timer.";
}
PageTimerRuntime::PageTimerRuntime(PageArenaAllocator& allocator, PageEntitySubscriptions& subscriptions,
                                   Now now, Epoch epoch, size_t sources, size_t count, size_t names)
    : allocator_(allocator),
      subscriptions_(subscriptions),
      now_(now),
      epoch_(epoch),
      max_sources_(sources),
      max_cards_(count),
      max_key_bytes_(names),
      connection_(subscriptions.connection_generation()) {}
PageTimerRuntime::~PageTimerRuntime() {
  discard_prepared();
  retire_active();
}
void PageTimerRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PageTimerRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t count = 0, sources = 0, names = 0;
  const auto fail = [&]() {
    ++failures_;
    discard_prepared();
    return false;
  };
  if (!now_ || !epoch_ || !store.source_document() ||
      !cards(store,
             [&](CardId id, std::string_view key) {
               if (count >= max_cards_) return false;
               ++count;
               bool duplicate = false;
               cards(store, [&](CardId before, std::string_view previous) {
                 if (before < id && key == previous) duplicate = true;
                 return true;
               });
               if (duplicate) return true;
               if (sources >= std::min<size_t>(max_sources_, UINT16_MAX) || names > max_key_bytes_ ||
                   key.size() >= max_key_bytes_ - names)
                 return false;
               ++sources;
               names += key.size() + 1;
               return true;
             }) ||
      count > (SIZE_MAX - names) / sizeof(Card))
    return fail();
  const size_t card_bytes = count * sizeof(Card);
  if (sources > (SIZE_MAX - names - card_bytes) / sizeof(Source)) return fail();
  const size_t bytes = sources * sizeof(Source) + card_bytes + names;
  auto* arena = bytes ? allocator_.allocate(bytes) : nullptr;
  if (bytes && !arena) return fail();
  staged_.arena = arena;
  staged_.bytes = bytes;
  staged_.generation = store.generation();
  staged_.valid = true;
  staged_.source = static_cast<Source*>(arena);
  staged_.card =
      arena ? reinterpret_cast<Card*>(static_cast<char*>(arena) + sources * sizeof(Source)) : nullptr;
  char* output = arena ? static_cast<char*>(arena) + sources * sizeof(Source) + card_bytes : nullptr;
  cards(store, [&](CardId id, std::string_view key) {
    size_t index = 0;
    for (; index < staged_.sources; ++index)
      if (staged_.source[index].entity == key) break;
    if (index == staged_.sources) {
      auto& source = *new (staged_.source + staged_.sources++) Source{};
      std::memcpy(output, key.data(), key.size());
      output[key.size()] = '\0';
      source.entity = {output, key.size()};
      if (active_.valid)
        for (size_t previous = 0; previous < active_.sources; ++previous) {
          if (active_.source[previous].entity != key) continue;
          const auto owned_key = source.entity;
          source = active_.source[previous];
          source.entity = owned_key;
          break;
        }
      output += key.size() + 1;
    }
    auto& card = *new (staged_.card + staged_.cards++) Card{};
    card.card = id;
    card.source = static_cast<uint16_t>(index);
    card.confirmation = store.field(*store.card(id), CardField::SENSOR) == "confirm";
    card.confirmation_seconds = confirmation_timeout(store.field(*store.card(id), CardField::UNIT));
    return true;
  });
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PageTimerRuntime::discard_prepared() {
  clear(staged_);
}
void PageTimerRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.valid || !page_prepared_generation_matches(staged_.generation, store.generation())) return;
  clear(active_);
  active_ = staged_;
  active_.generation = store.generation();
  staged_ = {};
  ++revision_;
}
void PageTimerRuntime::retire_active() {
  if (active_.valid) ++revision_;
  clear(active_);
}

int64_t PageTimerRuntime::remaining(const Source& source) const {
  const auto epoch = epoch_ ? epoch_() : 0;
  if (source.finish_epoch != 0 && epoch > 100000) return source.finish_epoch - epoch;
  return static_cast<int64_t>(source.remaining) - static_cast<uint32_t>(now_ms() - source.anchor) / 1000;
}
void PageTimerRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                                std::string_view value, bool copied) {
  loop();
  const uint8_t field = attribute.empty()            ? PRIMARY
                        : attribute == "duration"    ? DURATION
                        : attribute == "remaining"   ? REMAINING
                        : attribute == "finishes_at" ? FINISH
                                                     : 0;
  if (!field) return;
  for (auto* slot : {&active_, &staged_}) {
    if (!slot->valid || slot->generation != generation) continue;
    for (size_t i = 0; i < slot->sources; ++i) {
      auto& source = slot->source[i];
      if (source.entity != entity || (copied && (source.known & field))) continue;
      source.known |= field;
      if (field == PRIMARY) {
        const auto previous = source.mode;
        const auto next = mode(value);
        const auto left = remaining(source);
        source.mode = next;
        if (next != PageTimerMode::ACTIVE) ++source.confirmation_revision;
        for (size_t id = 0; id < slot->cards; ++id) {
          auto& card = slot->card[id];
          if (card.source != i) continue;
          if (previous == PageTimerMode::ACTIVE && next == PageTimerMode::IDLE)
            card.finished_ms = !card.cancel_requested && left < 2 ? std::max<uint32_t>(1, now_ms()) : 0;
          else if (next == PageTimerMode::ACTIVE)
            card.finished_ms = 0;
          if (next != PageTimerMode::ACTIVE) card.cancel_requested = false;
        }
      } else if (field == DURATION)
        source.duration = timer_state::parse_duration(value);
      else if (field == REMAINING) {
        source.remaining = timer_state::parse_duration(value);
        source.anchor = now_ms();
      } else
        source.finish_epoch = timer_state::parse_finish(value);
      ++revision_;
    }
  }
}
void PageTimerRuntime::loop() {
  const auto connection = subscriptions_.connection_generation();
  if (connection != connection_) {
    connection_ = connection;
    for (auto* slot : {&active_, &staged_}) {
      for (size_t i = 0; i < slot->sources; ++i) {
        auto& source = slot->source[i];
        const auto entity = source.entity;
        source = {};
        source.entity = entity;
      }
      for (size_t i = 0; i < slot->cards; ++i) {
        slot->card[i].finished_ms = 0;
        slot->card[i].cancel_requested = false;
      }
    }
    ++revision_;
  }
  const auto now = now_ms();
  if (active_.valid && active_.sources && static_cast<uint32_t>(now - last_tick_) >= 250) {
    last_tick_ = now;
    ++revision_;
  }
}
PageTimerRuntime::Card* PageTimerRuntime::find(ConfigurationHandle handle) {
  if (!active_.valid || handle.generation != active_.generation) return nullptr;
  for (size_t i = 0; i < active_.cards; ++i)
    if (active_.card[i].card == handle.index) return active_.card + i;
  return nullptr;
}
PageTimerSnapshot PageTimerRuntime::read(ConfigurationHandle handle) const {
  if (!active_.valid || handle.generation != active_.generation) return {};
  for (size_t i = 0; i < active_.cards; ++i) {
    const auto& card = active_.card[i];
    if (card.card != handle.index) continue;
    const auto& source = active_.source[card.source];
    int64_t seconds = 0;
    if (source.mode == PageTimerMode::ACTIVE)
      seconds = remaining(source);
    else if (source.mode == PageTimerMode::PAUSED)
      seconds = source.remaining;
    else if (source.mode == PageTimerMode::IDLE) {
      const bool finished = card.finished_ms && static_cast<uint32_t>(now_ms() - card.finished_ms) < 5000;
      seconds = finished ? 0 : source.duration;
    }
    return {source.mode, static_cast<int>(std::max<int64_t>(0, std::min<int64_t>(INT32_MAX, seconds))),
            card.confirmation_seconds, card.confirmation, source.confirmation_revision};
  }
  return {};
}
PageTimerAction PageTimerRuntime::action(ConfigurationHandle handle) {
  loop();
  const auto state = read(handle).mode;
  return state == PageTimerMode::ACTIVE                                   ? PageTimerAction::CANCEL
         : state == PageTimerMode::IDLE || state == PageTimerMode::PAUSED ? PageTimerAction::START
                                                                          : PageTimerAction::NONE;
}
void PageTimerRuntime::cancel_sent(ConfigurationHandle handle, bool accepted) {
  loop();
  auto* card = find(handle);
  if (!card) return;
  if (active_.source[card->source].mode == PageTimerMode::ACTIVE) card->cancel_requested = accepted;
}
PageTimerMemory PageTimerRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.sources, active_.cards, failures_};
}
}  // namespace espcontrol::pages
