#include "page_forecast_runtime.h"

#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <new>

#include "button_grid_subpage_parser.h"
namespace espcontrol::pages {
namespace {
struct Depth {
  size_t& depth;
  explicit Depth(size_t& value) : depth(value) {
    ++depth;
  }
  ~Depth() {
    --depth;
  }
};
struct Preparing {
  bool& value;
  explicit Preparing(bool& v) : value(v) {
    value = true;
  }
  ~Preparing() {
    value = false;
  }
};
bool same(PageForecastToken a, PageForecastToken b) {
  return a.request && a.request == b.request && a.configuration == b.configuration &&
         a.connection == b.connection && a.source == b.source;
}
bool due(uint32_t now, uint32_t when) {
  return static_cast<int32_t>(now - when) >= 0;
}
bool forecast_precision(std::string_view precision) {
  if (!card_runtime_weather_forecast_supported()) return false;
  for (const auto* known : CARD_CONTRACT_WEATHER_FORECAST_PRECISIONS)
    if (precision == known) return true;
  return false;
}
}  // namespace
PageForecastRuntime::~PageForecastRuntime() {
  assert(depth_ == 0);
  discard_prepared();
  retire_active();
  if (cancel_.request) {
    const auto token = cancel_;
    cancel_ = {};
    requests_.cancel(this, token);
  }
  collect();
}
void PageForecastRuntime::release(int index) {
  if (index < 0) return;
  auto& slot = slots_[index];
  if (slot.arena) {
    for (size_t i = 0; i < slot.count; ++i) slot.sources[i].~Source();
    allocator_.release(slot.arena, slot.bytes);
  }
  slot = {};
}
void PageForecastRuntime::collect() {
  if (depth_ == 0)
    for (int i = 0; i < 2; ++i)
      if (retired_[i]) {
        retired_[i] = false;
        release(i);
      }
}
bool PageForecastRuntime::prepare(const PageStore& store) {
  if (preparing_ || !store.source_document()) return false;
  Preparing guard(preparing_);
  collect();
  discard_prepared();
  int free = -1;
  for (int i = 0; i < 2; ++i)
    if (i != active_ && !retired_[i]) {
      free = i;
      break;
    }
  if (free < 0) return false;
  std::array<std::string_view, weather::MAX_SOURCES> entities{};
  size_t count = 0, names = 0, cards = 0;
  for (PageId id = 0; id < store.page_count(); ++id) {
    const auto* page = store.page(id);
    if (!page->navigable) continue;
    for (CardId card = page->first_card; card < page->first_card + page->runtime_card_count; ++card) {
      const auto* definition = store.card(card);
      const auto type = store.field(*definition, CardField::TYPE);
      if (type != "weather" && type != "weather_forecast") continue;
      if (!forecast_precision(store.field(*definition, CardField::PRECISION))) continue;
      const auto entity = store.field(*definition, CardField::ENTITY);
      if (!weather::entity_id_safe(entity)) continue;
      if (cards == weather::MAX_CARDS) continue;
      bool found = false;
      for (size_t i = 0; i < count; ++i) found |= entities[i] == entity;
      if (found) {
        ++cards;
        continue;
      }
      if (count == entities.size()) continue;
      if (entity.size() >= SIZE_MAX - names) return false;
      entities[count++] = entity;
      names += entity.size() + 1;
      ++cards;
    }
  }
  const size_t records = count * sizeof(Source), mapping = count ? store.card_count() : 0;
  if (mapping > SIZE_MAX - records || names > SIZE_MAX - records - mapping) return false;
  const size_t bytes = records + mapping + names;
  void* memory = bytes ? allocator_.allocate(bytes) : nullptr;
  if (bytes && !memory) return false;
  auto& slot = slots_[free];
  slot = {memory, static_cast<Source*>(memory), bytes, count, store.generation()};
  slot.cards = mapping ? static_cast<uint8_t*>(memory) + records : nullptr;
  slot.card_count = mapping;
  if (mapping) std::memset(slot.cards, UINT8_MAX, mapping);
  char* text = bytes ? static_cast<char*>(memory) + records + mapping : nullptr;
  for (size_t i = 0; i < count; ++i) {
    auto* source = new (slot.sources + i) Source{};
    std::memcpy(text, entities[i].data(), entities[i].size());
    text[entities[i].size()] = '\0';
    source->entity = {text, entities[i].size()};
    text += entities[i].size() + 1;
    if (active_ >= 0)
      for (size_t old = 0; old < slots_[active_].count; ++old)
        if (slots_[active_].sources[old].entity == source->entity) {
          source->forecast = slots_[active_].sources[old].forecast;
          source->stale = slots_[active_].sources[old].stale ||
                          (source->forecast.count > 0 &&
                           (!requests_.connected() || requests_.connection_generation() != connection_));
          break;
        }
  }
  cards = 0;
  if (mapping)
    for (PageId id = 0; id < store.page_count(); ++id) {
      const auto* page = store.page(id);
      if (!page->navigable) continue;
      for (CardId card = page->first_card; card < page->first_card + page->runtime_card_count; ++card) {
        const auto* definition = store.card(card);
        const auto type = store.field(*definition, CardField::TYPE);
        if ((type != "weather" && type != "weather_forecast") ||
            !forecast_precision(store.field(*definition, CardField::PRECISION)))
          continue;
        if (cards == weather::MAX_CARDS) continue;
        const auto entity = store.field(*definition, CardField::ENTITY);
        for (size_t source = 0; source < count; ++source)
          if (slot.sources[source].entity == entity) {
            slot.cards[card] = static_cast<uint8_t>(source);
            ++cards;
            break;
          }
      }
    }
  staged_ = free;
  size_t retained = 0;
  for (const auto& s : slots_) retained += s.bytes;
  memory_.peak_bytes = std::max(memory_.peak_bytes, retained);
  return true;
}
void PageForecastRuntime::discard_prepared() {
  if (staged_ >= 0) {
    const auto old = staged_;
    staged_ = -1;
    release(old);
  }
}
void PageForecastRuntime::invalidate_pending() {
  if (pending_.request) {
    assert(cancel_.request == 0);
    cancel_ = pending_;
    pending_ = {};
  }
}
void PageForecastRuntime::activate_prepared(const PageStore& store) {
  if (staged_ < 0 || !page_prepared_generation_matches(slots_[staged_].generation, store.generation()))
    return;
  invalidate_pending();
  if (active_ >= 0) retired_[active_] = true;
  active_ = staged_;
  slots_[active_].generation = store.generation();
  staged_ = -1;
  connected_ = requests_.connected();
  connection_ = requests_.connection_generation();
  ready_at_ = requests_.now_ms() + READY_DELAY_MS;
  for (size_t i = 0; i < slots_[active_].count; ++i) {
    auto& source = slots_[active_].sources[i];
    source.due = requests_.now_ms();
    source.loading = connected_;
    source.stale |= !connected_ && source.forecast.count > 0;
  }
  ++revision_;
  collect();
}
void PageForecastRuntime::retire_active() {
  if (active_ < 0) return;
  invalidate_pending();
  retired_[active_] = true;
  active_ = -1;
  ++revision_;
  collect();
}
bool PageForecastRuntime::current(PageForecastToken token) const {
  return same(token, pending_) && active_ >= 0 && slots_[active_].generation == token.configuration &&
         token.source < slots_[active_].count && requests_.connected() &&
         requests_.connection_generation() == token.connection;
}
void PageForecastRuntime::synchronize() {
  const auto connection = requests_.connection_generation();
  const bool connected = requests_.connected();
  if (connection == connection_ && connected == connected_) return;
  invalidate_pending();
  connection_ = connection;
  connected_ = connected;
  ready_at_ = requests_.now_ms() + READY_DELAY_MS;
  if (active_ >= 0)
    for (size_t i = 0; i < slots_[active_].count; ++i) {
      auto& source = slots_[active_].sources[i];
      source.loading = connected;
      source.stale = source.forecast.count > 0;
      source.wanted = true;
      source.due = requests_.now_ms();
    }
  ++revision_;
}
void PageForecastRuntime::fail(PageForecastToken token, bool timeout) {
  if (!current(token)) return;
  pending_ = {};
  auto& source = slots_[active_].sources[token.source];
  source.loading = false;
  source.stale = source.forecast.count > 0;
  source.wanted = true;
  source.due = requests_.now_ms() + RETRY_MS;
  ++memory_.failures;
  if (timeout) ++memory_.timeouts;
  ++revision_;
}
void PageForecastRuntime::receive(void* owner, PageForecastToken token, std::string_view payload,
                                  bool success) {
  static_cast<PageForecastRuntime*>(owner)->accept(token, payload, success);
}
void PageForecastRuntime::accept(PageForecastToken token, std::string_view payload, bool success) {
  Depth guard(depth_);
  if (!current(token)) {
    ++memory_.stale_responses;
    return;
  }
  if (requests_.now_ms() - started_ >= TIMEOUT_MS) {
    fail(token, true);
    return;
  }
  weather::Forecast parsed;
  if (!success || payload.size() > weather::MAX_PAYLOAD_BYTES ||
      !weather::parse(std::string(payload), parsed)) {
    fail(token);
    return;
  }
  if (!current(token)) {
    ++memory_.stale_responses;
    return;
  }
  auto& source = slots_[active_].sources[token.source];
  source.forecast = parsed;
  source.loading = false;
  source.stale = false;
  source.wanted = true;
  source.due = requests_.now_ms() + REFRESH_MS;
  pending_ = {};
  ++memory_.completed;
  ++revision_;
}
void PageForecastRuntime::refresh() {
  if (active_ >= 0)
    for (size_t i = 0; i < slots_[active_].count; ++i) {
      auto& source = slots_[active_].sources[i];
      source.wanted = true;
      source.due = requests_.now_ms();
    }
}
void PageForecastRuntime::loop() {
  if (depth_ > 0) return;
  collect();
  {
    Depth guard(depth_);
    synchronize();
    if (cancel_.request) {
      const auto token = cancel_;
      cancel_ = {};
      ++memory_.cancellations;
      requests_.cancel(this, token);
    }
    if (pending_.request && requests_.now_ms() - started_ >= TIMEOUT_MS) {
      const auto token = pending_;
      fail(token, true);
      cancel_ = token;
    }
    if (active_ < 0 || !connected_ || pending_.request || cancel_.request || next_request_ == 0 ||
        !due(requests_.now_ms(), ready_at_))
      return;
    for (size_t i = 0; i < slots_[active_].count; ++i) {
      auto& source = slots_[active_].sources[i];
      if (!source.wanted || !due(requests_.now_ms(), source.due)) continue;
      const PageForecastToken token{slots_[active_].generation, connection_, next_request_++,
                                    static_cast<uint8_t>(i)};
      pending_ = token;
      started_ = requests_.now_ms();
      source.loading = true;
      source.wanted = false;
      ++memory_.requests;
      ++revision_;
      if (!requests_.start(this, token, source.entity, receive) && current(token)) fail(token);
      break;
    }
  }
  collect();
}
bool PageForecastRuntime::snapshot(ConfigurationHandle card, std::string_view entity,
                                   PageForecastSnapshot& output) const {
  if (active_ < 0 || slots_[active_].generation != card.generation) return false;
  const auto& slot = slots_[active_];
  if (card.index >= slot.card_count) return false;
  const auto source_index = slot.cards[card.index];
  if (source_index >= slot.count) return false;
  const auto& source = slot.sources[source_index];
  if (source.entity != entity) return false;
  const bool connected = requests_.connected() && requests_.connection_generation() == connection_;
  output = {source.forecast, source.loading && connected,
            source.forecast.count > 0 && (source.stale || !connected)};
  return true;
}
PageForecastMemory PageForecastRuntime::memory() const {
  auto result = memory_;
  if (active_ >= 0) {
    result.active_bytes = slots_[active_].bytes;
    result.sources = slots_[active_].count;
  }
  if (staged_ >= 0) result.staged_bytes = slots_[staged_].bytes;
  for (int i = 0; i < 2; ++i)
    if (retired_[i]) result.retired_bytes += slots_[i].bytes;
  result.pending = pending_.request ? 1 : 0;
  return result;
}
}  // namespace espcontrol::pages
