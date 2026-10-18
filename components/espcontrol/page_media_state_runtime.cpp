#include "page_media_state_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "button_grid_subpage_parser.h"
#include "media_playlist_match.h"

namespace espcontrol::pages {
namespace {
constexpr uint8_t REQUIRE_METADATA = 1, REQUIRE_PROGRESS = 2, REQUIRE_DETAILS = 4, REQUIRE_PRIMARY_COVER = 8;
constexpr uint16_t TITLE_BIT = 1u << 2, ARTIST_BIT = 1u << 3;
constexpr uint16_t CONTENT_BITS = (1u << 0) | (1u << 2) | (1u << 3) | (1u << 5) | (1u << 6);
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
uint8_t requirements(const PageStore& store, CardId card) {
  const auto* definition = store.card(card);
  const auto mode = card_runtime_media_mode(std::string(store.field(*definition, CardField::SENSOR)));
  const bool cover = mode == "cover_art" ||
                     (mode == "now_playing" &&
                      cfg_option_token_present(std::string(store.field(*definition, CardField::OPTIONS)),
                                               MEDIA_COVER_ART_OPTION));
  uint8_t result = (mode == "playlist" || mode == "now_playing" || cover) ? REQUIRE_METADATA : 0;
  if (cover) result |= REQUIRE_PRIMARY_COVER;
  if (mode == "now_playing" || cover || mode == "control_modal" || mode == "speaker_group")
    result |= REQUIRE_METADATA | REQUIRE_DETAILS;
  if (mode == "position" ||
      (mode == "now_playing" && store.field(*definition, CardField::PRECISION) == "progress") || cover ||
      mode == "control_modal" || mode == "speaker_group")
    result |= REQUIRE_PROGRESS;
  return result;
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
    source->requirements = 0;
    for (CardId sibling = 0; sibling < store.card_count(); ++sibling)
      if (configured_entity(store, sibling) == entity) source->requirements |= requirements(store, sibling);
    if (!(source->requirements & REQUIRE_METADATA)) {
      source->fresh = source->present = 0;
      source->resync = false;
    } else if (source->state_known && usable(source->state) && !(source->present & CONTENT_BITS))
      source->resync = true;
    if (!(source->requirements & REQUIRE_DETAILS)) {
      source->fresh &= ~(TITLE_BIT | ARTIST_BIT);
      source->present &= ~(TITLE_BIT | ARTIST_BIT);
      source->input = {};
      source->content_fingerprint = 0;
      source->content_kind = source->type_kind = media::MediaItemKind::UNKNOWN;
      source->title_awaiting_refresh = source->metadata_refresh = false;
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
  tick_ms_ = transport_.now_ms();
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
  if (attribute == "entity_picture_local") return 1u << 6;
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
      source.clock = {};
      source.input = {};
      source.content_fingerprint = 0;
      source.content_kind = source.type_kind = media::MediaItemKind::UNKNOWN;
      source.title_awaiting_refresh = source.metadata_refresh = false;
      source.resync = false;
    }
  ++revision_;
}
void PageMediaStateRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                                     std::string_view value, bool copied) {
  observed_update(generation, entity, attribute, value, copied, {});
}
void PageMediaStateRuntime::observed_update(uint32_t generation, std::string_view entity,
                                            std::string_view attribute, std::string_view value, bool copied,
                                            std::string_view previous) {
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
    source->clock.playback(value.substr(0, HA_SHORT_STATE_MAX_LEN) == "playing", transport_.now_ms());
    if (!copied) source->input.state_observed();
    const bool next_usable = usable(state);
    if (!next_usable && (!source->state_known || std::strcmp(source->state, state) != 0)) {
      source->fresh = source->present = 0;
      source->input.invalidate_content();
      source->content_fingerprint = 0;
      source->content_kind = source->type_kind = media::MediaItemKind::UNKNOWN;
      source->title_awaiting_refresh = source->metadata_refresh = false;
    }
    if ((source->requirements & REQUIRE_METADATA) && next_usable &&
        (!source->state_known || !usable(source->state)) && !(source->present & CONTENT_BITS))
      source->resync = true;
    std::memcpy(source->state, state, i + 1);
    source->state_known = true;
    ++revision_;
    return;
  }
  // A shared copy carries the old model's validity, not a fresh observation.
  // In particular, replacing configuration must not resurrect idle metadata.
  if (copied) return;
  if (attribute == "media_duration") {
    source->clock.duration_value(value);
    ++revision_;
    return;
  }
  if (attribute == "media_position") {
    source->clock.position_value(value, transport_.now_ms());
    ++revision_;
    return;
  }
  if (attribute == "media_position_updated_at") {
    source->clock.timestamp_value(value, transport_.epoch_seconds(), transport_.now_ms());
    ++revision_;
    return;
  }
  const auto bit = attribute_bit(attribute);
  if (!bit) return;
  char decoded[HA_STATE_TEXT_MAX_LEN + 1];
  const size_t capacity = attribute == "media_content_type" ? HA_SHORT_STATE_MAX_LEN + 1 : sizeof(decoded);
  auto metadata = decode_html_entities_bounded(decoded, capacity, value);
  if (metadata == "unknown" || metadata == "unavailable") metadata = {};
  const bool present = !metadata.empty();
  if (source->requirements & REQUIRE_DETAILS) {
    bool invalidate_external = false;
    if (attribute == "source") {
      char old_decoded[HA_STATE_TEXT_MAX_LEN + 1];
      auto old_source = decode_html_entities_bounded(old_decoded, sizeof(old_decoded), previous);
      if (old_source == "unknown" || old_source == "unavailable") old_source = {};
      invalidate_external = source->input.source(metadata, old_source);
    } else if (attribute == "media_content_id") {
      invalidate_external = source->input.content(metadata);
    } else if (attribute == "media_title") {
      source->input.current_content(present);
    }
    if (invalidate_external && (source->requirements & REQUIRE_PRIMARY_COVER)) {
      constexpr auto cleared = TITLE_BIT | ARTIST_BIT | (1u << 5) | (1u << 6);
      source->fresh &= ~cleared;
      source->present &= ~cleared;
      const bool playing = source->clock.playing, known = source->clock.state_known;
      source->clock = {};
      source->clock.playing = playing;
      source->clock.state_known = known;
    }
    if (attribute == "media_title") {
      char old_decoded[HA_STATE_TEXT_MAX_LEN + 1];
      auto old_title = (source->fresh & TITLE_BIT)
                           ? decode_html_entities_bounded(old_decoded, sizeof(old_decoded), previous)
                           : std::string_view{};
      if (old_title == "unknown" || old_title == "unavailable") old_title = {};
      const bool pending = media::media_title_refresh_pending(
          source->title_awaiting_refresh, source->title_refresh_started_ms, transport_.now_ms());
      if (metadata != old_title && !pending) {
        source->fresh &= ~ARTIST_BIT;
        source->present &= ~ARTIST_BIT;
        source->metadata_refresh = true;
      }
      source->title_awaiting_refresh = false;
    } else if (attribute == "media_content_type") {
      source->type_kind = media::media_item_kind_from_token_view(metadata);
      if (source->type_kind == media::MediaItemKind::VIDEO) {
        source->fresh &= ~ARTIST_BIT;
        source->present &= ~ARTIST_BIT;
      }
    } else if (attribute == "media_content_id") {
      const auto fingerprint =
          present ? media::media_content_identity_fingerprint(value.data(), value.size()) : 0;
      auto kind = media::media_item_kind_view(metadata);
      if (kind == media::MediaItemKind::UNKNOWN) kind = source->type_kind;
      const auto decision = media::media_metadata_clear_decision(source->content_fingerprint,
                                                                 source->content_kind, fingerprint, kind);
      if (present) {
        source->content_fingerprint = fingerprint;
        source->content_kind = kind;
      }
      if (decision.item_changed) {
        source->fresh &= ~(TITLE_BIT | ARTIST_BIT);
        source->present &= ~(TITLE_BIT | ARTIST_BIT);
        source->title_awaiting_refresh = true;
        source->title_refresh_started_ms = transport_.now_ms();
        source->metadata_refresh = true;
      }
    }
  }
  source->fresh |= bit;
  if (present)
    source->present |= bit;
  else
    source->present &= ~bit;
  ++revision_;
}
const PageMediaStateRuntime::Source* PageMediaStateRuntime::source(ConfigurationHandle handle,
                                                                   std::string_view entity) const {
  if (active_ < 0 || slots_[active_].generation != handle.generation ||
      handle.index >= slots_[active_].card_count || !transport_.connected() ||
      connection_ != transport_.connection_generation())
    return nullptr;
  const auto& slot = slots_[active_];
  for (size_t i = 0; i < slot.count; ++i)
    if (slot.sources[i].entity == entity) return slot.sources + i;
  return nullptr;
}
bool PageMediaStateRuntime::current(ConfigurationHandle handle, std::string_view entity,
                                    std::string_view attribute) const {
  const auto* item = source(handle, entity);
  return item && (item->fresh & attribute_bit(attribute));
}
bool PageMediaStateRuntime::external_input(ConfigurationHandle handle, std::string_view entity) const {
  const auto* item = source(handle, entity);
  return item && item->input.external;
}
bool PageMediaStateRuntime::playback(ConfigurationHandle handle, std::string_view entity,
                                     PageMediaPlaybackSnapshot& output) const {
  const auto* item = source(handle, entity);
  if (!item || !(item->requirements & REQUIRE_PROGRESS)) return false;
  const auto now = transport_.now_ms();
  const auto& clock = item->clock;
  const bool available = item->state_known && item->state[0] && std::strcmp(item->state, "unknown") != 0 &&
                         std::strcmp(item->state, "unavailable") != 0;
  output = {clock.seconds(now), clock.duration,     clock.percent(now), available,
            clock.playing,      clock.has_duration, clock.has_position};
  return true;
}
void PageMediaStateRuntime::loop() {
  synchronize();
  if (active_ < 0 || !transport_.connected()) return;
  bool reannounce = false, ticking = false;
  auto& slot = slots_[active_];
  for (size_t i = 0; i < slot.count; ++i) {
    auto& source = slot.sources[i];
    ticking |= (source.requirements & REQUIRE_PROGRESS) && source.clock.playing && source.clock.has_position;
    reannounce |= (source.requirements & REQUIRE_METADATA) && source.resync && usable(source.state) &&
                  !(source.present & CONTENT_BITS);
    reannounce |= (source.requirements & REQUIRE_DETAILS) && source.metadata_refresh &&
                  (!source.state_known || usable(source.state));
    source.resync = source.metadata_refresh = false;
  }
  const auto now = transport_.now_ms();
  if (!ticking)
    tick_ms_ = now;
  else if (static_cast<uint32_t>(now - tick_ms_) >= 1000) {
    tick_ms_ = now;
    ++revision_;
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
