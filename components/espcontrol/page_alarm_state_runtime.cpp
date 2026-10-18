#include "page_alarm_state_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "bounded_status_text.h"

namespace espcontrol::pages {
namespace {
std::string_view inherited_field(const PageStore& store, CardId id, CardField field) {
  const auto* card = store.card(id);
  if (!card) return {};
  auto value = store.field(*card, field);
  if (!value.empty()) return value;
  for (PageId p = 1; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (id >= page->first_card && id < page->first_card + page->card_count) {
      const auto* parent = store.card(page->owner);
      return parent ? store.field(*parent, field) : std::string_view{};
    }
  }
  return {};
}
bool navigable(const PageStore& store, CardId id) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (id >= page->first_card && id < page->first_card + page->card_count) return page->navigable;
  }
  return false;
}
template <typename Emit>
bool keys(const PageStore& store, Emit emit) {
  for (CardId id = 0; id < store.card_count(); ++id) {
    if (!navigable(store, id)) continue;
    const auto kind = PageAlarmStateRuntime::classify(store, id);
    if (kind == PageAlarmSourceKind::NONE) continue;
    const auto key = PageAlarmStateRuntime::effective_entity(store, id);
    bool duplicate = false;
    for (CardId previous = 0; previous < id; ++previous)
      if (navigable(store, previous) && PageAlarmStateRuntime::classify(store, previous) == kind &&
          PageAlarmStateRuntime::effective_entity(store, previous) == key) {
        duplicate = true;
        break;
      }
    if (!duplicate && !emit(key, kind)) return false;
  }
  return true;
}
}  // namespace
PageAlarmSourceKind PageAlarmStateRuntime::classify(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  if (!card || effective_entity(store, id).empty()) return PageAlarmSourceKind::NONE;
  const auto type = store.field(*card, CardField::TYPE);
  return type == "alarm"          ? PageAlarmSourceKind::CONTROL
         : type == "alarm_action" ? PageAlarmSourceKind::ACTION
                                  : PageAlarmSourceKind::NONE;
}
std::string_view PageAlarmStateRuntime::effective_entity(const PageStore& store, CardId id) {
  return inherited_field(store, id, CardField::ENTITY);
}
std::string_view PageAlarmStateRuntime::effective_options(const PageStore& store, CardId id) {
  return inherited_field(store, id, CardField::OPTIONS);
}
PageAlarmStateRuntime::PageAlarmStateRuntime(PageArenaAllocator& allocator,
                                             PageEntitySubscriptions& subscriptions, Now now, size_t sources,
                                             size_t names, Translate translate)
    : allocator_(allocator),
      subscriptions_(subscriptions),
      now_(now),
      max_sources_(sources),
      max_key_bytes_(names),
      translate_(translate),
      connection_(subscriptions.connection_generation()) {}
PageAlarmStateRuntime::~PageAlarmStateRuntime() {
  discard_prepared();
  retire_active();
}
void PageAlarmStateRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PageAlarmStateRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t count = 0, names = 0;
  if (!now_ || !store.source_document() ||
      !keys(store,
            [&](std::string_view key, PageAlarmSourceKind) {
              if (count >= max_sources_ || count >= UINT16_MAX || names >= max_key_bytes_ ||
                  key.size() >= max_key_bytes_ - names)
                return false;
              ++count;
              names += key.size() + 1;
              return true;
            }) ||
      store.card_count() > (SIZE_MAX - names) / sizeof(uint16_t) ||
      count > (SIZE_MAX - names - store.card_count() * sizeof(uint16_t)) / sizeof(Source)) {
    ++failures_;
    return false;
  }
  const size_t map_bytes = count ? store.card_count() * sizeof(uint16_t) : 0;
  const size_t bytes = count * sizeof(Source) + map_bytes + names;
  auto* memory = bytes ? allocator_.allocate(bytes) : nullptr;
  if (bytes && !memory) {
    ++failures_;
    return false;
  }
  staged_.arena = memory;
  staged_.bytes = bytes;
  staged_.store = store;
  staged_.sources = static_cast<Source*>(memory);
  staged_.card_source =
      memory ? reinterpret_cast<uint16_t*>(static_cast<char*>(memory) + count * sizeof(Source)) : nullptr;
  if (count) std::fill_n(staged_.card_source, store.card_count(), UINT16_MAX);
  auto* output = memory ? static_cast<char*>(memory) + count * sizeof(Source) + map_bytes : nullptr;
  keys(store, [&](std::string_view key, PageAlarmSourceKind kind) {
    const auto index = staged_.count++;
    auto& source = *new (staged_.sources + index) Source{};
    for (size_t old = 0; old < active_.count; ++old)
      if (active_.sources[old].entity == key && active_.sources[old].kind == kind) {
        source = active_.sources[old];
        source.inherited = true;
        if (active_.selected == old) staged_.selected = static_cast<uint16_t>(index);
        break;
      }
    if (!key.empty()) std::memcpy(output, key.data(), key.size());
    output[key.size()] = '\0';
    source.entity = {output, key.size()};
    source.kind = kind;
    source.origin = NO_CARD;
    if (!source.reported && !source.invalidated) source.available = kind == PageAlarmSourceKind::ACTION;
    output += key.size() + 1;
    return true;
  });
  if (count)
    for (CardId id = 0; id < store.card_count(); ++id) {
      if (!navigable(store, id)) continue;
      const auto kind = classify(store, id);
      if (kind == PageAlarmSourceKind::NONE) continue;
      const auto key = effective_entity(store, id);
      for (size_t i = 0; i < staged_.count; ++i)
        if (staged_.sources[i].kind == kind && staged_.sources[i].entity == key) {
          staged_.card_source[id] = static_cast<uint16_t>(i);
          staged_.sources[i].origin = id;
          break;
        }
    }
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PageAlarmStateRuntime::discard_prepared() {
  clear(staged_);
}
void PageAlarmStateRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.store.source_document() ||
      !page_prepared_generation_matches(staged_.store.generation(), store.generation()))
    return;
  clear(active_);
  active_ = staged_;
  active_.store = store;
  staged_ = {};
  ++revision_;
}
void PageAlarmStateRuntime::retire_active() {
  if (active_.store.source_document()) ++revision_;
  clear(active_);
}
bool PageAlarmStateRuntime::eligible(const Source& source) const {
  return source.available &&
         (state(source) == "triggered" ||
          (alarm::delay(state(source)) && alarm::remaining(source.seconds, source.started, now_()) > 0));
}
void PageAlarmStateRuntime::status(Source& source) {
  char next[97]{};
  const char* literal = alarm::state_label(state(source));
  if (literal && translate_) {
    const auto* translated = translate_(literal);
    if (translated) literal = translated;
  }
  const size_t size = literal ? 0 : format_status_text(next, sizeof(next), state(source), false);
  const std::string_view incoming = literal ? std::string_view(literal) : std::string_view(next, size);
  if (source.status_known && caption(source) == incoming) return;
  source.literal = literal;
  source.caption_size = static_cast<uint8_t>(size);
  if (!literal) std::memcpy(source.caption, next, sizeof(next));
  const bool revert =
      alarm::releases_label(state(source)) && source.available && state(source) != "triggered";
  const bool initial = !source.status_known;
  source.status_known = true;
  source.revert = revert;
  source.changed = now_();
  source.showing = !(initial && revert);
}
void PageAlarmStateRuntime::update(Source& source, std::string_view attribute, std::string_view raw,
                                   bool copied) {
  raw = raw.substr(0, 32);
  if (attribute.empty()) {
    const bool changed_delay = alarm::delay(raw) && state(source) != raw;
    if (source.kind == PageAlarmSourceKind::CONTROL) {
      if (changed_delay) {
        source.started = now_();
        source.total = source.seconds > 0 ? source.seconds : -1;
      }
      if (!alarm::delay(raw)) source.total = -1;
    }
    if (!raw.empty()) std::memcpy(source.state, raw.data(), raw.size());
    source.state[raw.size()] = '\0';
    source.state_size = raw.size();
    source.reported = true;
    source.available = alarm::available(raw);
    source.fresh = true;
    source.invalidated = false;
    if (source.kind == PageAlarmSourceKind::CONTROL) status(source);
  } else if (attribute == "arm_mode") {
    source.mode_reported = true;
    if (!raw.empty()) std::memcpy(source.arm_mode, raw.data(), raw.size());
    source.arm_mode[raw.size()] = '\0';
    source.mode_size = raw.size();
  } else if (attribute == "delay") {
    const int seconds = alarm::parse_delay(raw);
    if (copied && source.inherited && seconds == source.seconds) return;
    source.seconds = seconds;
    if (seconds >= 0 && seconds > source.total)
      source.total = seconds;
    else if (seconds < 0)
      source.total = -1;
    source.started = now_();
  }
}
void PageAlarmStateRuntime::observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                                     std::string_view value, bool copied) {
  if (!attribute.empty() && attribute != "arm_mode" && attribute != "delay") return;
  loop();
  for (auto* slot : {&active_, &staged_}) {
    if (!slot->store.source_document() || slot->store.generation() != generation) continue;
    uint16_t winner = UINT16_MAX;
    CardId origin = 0;
    bool changed = false;
    for (size_t i = 0; i < slot->count; ++i) {
      auto& source = slot->sources[i];
      if (source.entity != entity) continue;
      update(source, attribute, value, copied);
      changed = true;
      if (attribute == "arm_mode" && source.kind == PageAlarmSourceKind::CONTROL) continue;
      if (eligible(source)) {
        if (winner == UINT16_MAX || source.origin >= origin) {
          winner = static_cast<uint16_t>(i);
          origin = source.origin;
        }
      } else if (slot->selected == i)
        slot->selected = UINT16_MAX;
    }
    if (winner != UINT16_MAX) {
      slot->selected = winner;
      if (slot == &active_ && !copied) ++takeover_events_;
    }
    if (changed) ++revision_;
  }
}
void PageAlarmStateRuntime::loop() {
  if (!now_) return;
  const auto connection = subscriptions_.connection_generation();
  const bool disconnected = connection != connection_;
  connection_ = connection;
  for (auto* slot : {&active_, &staged_}) {
    for (size_t i = 0; i < slot->count; ++i) {
      auto& source = slot->sources[i];
      if (disconnected) {
        source.fresh = false;
        source.invalidated = true;
        ++revision_;
      }
      const int left =
          alarm::delay(state(source)) ? alarm::remaining(source.seconds, source.started, now_()) : -1;
      if (left != source.last_remaining) {
        source.last_remaining = left;
        ++revision_;
      }
      if (source.showing && source.revert && static_cast<uint32_t>(now_() - source.changed) >= 3000) {
        source.showing = false;
        ++revision_;
      }
    }
    if (slot->selected != UINT16_MAX && !eligible(slot->sources[slot->selected])) {
      slot->selected = UINT16_MAX;
      ++revision_;
    }
  }
}
PageAlarmSnapshot PageAlarmStateRuntime::read(ConfigurationHandle handle) const {
  PageAlarmSnapshot result;
  if (!active_.store.source_document() || handle.generation != active_.store.generation() ||
      handle.index >= active_.store.card_count() || !active_.card_source)
    return result;
  const auto index = active_.card_source[handle.index];
  if (index == UINT16_MAX) return result;
  const auto& source = active_.sources[index];
  result.valid = true;
  result.reported = source.reported;
  result.icon_reported = source.reported || source.mode_reported;
  result.available = source.available;
  result.command_available =
      source.available && (source.fresh || (!source.reported && !source.invalidated &&
                                            source.kind == PageAlarmSourceKind::ACTION));
  result.status_known = source.status_known;
  result.status_showing = source.showing;
  result.status = caption(source);
  result.entity = source.entity;
  result.state = state(source);
  result.arm_mode = mode(source);
  result.active = alarm::active(result.state) || result.state == "triggered";
  result.triggered = result.state == "triggered";
  result.remaining = alarm::remaining(source.seconds, source.started, now_());
  result.progress = alarm::progress(result.remaining, source.total, source.seconds);
  return result;
}
PageAlarmTakeover PageAlarmStateRuntime::takeover() const {
  if (!active_.store.source_document() || active_.selected == UINT16_MAX) return {};
  const auto& source = active_.sources[active_.selected];
  if (!eligible(source)) return {};
  const int left = alarm::remaining(source.seconds, source.started, now_());
  return {true,          {active_.store.generation(), source.origin},
          source.entity, state(source) == "triggered",
          left,          alarm::progress(left, source.total, source.seconds)};
}
PageAlarmMemory PageAlarmStateRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.count, staged_.count, failures_};
}
}  // namespace espcontrol::pages
