#include "page_entity_state_runtime.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

namespace espcontrol::pages {
namespace {
bool add_size(size_t &total, size_t count, size_t unit = 1) {
  if (unit != 0 && count > (std::numeric_limits<size_t>::max() - total) / unit) return false;
  total += count * unit;
  return true;
}
bool align_size(size_t &total, size_t alignment) {
  const size_t padding = (alignment - total % alignment) % alignment;
  return add_size(total, padding);
}
bool valid_name(std::string_view text) { return text.find('\0') == std::string_view::npos; }
}  // namespace

class PageEntityStateRuntime::Counter final : public PageDependencySink {
 public:
  Counter(const PageStore &store, const PageEntityBudget &budget, PageArenaAllocator &allocator)
      : store(store), budget(budget), allocator(allocator) {}
  ~Counter() override { clear(); }
  bool add(CardId card, std::string_view entity, std::string_view attribute) override {
    if (!valid || card >= store.card_count() || entity.empty() ||
        !valid_name(entity) || !valid_name(attribute)) return valid = false;
    Pair *pair = pairs;
    for (; pair != nullptr; pair = pair->next)
      if (pair->entity() == entity && pair->attribute() == attribute) break;
    for (const Link *link = links; link != nullptr; link = link->next)
      if (link->card == card && link->pair == pair) return true;
    if (count >= budget.dependencies) return valid = false;
    if (pair == nullptr) {
      size_t bytes = entity.size();
      size_t total_names = names;
      if (channels >= budget.channels || !add_size(bytes, attribute.size()) || !add_size(bytes, 2) ||
          !add_size(total_names, bytes) || total_names > UINT32_MAX ||
          !add_size(bytes, sizeof(Pair))) return valid = false;
      size_t total_scratch = scratch_bytes;
      if (!add_size(total_scratch, bytes)) return valid = false;
      void *memory = allocator.allocate(bytes);
      if (memory == nullptr) return valid = false;
      pair = new (memory) Pair{nullptr, entity.size(), attribute.size(), bytes};
      char *text = reinterpret_cast<char *>(pair + 1);
      std::memcpy(text, entity.data(), entity.size()); text[entity.size()] = '\0';
      if (!attribute.empty()) std::memcpy(text + entity.size() + 1, attribute.data(), attribute.size());
      text[entity.size() + 1 + attribute.size()] = '\0';
      pair->next = pairs; pairs = pair;
      ++channels; names = total_names; scratch_bytes = total_scratch;
    }
    size_t total_scratch = scratch_bytes;
    if (!add_size(total_scratch, sizeof(Link))) return valid = false;
    void *memory = allocator.allocate(sizeof(Link));
    if (memory == nullptr) return valid = false;
    auto *link = new (memory) Link{nullptr, pair, card};
    if (last != nullptr) last->next = link; else links = link;
    last = link;
    ++count; scratch_bytes = total_scratch;
    return true;
  }
  bool replay(PageDependencySink &sink) const {
    for (const Link *link = links; link != nullptr; link = link->next)
      if (!sink.add(link->card, link->pair->entity(), link->pair->attribute())) return false;
    return true;
  }
  void clear() {
    while (links != nullptr) {
      auto *next = links->next;
      allocator.release(links, sizeof(Link)); links = next;
    }
    last = nullptr;
    while (pairs != nullptr) {
      auto *next = pairs->next;
      const auto bytes = pairs->allocation_bytes;
      allocator.release(pairs, bytes); pairs = next;
    }
    scratch_bytes = 0;
  }
  const PageStore &store;
  const PageEntityBudget &budget;
  PageArenaAllocator &allocator;
  size_t count{0}, channels{0}, names{0}, scratch_bytes{0};
  bool valid{true};
 private:
  // Fallible scratch storage grows only for unique channels and card/channel
  // links. Copy names because sources may emit temporary parsed strings.
  struct Pair {
    Pair *next;
    size_t entity_size, attribute_size, allocation_bytes;
    std::string_view entity() const { return {reinterpret_cast<const char *>(this + 1), entity_size}; }
    std::string_view attribute() const {
      return {reinterpret_cast<const char *>(this + 1) + entity_size + 1, attribute_size};
    }
  };
  struct Link { Link *next; Pair *pair; CardId card; };
  Pair *pairs{nullptr};
  Link *links{nullptr}, *last{nullptr};
};

class PageEntityStateRuntime::Builder final : public PageDependencySink {
 public:
  Builder(Slot &slot, const PageStore &store, size_t count, size_t names, size_t channels)
      : slot(slot), store(store), count_limit(count), names_limit(names), channels_limit(channels) {}
  bool add(CardId card, std::string_view entity, std::string_view attribute) override {
    if (!valid || ++count > count_limit || card >= store.card_count() || entity.empty() ||
        !valid_name(entity) || !valid_name(attribute)) return valid = false;
    size_t channel = 0;
    for (; channel < slot.channel_count; ++channel) {
      const auto &entry = slot.channels[channel];
      if (std::string_view(slot.names + entry.entity_offset, entry.entity_size) == entity &&
          std::string_view(slot.names + entry.attribute_offset, entry.attribute_size) == attribute) break;
    }
    if (channel == slot.channel_count) {
      size_t bytes = entity.size();
      if (channel >= channels_limit || !add_size(bytes, attribute.size()) || !add_size(bytes, 2) ||
          bytes > names_limit - slot.names_used) return valid = false;
      auto &entry = *new (slot.channels + channel) Channel{};
      entry.entity_offset = static_cast<uint32_t>(slot.names_used);
      entry.entity_size = static_cast<uint32_t>(entity.size());
      std::memcpy(slot.names + slot.names_used, entity.data(), entity.size());
      slot.names_used += entity.size();
      slot.names[slot.names_used++] = '\0';
      entry.attribute_offset = static_cast<uint32_t>(slot.names_used);
      entry.attribute_size = static_cast<uint32_t>(attribute.size());
      if (!attribute.empty()) std::memcpy(slot.names + slot.names_used, attribute.data(), attribute.size());
      slot.names_used += attribute.size();
      slot.names[slot.names_used++] = '\0';
      ++slot.channel_count;
    }
    for (size_t i = 0; i < slot.edge_count; ++i)
      if (slot.edges[i].card == card && slot.edges[i].channel == channel) return true;
    new (slot.edges + slot.edge_count++) Edge{card, static_cast<uint32_t>(channel)};
    return true;
  }
  Slot &slot;
  const PageStore &store;
  size_t count_limit, names_limit, channels_limit, count{0};
  bool valid{true};
};

PageEntityStateRuntime::PageEntityStateRuntime(const PageDependencySource &source,
    PageArenaAllocator &allocator, PageEntitySubscriptions &subscriptions, PageEntityBudget budget)
    : source_(source), allocator_(allocator), subscriptions_(subscriptions), budget_(budget),
      connection_generation_(subscriptions.connection_generation()) {
  for (auto &slot : slots_) slot.runtime = this;
}
PageEntityStateRuntime::~PageEntityStateRuntime() { discard_prepared(); retire_active(); }

bool PageEntityStateRuntime::prepare(const PageStore &store) {
  discard_prepared();
  Counter counter(store, budget_, allocator_);
  auto fail = [this, &counter]() {
    const auto current = memory();
    memory_.peak_arena_bytes = std::max(memory_.peak_arena_bytes,
        current.active_arena_bytes + current.staged_arena_bytes + counter.scratch_bytes);
    ++memory_.preparation_failures; discard_prepared(); return false;
  };
  if (next_revision_ == 0 || budget_.state_bytes > UINT32_MAX || budget_.dependencies > UINT32_MAX ||
      budget_.channels > UINT32_MAX || !source_.enumerate(store, counter) || !counter.valid) return fail();
  const size_t channels = counter.channels;
  size_t bytes = 0;
  if (!add_size(bytes, channels, sizeof(Channel)) || !align_size(bytes, alignof(Edge))) return fail();
  const size_t edge_offset = bytes;
  if (!add_size(bytes, counter.count, sizeof(Edge))) return fail();
  const size_t names_offset = bytes;
  if (!add_size(bytes, counter.names)) return fail();
  const size_t values_offset = bytes;
  if (!add_size(bytes, channels == 0 ? 0 : budget_.state_bytes) || bytes > UINT32_MAX) return fail();
  bytes = std::max(bytes, size_t{1});
  void *arena = allocator_.allocate(bytes);
  if (arena == nullptr) return fail();
  staged_ = active_ == 0 ? 1 : 0;
  auto &slot = slots_[staged_];
  slot.arena = arena;
  slot.arena_size = bytes;
  slot.revision = next_revision_++;
  auto *raw = static_cast<char *>(arena);
  slot.channels = reinterpret_cast<Channel *>(raw);
  slot.edges = reinterpret_cast<Edge *>(raw + edge_offset);
  slot.names = raw + names_offset;
  slot.values = raw + values_offset;
  const auto current = memory();
  memory_.peak_arena_bytes = std::max(memory_.peak_arena_bytes,
      current.active_arena_bytes + current.staged_arena_bytes + counter.scratch_bytes);
  Builder builder(slot, store, counter.count, counter.names, channels);
  if (!counter.replay(builder) || !builder.valid || builder.count != counter.count) return fail();
  counter.clear();
  synchronize_connection();
  for (size_t i = 0; i < slot.channel_count; ++i) {
    const auto &entry = slot.channels[i];
    // Preserve shared live values without asking the coordinator to keep a
    // second cache. Both arenas remain alive throughout this preparation.
    if (active_ >= 0) {
      const auto &old = slots_[active_];
      for (size_t j = 0; j < old.channel_count; ++j) {
        const auto &previous = old.channels[j];
        if (previous.status != PageEntityStatus::AVAILABLE ||
            std::string_view(slot.names + entry.entity_offset, entry.entity_size) !=
                std::string_view(old.names + previous.entity_offset, previous.entity_size) ||
            std::string_view(slot.names + entry.attribute_offset, entry.attribute_size) !=
                std::string_view(old.names + previous.attribute_offset, previous.attribute_size)) continue;
        receive(slot, slot.revision, static_cast<uint32_t>(i),
            {old.values + previous.value_offset, previous.value_size});
        break;
      }
    }
    if (!subscriptions_.subscribe(slot.names + entry.entity_offset, slot.names + entry.attribute_offset,
          &slot, slot.revision, static_cast<uint32_t>(i), deliver)) return fail();
  }
  return true;
}

void PageEntityStateRuntime::release_slot(int index) {
  if (index < 0) return;
  auto &slot = slots_[index];
  slot.revision = 0;  // Disable late delivery before releasing any storage.
  subscriptions_.release(&slot);
  if (slot.arena != nullptr) allocator_.release(slot.arena, slot.arena_size);
  slot = {};
  slot.runtime = this;
}
void PageEntityStateRuntime::discard_prepared() { release_slot(staged_); staged_ = -1; }
void PageEntityStateRuntime::retire_active() {
  if (active_ >= 0) ++revision_;
  release_slot(active_); active_ = -1;
}
void PageEntityStateRuntime::activate_prepared(uint32_t generation) {
  if (staged_ < 0) return;
  const int retired = active_;
  active_ = staged_;
  staged_ = -1;
  slots_[active_].generation = generation;
  release_slot(retired);
  ++revision_;
}

void PageEntityStateRuntime::remove_value(Slot &slot, Channel &channel) {
  const size_t offset = channel.value_offset, bytes = channel.value_size;
  if (bytes != 0) {
    std::memmove(slot.values + offset, slot.values + offset + bytes, slot.values_used - offset - bytes);
    slot.values_used -= bytes;
    for (size_t i = 0; i < slot.channel_count; ++i) {
      auto &other = slot.channels[i];
      if (other.value_size != 0 && other.value_offset > offset) other.value_offset -= bytes;
    }
  }
  channel.value_offset = 0;
  channel.value_size = 0;
}
void PageEntityStateRuntime::deliver(void *owner, uint32_t revision, uint32_t channel,
                                   std::string_view value) {
  auto &slot = *static_cast<Slot *>(owner);
  slot.runtime->receive(slot, revision, channel, value);
}
void PageEntityStateRuntime::receive(Slot &slot, uint32_t revision, uint32_t channel,
                                    std::string_view value) {
  if (revision == 0 || slot.revision != revision || slot.arena == nullptr || channel >= slot.channel_count) {
    ++memory_.stale_deliveries;
    return;
  }
  synchronize_connection();
  auto &entry = slot.channels[channel];
  if (entry.status == PageEntityStatus::AVAILABLE && entry.value_size == value.size() &&
      std::string_view(slot.values + entry.value_offset, entry.value_size) == value) return;
  const size_t available = budget_.state_bytes - (slot.values_used - entry.value_size);
  ++revision_;
  remove_value(slot, entry);
  ++entry.revision;
  if (value.size() > available) {
    entry.status = PageEntityStatus::EXHAUSTED;
    ++memory_.state_overflows;
    return;
  }
  entry.value_offset = static_cast<uint32_t>(slot.values_used);
  entry.value_size = static_cast<uint32_t>(value.size());
  if (!value.empty()) std::memcpy(slot.values + slot.values_used, value.data(), value.size());
  slot.values_used += value.size();
  entry.status = PageEntityStatus::AVAILABLE;
}
void PageEntityStateRuntime::invalidate_disconnected_state() {
  ++revision_;
  connection_generation_ = subscriptions_.connection_generation();
  for (auto &slot : slots_) {
    slot.values_used = 0;
    for (size_t i = 0; i < slot.channel_count; ++i) {
      auto &entry = slot.channels[i];
      entry.value_offset = entry.value_size = 0;
      entry.status = PageEntityStatus::UNKNOWN;
      ++entry.revision;
    }
  }
}
void PageEntityStateRuntime::synchronize_connection() {
  if (subscriptions_.connection_generation() != connection_generation_) invalidate_disconnected_state();
}
size_t PageEntityStateRuntime::dependency_count(ConfigurationHandle card) const {
  if (active_ < 0 || slots_[active_].generation != card.generation) return 0;
  const auto &slot = slots_[active_];
  size_t count = 0;
  for (size_t i = 0; i < slot.edge_count; ++i) if (slot.edges[i].card == card.index) ++count;
  return count;
}
PageEntityValue PageEntityStateRuntime::dependency(ConfigurationHandle card, size_t ordinal) {
  synchronize_connection();
  if (active_ < 0 || slots_[active_].generation != card.generation) return {};
  const auto &slot = slots_[active_];
  for (size_t i = 0; i < slot.edge_count; ++i) {
    if (slot.edges[i].card != card.index || ordinal-- != 0) continue;
    const auto &entry = slot.channels[slot.edges[i].channel];
    return {entry.status, {slot.names + entry.entity_offset, entry.entity_size},
        {slot.names + entry.attribute_offset, entry.attribute_size},
        {slot.values + entry.value_offset, entry.value_size}, entry.revision};
  }
  return {};
}
PageEntityMemory PageEntityStateRuntime::memory() const {
  auto result = memory_;
  if (active_ >= 0) {
    const auto &slot = slots_[active_];
    result.active_arena_bytes = slot.arena_size;
    result.active_channels = slot.channel_count;
    result.active_dependencies = slot.edge_count;
    result.active_state_bytes = slot.values_used;
  }
  if (staged_ >= 0) result.staged_arena_bytes = slots_[staged_].arena_size;
  return result;
}
}  // namespace espcontrol::pages
