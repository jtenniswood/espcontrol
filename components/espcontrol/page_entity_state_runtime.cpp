#include "page_entity_state_runtime.h"
#include "bounded_percent_state.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace espcontrol::pages {
namespace {
struct PreparationIndex {
  PageArenaAllocator &allocator;
  size_t &bytes;
  uint32_t *data{nullptr};
  ~PreparationIndex() {
    if (data != nullptr)
      allocator.release(data, bytes);
    bytes = 0;
  }
};

uint32_t channel_hash(std::string_view entity, std::string_view attribute) {
  uint32_t value = 2166136261u;
  for (const unsigned char byte : entity)
    value = (value ^ byte) * 16777619u;
  value *= 16777619u; // NUL separates the two validated, NUL-free names.
  for (const unsigned char byte : attribute)
    value = (value ^ byte) * 16777619u;
  return value;
}

uint32_t dependency_link_hash(CardId card, uint32_t channel) {
  return (static_cast<uint32_t>(card) * 0x85ebca6bu) ^ (channel * 0x9e3779b1u);
}

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
  Counter(PageEntityStateRuntime &runtime, const PageStore &store)
      : runtime(runtime), store(store), budget(runtime.budget_) {}
  ~Counter() override { clear(); }
  bool add(CardId card, std::string_view entity, std::string_view attribute) override {
    if (!valid || card >= store.card_count() || entity.empty() ||
        !valid_name(entity) || !valid_name(attribute)) return valid = false;
    if (card != previous_card) {
      if (previous_card != NO_CARD && card < previous_card) ordered = false;
      previous_card = card; card_links = 0;
    }
    uint32_t channel = find_pair(entity, attribute);
    if (channel != UINT32_MAX && has_link(card, channel)) return true;
    if (count >= budget.dependencies) return valid = false;
    if (channel == UINT32_MAX) {
      size_t total = names;
      if (channels >= budget.channels || !add_size(total, entity.size()) ||
          !add_size(total, attribute.size()) || !add_size(total, 2) || total > UINT32_MAX ||
          !grow(pairs, pair_capacity, channels, channels + 1, budget.channels) ||
          !grow_index(pair_index, pair_slots, channels + 1, true)) return valid = false;
      char *record = reserve_name(total - names);
      if (record == nullptr) return valid = false;
      channel = static_cast<uint32_t>(channels);
      new (pairs + channels) Pair{record, static_cast<uint32_t>(entity.size()),
                                  static_cast<uint32_t>(attribute.size())};
      std::memcpy(record, entity.data(), entity.size());
      record[entity.size()] = '\0';
      if (!attribute.empty()) std::memcpy(record + entity.size() + 1, attribute.data(), attribute.size());
      record[total - names - 1] = '\0';
      ++channels; names = total;
      insert(pair_index, pair_slots, channel_hash(entity, attribute), channel);
    }
    if (!grow(links, link_capacity, count, count + 1, budget.dependencies) ||
        !grow_index(link_index, link_slots, count + 1, false)) return valid = false;
    new (links + count) Link{card, channel};
    insert(link_index, link_slots, dependency_link_hash(card, channel), static_cast<uint32_t>(count));
    ++count;
    max_card_links = std::max(max_card_links, ++card_links);
    return true;
  }
  void release_lookup() {
    release(pair_index, pair_slots * sizeof(uint32_t)); pair_index = nullptr; pair_slots = 0;
    release(link_index, link_slots * sizeof(uint32_t)); link_index = nullptr; link_slots = 0;
  }
  void clear() {
    // Keep scalar sizes for the next stable enumeration, but retain no scratch
    // alongside its final arena. Name blocks never need reallocating/copying.
    release_lookup();
    release(pairs, pair_capacity * sizeof(Pair)); pairs = nullptr; pair_capacity = 0;
    release(links, link_capacity * sizeof(Link)); links = nullptr; link_capacity = 0;
    while (names_head != nullptr) {
      auto *old = names_head; names_head = old->next;
      release(old, sizeof(NameBlock) + old->capacity);
    }
  }
  size_t count{0}, channels{0}, names{0}, max_card_links{0};
  bool valid{true}, ordered{true};
 private:
  // Fallible, demand-grown scratch owns borrowed names and keeps the first
  // declaration order. Both indexes avoid repeated scans of large graphs.
  struct Pair { const char *text; uint32_t entity_size, attribute_size; };
  struct Link { CardId card; uint32_t channel; };
  struct NameBlock { NameBlock *next; size_t capacity, used; };
  void *allocate(size_t bytes) {
    size_t total = runtime.dependency_scratch_bytes_;
    if (!add_size(total, bytes)) return nullptr;
    void *result = runtime.allocator_.allocate(bytes);
    if (result != nullptr) {
      runtime.dependency_scratch_bytes_ = total;
      runtime.record_preparation_peak();
    }
    return result;
  }
  void release(void *pointer, size_t bytes) {
    if (pointer == nullptr) return;
    runtime.allocator_.release(pointer, bytes);
    runtime.dependency_scratch_bytes_ -= bytes;
  }
  template<typename T>
  bool grow(T *&data, size_t &capacity, size_t used, size_t required, size_t limit) {
    if (required <= capacity) return true;
    if (required > limit) return false;
    size_t next = capacity != 0 ? capacity : std::min(size_t{8}, limit);
    while (next < required) next = next > limit / 2 ? limit : next * 2;
    size_t bytes = 0;
    if (!add_size(bytes, next, sizeof(T))) return false;
    auto *replacement = static_cast<T *>(allocate(bytes));
    if (replacement == nullptr) return false;
    for (size_t i = 0; i < used; ++i) new (replacement + i) T(data[i]);
    release(data, capacity * sizeof(T)); data = replacement; capacity = next;
    return true;
  }
  char *reserve_name(size_t bytes) {
    if (names_head == nullptr || bytes > names_head->capacity - names_head->used) {
      size_t capacity = names_head == nullptr ? 32 :
          (names_head->capacity >= 4096 ? 4096 : names_head->capacity * 2);
      capacity = std::max(capacity, bytes);
      size_t allocation = sizeof(NameBlock);
      if (!add_size(allocation, capacity)) return nullptr;
      void *memory = allocate(allocation);
      if (memory == nullptr) return nullptr;
      names_head = new (memory) NameBlock{names_head, capacity, 0};
    }
    auto *result = reinterpret_cast<char *>(names_head + 1) + names_head->used;
    names_head->used += bytes;
    return result;
  }
  static void insert(uint32_t *index, size_t slots, uint32_t hash, uint32_t value) {
    size_t bucket = hash & (slots - 1);
    while (index[bucket] != UINT32_MAX) bucket = (bucket + 1) & (slots - 1);
    index[bucket] = value;
  }
  std::string_view entity(uint32_t channel) const {
    const auto &pair = pairs[channel]; return {pair.text, pair.entity_size};
  }
  std::string_view attribute(uint32_t channel) const {
    const auto &pair = pairs[channel]; return {pair.text + pair.entity_size + 1, pair.attribute_size};
  }
  bool grow_index(uint32_t *&index, size_t &slots, size_t required, bool pair_lookup) {
    if (slots != 0 && required <= slots / 2) return true;
    size_t next = slots != 0 ? slots : 16;
    while (required > next / 2) {
      if (next > SIZE_MAX / 2) return false;
      next *= 2;
    }
    size_t bytes = 0;
    if (!add_size(bytes, next, sizeof(uint32_t))) return false;
    auto *replacement = static_cast<uint32_t *>(allocate(bytes));
    if (replacement == nullptr) return false;
    std::fill_n(replacement, next, UINT32_MAX);
    const size_t used = pair_lookup ? channels : count;
    for (size_t i = 0; i < used; ++i) {
      const auto hash = pair_lookup ? channel_hash(entity(static_cast<uint32_t>(i)),
                                                   attribute(static_cast<uint32_t>(i)))
                                    : dependency_link_hash(links[i].card, links[i].channel);
      insert(replacement, next, hash, static_cast<uint32_t>(i));
    }
    release(index, slots * sizeof(uint32_t)); index = replacement; slots = next;
    return true;
  }
  uint32_t find_pair(std::string_view wanted_entity, std::string_view wanted_attribute) const {
    if (pair_slots == 0) return UINT32_MAX;
    size_t bucket = channel_hash(wanted_entity, wanted_attribute) & (pair_slots - 1);
    while (pair_index[bucket] != UINT32_MAX) {
      const auto channel = pair_index[bucket];
      if (entity(channel) == wanted_entity && attribute(channel) == wanted_attribute) return channel;
      bucket = (bucket + 1) & (pair_slots - 1);
    }
    return UINT32_MAX;
  }
  bool has_link(CardId card, uint32_t channel) const {
    if (link_slots == 0) return false;
    size_t bucket = dependency_link_hash(card, channel) & (link_slots - 1);
    while (link_index[bucket] != UINT32_MAX) {
      const auto &link = links[link_index[bucket]];
      if (link.card == card && link.channel == channel) return true;
      bucket = (bucket + 1) & (link_slots - 1);
    }
    return false;
  }
  PageEntityStateRuntime &runtime;
  const PageStore &store;
  const PageEntityBudget &budget;
  Pair *pairs{nullptr}; Link *links{nullptr}; NameBlock *names_head{nullptr};
  uint32_t *pair_index{nullptr}, *link_index{nullptr};
  size_t pair_capacity{0}, link_capacity{0}, pair_slots{0}, link_slots{0}, card_links{0};
  CardId previous_card{NO_CARD};
};

class PageEntityStateRuntime::Builder final : public PageDependencySink {
 public:
   Builder(Slot &slot, const PageStore &store, size_t count, size_t names,
           size_t channels, uint32_t *lookup, size_t lookup_size,
           uint32_t *edge_lookup, size_t edge_lookup_size)
       : slot(slot), store(store), count_limit(count), names_limit(names),
         channels_limit(channels), lookup(lookup), lookup_size(lookup_size),
         edge_lookup(edge_lookup), edge_lookup_size(edge_lookup_size) {}
   bool add(CardId card, std::string_view entity,
            std::string_view attribute) override {
     if (!valid || card >= store.card_count() ||
         entity.empty() || !valid_name(entity) || !valid_name(attribute))
       return valid = false;
     const auto matches = [&](size_t channel) {
       const auto &entry = slot.channels[channel];
       return std::string_view(slot.names + entry.entity_offset,
                               entry.entity_size) == entity &&
              std::string_view(slot.names + entry.attribute_offset,
                               entry.attribute_size) == attribute;
     };
     size_t channel = slot.channel_count, bucket = 0;
     if (lookup != nullptr) {
       bucket = channel_hash(entity, attribute) & (lookup_size - 1);
       while (lookup[bucket] != UINT32_MAX) {
         if (matches(lookup[bucket])) {
           channel = lookup[bucket];
           break;
         }
         bucket = (bucket + 1) & (lookup_size - 1);
       }
     } else {
       for (channel = 0; channel < slot.channel_count; ++channel)
         if (matches(channel))
           break;
     }
     if (channel == slot.channel_count) {
       size_t bytes = entity.size();
       if (channel >= channels_limit || !add_size(bytes, attribute.size()) ||
           !add_size(bytes, 2) || bytes > names_limit - slot.names_used)
         return valid = false;
       auto &entry = *new (slot.channels + channel) Channel{};
       entry.entity_offset = static_cast<uint32_t>(slot.names_used);
       entry.entity_size = static_cast<uint32_t>(entity.size());
       std::memcpy(slot.names + slot.names_used, entity.data(), entity.size());
       slot.names_used += entity.size();
       slot.names[slot.names_used++] = '\0';
       entry.attribute_offset = static_cast<uint32_t>(slot.names_used);
       entry.attribute_size = static_cast<uint32_t>(attribute.size());
       if (!attribute.empty())
         std::memcpy(slot.names + slot.names_used, attribute.data(),
                     attribute.size());
       slot.names_used += attribute.size();
       slot.names[slot.names_used++] = '\0';
       ++slot.channel_count;
       if (lookup != nullptr)
         lookup[bucket] = static_cast<uint32_t>(channel);
     }
     size_t edge_bucket = 0;
     if (edge_lookup != nullptr) {
       edge_bucket = dependency_link_hash(card, static_cast<uint32_t>(channel)) & (edge_lookup_size - 1);
       while (edge_lookup[edge_bucket] != UINT32_MAX) {
         const auto &edge = slot.edges[edge_lookup[edge_bucket]];
         if (edge.card == card && edge.channel == channel) return true;
         edge_bucket = (edge_bucket + 1) & (edge_lookup_size - 1);
       }
     } else {
       if (card != previous_card) {
         if (previous_card != NO_CARD && card < previous_card) ordered = false;
         previous_card = card; card_edge_begin = slot.edge_count;
       }
       for (size_t i = ordered ? card_edge_begin : 0; i < slot.edge_count; ++i)
         if (slot.edges[i].card == card && slot.edges[i].channel == channel) return true;
     }
     if (count >= count_limit) return valid = false;
     if (edge_lookup != nullptr) edge_lookup[edge_bucket] = static_cast<uint32_t>(slot.edge_count);
     new (slot.edges + slot.edge_count++)
         Edge{card, static_cast<uint32_t>(channel)};
     ++count;
     return true;
   }
  Slot &slot;
  const PageStore &store;
  size_t count_limit, names_limit, channels_limit, count{0};
  uint32_t *lookup;
  size_t lookup_size;
  uint32_t *edge_lookup;
  size_t edge_lookup_size, card_edge_begin{0};
  CardId previous_card{NO_CARD};
  bool valid{true}, ordered{true};
};

PageEntityStateRuntime::PageEntityStateRuntime(const PageDependencySource &source,
    PageArenaAllocator &allocator, PageEntitySubscriptions &subscriptions, PageEntityBudget budget,
    PageEntityObserver *observer)
    : source_(source), allocator_(allocator), subscriptions_(subscriptions), budget_(budget), observer_(observer),
      connection_generation_(subscriptions.connection_generation()) {
  for (auto &slot : slots_) slot.runtime = this;
}
PageEntityStateRuntime::~PageEntityStateRuntime() { discard_prepared(); retire_active(); }

void PageEntityStateRuntime::record_preparation_peak(size_t extra) {
  const auto current = memory();
  memory_.peak_arena_bytes = std::max(memory_.peak_arena_bytes,
      current.active_arena_bytes + current.staged_arena_bytes +
      preparation_scratch_bytes_ + dependency_scratch_bytes_ + extra);
}

bool PageEntityStateRuntime::prepare(const PageStore &store) {
  discard_prepared();
  Counter counter(*this, store);
  auto fail = [this]() { ++memory_.preparation_failures; discard_prepared(); return false; };
  if (next_revision_ == 0 || budget_.state_bytes > UINT32_MAX || budget_.dependencies > UINT32_MAX ||
      budget_.channels > UINT32_MAX || !source_.enumerate(store, counter) || !counter.valid) return fail();
  // Counting and final construction must not retain two copies of the graph.
  counter.clear();
  const size_t channels = counter.channels;
  size_t bytes = 0;
  if (!add_size(bytes, channels, sizeof(Channel)) || !align_size(bytes, alignof(Edge))) return fail();
  const size_t edge_offset = bytes;
  if (!add_size(bytes, counter.count, sizeof(Edge))) return fail();
  const size_t index_offset = bytes;
  if (counter.count != 0 && !add_size(bytes, store.card_count() + 1, sizeof(uint32_t))) return fail();
  const size_t names_offset = bytes;
  if (!add_size(bytes, counter.names)) return fail();
  const size_t values_offset = bytes;
  const bool elastic = budget_.initial_state_bytes < budget_.state_bytes;
  const size_t embedded_values =
      channels == 0 || elastic ? 0 : budget_.state_bytes;
  if (!add_size(bytes, embedded_values) || bytes > UINT32_MAX)
    return fail();
  bytes = std::max(bytes, size_t{1});
  void *arena = allocator_.allocate(bytes);
  if (arena == nullptr) return fail();
  staged_ = active_ == 0 ? 1 : 0;
  auto &slot = slots_[staged_];
  slot.arena = arena;
  slot.arena_size = bytes;
  slot.revision = next_revision_++;
  slot.generation=store.generation();
  auto *raw = static_cast<char *>(arena);
  slot.channels = reinterpret_cast<Channel *>(raw);
  slot.edges = reinterpret_cast<Edge *>(raw + edge_offset);
  slot.card_count = store.card_count();
  if (counter.count != 0) {
    slot.card_offsets = reinterpret_cast<uint32_t *>(raw + index_offset);
    for (size_t i = 0; i <= slot.card_count; ++i) new (slot.card_offsets + i) uint32_t{0};
  }
  slot.names = raw + names_offset;
  slot.values = raw + values_offset;
  slot.values_capacity = embedded_values;
  record_preparation_peak();
  if (elastic && channels != 0 &&
      !reserve_values(slot, budget_.initial_state_bytes))
    return fail();
  {
    PreparationIndex lookup{allocator_, preparation_scratch_bytes_};
    size_t lookup_size = 0, edge_lookup_size = 0;
    const auto table_size = [](size_t entries, size_t &slots) {
      slots = 1;
      // Keep the table at most half full, including when all names differ.
      while (slots < entries) {
        if (slots > SIZE_MAX / 2) return false;
        slots *= 2;
      }
      if (slots > SIZE_MAX / 2) return false;
      slots *= 2;return true;
    };
    if (channels > 16 && !table_size(channels, lookup_size)) return fail();
    // Normal ordered card declarations need only short per-card scans. Large
    // fan-out or interleaved sources use a link index instead of quadratic work.
    if (counter.count > 16 && (!counter.ordered || counter.max_card_links > 32) &&
        !table_size(counter.count, edge_lookup_size)) return fail();
    size_t lookup_slots = lookup_size;
    if (!add_size(lookup_slots, edge_lookup_size)) return fail();
    if (lookup_slots != 0) {
      if (!add_size(preparation_scratch_bytes_, lookup_slots, sizeof(uint32_t)))
        return fail();
      lookup.data = static_cast<uint32_t *>(
          allocator_.allocate(preparation_scratch_bytes_));
      if (lookup.data == nullptr)
        return fail();
      record_preparation_peak();
      std::fill_n(lookup.data, lookup_slots, UINT32_MAX);
    }
    Builder builder(slot, store, counter.count, counter.names, channels,
                    lookup_size != 0 ? lookup.data : nullptr, lookup_size,
                    edge_lookup_size != 0 ? lookup.data + lookup_size : nullptr, edge_lookup_size);
    if (!source_.enumerate(store, builder) || !builder.valid ||
        builder.count != counter.count || slot.channel_count != channels ||
        slot.names_used != counter.names)
      return fail();
  } // Release build lookup before edge grouping, shared-value copy or transport
    // setup.
  if (!index_dependencies(slot))
    return fail();
  if (!subscriptions_.prepare_batch(slot.subscription_batch, &slot,
                                    slot.revision, slot.channel_count, deliver))
    return fail();
  synchronize_connection();
  // Keep large replacements bounded by sorting channel indices once. Names
  // and values remain owned by the old arena; no second state cache is kept.
  {
    PreparationIndex copy_index{allocator_, preparation_scratch_bytes_};
    const Slot *old = active_ >= 0 && subscriptions_.copy_shared_values()
                          ? &slots_[active_]
                          : nullptr;
    const auto key = [](const Slot &source, size_t channel) {
      const auto &entry = source.channels[channel];
      return std::pair<std::string_view, std::string_view>{
          {source.names + entry.entity_offset, entry.entity_size},
          {source.names + entry.attribute_offset, entry.attribute_size}};
    };
    if (old != nullptr && old->channel_count > 16) {
      if (!add_size(preparation_scratch_bytes_, old->channel_count,
                    sizeof(uint32_t)))
        return fail();
      copy_index.data = static_cast<uint32_t *>(
          allocator_.allocate(preparation_scratch_bytes_));
      if (copy_index.data == nullptr)
        return fail();
      record_preparation_peak();
      for (size_t i = 0; i < old->channel_count; ++i)
        copy_index.data[i] = static_cast<uint32_t>(i);
      std::sort(copy_index.data, copy_index.data + old->channel_count,
                [&](uint32_t left, uint32_t right) {
                  return key(*old, left) < key(*old, right);
                });
    }
    for (size_t i = 0; i < slot.channel_count; ++i) {
      synchronize_connection();
      if (old != nullptr) {
        const auto wanted = key(slot, i);
        size_t previous_channel = old->channel_count;
        if (copy_index.data != nullptr) {
          const auto *found = std::lower_bound(
              copy_index.data, copy_index.data + old->channel_count, wanted,
              [&](uint32_t channel, const auto &value) {
                return key(*old, channel) < value;
              });
          if (found != copy_index.data + old->channel_count &&
              key(*old, *found) == wanted)
            previous_channel = *found;
        } else {
          for (size_t j = 0; j < old->channel_count; ++j) {
            if (key(*old, j) == wanted) {
              previous_channel = j;
              break;
            }
          }
        }
        if (previous_channel != old->channel_count) {
          const auto &previous = old->channels[previous_channel];
          if (previous.status == PageEntityStatus::AVAILABLE) {
            if (!receive(
                  slot, slot.revision, static_cast<uint32_t>(i),
                  {old->values + previous.value_offset, previous.value_size},
                  true))
              return fail();
          } else if (previous.status == PageEntityStatus::EXHAUSTED) {
            auto &entry = slot.channels[i];
            entry.status = PageEntityStatus::EXHAUSTED;
            ++entry.revision;
          }
        }
      }
    }
  } // Shared-value lookup is gone before reserving transport lookup scratch.
  if (slot.channel_count == 0)
    return true;
  PreparationIndex registration_index{allocator_, preparation_scratch_bytes_};
  preparation_scratch_bytes_ = subscriptions_.registration_scratch_bytes();
  if (preparation_scratch_bytes_ == SIZE_MAX)
    return fail();
  if (preparation_scratch_bytes_) {
    registration_index.data = static_cast<uint32_t *>(
        allocator_.allocate(preparation_scratch_bytes_));
    if (!registration_index.data)
      return fail();
    record_preparation_peak();
  }
  struct RegistrationGuard {
    PageEntitySubscriptions &subscriptions;
    bool active{false};
    ~RegistrationGuard() {
      if (active)
        subscriptions.end_registration_batch();
    }
  } registration_guard{subscriptions_};
  if (!subscriptions_.begin_registration_batch(registration_index.data,
                                               preparation_scratch_bytes_))
    return fail();
  registration_guard.active = true;
  for (size_t i = 0; i < slot.channel_count; ++i) {
    const auto &entry = slot.channels[i];
    synchronize_connection();
    if (!subscriptions_.subscribe(slot.names + entry.entity_offset, slot.names + entry.attribute_offset,
          &slot, slot.revision, static_cast<uint32_t>(i), deliver)) return fail();
  }
  return true;
}

bool PageEntityStateRuntime::index_dependencies(Slot &slot) {
  if (slot.edge_count == 0) return true;
  bool ordered = true;
  for (size_t i = 0; i < slot.edge_count; ++i) {
    ++slot.card_offsets[slot.edges[i].card + 1];
    if (i != 0 && slot.edges[i - 1].card > slot.edges[i].card) ordered = false;
  }
  for (size_t card = 1; card <= slot.card_count; ++card)
    slot.card_offsets[card] += slot.card_offsets[card - 1];
  // Production declarations already arrive in card order. Other sources can
  // interleave cards: group them stably without changing per-card ordinals.
  if (ordered) return true;
  const size_t bytes = slot.edge_count * sizeof(Edge);
  auto *scratch = static_cast<Edge *>(allocator_.allocate(bytes));
  if (scratch == nullptr) return false;
  record_preparation_peak(bytes);
  for (size_t i = 0; i < slot.edge_count; ++i) {
    const auto edge = slot.edges[i];
    new (scratch + slot.card_offsets[edge.card]++) Edge(edge);
  }
  // The write cursors now contain row ends; shift them back to row starts.
  for (size_t card = slot.card_count; card != 0; --card)
    slot.card_offsets[card] = slot.card_offsets[card - 1];
  slot.card_offsets[0] = 0;
  for (size_t i = 0; i < slot.edge_count; ++i) {
    slot.edges[i] = scratch[i];scratch[i].~Edge();
  }
  allocator_.release(scratch, bytes);
  return true;
}

void PageEntityStateRuntime::release_slot(int index) {
  if (index < 0) return;
  auto &slot = slots_[index];
  slot.revision = 0;  // Disable late delivery before releasing any storage.
  subscriptions_.release(&slot);
  if (slot.value_storage != nullptr)
    allocator_.release(slot.value_storage, slot.values_capacity);
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
bool PageEntityStateRuntime::receive(Slot &slot, uint32_t revision,
                                     uint32_t channel, std::string_view value,
                                     bool copied) {
  if (revision == 0 || slot.revision != revision || slot.arena == nullptr || channel >= slot.channel_count) {
    ++memory_.stale_deliveries;
    return false;
  }
  synchronize_connection();
  auto &entry = slot.channels[channel];
  const std::string_view attribute(slot.names+entry.attribute_offset,entry.attribute_size);
  if(observer_)observer_->observed_update(slot.generation,
      {slot.names+entry.entity_offset,entry.entity_size},attribute,value,copied,
      entry.status==PageEntityStatus::AVAILABLE
          ? std::string_view(slot.values+entry.value_offset,entry.value_size) : std::string_view{});
  if(attribute=="current_position"||attribute=="current_tilt_position") {
    uint8_t percent;if(parse_bounded_percent(value,percent)){entry.percent=percent;entry.percent_known=true;}
  }
  if (entry.status == PageEntityStatus::AVAILABLE &&
      entry.value_size == value.size() &&
      std::string_view(slot.values + entry.value_offset, entry.value_size) ==
          value)
    return true;
  if (entry.status == PageEntityStatus::AVAILABLE && entry.value_size == value.size()) {
    ++revision_;++entry.revision;
    if (!value.empty()) std::memmove(slot.values + entry.value_offset, value.data(), value.size());
    return true; // Equal-width updates need not move any neighbouring packed
                 // state.
  }
  const size_t available = budget_.state_bytes - (slot.values_used - entry.value_size);
  ++revision_;
  ++entry.revision;
  if (value.size() > available || value.size() > budget_.channel_bytes) {
    remove_value(slot, entry);
    entry.status = PageEntityStatus::EXHAUSTED;
    ++memory_.state_overflows;
    return false;
  }
  // A borrowed value may come from this same packed pool. Remember its offset
  // before growth or compaction, rather than retaining a pointer to retired
  // RAM.
  const uintptr_t input = reinterpret_cast<uintptr_t>(value.data());
  const uintptr_t base = reinterpret_cast<uintptr_t>(slot.values);
  const bool borrowed = !value.empty() && input >= base &&
                        input - base < slot.values_used &&
                        value.size() <= slot.values_used - (input - base);
  const size_t input_offset = borrowed ? input - base : 0;
  const size_t required = slot.values_used - entry.value_size + value.size();
  if (!reserve_values(slot, required)) {
    remove_value(slot, entry);
    entry.status = PageEntityStatus::EXHAUSTED;
    return false;
  }
  const size_t offset =
      entry.value_size == 0 ? slot.values_used : entry.value_offset;
  const size_t tail = offset + entry.value_size;
  if (value.size() <= entry.value_size) {
    // Copy before shrinking: moving the tail left could overwrite a borrowed
    // input.
    if (!value.empty())
      std::memmove(slot.values + offset,
                   borrowed ? slot.values + input_offset : value.data(),
                   value.size());
    std::memmove(slot.values + offset + value.size(), slot.values + tail,
                 slot.values_used - tail);
  } else {
    const size_t growth = value.size() - entry.value_size;
    std::memmove(slot.values + tail + growth, slot.values + tail,
                 slot.values_used - tail);
    if (borrowed) {
      // Input crossing the old tail boundary now has two source fragments.
      // The left destination ends before the right source, so this order also
      // handles views spanning multiple packed values without scratch storage.
      const size_t prefix =
          input_offset < tail ? std::min(value.size(), tail - input_offset) : 0;
      if (prefix != 0)
        std::memmove(slot.values + offset, slot.values + input_offset, prefix);
      if (prefix != value.size())
        std::memmove(slot.values + offset + prefix,
                     slot.values + input_offset + prefix + growth,
                     value.size() - prefix);
    } else if (!value.empty()) {
      std::memmove(slot.values + offset, value.data(), value.size());
    }
  }
  // Appending an initial/copied value or resizing the final packed value
  // moves no neighbours. Avoid rescanning every channel for each copied key.
  if (tail != slot.values_used) {
    for (size_t i = 0; i < slot.channel_count; ++i) {
      auto &other = slot.channels[i];
      if (&other != &entry && other.value_size != 0 &&
          other.value_offset >= tail)
        other.value_offset = static_cast<uint32_t>(
            other.value_offset - entry.value_size + value.size());
    }
  }
  entry.value_offset = static_cast<uint32_t>(offset);
  entry.value_size = static_cast<uint32_t>(value.size());
  slot.values_used = required;
  entry.status = PageEntityStatus::AVAILABLE;
  return true;
}

bool PageEntityStateRuntime::reserve_values(Slot &slot, size_t required) {
  if (required <= slot.values_capacity)
    return true;
  if (required > budget_.state_bytes ||
      budget_.initial_state_bytes >= budget_.state_bytes)
    return false;
  size_t capacity = std::max(slot.values_capacity, size_t{1});
  while (capacity < required) {
    capacity =
        capacity > budget_.state_bytes / 2 ? budget_.state_bytes : capacity * 2;
  }
  auto *replacement = static_cast<char *>(allocator_.allocate(capacity));
  if (replacement == nullptr) {
    ++memory_.state_allocation_failures;
    return false;
  }
  record_preparation_peak(capacity);
  if (slot.values_used != 0)
    std::memcpy(replacement, slot.values, slot.values_used);
  if (slot.value_storage != nullptr)
    allocator_.release(slot.value_storage, slot.values_capacity);
  slot.value_storage = replacement;
  slot.values = replacement;
  slot.values_capacity = capacity;
  ++memory_.state_reservations;
  return true;
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
      entry.percent_known=false;
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
  if (card.index >= slot.card_count || slot.edge_count == 0) return 0;
  return slot.card_offsets[card.index + 1] - slot.card_offsets[card.index];
}
PageEntityValue PageEntityStateRuntime::dependency(ConfigurationHandle card, size_t ordinal) {
  synchronize_connection();
  if (active_ < 0 || slots_[active_].generation != card.generation) return {};
  const auto &slot = slots_[active_];
  if (card.index >= slot.card_count || slot.edge_count == 0) return {};
  const size_t begin = slot.card_offsets[card.index], end = slot.card_offsets[card.index + 1];
  if (ordinal >= end - begin) return {};
  const auto &entry = slot.channels[slot.edges[begin + ordinal].channel];
  return {entry.status, {slot.names + entry.entity_offset, entry.entity_size},
      {slot.names + entry.attribute_offset, entry.attribute_size},
      {slot.values + entry.value_offset, entry.value_size}, entry.revision};
}
std::optional<uint8_t> PageEntityStateRuntime::last_percent(ConfigurationHandle card,size_t ordinal) {
  synchronize_connection();
  if(active_<0||slots_[active_].generation!=card.generation)return std::nullopt;
  const auto &slot=slots_[active_];if(card.index>=slot.card_count||slot.edge_count==0)return std::nullopt;
  const auto begin=slot.card_offsets[card.index],end=slot.card_offsets[card.index+1];
  if(ordinal>=end-begin)return std::nullopt;
  const auto &entry=slot.channels[slot.edges[begin+ordinal].channel];
  return entry.percent_known?std::optional<uint8_t>(entry.percent):std::nullopt;
}
PageEntityMemory PageEntityStateRuntime::memory() const {
  auto result = memory_;
  if (active_ >= 0) {
    const auto &slot = slots_[active_];
    result.active_arena_bytes =
        slot.arena_size +
        (slot.value_storage != nullptr ? slot.values_capacity : 0);
    result.active_channels = slot.channel_count;
    result.active_dependencies = slot.edge_count;
    result.active_state_bytes = slot.values_used;
    result.active_state_capacity_bytes = slot.values_capacity;
  }
  if (staged_ >= 0) {
    const auto &slot = slots_[staged_];
    result.staged_arena_bytes =
        slot.arena_size +
        (slot.value_storage != nullptr ? slot.values_capacity : 0);
    result.staged_state_capacity_bytes = slot.values_capacity;
  }
  return result;
}
}  // namespace espcontrol::pages
