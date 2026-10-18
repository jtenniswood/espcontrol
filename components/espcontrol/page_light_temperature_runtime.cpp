#include "page_light_temperature_runtime.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

#include "ha_state_limits.h"
#include "page_fan_mode.h"
namespace espcontrol::pages {
namespace {
bool parse(std::string_view value, int& output) {
  char text[65];
  if (value.empty() || value.size() >= sizeof(text)) return false;
  std::memcpy(text, value.data(), value.size());
  text[value.size()] = '\0';
  char* end = nullptr;
  const float number = std::strtof(text, &end);
  const double rounded = static_cast<double>(number + 0.5f);
  if (end == text || !std::isfinite(number) || rounded < std::numeric_limits<int>::min() ||
      rounded > std::numeric_limits<int>::max())
    return false;
  output = static_cast<int>(rounded);
  return true;
}
void range(std::string_view value, int& minimum, int& maximum) {
  minimum = 2000;
  maximum = 6500;
  const auto dash = value.find('-');
  if (dash == std::string_view::npos || dash == 0) return;
  const auto integer = [](std::string_view input, int& output) {
    char buffer[65];
    if (input.size() >= sizeof(buffer)) return false;
    std::memcpy(buffer, input.data(), input.size());
    buffer[input.size()] = '\0';
    char* end = nullptr;
    const auto number = std::strtoll(buffer, &end, 10);
    if (end == buffer || number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
      return false;
    output = static_cast<int>(number);
    return true;
  };
  int a, b;
  if (integer(value.substr(0, dash), a) && integer(value.substr(dash + 1), b) && a >= 1000 && b > a) {
    minimum = a;
    maximum = b;
  }
}
bool on(std::string_view value) {
  for (const auto state : {"on", "true", "1", "home", "playing", "open", "opened", "opening", "closing",
                           "unlocked", "unlocking", "jammed"})
    if (fan_mode_equal(value, state)) return true;
  return false;
}
template <typename Emit>
bool cards(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id)
      if (PageLightTemperatureRuntime::supported(store, id) &&
          !emit(id, store.field(*store.card(id), CardField::ENTITY)))
        return false;
  }
  return true;
}
}  // namespace
bool PageLightTemperatureRuntime::supported(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  return card && store.field(*card, CardField::TYPE) == "light_temperature";
}
PageLightTemperatureRuntime::PageLightTemperatureRuntime(PageArenaAllocator& allocator,
                                                         PageEntitySubscriptions& subscriptions,
                                                         size_t sources, size_t count, size_t names)
    : allocator_(allocator),
      subscriptions_(subscriptions),
      max_sources_(sources),
      max_cards_(count),
      max_key_bytes_(names),
      connection_(subscriptions.connection_generation()) {}
PageLightTemperatureRuntime::~PageLightTemperatureRuntime() {
  discard_prepared();
  retire_active();
}
void PageLightTemperatureRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PageLightTemperatureRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t count = 0, sources = 0, names = 0;
  const auto fail = [&]() {
    ++failures_;
    discard_prepared();
    return false;
  };
  if (!store.source_document() ||
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
      output += key.size() + 1;
    }
    auto& card = *new (staged_.card + staged_.cards++) Card{};
    card.card = id;
    card.available = !key.empty();
    card.source = static_cast<uint16_t>(index);
    range(store.field(*store.card(id), CardField::UNIT), card.minimum, card.maximum);
    card.last_kelvin = card.display_kelvin = card.minimum;
    card.fill_kelvin = card.minimum + (card.maximum - card.minimum) / 2;
    card.color = store.field(*store.card(id), CardField::PRECISION) == "color";
    return true;
  });
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PageLightTemperatureRuntime::discard_prepared() {
  clear(staged_);
}
void PageLightTemperatureRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.valid || !page_prepared_generation_matches(staged_.generation, store.generation())) return;
  clear(active_);
  active_ = staged_;
  active_.generation = store.generation();
  staged_ = {};
  ++revision_;
}
void PageLightTemperatureRuntime::retire_active() {
  if (active_.valid) ++revision_;
  clear(active_);
}

void PageLightTemperatureRuntime::apply_kelvin(Card& card) {
  const int value = std::max(card.minimum, std::min(card.maximum, card.last_kelvin));
  card.percent =
      static_cast<uint8_t>(static_cast<int64_t>(value - card.minimum) * 100 / (card.maximum - card.minimum));
  card.fill_kelvin = value;
  ++card.write_revision;
}
void PageLightTemperatureRuntime::observed(uint32_t generation, std::string_view entity,
                                           std::string_view attribute, std::string_view value, bool copied) {
  loop();
  if (!attribute.empty() && attribute != "color_temp_kelvin") return;
  int kelvin = 0;
  if (!attribute.empty() && !parse(value, kelvin)) return;
  for (auto* slot : {&active_, &staged_}) {
    if (!slot->valid || slot->generation != generation) continue;
    for (size_t source = 0; source < slot->sources; ++source) {
      auto& item = slot->source[source];
      if (item.entity != entity) continue;
      auto& live = attribute.empty() ? item.live_primary : item.live_kelvin;
      if (copied && live) continue;
      if (!copied) live = true;
      for (size_t i = 0; i < slot->cards; ++i) {
        auto& card = slot->card[i];
        if (card.source != source) continue;
        if (attribute.empty()) {
          value = value.substr(0, HA_SHORT_STATE_MAX_LEN);
          card.available = !fan_mode_trim(value).empty() && !fan_mode_equal(value, "unknown") &&
                           !fan_mode_equal(value, "unavailable");
          card.known = true;
          card.on = on(value);
          if (!card.on) {
            card.percent = 0;
            ++card.write_revision;
          } else if (card.has_kelvin)
            apply_kelvin(card);
          if (!card.available) card.dragging = card.gesture = false;
        } else {
          card.last_kelvin = kelvin;
          card.has_kelvin = true;
          if (card.known && card.on) apply_kelvin(card);
        }
      }
      ++revision_;
    }
  }
}
void PageLightTemperatureRuntime::loop() {
  const auto connection = subscriptions_.connection_generation();
  if (connection_ == connection) return;
  connection_ = connection;
  for (auto* slot : {&active_, &staged_}) {
    for (size_t i = 0; i < slot->sources; ++i)
      slot->source[i].live_primary = slot->source[i].live_kelvin = false;
    for (size_t i = 0; i < slot->cards; ++i) {
      auto& card = slot->card[i];
      card.available = card.known = card.on = card.has_kelvin = card.dragging = card.gesture = false;
      card.percent = 0;
      card.last_kelvin = card.display_kelvin = card.minimum;
      card.fill_kelvin = card.minimum + (card.maximum - card.minimum) / 2;
      ++card.write_revision;
    }
  }
  ++revision_;
}
PageLightTemperatureRuntime::Card* PageLightTemperatureRuntime::find(ConfigurationHandle handle) {
  if (!active_.valid || handle.generation != active_.generation) return nullptr;
  for (size_t i = 0; i < active_.cards; ++i)
    if (active_.card[i].card == handle.index) return active_.card + i;
  return nullptr;
}
PageLightTemperatureSnapshot PageLightTemperatureRuntime::read(ConfigurationHandle handle) const {
  if (!active_.valid || handle.generation != active_.generation) return {};
  for (size_t i = 0; i < active_.cards; ++i) {
    const auto& card = active_.card[i];
    if (card.card != handle.index) continue;
    return {card.minimum,   card.maximum,  card.fill_kelvin, card.display_kelvin, card.percent,
            card.available, card.dragging, card.gesture,     card.color,          card.write_revision};
  }
  return {};
}
bool PageLightTemperatureRuntime::begin(ConfigurationHandle handle) {
  loop();
  auto* card = find(handle);
  if (!card || !card->available || card->gesture) return false;
  card->gesture = true;
  ++revision_;
  return true;
}
bool PageLightTemperatureRuntime::drag(ConfigurationHandle handle, int percent) {
  loop();
  auto* card = find(handle);
  if (!card || !card->available || !card->gesture) return false;
  percent = std::max(0, std::min(100, percent));
  if (card->percent == percent) return true;
  card->percent = static_cast<uint8_t>(percent);
  card->last_kelvin = card->minimum + static_cast<int64_t>(percent) * (card->maximum - card->minimum) / 100;
  card->fill_kelvin = card->last_kelvin;
  const auto rounded = (static_cast<int64_t>(card->last_kelvin) + 25) / 50 * 50;
  card->display_kelvin =
      static_cast<int>(std::max<int64_t>(card->minimum, std::min<int64_t>(card->maximum, rounded)));
  card->on = card->known = card->has_kelvin = card->dragging = true;
  ++card->write_revision;
  ++revision_;
  return true;
}
bool PageLightTemperatureRuntime::finish(ConfigurationHandle handle, int percent, int& kelvin) {
  if (!drag(handle, percent)) return false;
  auto* card = find(handle);
  kelvin = card->minimum + static_cast<int64_t>(card->percent) * (card->maximum - card->minimum) / 100;
  card->gesture = card->dragging = false;
  ++revision_;
  return true;
}
void PageLightTemperatureRuntime::cancel(ConfigurationHandle handle) {
  loop();
  auto* card = find(handle);
  if (!card || !card->gesture) return;
  card->gesture = card->dragging = false;
  ++revision_;
}
PageLightTemperatureMemory PageLightTemperatureRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.sources, active_.cards, failures_};
}
}  // namespace espcontrol::pages
