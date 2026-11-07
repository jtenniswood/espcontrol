#include "page_access_state_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "bounded_status_text.h"
#include "button_grid_subpage_parser.h"
#include "ha_state_limits.h"
#include "page_fan_mode.h"
namespace espcontrol::pages {
namespace {
template <typename Emit>
bool keys(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->runtime_card_count; ++id) {
      const auto policy = PageAccessStateRuntime::classify(store, id);
      if (policy == PageAccessHistory::NONE) continue;
      const auto key = store.field(*store.card(id), CardField::ENTITY);
      bool duplicate = false;
      for (PageId before = 0; before <= p && !duplicate; ++before) {
        const auto* earlier = store.page(before);
        if (!earlier->navigable) continue;
        for (CardId previous = earlier->first_card;
             previous < earlier->first_card + earlier->runtime_card_count && previous < id; ++previous)
          if (PageAccessStateRuntime::classify(store, previous) != PageAccessHistory::NONE &&
              store.field(*store.card(previous), CardField::ENTITY) == key &&
              PageAccessStateRuntime::classify(store, previous) ==
                  PageAccessStateRuntime::classify(store, id)) {
            duplicate = true;
            break;
          }
      }
      if (!duplicate && !emit(key, policy)) return false;
    }
  }
  return true;
}

}  // namespace
PageAccessHistory PageAccessStateRuntime::classify(const PageStore& store, CardId id) {
  const auto* card = store.runtime_card(id);
  if (!card || store.field(*card, CardField::ENTITY).empty()) return PageAccessHistory::NONE;
  const auto type = store.field(*card, CardField::TYPE);
  if (type != "lock" && type != "garage" && type != "gate") return PageAccessHistory::NONE;
  const std::string mode(store.field(*card, CardField::SENSOR));
  if (type == "lock")
    return card_runtime_lock_command_mode(mode) ? PageAccessHistory::NONE : PageAccessHistory::LOCK;
  if (type != "garage" && type != "gate") return PageAccessHistory::NONE;
  const std::string options(store.field(*card, CardField::OPTIONS));
  const auto label = cfg_option_value(options, "label_display");
  const bool persistent = type == "garage" ? normalize_garage_label_display(label) == "status"
                                           : normalize_gate_label_display(label) == "status";
  const bool command =
      type == "garage" ? card_runtime_garage_command_mode(mode) : card_runtime_gate_command_mode(mode);
  return persistent ? PageAccessHistory::PERSISTENT
         : command  ? PageAccessHistory::NONE
                    : PageAccessHistory::TRANSIENT;
}
PageAccessStateRuntime::PageAccessStateRuntime(PageArenaAllocator& allocator, PageEntitySubscriptions& subs,
                                               Now now, size_t histories, size_t names,
                                               PageStatusCaptions captions)
    : allocator_(allocator),
      subscriptions_(subs),
      now_(now),
      max_histories_(histories),
      max_key_bytes_(names),
      captions_(captions),
      connection_(subs.connection_generation()) {}
PageAccessStateRuntime::~PageAccessStateRuntime() {
  discard_prepared();
  retire_active();
}
void PageAccessStateRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
bool PageAccessStateRuntime::prepare(const PageStore& store) {
  discard_prepared();
  loop();
  size_t count = 0, names = 0;
  if (!store.source_document() || !now_ ||
      (static_cast<bool>(captions_.text) != static_cast<bool>(captions_.key)) ||
      !keys(store,
            [&](std::string_view key, PageAccessHistory) {
              if (count >= max_histories_ || count >= UINT16_MAX || names > max_key_bytes_ ||
                  key.size() >= max_key_bytes_ - names)
                return false;
              ++count;
              names += key.size() + 1;
              return true;
            }) ||
      store.card_count() > (SIZE_MAX - names) / sizeof(uint16_t) ||
      count > (SIZE_MAX - names - store.card_count() * sizeof(uint16_t)) / sizeof(History)) {
    ++failures_;
    return false;
  }
  const size_t map_bytes = count ? store.card_count() * sizeof(uint16_t) : 0;
  const size_t bytes = count * sizeof(History) + map_bytes + names;
  auto* arena = bytes ? allocator_.allocate(bytes) : nullptr;
  if (bytes && !arena) {
    ++failures_;
    return false;
  }
  staged_.arena = arena;
  staged_.bytes = bytes;
  staged_.histories = static_cast<History*>(arena);
  staged_.store = store;
  staged_.card_history =
      arena ? reinterpret_cast<uint16_t*>(static_cast<char*>(arena) + count * sizeof(History)) : nullptr;
  if (count) std::fill_n(staged_.card_history, store.card_count(), UINT16_MAX);
  char* output = arena ? static_cast<char*>(arena) + count * sizeof(History) + map_bytes : nullptr;
  keys(store, [&](std::string_view key, PageAccessHistory policy) {
    auto& source = *new (staged_.histories + staged_.count++) History{};
    if (!key.empty()) std::memcpy(output, key.data(), key.size());
    output[key.size()] = '\0';
    source.entity = {output, key.size()};
    source.policy = policy;
    output += key.size() + 1;
    return true;
  });
  if (count)
    for (PageId p = 0; p < store.page_count(); ++p) {
      const auto* page = store.page(p);
      if (!page->navigable) continue;
      for (CardId id = page->first_card; id < page->first_card + page->runtime_card_count; ++id) {
        const auto policy = classify(store, id);
        if (policy == PageAccessHistory::NONE) continue;
        const auto entity = store.field(*store.card(id), CardField::ENTITY);
        for (size_t index = 0; index < staged_.count; ++index)
          if (staged_.histories[index].policy == policy && staged_.histories[index].entity == entity) {
            staged_.card_history[id] = static_cast<uint16_t>(index);
            break;
          }
      }
    }
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  return true;
}
void PageAccessStateRuntime::discard_prepared() {
  clear(staged_);
}
void PageAccessStateRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.store.source_document() ||
      !page_prepared_generation_matches(staged_.store.generation(), store.generation()))
    return;
  clear(active_);
  active_ = staged_;
  active_.store = store;
  staged_ = {};
  ++revision_;
}
void PageAccessStateRuntime::retire_active() {
  if (active_.store.source_document()) ++revision_;
  clear(active_);
}

void PageAccessStateRuntime::update(History& source, std::string_view raw) {
  raw = raw.substr(0, HA_SHORT_STATE_MAX_LEN);
  const bool lock = source.policy == PageAccessHistory::LOCK;
  const bool on =
      lock ? (raw == "unlocked" || raw == "unlocking" || raw == "open" || raw == "opening" || raw == "jammed")
           : (raw == "open" || raw == "opening" || raw == "closing");
  const bool icon_on = lock ? on : (raw == "open" || raw == "opening");
  const bool available =
      !fan_mode_trim(raw).empty() && !fan_mode_equal(raw, "unknown") && !fan_mode_equal(raw, "unavailable");
  const bool locked = raw == "locked";
  if (source.on != on || source.icon_on != icon_on || source.available != available ||
      source.locked != locked) {
    source.on = on;
    source.icon_on = icon_on;
    source.available = available;
    source.locked = locked;
    ++revision_;
  }
  char caption[97]{};
  const char* translated = nullptr;
  if (raw.empty())
    translated = "--";
  else if (!lock && raw == "open")
    translated = captions_.key ? captions_.key("state_open") : "Open";
  else {
    format_status_text(caption, sizeof(caption), raw);
    translated = !lock && captions_.text ? captions_.text(caption) : caption;
  }
  if (!translated) translated = !lock && raw == "open" ? "Open" : caption;
  if (source.has_status && std::strcmp(text(source), translated) == 0) return;
  if (translated == caption) {
    std::memcpy(source.caption, caption, std::strlen(caption) + 1);
    source.literal = nullptr;
  } else
    source.literal = translated;
  const bool release =
      source.policy != PageAccessHistory::PERSISTENT &&
      (lock ? (raw == "locked" || raw == "unlocked" || raw == "open") : (raw == "open" || raw == "closed"));
  const bool first = !source.has_status;
  source.has_status = true;
  source.showing = !first || !release;
  source.revert = source.showing && release;
  source.changed_ms = now_();
  ++revision_;
}
void PageAccessStateRuntime::observed(uint32_t generation, std::string_view entity,
                                      std::string_view attribute, std::string_view value, bool) {
  loop();
  if (!attribute.empty()) return;
  for (auto* slot : {&active_, &staged_}) {
    if (!slot->store.source_document() || slot->store.generation() != generation) continue;
    for (size_t i = 0; i < slot->count; ++i)
      if (slot->histories[i].entity == entity) update(slot->histories[i], value);
  }
}
void PageAccessStateRuntime::loop() {
  if (!now_) return;
  const auto connection = subscriptions_.connection_generation();
  if (connection_ != connection) {
    connection_ = connection;
    for (auto* slot : {&active_, &staged_})
      for (size_t i = 0; i < slot->count; ++i) update(slot->histories[i], {});
    ++revision_;
  }
  for (auto* slot : {&active_, &staged_})
    for (size_t i = 0; i < slot->count; ++i) {
      auto& source = slot->histories[i];
      if (source.showing && source.revert && static_cast<uint32_t>(now_() - source.changed_ms) >= 3000) {
        source.showing = source.revert = false;
        ++revision_;
      }
    }
}
PageAccessSnapshot PageAccessStateRuntime::read(ConfigurationHandle handle) const {
  if (handle.generation != active_.store.generation() || !active_.card_history ||
      handle.index >= active_.store.card_count())
    return {};
  const auto index = active_.card_history[handle.index];
  if (index == UINT16_MAX || index >= active_.count) return {};
  const auto& source = active_.histories[index];
  return {source.on,      source.icon_on,    source.available, source.locked,
          source.showing, source.has_status, text(source)};
}
PageAccessMemory PageAccessStateRuntime::memory() const {
  return {sizeof(*this), active_.bytes, staged_.bytes, peak_, active_.count, failures_};
}
}  // namespace espcontrol::pages
