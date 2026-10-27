#pragma once

#include <limits>
#include <string>
#include <type_traits>

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {

template <typename Coordinator, typename = void>
struct HasUniqueSubscriptionBatch : std::false_type {};
template <typename Coordinator>
struct HasUniqueSubscriptionBatch<
    Coordinator, std::void_t<decltype(std::declval<Coordinator &>()
                                          .begin_unique_subscription_batch(
                                              nullptr, size_t{}))>>
    : std::true_type {};

// Reuse the application's deduplicated transport channels across staged and
// active configurations. Only the configuration cache retains these values;
// legacy widget/modal consumers can continue to share the same channels.
// PageEntityStateRuntime explicitly copies live values during staging. Skip
// coordinator initial replay so an unrelated retained consumer cannot revive
// metadata that the configuration-owned media lifecycle has invalidated.
template <typename Coordinator>
class CoordinatorPageEntitySubscriptions final
    : public PageEntitySubscriptions {
public:
  CoordinatorPageEntitySubscriptions(Coordinator &coordinator, uint32_t scope)
      : coordinator_(coordinator), scope_(scope) {}
  CoordinatorPageEntitySubscriptions(
      const CoordinatorPageEntitySubscriptions &) = delete;
  CoordinatorPageEntitySubscriptions &
  operator=(const CoordinatorPageEntitySubscriptions &) = delete;
  ~CoordinatorPageEntitySubscriptions() override {
    // The graph normally releases first. Revoking remaining leases also
    // makes copied callbacks harmless after this adapter's destruction.
    auto **link = &batches_;
    while (*link) {
      auto *batch = *link;
      if (batch->adapter != this) {
        link = &batch->next;
        continue;
      }
      *link = batch->next;
      auto *owner = batch->owner;
      *batch = {};
      coordinator_.release_owner(owner);
    }
  }
  bool prepare_batch(PageEntitySubscriptionBatch &batch, void *owner,
                     uint32_t revision, size_t channels,
                     Deliver deliver) override {
    if (batch.adapter || !owner || !deliver)
      return false;
    for (const auto *existing = batches_; existing; existing = existing->next)
      if (existing->adapter == this && existing->owner == owner)
        return false;
    if (!channels)
      return true;
    if (!next_token_ ||
        channels >
            uint64_t{std::numeric_limits<uint32_t>::max()} - next_token_ + 1)
      return false;
    batch = {batches_,
             this,
             owner,
             deliver,
             revision,
             next_token_,
             static_cast<uint32_t>(channels)};
    next_token_ = static_cast<uint32_t>(uint64_t{next_token_} + channels);
    batches_ = &batch;
    return true;
  }
  bool subscribe(const char *entity, const char *attribute, void *owner,
                 uint32_t revision, uint32_t channel,
                 Deliver deliver) override {
    const auto *batch = batches_;
    while (batch && (batch->adapter != this || batch->owner != owner))
      batch = batch->next;
    if (!batch || batch->revision != revision || batch->deliver != deliver ||
        channel >= batch->channels)
      return false;
    if constexpr (HasUniqueSubscriptionBatch<Coordinator>::value) {
      return coordinator_.subscribe_unique(
          std::string(entity), std::string(attribute),
          CallbackToken{batch->first_token + channel}, scope_, owner);
    } else {
      return coordinator_.subscribe(std::string(entity), std::string(attribute),
                                    CallbackToken{batch->first_token + channel},
                                    scope_, owner, false, false, false);
    }
  }
  size_t registration_scratch_bytes() const override {
    if constexpr (HasUniqueSubscriptionBatch<Coordinator>::value)
      return coordinator_.unique_subscription_scratch_bytes();
    return 0;
  }
  bool begin_registration_batch(void *scratch, size_t bytes) override {
    if constexpr (HasUniqueSubscriptionBatch<Coordinator>::value)
      return coordinator_.begin_unique_subscription_batch(scratch, bytes);
    return true;
  }
  void end_registration_batch() override {
    if constexpr (HasUniqueSubscriptionBatch<Coordinator>::value)
      coordinator_.end_unique_subscription_batch();
  }
  void release(void *owner) override {
    auto **link = &batches_;
    while (*link) {
      auto *batch = *link;
      if (batch->adapter != this || batch->owner != owner) {
        link = &batch->next;
        continue;
      }
      *link = batch->next;
      *batch = {};
    }
    coordinator_.release_owner(owner);
  }
  uint32_t connection_generation() const override {
    return coordinator_.connection_generation();
  }

private:
  // libstdc++ on the S3 stores only eight bytes inline. The old four-field
  // capture was sixteen bytes, allocating internal RAM for every channel.
  struct CallbackToken {
    uint32_t token;
    void operator()(typename Coordinator::State value) const {
      for (const auto *batch = batches_; batch; batch = batch->next) {
        if (token < batch->first_token ||
            token - batch->first_token >= batch->channels)
          continue;
        // Delivery may retire this lease or destroy its graph. Borrow no node
        // after entering consumer code, including on reentrant delivery.
        const auto deliver = batch->deliver;
        auto *owner = batch->owner;
        const auto revision = batch->revision;
        const auto channel = token - batch->first_token;
        deliver(owner, revision, channel, {value.c_str(), value.size()});
        return;
      }
    }
  };
  static_assert(sizeof(CallbackToken) == sizeof(uint32_t) &&
                std::is_trivially_copyable_v<CallbackToken>);
  // Tokens never repeat, even across adapters and graph address reuse. Nodes
  // live in the graph's two stable Slot objects, not its replaceable arenas.
  inline static PageEntitySubscriptionBatch *batches_{nullptr};
  inline static uint32_t next_token_{1};
  Coordinator &coordinator_;
  uint32_t scope_;
};
} // namespace espcontrol::pages
