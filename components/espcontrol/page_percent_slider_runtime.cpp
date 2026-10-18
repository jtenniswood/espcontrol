#include "page_percent_slider_runtime.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#include "bounded_percent_state.h"
#include "ha_state_limits.h"
#include "page_fan_mode.h"

namespace espcontrol::pages {
namespace {
bool domain(std::string_view entity, std::string_view name) {
  return entity.size() > name.size() && entity.substr(0, name.size()) == name;
}
bool on(std::string_view value) {
  for (const auto state : {"on", "true", "1", "home", "playing", "open", "opened", "opening", "closing",
                           "unlocked", "unlocking", "jammed"})
    if (fan_mode_equal(value, state)) return true;
  return false;
}
bool parse_level(PagePercentSliderKind kind, std::string_view value, uint8_t& percent) {
  if (kind != PagePercentSliderKind::LIGHT) return parse_bounded_percent(value, percent);
  char text[65];
  if (value.empty() || value.size() >= sizeof(text)) return false;
  std::memcpy(text, value.data(), value.size());
  text[value.size()] = '\0';
  char* end = nullptr;
  const float brightness = std::strtof(text, &end);
  if (end == text || !std::isfinite(brightness)) return false;
  if (brightness <= 0)
    percent = 0;
  else
    percent = static_cast<uint8_t>(std::max(1.0f, (std::min(255.0f, brightness) * 100.0f + 127.0f) / 255.0f));
  return true;
}
template <typename Emit>
bool cards(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id) {
      const auto kind = PagePercentSliderRuntime::classify(store, id);
      if (kind != PagePercentSliderKind::NONE &&
          !emit(id, kind, store.field(*store.card(id), CardField::ENTITY)))
        return false;
    }
  }
  return true;
}
}  // namespace
PagePercentSliderKind PagePercentSliderRuntime::classify(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  if (!card) return PagePercentSliderKind::NONE;
  const auto type = store.field(*card, CardField::TYPE);
  const auto entity = store.field(*card, CardField::ENTITY);
  const auto mode = store.field(*card, CardField::SENSOR);
  if (type == "cover") {
    if (!domain(entity, "cover.")) return PagePercentSliderKind::NONE;
    if (mode.empty()) return PagePercentSliderKind::COVER_POSITION;
    return mode == "tilt" ? PagePercentSliderKind::COVER_TILT : PagePercentSliderKind::NONE;
  }
  if (type != "slider" && type != "light_brightness" && type != "fan_speed")
    return PagePercentSliderKind::NONE;
  if (domain(entity, "light.")) return PagePercentSliderKind::LIGHT;
  if (domain(entity, "fan.")) return PagePercentSliderKind::FAN;
  if (domain(entity, "cover.")) return PagePercentSliderKind::COVER_POSITION;
  return PagePercentSliderKind::NONE;
}
const char* PagePercentSliderRuntime::attribute(PagePercentSliderKind kind) {
  switch (kind) {
    case PagePercentSliderKind::LIGHT:
      return "brightness";
    case PagePercentSliderKind::FAN:
      return "percentage";
    case PagePercentSliderKind::COVER_POSITION:
      return "current_position";
    case PagePercentSliderKind::COVER_TILT:
      return "current_tilt_position";
    default:
      return "";
  }
}
PagePercentSliderRuntime::PagePercentSliderRuntime(PageArenaAllocator& allocator,
                                                   PageEntitySubscriptions& subscriptions, size_t sources,
                                                   size_t count, size_t names)
    : allocator_(allocator),
      subscriptions_(subscriptions),
      max_sources_(sources),
      max_cards_(count),
      max_key_bytes_(names),
      connection_(subscriptions.connection_generation()) {}
PagePercentSliderRuntime::~PagePercentSliderRuntime() {
  discard_prepared();
  retire_active();
}
void PagePercentSliderRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PagePercentSliderRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t sources = 0, count = 0, names = 0;
  const auto fail = [&]() {
    ++failures_;
    discard_prepared();
    return false;
  };
  if (!store.source_document() ||
      !cards(store,
             [&](CardId id, PagePercentSliderKind kind, std::string_view key) {
               if (count >= max_cards_) return false;
               ++count;
               bool duplicate = false;
               cards(store, [&](CardId before, PagePercentSliderKind earlier, std::string_view previous) {
                 if (before < id && earlier == kind && previous == key) duplicate = true;
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
  staged_.source = static_cast<Source*>(arena);
  staged_.card =
      arena ? reinterpret_cast<Card*>(static_cast<char*>(arena) + sources * sizeof(Source)) : nullptr;
  staged_.generation = store.generation();
  staged_.valid = true;
  char* output = arena ? static_cast<char*>(arena) + sources * sizeof(Source) + card_bytes : nullptr;
  cards(store, [&](CardId id, PagePercentSliderKind kind, std::string_view key) {
    size_t index = 0;
    for (; index < staged_.sources; ++index)
      if (staged_.source[index].kind == kind && staged_.source[index].entity == key) break;
    if (index == staged_.sources) {
      auto& source = *new (staged_.source + staged_.sources++) Source{};
      source.kind = kind;
      std::memcpy(output, key.data(), key.size());
      output[key.size()] = '\0';
      source.entity = {output, key.size()};
      output += key.size() + 1;
    }
    new (staged_.card + staged_.cards++) Card{id, static_cast<uint16_t>(index), 0, false};
    return true;
  });
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PagePercentSliderRuntime::discard_prepared() {
  clear(staged_);
}
void PagePercentSliderRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.valid || !page_prepared_generation_matches(staged_.generation, store.generation())) return;
  clear(active_);
  active_ = staged_;
  active_.generation = store.generation();
  staged_ = {};
  ++revision_;
}
void PagePercentSliderRuntime::retire_active() {
  if (active_.valid) ++revision_;
  clear(active_);
}
void PagePercentSliderRuntime::clear_previews(Slot& slot, size_t source) {
  for (size_t i = 0; i < slot.cards; ++i)
    if (slot.card[i].source == source && slot.card[i].preview) {
      slot.card[i].preview = false;
      ++revision_;
    }
}
void PagePercentSliderRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attr,
                                        std::string_view value, bool copied) {
  loop();
  for (auto* slot : {&active_, &staged_}) {
    if (!slot->valid || generation != slot->generation) continue;
    for (size_t i = 0; i < slot->sources; ++i) {
      auto& source = slot->source[i];
      if (source.entity != entity || (!attr.empty() && attr != attribute(source.kind))) continue;
      const auto old_percent = source.percent;
      const bool old_available = source.available, old_icon = source.icon_on;
      if (attr.empty()) {
        const auto state = value.substr(0, HA_SHORT_STATE_MAX_LEN);
        const bool available = !fan_mode_trim(state).empty() && !fan_mode_equal(state, "unknown") &&
                               !fan_mode_equal(state, "unavailable");
        const bool active = on(state);
        if (!copied || !source.live_primary) source.available = available;
        if (copied) {
          source.seed_primary = true;
          source.seed_off = !active;
          if (!source.live_fill) source.percent = source.seed_off ? 0 : source.seed_level;
          if (!source.live_icon) source.icon_on = active;
        } else {
          source.live_primary = source.live_icon = true;
          source.icon_on = active;
          if (!active) {
            source.live_fill = true;
            ++source.write_revision;
            ++revision_;
            source.percent = 0;
            clear_previews(*slot, i);
          }
        }
      } else {
        uint8_t percent = 0;
        if (!parse_level(source.kind, value, percent)) continue;
        const bool cover = source.kind == PagePercentSliderKind::COVER_POSITION ||
                           source.kind == PagePercentSliderKind::COVER_TILT;
        if (copied) {
          source.seed_level = percent;
          source.seed_level_known = true;
          if (!source.live_fill) source.percent = source.seed_off ? 0 : percent;
          if (cover && !source.live_icon && !source.seed_primary) source.icon_on = percent > 0;
        } else {
          source.live_fill = true;
          ++source.write_revision;
          ++revision_;
          source.percent = percent;
          if (cover) {
            source.live_icon = true;
            source.icon_on = percent > 0;
          }
          clear_previews(*slot, i);
        }
      }
      if (source.percent != old_percent || source.available != old_available || source.icon_on != old_icon)
        ++revision_;
    }
  }
}
void PagePercentSliderRuntime::loop() {
  const auto connection = subscriptions_.connection_generation();
  if (connection_ == connection) return;
  connection_ = connection;
  for (auto* slot : {&active_, &staged_}) {
    for (size_t i = 0; i < slot->sources; ++i) {
      auto& source = slot->source[i];
      const auto entity = source.entity;
      const auto kind = source.kind;
      source = {};
      source.entity = entity;
      source.kind = kind;
      source.available = false;
    }
    for (size_t i = 0; i < slot->cards; ++i) slot->card[i].preview = false;
  }
  ++revision_;
}
PagePercentSliderSnapshot PagePercentSliderRuntime::read(ConfigurationHandle handle) const {
  if (!active_.valid || handle.generation != active_.generation) return {};
  for (size_t i = 0; i < active_.cards; ++i) {
    const auto& card = active_.card[i];
    if (card.card != handle.index) continue;
    const auto& source = active_.source[card.source];
    return {
        source.kind,          card.preview ? card.percent : source.percent,
        source.available,     source.icon_on,
        card.preview,         card.preview || source.live_fill || source.seed_level_known || source.seed_off,
        source.write_revision};
  }
  return {};
}
bool PagePercentSliderRuntime::preview(ConfigurationHandle handle, uint8_t percent) {
  loop();
  if (!active_.valid || handle.generation != active_.generation || percent > 100) return false;
  for (size_t i = 0; i < active_.cards; ++i) {
    auto& card = active_.card[i];
    if (card.card != handle.index || !active_.source[card.source].available) continue;
    if (!card.preview || card.percent != percent) ++revision_;
    card.percent = percent;
    card.preview = true;
    return true;
  }
  return false;
}
PagePercentSliderMemory PagePercentSliderRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.sources, active_.cards, failures_};
}
}  // namespace espcontrol::pages
