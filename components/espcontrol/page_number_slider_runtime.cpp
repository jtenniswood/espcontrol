#include "page_number_slider_runtime.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

namespace espcontrol::pages {
namespace {
constexpr uint8_t MINIMUM = 1, MAXIMUM = 2, STEP = 4, STATE = 8;
static_assert(number_slider::MAX_SLIDER_POSITIONS <= UINT16_MAX);
bool domain(std::string_view entity, std::string_view name) {
  return entity.size() > name.size() && entity.substr(0, name.size()) == name;
}
bool parse(std::string_view value, double& output) {
  char input[65];
  if (value.empty() || value.size() >= sizeof(input)) return false;
  std::memcpy(input, value.data(), value.size());
  input[value.size()] = '\0';
  char* end = nullptr;
  output = std::strtod(input, &end);
  return end != input && std::isfinite(output);
}
template <typename Emit>
bool cards(const PageStore& store, Emit emit) {
  for (PageId page = 0; page < store.page_count(); ++page) {
    const auto* definition = store.page(page);
    if (!definition->navigable) continue;
    for (CardId id = definition->first_card; id < definition->first_card + definition->card_count; ++id)
      if (PageNumberSliderRuntime::supported(store, id) &&
          !emit(id, store.field(*store.card(id), CardField::ENTITY)))
        return false;
  }
  return true;
}
}  // namespace
bool PageNumberSliderRuntime::supported(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  if (!card) return false;
  const auto entity = store.field(*card, CardField::ENTITY);
  if (!domain(entity, "number.") && !domain(entity, "input_number.")) return false;
  const auto type = store.field(*card, CardField::TYPE);
  if (type == "slider" || type == "light_brightness" || type == "fan_speed") return true;
  const auto mode = store.field(*card, CardField::SENSOR);
  return type == "cover" && (mode.empty() || mode == "tilt");
}
PageNumberSliderRuntime::PageNumberSliderRuntime(PageArenaAllocator& allocator,
                                                 PageEntitySubscriptions& subscriptions, size_t sources,
                                                 size_t count, size_t names)
    : allocator_(allocator),
      subscriptions_(subscriptions),
      max_sources_(sources),
      max_cards_(count),
      max_key_bytes_(names),
      connection_(subscriptions.connection_generation()) {}
PageNumberSliderRuntime::~PageNumberSliderRuntime() {
  discard_prepared();
  retire_active();
}
void PageNumberSliderRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PageNumberSliderRuntime::prepare(const PageStore& store) {
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
    new (staged_.card + staged_.cards++) Card{id, static_cast<uint16_t>(index), 0, 100, 0, false, false};
    return true;
  });
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PageNumberSliderRuntime::discard_prepared() {
  clear(staged_);
}
void PageNumberSliderRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.valid || !page_prepared_generation_matches(staged_.generation, store.generation())) return;
  clear(active_);
  active_ = staged_;
  active_.generation = store.generation();
  staged_ = {};
  ++revision_;
}
void PageNumberSliderRuntime::retire_active() {
  if (active_.valid) ++revision_;
  clear(active_);
}
bool PageNumberSliderRuntime::ready(const Source& source) {
  return (source.known & 7) == 7 && number_slider::metadata_valid(source.metadata);
}
void PageNumberSliderRuntime::set_position(Card& card, int position) {
  card.position = static_cast<uint16_t>(std::max(0, std::min<int>(card.positions, position)));
  card.fill =
      card.positions
          ? static_cast<uint8_t>(std::llround(static_cast<double>(card.position) * 100 / card.positions))
          : 0;
}
void PageNumberSliderRuntime::apply_state(Card& card, const Source& source) {
  if (ready(source) && (source.known & STATE) && !card.dragging)
    set_position(card, number_slider::position_for_value(source.metadata, source.state));
}
void PageNumberSliderRuntime::observed(uint32_t generation, std::string_view entity,
                                       std::string_view attribute, std::string_view value, bool) {
  loop();
  uint8_t field = attribute.empty()     ? STATE
                  : attribute == "min"  ? MINIMUM
                  : attribute == "max"  ? MAXIMUM
                  : attribute == "step" ? STEP
                                        : 0;
  if (!field) return;
  for (auto* slot : {&active_, &staged_}) {
    if (!slot->valid || generation != slot->generation) continue;
    for (size_t i = 0; i < slot->sources; ++i) {
      auto& source = slot->source[i];
      if (source.entity != entity) continue;
      const auto previous_metadata = source.metadata;
      const auto previous_known = source.known;
      double number = 0;
      const bool valid = parse(value, number);
      if (valid)
        source.known |= field;
      else
        source.known &= static_cast<uint8_t>(~field);
      if (valid) {
        if (field == STATE)
          source.state = number;
        else if (field == MINIMUM)
          source.metadata.minimum = number;
        else if (field == MAXIMUM)
          source.metadata.maximum = number;
        else
          source.metadata.step = number;
      }
      const bool metadata = field != STATE;
      const bool changed_range = (source.known & 7) != (previous_known & 7) ||
                                 source.metadata.minimum != previous_metadata.minimum ||
                                 source.metadata.maximum != previous_metadata.maximum ||
                                 source.metadata.step != previous_metadata.step;
      const bool usable = ready(source);
      const auto positions = usable ? number_slider::slider_position_count(source.metadata) : 0;
      for (size_t id = 0; id < slot->cards; ++id) {
        auto& card = slot->card[id];
        if (card.source != i) continue;
        if (metadata) {
          if (usable) {
            card.positions = static_cast<uint16_t>(positions);
            card.position = std::min(card.position, card.positions);
          }
          card.dragging = false;
          if (changed_range || !usable) card.gesture = false;
        } else if (!valid)
          card.dragging = card.gesture = false;
        apply_state(card, source);
      }
      // Metadata always resets the numeric overlay. Equivalent metadata keeps
      // the held gesture valid; a changed range cancels its eventual command.
      // Identical state still confirms a released thumb.
      ++revision_;
    }
  }
}
void PageNumberSliderRuntime::loop() {
  const auto connection = subscriptions_.connection_generation();
  if (connection == connection_) return;
  connection_ = connection;
  for (auto* slot : {&active_, &staged_}) {
    for (size_t i = 0; i < slot->sources; ++i) {
      auto& source = slot->source[i];
      const auto entity = source.entity;
      source = {};
      source.entity = entity;
    }
    for (size_t i = 0; i < slot->cards; ++i) {
      auto& card = slot->card[i];
      card.position = 0;
      card.positions = 100;
      card.fill = 0;
      card.dragging = card.gesture = false;
    }
  }
  ++revision_;
}
PageNumberSliderRuntime::Card* PageNumberSliderRuntime::find(ConfigurationHandle handle) {
  if (!active_.valid || handle.generation != active_.generation) return nullptr;
  for (size_t i = 0; i < active_.cards; ++i)
    if (active_.card[i].card == handle.index) return active_.card + i;
  return nullptr;
}
PageNumberSliderSnapshot PageNumberSliderRuntime::read(ConfigurationHandle handle) const {
  if (!active_.valid || handle.generation != active_.generation) return {};
  for (size_t i = 0; i < active_.cards; ++i) {
    const auto& card = active_.card[i];
    if (card.card != handle.index) continue;
    const auto& source = active_.source[card.source];
    return {source.metadata, card.position, card.positions, card.fill, (source.known & STATE) != 0,
            ready(source),   card.dragging, card.gesture};
  }
  return {};
}
bool PageNumberSliderRuntime::begin(ConfigurationHandle handle) {
  loop();
  auto* card = find(handle);
  if (!card || card->gesture) return false;
  const auto& source = active_.source[card->source];
  if (!(source.known & STATE) || !ready(source)) return false;
  card->dragging = card->gesture = true;
  ++revision_;
  return true;
}
bool PageNumberSliderRuntime::drag(ConfigurationHandle handle, int position) {
  loop();
  auto* card = find(handle);
  if (!card || !card->gesture) return false;
  const auto& source = active_.source[card->source];
  if (!(source.known & STATE) || !ready(source)) return false;
  const auto before = card->position;
  set_position(*card, position);
  if (before != card->position) ++revision_;
  return true;
}
bool PageNumberSliderRuntime::finish(ConfigurationHandle handle, int position, double& value) {
  if (!drag(handle, position)) return false;
  auto* card = find(handle);
  const auto& source = active_.source[card->source];
  value = number_slider::value_for_position(source.metadata, card->position);
  card->dragging = card->gesture = false;
  ++revision_;
  return true;
}
void PageNumberSliderRuntime::cancel(ConfigurationHandle handle) {
  loop();
  auto* card = find(handle);
  if (!card || !card->gesture) return;
  card->dragging = card->gesture = false;
  apply_state(*card, active_.source[card->source]);
  ++revision_;
}
void PageNumberSliderRuntime::discard_finished(ConfigurationHandle handle, double expected,
                                               const number_slider::Metadata& metadata) {
  loop();
  auto* card = find(handle);
  if (!card || card->gesture) return;
  const auto& source = active_.source[card->source];
  if (metadata.minimum != source.metadata.minimum || metadata.maximum != source.metadata.maximum ||
      metadata.step != source.metadata.step ||
      number_slider::value_for_position(source.metadata, card->position) != expected)
    return;
  apply_state(*card, source);
  ++revision_;
}
PageNumberSliderMemory PageNumberSliderRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.sources, active_.cards, failures_};
}
}  // namespace espcontrol::pages
