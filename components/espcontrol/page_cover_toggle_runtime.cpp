#include "page_cover_toggle_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "bounded_status_text.h"
#include "ha_state_limits.h"
namespace espcontrol::pages {
namespace {
template <typename Emit>
bool keys(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->card_count; ++id) {
      if (!PageCoverToggleRuntime::supported(store, id)) continue;
      const auto key = store.field(*store.card(id), CardField::ENTITY);
      bool duplicate = false;
      for (PageId before = 0; before <= p && !duplicate; ++before) {
        const auto* earlier = store.page(before);
        if (!earlier->navigable) continue;
        for (CardId previous = earlier->first_card;
             previous < earlier->first_card + earlier->card_count && previous < id; ++previous)
          if (PageCoverToggleRuntime::supported(store, previous) &&
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

}  // namespace
bool PageCoverToggleRuntime::supported(const PageStore& store, CardId id) {
  const auto* card = store.card(id);
  return card && store.field(*card, CardField::TYPE) == "cover" &&
         store.field(*card, CardField::SENSOR) == "toggle" && !store.field(*card, CardField::ENTITY).empty();
}
PageCoverToggleRuntime::PageCoverToggleRuntime(PageArenaAllocator& allocator, PageEntitySubscriptions& subs,
                                               Now now, size_t sources, size_t names,
                                               PageCoverStatusCaptions captions)
    : allocator_(allocator),
      subscriptions_(subs),
      now_(now),
      max_sources_(sources),
      max_key_bytes_(names),
      captions_(captions),
      connection_(subs.connection_generation()) {}
PageCoverToggleRuntime::~PageCoverToggleRuntime() {
  discard_prepared();
  retire_active();
}
void PageCoverToggleRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PageCoverToggleRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t count = 0, names = 0;
  if (!store.source_document() || !now_ ||
      (static_cast<bool>(captions_.text) != static_cast<bool>(captions_.key)) ||
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
void PageCoverToggleRuntime::discard_prepared() {
  clear(staged_);
}
void PageCoverToggleRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.store.source_document() ||
      !page_prepared_generation_matches(staged_.store.generation(), store.generation()))
    return;
  clear(active_);
  active_ = staged_;
  active_.store = store;
  staged_ = {};
  ++revision_;
}
void PageCoverToggleRuntime::retire_active() {
  if (active_.store.source_document()) ++revision_;
  clear(active_);
}

void PageCoverToggleRuntime::update(Source& source, std::string_view raw) {
  raw = raw.substr(0, HA_SHORT_STATE_MAX_LEN);
  const bool on = raw == "closed" || raw == "closing", icon_open = raw == "open" || raw == "opening";
  if (source.on != on || source.icon_open != icon_open) {
    source.on = on;
    source.icon_open = icon_open;
    ++revision_;
  }
  char caption[97]{};
  const char* translated = nullptr;
  if (raw.empty())
    translated = "--";
  else if (raw == "open")
    translated = captions_.key ? captions_.key("state_open") : "Open";
  else {
    format_status_text(caption, sizeof(caption), raw);
    translated = captions_.text ? captions_.text(caption) : caption;
  }
  if (!translated) translated = raw == "open" ? "Open" : caption;
  if (source.has_status && std::strcmp(text(source), translated) == 0) return;
  if (translated == caption) {
    std::memcpy(source.caption, caption, std::strlen(caption) + 1);
    source.literal = nullptr;
  } else
    source.literal = translated;
  const bool release = raw == "open" || raw == "closed";
  const bool first = !source.has_status;
  source.has_status = true;
  source.showing = !first || !release;
  source.revert = source.showing && release;
  source.changed_ms = now_();
  ++revision_;
}
void PageCoverToggleRuntime::observed(uint32_t generation, std::string_view entity,
                                      std::string_view attribute, std::string_view value, bool) {
  loop();
  if (!attribute.empty()) return;
  for (auto* slot : {&active_, &staged_}) {
    if (!slot->store.source_document() || slot->store.generation() != generation) continue;
    for (size_t i = 0; i < slot->count; ++i)
      if (slot->sources[i].entity == entity) update(slot->sources[i], value);
  }
}
void PageCoverToggleRuntime::loop() {
  if (!now_) return;
  const auto connection = subscriptions_.connection_generation();
  if (connection_ != connection) {
    connection_ = connection;
    for (auto* slot : {&active_, &staged_})
      for (size_t i = 0; i < slot->count; ++i) update(slot->sources[i], {});
    ++revision_;
  }
  for (auto* slot : {&active_, &staged_})
    for (size_t i = 0; i < slot->count; ++i) {
      auto& source = slot->sources[i];
      if (source.showing && source.revert && static_cast<uint32_t>(now_() - source.changed_ms) >= 3000) {
        source.showing = source.revert = false;
        ++revision_;
      }
    }
}
PageCoverToggleSnapshot PageCoverToggleRuntime::read(ConfigurationHandle handle) const {
  if (handle.generation != active_.store.generation() || !supported(active_.store, handle.index)) return {};
  const auto key = active_.store.field(*active_.store.card(handle.index), CardField::ENTITY);
  for (size_t i = 0; i < active_.count; ++i) {
    const auto& source = active_.sources[i];
    if (source.entity == key) return {source.on, source.icon_open, source.showing, text(source)};
  }
  return {};
}
PageCoverToggleMemory PageCoverToggleRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.count, failures_};
}
}  // namespace espcontrol::pages
