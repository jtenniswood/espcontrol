#include "page_local_relay_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "button_grid_card_runtime.h"

namespace espcontrol::pages {
namespace {
template <typename Emit>
bool unique_keys(const PageStore& store, Emit emit) {
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->runtime_card_count; ++id) {
      const auto& card = *store.card(id);
      if (!PageLocalRelayRuntime::toggle(store, card)) continue;
      const auto key = store.field(card, CardField::ENTITY);
      if (key.empty()) continue;
      bool found = false;
      for (PageId before = 0; before < store.page_count() && !found; ++before) {
        const auto* earlier = store.page(before);
        if (!earlier->navigable) continue;
        for (CardId previous = earlier->first_card;
             previous < earlier->first_card + earlier->runtime_card_count && previous < id; ++previous)
          if (PageLocalRelayRuntime::toggle(store, *store.card(previous)) &&
              store.field(*store.card(previous), CardField::ENTITY) == key) {
            found = true;
            break;
          }
      }
      if (!found && !emit(key)) return false;
    }
  }
  return true;
}
}  // namespace
PageLocalRelayRuntime::PageLocalRelayRuntime(PageArenaAllocator& allocator, PageLocalRelayProvider& provider,
                                             size_t sources, size_t names)
    : allocator_(allocator), provider_(provider), max_sources_(sources), max_key_bytes_(names) {}
PageLocalRelayRuntime::~PageLocalRelayRuntime() {
  if (page_local_relay_feed().owner == this) page_local_relay_feed() = {};
  discard_prepared();
  retire_active();
}
bool PageLocalRelayRuntime::toggle(const PageStore& store, const CardDefinition& card) {
  return store.field(card, CardField::TYPE) == "internal" &&
         !card_runtime_internal_push_mode(std::string(store.field(card, CardField::SENSOR)));
}
bool PageLocalRelayRuntime::prepare(const PageStore& store) {
  discard_prepared();
  if(!store.source_document()){++failures_;return false;}
  size_t count = 0, names = 0;
  const auto fail = [&]() {
    ++failures_;
    discard_prepared();
    return false;
  };
  if (!unique_keys(store,
                   [&](std::string_view key) {
                     if (count >= max_sources_ || key.size() > UINT16_MAX || names > max_key_bytes_ ||
                         key.size() >= max_key_bytes_ - names || names > UINT32_MAX ||
                         key.size() >= UINT32_MAX - names)
                       return false;
                     ++count;
                     names += key.size() + 1;
                     return true;
                   }) ||
      count > (SIZE_MAX - names) / sizeof(Target))
    return fail();
  staged_.generation = store.generation();
  staged_.valid = true;
  if (!count) return true;
  const size_t bytes = count * sizeof(Target) + names;
  void* arena = allocator_.allocate(bytes);
  if (!arena) return fail();
  staged_.arena = arena;
  staged_.bytes = bytes;
  staged_.targets = static_cast<Target*>(arena);
  staged_.names = static_cast<char*>(arena) + count * sizeof(Target);
  peak_ = std::max(peak_, active_.bytes + staged_.bytes);
  size_t used = 0;
  if (!unique_keys(store, [&](std::string_view key) {
        if (staged_.count >= count || key.size() + 1 > names - used) return false;
        auto& target = *new (staged_.targets + staged_.count++) Target{
            static_cast<uint32_t>(used), static_cast<uint16_t>(key.size()), provider_.read(key).on};
        (void)target;
        std::memcpy(staged_.names + used, key.data(), key.size());
        used += key.size();
        staged_.names[used++] = '\0';
        return true;
      }))
    return fail();
  return true;
}
void PageLocalRelayRuntime::clear(Slot& slot) {
  if (slot.arena) allocator_.release(slot.arena, slot.bytes);
  slot = {};
}
void PageLocalRelayRuntime::discard_prepared() {
  clear(staged_);
}
void PageLocalRelayRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.valid || !page_prepared_generation_matches(staged_.generation, store.generation())) return;
  // Fixed registry reads refresh the staged snapshot without allocations/actions.
  for (size_t i = 0; i < staged_.count; ++i) {
    auto& target = staged_.targets[i];
    target.on = provider_.read({staged_.names + target.offset, target.length}).on;
  }
  clear(active_);
  active_ = staged_;
  active_.generation = store.generation();
  staged_ = {};
  ++revision_;
}
void PageLocalRelayRuntime::retire_active() {
  if (active_.valid) ++revision_;
  clear(active_);
}
bool PageLocalRelayRuntime::install(bool retired) {
  auto& feed = page_local_relay_feed();
  if (!retired || (feed.owner && feed.owner != this)) return false;
  feed = {this, [](void* owner, std::string_view key, bool on) {
            static_cast<PageLocalRelayRuntime*>(owner)->changed(key, on);
          }};
  return true;
}
void PageLocalRelayRuntime::changed(std::string_view key, bool on) {
  bool changed = false;
  for (auto* slot : {&active_, &staged_})
    for (size_t i = 0; i < slot->count; ++i) {
      auto& target = slot->targets[i];
      if (std::string_view(slot->names + target.offset, target.length) == key && target.on != on) {
        target.on = on;
        changed = true;
      }
    }
  if (changed) ++revision_;
}
bool PageLocalRelayRuntime::on(uint32_t generation, std::string_view key) const {
  if (!active_.valid || generation != active_.generation) return false;
  for (size_t i = 0; i < active_.count; ++i) {
    const auto& target = active_.targets[i];
    if (std::string_view(active_.names + target.offset, target.length) == key) return target.on;
  }
  return false;
}
PageLocalRelayMemory PageLocalRelayRuntime::memory() const {
  return {sizeof(*this), sizeof(PageLocalRelayFeed), active_.bytes, staged_.bytes, peak_, active_.count,
          failures_};
}
}  // namespace espcontrol::pages
