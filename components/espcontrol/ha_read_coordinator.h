#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Instance-owned Home Assistant read state. Transport and HeapProbe are
// compile-time policies so production calls remain direct and allocation-free
// beyond the callback ownership already required by ESPHome's API.
template<typename Transport, typename HeapProbe>
class HaReadCoordinator {
 public:
  using State = typename Transport::State;
  using Callback = typename Transport::Callback;

  explicit HaReadCoordinator(Transport transport = Transport(), HeapProbe heap_probe = HeapProbe())
      : transport_(std::move(transport)), heap_probe_(std::move(heap_probe)) {}

  Transport &transport() { return transport_; }
  const Transport &transport() const { return transport_; }
  HeapProbe &heap_probe() { return heap_probe_; }

  bool available() const { return transport_.available(); }
  bool state_connected() const { return transport_.state_connected(); }
  uint32_t generation() const { return generation_; }
  uint32_t &generation_ref() { return generation_; }
  size_t deferred_count() const { return deferred_.size(); }
  size_t subscription_count() const { return subscriptions_.size(); }

  bool get(const std::string &entity_id,
           const std::string &attribute,
           Callback callback,
           bool has_attribute,
           size_t min_free,
           size_t min_largest,
           void *owner = nullptr) {
    if (!available() || entity_id.empty() || !callback) return false;
    if (!heap_probe_.available("Home Assistant state request", min_free, min_largest)) return false;
    CallbackRef callback_ref{std::make_shared<Callback>(std::move(callback)), owner, owner_generation(owner)};
    if (callback_depth_ != 0 || !state_connected()) {
      return queue(entity_id, attribute, std::move(callback_ref), has_attribute);
    }
    dispatch_one(entity_id, attribute, std::move(callback_ref), has_attribute, generation_);
    return true;
  }

  bool subscribe(const std::string &entity_id,
                 const std::string &attribute,
                 Callback callback,
                 uint32_t scope,
                 void *owner = nullptr,
                 bool retain_latest = false) {
    if (!available() || entity_id.empty() || !callback) return false;
    auto callback_ref = std::make_shared<Callback>(std::move(callback));
    size_t channel = subscription_channels_.size();
    for (size_t i = 0; i < subscription_channels_.size(); i++) {
      if (subscription_channels_[i].entity_id == entity_id &&
          subscription_channels_[i].attribute == attribute) {
        channel = i;
        break;
      }
    }
    if (channel == subscription_channels_.size()) {
      SubscriptionChannel subscription_channel;
      subscription_channel.entity_id = entity_id;
      subscription_channel.attribute = attribute;
      subscription_channels_.push_back(std::move(subscription_channel));
      transport_.subscribe(
          entity_id, attribute,
          [this, channel](State state) { invoke_subscription_channel(channel, state); });
    }
    subscriptions_.push_back({callback_ref, scope, owner, channel, retain_latest});
    return true;
  }

  void flush(size_t max_requests,
             size_t min_free,
             size_t min_largest) {
    if (callback_depth_ != 0 || !state_connected()) return;
    size_t processed = 0;
    while (!deferred_.empty() && processed < max_requests) {
      DeferredRequest request = std::move(deferred_.front());
      deferred_.erase(deferred_.begin());
      if (request.callbacks.empty() || request.generation != generation_) continue;
      if (!heap_probe_.available(
              "deferred Home Assistant state request", min_free, min_largest)) {
        deferred_.insert(deferred_.begin(), std::move(request));
        return;
      }
      dispatch_many(
          std::move(request.entity_id), std::move(request.attribute),
          std::move(request.callbacks), request.has_attribute, request.generation);
      processed++;
    }
    release_empty_deferred_storage();
  }

  void reset_deferred() {
    std::vector<DeferredRequest>().swap(deferred_);
  }

  void invalidate_retained_state() {
    // Retained values are scoped to one Home Assistant API connection.  In
    // particular, artwork URLs and access tokens may change while the panel is
    // offline, so reads after a reconnect must wait for a fresh announcement.
    for (auto &channel : subscription_channels_) {
      channel.cached_state.clear();
      channel.has_cached_state = false;
    }
  }

  void reset_subscriptions(uint32_t scope = 0) {
    if (callback_depth_ != 0) {
      pending_reset_mask_ = scope == 0 ? UINT32_MAX : (pending_reset_mask_ | scope);
      return;
    }
    release_subscriptions(scope);
  }

  void bump_generation(uint32_t default_scope) {
    generation_++;
    if (generation_ == 0) generation_ = 1;
    reset_deferred();
    for (auto &channel : subscription_channels_) {
      channel.pending_reads.clear();
      channel.cached_state.clear();
      channel.has_cached_state = false;
    }
    reset_subscriptions(default_scope);
  }

  void release_owner(void *owner) {
    if (!owner) return;
    release_owner_generation(owner);
    for (auto &channel : subscription_channels_) {
      channel.pending_reads.erase(
          std::remove_if(channel.pending_reads.begin(), channel.pending_reads.end(),
                         [owner](const CallbackRef &ref) { return ref.owner == owner; }),
          channel.pending_reads.end());
    }
    if (callback_depth_ != 0) {
      pending_owner_releases_.push_back(owner);
      return;
    }
    release_owner_subscriptions(owner);
  }

 private:
  struct CallbackRef {
    std::shared_ptr<Callback> callback;
    void *owner = nullptr;
    uint32_t owner_generation = 0;
  };

  struct DeferredRequest {
    std::string entity_id;
    std::string attribute;
    std::vector<CallbackRef> callbacks;
    uint32_t generation = 0;
    bool has_attribute = false;
  };

  struct OwnerGeneration {
    void *owner = nullptr;
    uint32_t generation = 0;
  };

  struct SubscriptionRef {
    std::shared_ptr<Callback> callback;
    uint32_t scope = 0;
    void *owner = nullptr;
    size_t channel = 0;
    bool retain_latest = false;
  };

  struct SubscriptionChannel {
    std::string entity_id;
    std::string attribute;
    std::string cached_state;
    std::vector<CallbackRef> pending_reads;
    bool has_cached_state = false;
  };

  static constexpr size_t MAX_DEFERRED_REQUESTS = 64;
  static constexpr size_t MAX_PENDING_CHANNEL_READS = 64;

  uint32_t owner_generation(void *owner) {
    if (!owner) return 0;
    for (const auto &entry : owner_generations_) {
      if (entry.owner == owner) return entry.generation;
    }
    uint32_t generation = next_owner_generation_++;
    if (generation == 0) generation = next_owner_generation_++;
    owner_generations_.push_back({owner, generation});
    return generation;
  }

  bool owner_generation_current(const CallbackRef &callback_ref) const {
    if (!callback_ref.owner) return true;
    for (const auto &entry : owner_generations_) {
      if (entry.owner == callback_ref.owner) return entry.generation == callback_ref.owner_generation;
    }
    return false;
  }

  void release_owner_generation(void *owner) {
    owner_generations_.erase(
        std::remove_if(owner_generations_.begin(), owner_generations_.end(),
                       [owner](const OwnerGeneration &entry) { return entry.owner == owner; }),
        owner_generations_.end());
  }

  bool queue(const std::string &entity_id,
             const std::string &attribute,
             CallbackRef callback,
             bool has_attribute) {
    for (auto &request : deferred_) {
      if (request.generation == generation_ &&
          request.has_attribute == has_attribute &&
          request.entity_id == entity_id &&
          request.attribute == attribute) {
        request.callbacks.push_back(std::move(callback));
        return true;
      }
    }
    if (deferred_.size() >= MAX_DEFERRED_REQUESTS) return false;
    deferred_.push_back({entity_id, attribute, {std::move(callback)}, generation_, has_attribute});
    return true;
  }

  void dispatch_one(std::string entity_id,
                    std::string attribute,
                    CallbackRef callback_ref,
                    bool has_attribute,
                    uint32_t generation) {
    if (queue_on_subscription_channel(entity_id, attribute, callback_ref, has_attribute)) {
      return;
    }
    transport_.get(
        std::move(entity_id), has_attribute ? std::move(attribute) : std::string(),
        [this, callback_ref, generation](State state) {
          if (generation == generation_ && owner_generation_current(callback_ref)) {
            invoke(callback_ref.callback, state);
          }
        });
  }

  void dispatch_many(std::string entity_id,
                     std::string attribute,
                     std::vector<CallbackRef> callbacks,
                     bool has_attribute,
                     uint32_t generation) {
    size_t channel = find_subscription_channel(entity_id, attribute, has_attribute);
    if (channel != subscription_channels_.size() && channel_reuses_reads(channel)) {
      auto &subscription = subscription_channels_[channel];
      if (subscription.has_cached_state) {
        State state(subscription.cached_state);
        for (const auto &callback_ref : callbacks) {
          if (owner_generation_current(callback_ref)) invoke(callback_ref.callback, state);
        }
      } else {
        for (auto &callback_ref : callbacks) {
          queue_pending_channel_read(subscription, std::move(callback_ref));
        }
      }
      return;
    }
    auto callback_refs = std::make_shared<std::vector<CallbackRef>>(std::move(callbacks));
    transport_.get(
        std::move(entity_id), has_attribute ? std::move(attribute) : std::string(),
        [this, callback_refs, generation](State state) {
          if (generation != generation_) return;
          for (const auto &callback_ref : *callback_refs) {
            if (owner_generation_current(callback_ref)) invoke(callback_ref.callback, state);
          }
        });
  }

  void invoke(const std::shared_ptr<Callback> &callback, State state) {
    if (!callback || !*callback) return;
    callback_depth_++;
    (*callback)(state);
    callback_depth_--;
    if (callback_depth_ == 0 && pending_reset_mask_ != 0) {
      uint32_t mask = pending_reset_mask_;
      pending_reset_mask_ = 0;
      release_subscriptions(mask == UINT32_MAX ? 0 : mask);
    }
    if (callback_depth_ == 0 && !pending_owner_releases_.empty()) {
      std::vector<void *> owners;
      owners.swap(pending_owner_releases_);
      for (void *owner : owners) release_owner_subscriptions(owner);
    }
  }

  void invoke_subscription_channel(size_t channel, State state) {
    if (channel >= subscription_channels_.size()) return;
    SubscriptionChannel &subscription = subscription_channels_[channel];
    std::vector<CallbackRef> pending_reads;
    pending_reads.swap(subscription.pending_reads);
    std::vector<std::shared_ptr<Callback>> callbacks;
    callbacks.reserve(subscriptions_.size());
    bool retain_latest = false;
    for (const auto &ref : subscriptions_) {
      if (ref.channel == channel && ref.callback && *ref.callback) {
        callbacks.push_back(ref.callback);
        retain_latest = retain_latest || ref.retain_latest;
      }
    }
    if (retain_latest) {
      subscription.cached_state.assign(state.c_str(), state.size());
      subscription.has_cached_state = true;
    } else {
      subscription.cached_state.clear();
      subscription.has_cached_state = false;
    }
    for (const auto &callback_ref : pending_reads) {
      if (owner_generation_current(callback_ref)) invoke(callback_ref.callback, state);
    }
    for (const auto &callback : callbacks) invoke(callback, state);
  }

  size_t find_subscription_channel(const std::string &entity_id,
                                   const std::string &attribute,
                                   bool has_attribute) const {
    const std::string expected_attribute = has_attribute ? attribute : std::string();
    for (size_t i = 0; i < subscription_channels_.size(); i++) {
      if (subscription_channels_[i].entity_id == entity_id &&
          subscription_channels_[i].attribute == expected_attribute) {
        return i;
      }
    }
    return subscription_channels_.size();
  }

  bool queue_on_subscription_channel(const std::string &entity_id,
                                      const std::string &attribute,
                                      CallbackRef callback_ref,
                                      bool has_attribute) {
    size_t channel = find_subscription_channel(entity_id, attribute, has_attribute);
    if (channel == subscription_channels_.size() || !channel_reuses_reads(channel)) return false;
    SubscriptionChannel &subscription = subscription_channels_[channel];
    if (subscription.has_cached_state) {
      State state(subscription.cached_state);
      if (owner_generation_current(callback_ref)) invoke(callback_ref.callback, state);
    } else {
      queue_pending_channel_read(subscription, std::move(callback_ref));
    }
    return true;
  }

  void queue_pending_channel_read(SubscriptionChannel &subscription,
                                  CallbackRef callback_ref) {
    // Repeated refreshes for one card supersede its earlier pending callback.
    // Keep distinct card owners independent, but cap them as a final guard
    // against a channel that Home Assistant never publishes.
    for (auto &pending : subscription.pending_reads) {
      if (callback_ref.owner != nullptr && pending.owner == callback_ref.owner) {
        pending = std::move(callback_ref);
        return;
      }
    }
    if (subscription.pending_reads.size() >= MAX_PENDING_CHANNEL_READS) {
      subscription.pending_reads.erase(subscription.pending_reads.begin());
    }
    subscription.pending_reads.push_back(std::move(callback_ref));
  }

  bool channel_reuses_reads(size_t channel) const {
    for (const auto &ref : subscriptions_) {
      if (ref.channel == channel && ref.retain_latest && ref.callback && *ref.callback) return true;
    }
    return false;
  }

  void release_inactive_channel_state() {
    for (size_t channel = 0; channel < subscription_channels_.size(); channel++) {
      if (channel_reuses_reads(channel)) continue;
      SubscriptionChannel &subscription = subscription_channels_[channel];
      subscription.cached_state.clear();
      subscription.pending_reads.clear();
      subscription.has_cached_state = false;
    }
  }

  void release_subscriptions(uint32_t scope) {
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < subscriptions_.size(); read_index++) {
      SubscriptionRef &ref = subscriptions_[read_index];
      if (scope == 0 || (ref.scope & scope) != 0) {
        if (ref.callback && *ref.callback) *ref.callback = nullptr;
        continue;
      }
      if (write_index != read_index) subscriptions_[write_index] = std::move(ref);
      write_index++;
    }
    subscriptions_.resize(write_index);
    if (subscriptions_.empty()) std::vector<SubscriptionRef>().swap(subscriptions_);
    release_inactive_channel_state();
  }

  void release_owner_subscriptions(void *owner) {
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < subscriptions_.size(); read_index++) {
      SubscriptionRef &ref = subscriptions_[read_index];
      if (ref.owner == owner) {
        if (ref.callback && *ref.callback) *ref.callback = nullptr;
        continue;
      }
      if (write_index != read_index) subscriptions_[write_index] = std::move(ref);
      write_index++;
    }
    subscriptions_.resize(write_index);
    if (subscriptions_.empty()) std::vector<SubscriptionRef>().swap(subscriptions_);
    release_inactive_channel_state();
  }

  void release_empty_deferred_storage() {
    if (!deferred_.empty() || deferred_.capacity() == 0) return;
    std::vector<DeferredRequest>().swap(deferred_);
  }

  Transport transport_;
  HeapProbe heap_probe_;
  std::vector<DeferredRequest> deferred_;
  std::vector<SubscriptionRef> subscriptions_;
  std::vector<SubscriptionChannel> subscription_channels_;
  std::vector<OwnerGeneration> owner_generations_;
  uint32_t generation_ = 1;
  uint32_t next_owner_generation_ = 1;
  uint32_t pending_reset_mask_ = 0;
  std::vector<void *> pending_owner_releases_;
  uint8_t callback_depth_ = 0;
};
