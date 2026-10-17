#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

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
};

struct PageEntityBudget {
  size_t dependencies;
  size_t channels;
  size_t state_bytes;
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
  size_t preparation_failures{0};
  size_t state_overflows{0};
  size_t stale_deliveries{0};
};

// Configuration-owned state, independent of view bindings. Names, dependency
// edges and a packed state cache occupy one fallible arena per generation.
// Queries return borrowed views until the next delivery/reconnect/replacement;
// consumers pull state from the event loop rather than retaining these views.
// The source, allocator and subscriptions must outlive this owner.
class PageEntityStateRuntime final : public PageRuntimeResources {
 public:
  PageEntityStateRuntime(const PageDependencySource &source, PageArenaAllocator &allocator,
                        PageEntitySubscriptions &subscriptions, PageEntityBudget budget);
  ~PageEntityStateRuntime() override;
  PageEntityStateRuntime(const PageEntityStateRuntime &) = delete;
  PageEntityStateRuntime &operator=(const PageEntityStateRuntime &) = delete;

  bool prepare(const PageStore &store) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore &active) override { activate_prepared(active.generation()); }
  void activate_prepared(uint32_t generation);
  void retire_active() override;
  PageEntityValue dependency(ConfigurationHandle card, size_t ordinal);
  size_t dependency_count(ConfigurationHandle card) const;
  PageEntityMemory memory() const;
  // Lets the visible renderer skip unchanged frames. Polling also observes a
  // disconnect before a new HA value arrives, so stale values do not linger.
  uint64_t revision() { synchronize_connection(); return revision_; }
  void invalidate_disconnected_state();

 private:
  struct Channel {
    uint32_t entity_offset{0}, entity_size{0};
    uint32_t attribute_offset{0}, attribute_size{0};
    uint32_t value_offset{0}, value_size{0};
    uint32_t revision{0};
    PageEntityStatus status{PageEntityStatus::UNKNOWN};
  };
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
    size_t channel_count{0}, edge_count{0}, names_used{0}, values_used{0};
    uint32_t revision{0}, generation{0};
  };
  class Counter;
  class Builder;
  static void deliver(void *owner, uint32_t revision, uint32_t channel, std::string_view value);
  void receive(Slot &slot, uint32_t revision, uint32_t channel, std::string_view value);
  void release_slot(int index);
  void remove_value(Slot &slot, Channel &channel);
  bool index_dependencies(Slot &slot);
  void synchronize_connection();

  const PageDependencySource &source_;
  PageArenaAllocator &allocator_;
  PageEntitySubscriptions &subscriptions_;
  PageEntityBudget budget_;
  Slot slots_[2];
  int active_{-1}, staged_{-1};
  uint32_t next_revision_{1}, connection_generation_{0};
  PageEntityMemory memory_;
  uint64_t revision_{0};
};
}  // namespace espcontrol::pages
