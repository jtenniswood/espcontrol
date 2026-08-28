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
  size_t subscription_channel_count() const { return subscription_channels_.size(); }
  size_t retained_channel_count() const {
    size_t count = 0;
    for (size_t i = 0; i < subscription_channels_.size(); i++) {
      if (channel_reuses_reads(i)) count++;
    }
    return count;
  }
  size_t pending_read_count() const {
    size_t count = 0;
    for (const auto &channel : subscription_channels_) count += channel.pending_reads.size();
    for (const auto &request : deferred_) count += request.callbacks.size();
    return count;
  }

  bool read_retained(const std::string &entity_id,
                     const std::string &attribute,
                     Callback callback,
                     bool has_attribute,
                     size_t min_free,
                     size_t min_largest,
                     void *owner = nullptr) {
    if (!available() || entity_id.empty() || !callback) return false;
    if (!heap_probe_.available("Home Assistant state request", min_free, min_largest)) return false;
    size_t channel = find_subscription_channel(entity_id, attribute, has_attribute);
    if (channel == subscription_channels_.size() || !channel_reuses_reads(channel)) return false;
    CallbackRef callback_ref{std::make_shared<Callback>(std::move(callback)), owner, owner_generation(owner)};
    if (callback_depth_ != 0) {
      return queue(channel, std::move(callback_ref));
    }
    return queue_on_subscription_channel(channel, std::move(callback_ref));
  }

  bool subscribe(const std::string &entity_id,
                 const std::string &attribute,
                 Callback callback,
                 uint32_t scope,
                 void *owner = nullptr,
                 bool retain_latest = false) {
    if (!available() || entity_id.empty() || !callback) return false;
    auto callback_ref = std::make_shared<Callback>(std::move(callback));
    size_t channel = find_subscription_channel(entity_id, attribute, true);
    const bool new_channel = channel == subscription_channels_.size();
    if (new_channel) {
      SubscriptionChannel subscription_channel;
      subscription_channel.entity_id = entity_id;
      subscription_channel.attribute = attribute;
      subscription_channels_.push_back(std::move(subscription_channel));
      // Transport callbacks retain this numeric index for the firmware
      // lifetime, so channels are append-only and their indices stay stable.
      transport_.subscribe(
          entity_id, attribute,
          [this, channel](State state) { invoke_subscription_channel(channel, state); });
    }
    subscriptions_.push_back({callback_ref, scope, owner, channel, retain_latest});
    // A grid rebuild re-subscribes on an existing channel; replay the last value
    // so rebuilt cards reflect the live state immediately.
    if (!new_channel && subscription_channels_[channel].has_last_state) {
      invoke(callback_ref, State(subscription_channels_[channel].last_state));
    }
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
      dispatch_many(request.channel, std::move(request.callbacks));
      processed++;
    }
  }

  void reset_deferred() {
    // Keep the small high-water allocation so reentrant refreshes do not
    // repeatedly release and regrow the same queue storage.
    deferred_.clear();
  }

  void invalidate_retained_state() {
    // Retained values are scoped to one Home Assistant API connection.  In
    // particular, artwork URLs and access tokens may change while the panel is
    // offline, so reads after a reconnect must wait for a fresh announcement.
    for (auto &channel : subscription_channels_) {
      channel.last_state.clear();
      channel.has_last_state = false;
      channel.cached_state.clear();
      channel.has_cached_state = false;
    }
  }

  void reset_subscriptions(uint32_t scope = 0) {
    if (callback_depth_ != 0) {
      // A callback can rebuild a card immediately after requesting its old
      // subscriptions be reset. Mark only the callbacks that exist now so the
      // replacements survive the deferred vector compaction.
      mark_subscriptions_for_release(scope);
      pending_subscription_compaction_ = true;
      release_inactive_channel_state();
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
      // Grid rebuilds can release an old card and attach its replacement to
      // the same persistent LVGL owner before this callback returns. Mark the
      // subscriptions that exist now; resolving by owner later would also
      // delete the replacement card's newly registered callbacks.
      mark_owner_subscriptions_for_release(owner);
      pending_subscription_compaction_ = true;
      release_inactive_channel_state();
      return;
    }
    release_owner_subscriptions(owner);
  }

 private:
  struct CallbackRef {
    std::shared_ptr<Callback> callback;
    void *owner = nullptr;
    // Only retained-read callbacks need this token. It prevents an old read
    // from being delivered after its LVGL owner address has been reused.
    uint32_t owner_generation = 0;
  };

  struct DeferredRequest {
    size_t channel = 0;
    std::vector<CallbackRef> callbacks;
    uint32_t generation = 0;
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
    bool pending_release = false;
  };

  struct SubscriptionChannel {
    std::string entity_id;
    std::string attribute;
    std::string last_state;
    bool has_last_state = false;
    std::string cached_state;
    std::vector<CallbackRef> pending_reads;
    bool has_cached_state = false;
  };

  static constexpr size_t MAX_DEFERRED_REQUESTS = 64;
  static constexpr size_t MAX_PENDING_READS = 64;
  static constexpr size_t MAX_REPLAY_STATES = 64;

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

  bool queue(size_t channel, CallbackRef callback) {
    for (auto &request : deferred_) {
      if (request.generation == generation_ &&
          request.channel == channel) {
        return queue_callback_ref(request.callbacks, std::move(callback));
      }
    }
    if (deferred_.size() >= MAX_DEFERRED_REQUESTS ||
        pending_read_count() >= MAX_PENDING_READS) return false;
    deferred_.push_back({channel, {std::move(callback)}, generation_});
    return true;
  }

  void dispatch_many(size_t channel, std::vector<CallbackRef> callbacks) {
    if (channel < subscription_channels_.size() && channel_reuses_reads(channel)) {
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
    }
  }

  void invoke(const std::shared_ptr<Callback> &callback, State state) {
    if (!callback || !*callback) return;
    callback_depth_++;
    (*callback)(state);
    callback_depth_--;
    if (callback_depth_ == 0 && pending_subscription_compaction_) {
      pending_subscription_compaction_ = false;
      compact_released_subscriptions();
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
      if (!ref.pending_release && ref.channel == channel &&
          ref.callback && *ref.callback) {
        callbacks.push_back(ref.callback);
        retain_latest = retain_latest || ref.retain_latest;
      }
    }
    if (callbacks.empty()) {
      subscription.last_state.clear();
      subscription.has_last_state = false;
    } else {
      retain_replay_state(channel, state);
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
    for (const auto &callback : callbacks) {
      // An earlier callback can reset this scope while the channel snapshot is
      // being dispatched. Skip callbacks marked by that reset, but keep the
      // currently executing std::function alive until it returns.
      if (subscription_callback_active(callback)) invoke(callback, state);
    }
    // Reentrant reads are deferred above, so this channel should still have an
    // empty pending list. Reacquire it by index because a callback may have
    // appended channels and reallocated subscription_channels_.
    pending_reads.clear();
    if (channel < subscription_channels_.size() &&
        subscription_channels_[channel].pending_reads.empty()) {
      pending_reads.swap(subscription_channels_[channel].pending_reads);
    }
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

  bool queue_on_subscription_channel(size_t channel, CallbackRef callback_ref) {
    if (channel == subscription_channels_.size() || !channel_reuses_reads(channel)) return false;
    SubscriptionChannel &subscription = subscription_channels_[channel];
    if (subscription.has_cached_state) {
      State state(subscription.cached_state);
      if (owner_generation_current(callback_ref)) invoke(callback_ref.callback, state);
    } else {
      return queue_pending_channel_read(subscription, std::move(callback_ref));
    }
    return true;
  }

  bool queue_pending_channel_read(SubscriptionChannel &subscription,
                                  CallbackRef callback_ref) {
    // Repeated refreshes for one card supersede its earlier pending callback.
    // Keep distinct card owners independent, but cap all retained-read work as
    // a final guard against channels that Home Assistant never publishes.
    return queue_callback_ref(subscription.pending_reads, std::move(callback_ref));
  }

  bool queue_callback_ref(std::vector<CallbackRef> &callbacks,
                          CallbackRef callback_ref) {
    for (auto &pending : callbacks) {
      if (callback_ref.owner != nullptr && pending.owner == callback_ref.owner) {
        pending = std::move(callback_ref);
        return true;
      }
    }
    if (pending_read_count() >= MAX_PENDING_READS) return false;
    callbacks.push_back(std::move(callback_ref));
    return true;
  }

  bool channel_reuses_reads(size_t channel) const {
    for (const auto &ref : subscriptions_) {
      if (!ref.pending_release && ref.channel == channel && ref.retain_latest &&
          ref.callback && *ref.callback) return true;
    }
    return false;
  }

  bool subscription_callback_active(const std::shared_ptr<Callback> &callback) const {
    for (const auto &ref : subscriptions_) {
      if (!ref.pending_release && ref.callback == callback && ref.callback && *ref.callback) return true;
    }
    return false;
  }

  void retain_replay_state(size_t channel, const State &state) {
    SubscriptionChannel &subscription = subscription_channels_[channel];
    if (!subscription.has_last_state) {
      size_t retained = 0;
      for (const auto &candidate : subscription_channels_) {
        if (candidate.has_last_state) retained++;
      }
      if (retained >= MAX_REPLAY_STATES) {
        for (size_t candidate = 0; candidate < subscription_channels_.size(); candidate++) {
          if (candidate == channel || !subscription_channels_[candidate].has_last_state) continue;
          subscription_channels_[candidate].last_state.clear();
          subscription_channels_[candidate].has_last_state = false;
          break;
        }
      }
    }
    subscription.last_state.assign(state.c_str(), state.size());
    subscription.has_last_state = true;
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
    mark_subscriptions_for_release(scope);
    compact_released_subscriptions();
    release_inactive_channel_state();
  }

  void mark_subscriptions_for_release(uint32_t scope) {
    for (SubscriptionRef &ref : subscriptions_) {
      if (scope != 0 && (ref.scope & scope) == 0) continue;
      ref.pending_release = true;
    }
  }

  void compact_released_subscriptions() {
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < subscriptions_.size(); read_index++) {
      SubscriptionRef &ref = subscriptions_[read_index];
      if (ref.pending_release) {
        // Compaction only runs outside a callback body, so it is now safe to
        // release the std::function target as well as its tracking entry.
        if (ref.callback && *ref.callback) *ref.callback = nullptr;
        continue;
      }
      if (write_index != read_index) subscriptions_[write_index] = std::move(ref);
      write_index++;
    }
    subscriptions_.resize(write_index);
    if (subscriptions_.empty()) std::vector<SubscriptionRef>().swap(subscriptions_);
  }

  void release_owner_subscriptions(void *owner) {
    mark_owner_subscriptions_for_release(owner);
    compact_released_subscriptions();
    release_inactive_channel_state();
  }

  void mark_owner_subscriptions_for_release(void *owner) {
    if (!owner) return;
    for (SubscriptionRef &ref : subscriptions_) {
      if (ref.owner == owner) ref.pending_release = true;
    }
  }

  Transport transport_;
  HeapProbe heap_probe_;
  std::vector<DeferredRequest> deferred_;
  std::vector<SubscriptionRef> subscriptions_;
  std::vector<SubscriptionChannel> subscription_channels_;
  std::vector<OwnerGeneration> owner_generations_;
  uint32_t generation_ = 1;
  uint32_t next_owner_generation_ = 1;
  bool pending_subscription_compaction_ = false;
  uint8_t callback_depth_ = 0;
};
