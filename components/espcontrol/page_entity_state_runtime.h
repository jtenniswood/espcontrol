#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <optional>

#include "page_store_runtime.h"

namespace espcontrol::pages {

class PageDependencySink {
 public:
  virtual ~PageDependencySink() = default;
  virtual bool add(CardId card, std::string_view entity, std::string_view attribute) = 0;
};

// A driver enumerates its dependencies without constructing widgets. Both
// passes must enumerate the same definition; failure rejects the replacement.
class PageDependencySource {
 public:
  virtual ~PageDependencySource() = default;
  virtual bool enumerate(const PageStore &store, PageDependencySink &sink) const = 0;
};

class PageEntitySubscriptions {
 public:
  using Deliver = void (*)(void *, uint32_t, uint32_t, std::string_view);
  virtual ~PageEntitySubscriptions() = default;
  virtual bool subscribe(const char *entity, const char *attribute, void *owner,
                         uint32_t revision, uint32_t channel, Deliver deliver) = 0;
  // Immediately cancel callbacks for this owner, including retained replay.
  virtual void release(void *owner) = 0;
  virtual uint32_t connection_generation() const = 0;
  // Local producers seed each replacement directly, including initial empty-text semantics.
  virtual bool copy_shared_values() const { return true; }
};

struct PageEntityBudget {
  size_t dependencies;
  size_t channels;
  size_t state_bytes;
  size_t channel_bytes{SIZE_MAX};
  // Default keeps the complete allocation-free state reserve. A smaller
  // initial capacity opts into a separately owned, demand-growing value pool;
  // state_bytes and channel_bytes remain the same hard admission ceilings.
  size_t initial_state_bytes{SIZE_MAX};
};

enum class PageEntityStatus : uint8_t { STALE, UNKNOWN, AVAILABLE, EXHAUSTED };
struct PageEntityValue {
  PageEntityStatus status{PageEntityStatus::STALE};
  std::string_view entity;
  std::string_view attribute;
  std::string_view value;
  uint32_t revision{0};
};
struct PageEntityMemory {
  size_t active_arena_bytes{0};
  size_t staged_arena_bytes{0};
  size_t peak_arena_bytes{0};
  size_t active_channels{0};
  size_t active_dependencies{0};
  size_t active_state_bytes{0};
  size_t active_state_capacity_bytes{0};
  size_t staged_state_capacity_bytes{0};
  size_t state_reservations{0};
  size_t state_allocation_failures{0};
  size_t preparation_failures{0};
  size_t state_overflows{0};
  size_t stale_deliveries{0};
};

class PageEntityObserver {
 public:
  virtual ~PageEntityObserver()=default;
  // Synchronous notification on the application loop. Views last only for this
  // call. Observers record bounded state only: they must not dispatch actions,
  // replace configurations, or mutate this cache from inside the notification.
  // Copied values seed a staged cache; they are not new network observations.
  virtual void observed(uint32_t configuration,std::string_view entity,
      std::string_view attribute,std::string_view value,bool copied)=0;
  // Previous bytes are borrowed from the packed cache before replacement; empty
  // when no available value exists. Observers may compare, never retain them.
  virtual void observed_update(uint32_t configuration, std::string_view entity,
      std::string_view attribute, std::string_view value, bool copied, std::string_view previous) {
    (void)previous;
    observed(configuration, entity, attribute, value, copied);
  }
};

// Configuration-owned state, independent of view bindings. Names, dependency
// edges and a packed state cache occupy fallible generation-owned storage.
// Fixed mode embeds values in the metadata arena; elastic mode reserves its
// separate value pool during preparation and can grow on network delivery.
// Activation, navigation and updates within the current capacity allocate
// nothing. Queries return borrowed views until the next
// delivery/reconnect/replacement; consumers pull state from the event loop
// rather than retaining these views. The source, allocator and subscriptions
// must outlive this owner.
class PageEntityStateRuntime final : public PageRuntimeResources {
 public:
  PageEntityStateRuntime(const PageDependencySource &source, PageArenaAllocator &allocator,
                        PageEntitySubscriptions &subscriptions, PageEntityBudget budget,
                        PageEntityObserver *observer=nullptr);
  ~PageEntityStateRuntime() override;
  PageEntityStateRuntime(const PageEntityStateRuntime &) = delete;
  PageEntityStateRuntime &operator=(const PageEntityStateRuntime &) = delete;

  bool prepare(const PageStore &store) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore &active) override { activate_prepared(active.generation()); }
  void activate_prepared(uint32_t generation);
  void retire_active() override;
  PageEntityValue dependency(ConfigurationHandle card, size_t ordinal);
  // A named query still follows this card's declared edges; another card
  // sharing the same channel cannot grant access to an undeclared attribute.
  PageEntityValue read_attribute(ConfigurationHandle card, std::string_view entity,
                                 std::string_view attribute) {
    const auto count = dependency_count(card);
    for (size_t ordinal = 0; ordinal < count; ++ordinal) {
      const auto value = dependency(card, ordinal);
      if (value.entity == entity && value.attribute == attribute) return value;
    }
    return {};
  }
  std::optional<uint8_t> last_percent(ConfigurationHandle card,size_t ordinal);
  size_t dependency_count(ConfigurationHandle card) const;
  PageEntityMemory memory() const;
  // Lets the visible renderer skip unchanged frames. Polling also observes a
  // disconnect before a new HA value arrives, so stale values do not linger.
  uint64_t revision() { synchronize_connection(); return revision_; }
  uint32_t connection_generation() const {
    return subscriptions_.connection_generation();
  }
  void invalidate_disconnected_state();

 private:
  struct Channel {
    uint32_t entity_offset{0}, entity_size{0};
    uint32_t attribute_offset{0}, attribute_size{0};
    uint32_t value_offset{0}, value_size{0};
    uint32_t revision{0};
    PageEntityStatus status{PageEntityStatus::UNKNOWN};
    uint8_t percent{0};
    bool percent_known{false};
  };
  static_assert(sizeof(Channel)==32,"Percent history must fit existing channel padding");
  struct Edge { CardId card; uint32_t channel; };
  struct Slot {
    PageEntityStateRuntime *runtime{nullptr};
    void *arena{nullptr};
    size_t arena_size{0};
    Channel *channels{nullptr};
    Edge *edges{nullptr};
    uint32_t *card_offsets{nullptr};
    size_t card_count{0};
    char *names{nullptr}, *values{nullptr};
    void *value_storage{nullptr};
    size_t values_capacity{0};
    size_t channel_count{0}, edge_count{0}, names_used{0}, values_used{0};
    uint32_t revision{0}, generation{0};
  };
  class Counter;
  class Builder;
  static void deliver(void *owner, uint32_t revision, uint32_t channel, std::string_view value);
  bool receive(Slot &slot, uint32_t revision, uint32_t channel,
               std::string_view value, bool copied = false);
  bool reserve_values(Slot &slot, size_t required);
  void release_slot(int index);
  void remove_value(Slot &slot, Channel &channel);
  bool index_dependencies(Slot &slot);
  void synchronize_connection();

  const PageDependencySource &source_;
  PageArenaAllocator &allocator_;
  PageEntitySubscriptions &subscriptions_;
  PageEntityBudget budget_;
  PageEntityObserver *observer_;
  Slot slots_[2];
  int active_{-1}, staged_{-1};
  uint32_t next_revision_{1}, connection_generation_{0};
  PageEntityMemory memory_;
  uint64_t revision_{0};
};
}  // namespace espcontrol::pages
