#include "page_media_state_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "button_grid_subpage_parser.h"
#include "media_playlist_match.h"

namespace espcontrol::pages {
namespace {
constexpr uint16_t CONTENT_BITS = (1u << 0) | (1u << 2) | (1u << 3) | (1u << 5);
std::string_view configured_entity(const PageStore& store, CardId card) {
  const auto* definition = store.card(card);
  if (!definition || store.field(*definition, CardField::TYPE) != "media") return {};
  for (PageId id = 0; id < store.page_count(); ++id) {
    const auto* page = store.page(id);
    if (card >= page->first_card && card < page->first_card + page->card_count)
      return page->navigable ? store.field(*definition, CardField::ENTITY) : std::string_view{};
  }
  return {};
}
bool usable(std::string_view state) {
  return state == "playing" || state == "paused" || state == "buffering";
}
}  // namespace
PageMediaStateRuntime::~PageMediaStateRuntime() {
  discard_prepared();
  retire_active();
}
void PageMediaStateRuntime::release(int index) {
  if (index < 0) return;
  auto& slot = slots_[index];
  if (slot.arena) {
    for (size_t i = 0; i < slot.count; ++i) slot.sources[i].~Source();
    allocator_.release(slot.arena, slot.bytes);
  }
  slot = {};
}
bool PageMediaStateRuntime::prepare(const PageStore& store) {
  discard_prepared();
  synchronize();
  if (!store.generation()) return false;
  size_t count = 0, names = 0;
  for (CardId card = 0; card < store.card_count(); ++card) {
    const auto entity = configured_entity(store, card);
    if (entity.empty()) continue;
    bool duplicate = false;
    for (CardId previous = 0; previous < card; ++previous)
      if (configured_entity(store, previous) == entity) {
        duplicate = true;
        break;
      }
    if (duplicate) continue;
    if (entity.size() >= SIZE_MAX - names) return false;
    names += entity.size() + 1;
    ++count;
  }
  if (count > (SIZE_MAX - names) / sizeof(Source)) return false;
  const size_t bytes = count * sizeof(Source) + names;
  auto* arena = bytes ? allocator_.allocate(bytes) : nullptr;
  if (bytes && !arena) return false;
  const int index = active_ == 0 ? 1 : 0;
  auto& slot = slots_[index];
  slot = {arena, static_cast<Source*>(arena), 0, bytes, store.generation(), store.card_count()};
  char* text = bytes ? static_cast<char*>(arena) + count * sizeof(Source) : nullptr;
  for (CardId card = 0; card < store.card_count(); ++card) {
    const auto entity = configured_entity(store, card);
    if (entity.empty()) continue;
    bool duplicate = false;
    for (size_t i = 0; i < slot.count; ++i)
      if (slot.sources[i].entity == entity) {
        duplicate = true;
        break;
      }
    if (duplicate) continue;
    auto* source = new (slot.sources + slot.count++) Source{};
    if (active_ >= 0)
      for (size_t i = 0; i < slots_[active_].count; ++i)
        if (slots_[active_].sources[i].entity == entity) {
          *source = slots_[active_].sources[i];
          break;
        }
    std::memcpy(text, entity.data(), entity.size());
    text[entity.size()] = '\0';
    source->entity = {text, entity.size()};
    text += entity.size() + 1;
  }
  staged_ = index;
  memory_.peak_bytes = std::max(memory_.peak_bytes, slots_[0].bytes + slots_[1].bytes);
  return true;
}
void PageMediaStateRuntime::discard_prepared() {
  release(staged_);
  staged_ = -1;
}
void PageMediaStateRuntime::activate_prepared(const PageStore& store) {
  if (staged_ < 0 || slots_[staged_].generation != store.generation()) return;
  const int old = active_;
  active_ = staged_;
  staged_ = -1;
  release(old);
  ++revision_;
}
void PageMediaStateRuntime::retire_active() {
  release(active_);
  active_ = -1;
  ++revision_;
}
uint16_t PageMediaStateRuntime::attribute_bit(std::string_view attribute) {
  if (attribute == "media_content_id") return 1u << 0;
  if (attribute == "media_content_type") return 1u << 1;
  if (attribute == "media_title") return 1u << 2;
  if (attribute == "media_artist") return 1u << 3;
  if (attribute == "source") return 1u << 4;
  if (attribute == "entity_picture") return 1u << 5;
  return 0;
}
void PageMediaStateRuntime::synchronize() {
  const auto connection = transport_.connection_generation();
  if (connection_ == connection) return;
  connection_ = connection;
  for (auto& slot : slots_)
    for (size_t i = 0; i < slot.count; ++i) {
      auto& source = slot.sources[i];
      source.fresh = source.present = 0;
      source.state_known = false;
      source.state[0] = '\0';
      source.resync = false;
    }
  ++revision_;
}
void PageMediaStateRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                                     std::string_view value, bool copied) {
  synchronize();
  Slot* slot = nullptr;
  for (int index : {active_, staged_})
    if (index >= 0 && slots_[index].generation == generation) {
      slot = &slots_[index];
      break;
    }
  if (!slot) {
    ++memory_.ignored;
    return;
  }
  Source* source = nullptr;
  for (size_t i = 0; i < slot->count; ++i)
    if (slot->sources[i].entity == entity) {
      source = &slot->sources[i];
      break;
    }
  if (!source) return;
  if (attribute.empty()) {
    char state[HA_SHORT_STATE_MAX_LEN + 1];
    auto normalized = media::playlist_trim(value.substr(0, HA_SHORT_STATE_MAX_LEN));
    size_t i = 0;
    for (; i < normalized.size(); ++i)
      state[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(normalized[i])));
    state[i] = '\0';
    const bool next_usable = usable(state);
    if (!next_usable && (!source->state_known || std::strcmp(source->state, state) != 0))
      source->fresh = source->present = 0;
    if (next_usable && (!source->state_known || !usable(source->state)) && !(source->present & CONTENT_BITS))
      source->resync = true;
    std::memcpy(source->state, state, i + 1);
    source->state_known = true;
    ++revision_;
    return;
  }
  // A shared copy carries the old model's validity, not a fresh observation.
  // In particular, replacing configuration must not resurrect idle metadata.
  if (copied) return;
  const auto bit = attribute_bit(attribute);
  if (!bit) return;
  char decoded[HA_STATE_TEXT_MAX_LEN + 1];
  const size_t capacity = attribute == "media_content_type" ? HA_SHORT_STATE_MAX_LEN + 1 : sizeof(decoded);
  const auto metadata = decode_html_entities_bounded(decoded, capacity, value);
  const bool present = !metadata.empty() && metadata != "unknown" && metadata != "unavailable";
  source->fresh |= bit;
  if (present)
    source->present |= bit;
  else
    source->present &= ~bit;
  ++revision_;
}
bool PageMediaStateRuntime::current(ConfigurationHandle handle, std::string_view entity,
                                    std::string_view attribute) const {
  if (active_ < 0 || slots_[active_].generation != handle.generation ||
      handle.index >= slots_[active_].card_count || !transport_.connected() ||
      connection_ != transport_.connection_generation())
    return false;
  const auto bit = attribute_bit(attribute);
  if (!bit) return false;
  const auto& slot = slots_[active_];
  for (size_t i = 0; i < slot.count; ++i)
    if (slot.sources[i].entity == entity) return slot.sources[i].fresh & bit;
  return false;
}
void PageMediaStateRuntime::loop() {
  synchronize();
  if (active_ < 0 || !transport_.connected()) return;
  bool reannounce = false;
  auto& slot = slots_[active_];
  for (size_t i = 0; i < slot.count; ++i) {
    auto& source = slot.sources[i];
    reannounce |= source.resync && usable(source.state) && !(source.present & CONTENT_BITS);
    source.resync = false;
  }
  // Clear all pending flags before the external call; it may replace the model.
  if (reannounce) {
    ++memory_.reannouncements;
    transport_.reannounce();
  }
}
PageMediaMemory PageMediaStateRuntime::memory() const {
  auto result = memory_;
  if (active_ >= 0) {
    result.active_bytes = slots_[active_].bytes;
    result.sources = slots_[active_].count;
  }
  if (staged_ >= 0) result.staged_bytes = slots_[staged_].bytes;
  return result;
}
}  // namespace espcontrol::pages
