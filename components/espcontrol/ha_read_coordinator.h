#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template<typename Transport, typename = void>
struct HaTransportOwnsSubscriptions : std::false_type {};
template<typename Transport>
struct HaTransportOwnsSubscriptions<Transport,
    std::void_t<decltype(Transport::RECLAIMABLE_SUBSCRIPTIONS)>>
    : std::bool_constant<Transport::RECLAIMABLE_SUBSCRIPTIONS> {};

// Instance-owned Home Assistant read state. Transport and HeapProbe are
// compile-time policies so production calls remain direct. Retained values and
// callback ownership outlive individual widgets; replay snapshots also survive
// a callback retiring the cache it is currently consuming.
template<typename Transport, typename HeapProbe,
         typename Allocator = std::allocator<std::byte>>
class HaReadCoordinator {
 public:
  using State = typename Transport::State;
  using Callback = typename Transport::Callback;
  template<typename T>
  using ReboundAllocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
  template<typename T>
  using AllocatedVector = std::vector<T, ReboundAllocator<T>>;

  explicit HaReadCoordinator(Transport transport = Transport(), HeapProbe heap_probe = HeapProbe())
      : transport_(std::move(transport)), heap_probe_(std::move(heap_probe)) {}

  ~HaReadCoordinator() {
    if constexpr (HaTransportOwnsSubscriptions<Transport>::value) {
      for (const auto &channel : subscription_channels_)
        transport_.unsubscribe(channel.transport_registration);
    }
  }
  HaReadCoordinator(const HaReadCoordinator &) = delete;
  HaReadCoordinator &operator=(const HaReadCoordinator &) = delete;

  // Maintenance runs after a complete grid rebuild, outside native dispatch.
  // Stable channel IDs keep old callbacks harmless when indices are compacted.
  size_t reclaim_unused_channels() {
    if constexpr (!HaTransportOwnsSubscriptions<Transport>::value) {
      return 0;  // Legacy host policies do not own their native registrations.
    } else {
      if (!channels_need_reclamation_ || callback_depth_ != 0 || channel_dispatch_depth_ != 0) return 0;
      const size_t before = subscription_channels_.size();
      size_t write = 0;
      for (size_t read = 0; read < before; ++read) {
        const bool live = std::any_of(subscriptions_.begin(), subscriptions_.end(),
            [read](const SubscriptionRef &ref) {
              return ref.channel == read && !ref.pending_release && ref.callback && *ref.callback;
            });
        if (!live) {
          transport_.unsubscribe(subscription_channels_[read].transport_registration);
          deferred_.erase(std::remove_if(deferred_.begin(), deferred_.end(),
              [read](const DeferredRequest &request) { return request.channel == read; }), deferred_.end());
          continue;
        }
        if (write != read) {
          subscription_channels_[write] = std::move(subscription_channels_[read]);
          for (auto &ref : subscriptions_) if (ref.channel == read) ref.channel = write;
          for (auto &request : deferred_) if (request.channel == read) request.channel = write;
        }
        ++write;
      }
      subscription_channels_.resize(write);
      trim_container(subscription_channels_);
      trim_container(subscriptions_);
      trim_container(owner_generations_);
      release_empty_deferred_storage();
      channels_need_reclamation_ = false;
      return before - write;
    }
  }

  Transport &transport() { return transport_; }
  const Transport &transport() const { return transport_; }
  HeapProbe &heap_probe() { return heap_probe_; }

  bool available() const { return transport_.available(); }
  bool state_connected() const { return transport_.state_connected(); }
  uint32_t generation() const { return generation_; }
  uint32_t connection_generation() const { return connection_generation_; }
  uint32_t &generation_ref() { return generation_; }
  size_t deferred_count() const { return deferred_.size(); }
  size_t subscription_count() const { return subscriptions_.size(); }
  size_t subscription_channel_count() const { return subscription_channels_.size(); }
  size_t subscription_channel_capacity() const {
    return subscription_channels_.capacity();
  }
  size_t persistent_container_capacity_bytes() const {
    size_t bytes = deferred_.capacity() * sizeof(DeferredRequest) +
                   subscriptions_.capacity() * sizeof(SubscriptionRef) +
                   subscription_channels_.capacity() * sizeof(SubscriptionChannel) +
                   owner_generations_.capacity() * sizeof(OwnerGeneration);
    for (const auto &request : deferred_) {
      bytes += request.callbacks.capacity() * sizeof(CallbackRef);
    }
    for (const auto &channel : subscription_channels_) {
      bytes += channel.pending_reads.capacity() * sizeof(CallbackRef);
    }
    return bytes;
  }
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

  // Refresh work lives on the existing entity/attribute channel. Consumers
  // share its debounce and only failed sends are retried, never absent values.
  bool schedule_fresh(const std::string &entity_id, const std::string &attribute,
                      uint32_t scope, uint32_t now) {
    const size_t channel = find_subscription_channel(entity_id, attribute, true);
    if (channel == subscription_channels_.size()) return false;
    const uint32_t scopes = active_channel_scopes(channel) & scope;
    if (scopes == 0) return false;
    auto &request = subscription_channels_[channel];
    request.refresh_scopes |= scopes;
    request.refresh_due = now + 100;
    request.fresh_request_pending = false;
    return true;
  }

  void cancel_fresh(uint32_t scope) {
    for (auto &channel : subscription_channels_) {
      channel.refresh_scopes &= ~scope;
    }
  }

  bool has_scheduled_fresh() const {
    for (const auto &channel : subscription_channels_) {
      if (channel.refresh_scopes != 0) return true;
    }
    return false;
  }

  void flush_fresh(uint32_t now, size_t min_free, size_t min_largest) {
    if (callback_depth_ != 0) return;
    size_t attempts = 0;
    for (size_t i = 0; i < subscription_channels_.size(); ++i) {
      auto &channel = subscription_channels_[i];
      channel.refresh_scopes &= active_channel_scopes(i);
      if (channel.refresh_scopes == 0 || static_cast<int32_t>(now - channel.refresh_due) < 0) continue;
      channel.refresh_due = now + 1000;
      if (!state_connected() ||
          !heap_probe_.available("fresh Home Assistant metadata request", min_free, min_largest)) continue;
      if (request_fresh(channel.entity_id, channel.attribute)) channel.refresh_scopes = 0;
      // Leave room for normal state traffic in the native API send queue.
      if (++attempts == 4) break;
    }
  }
  size_t transient_callback_capacity() const {
    size_t capacity = deferred_.capacity();
    for (const auto &request : deferred_) capacity += request.callbacks.capacity();
    for (const auto &channel : subscription_channels_) capacity += channel.pending_reads.capacity();
    return capacity;
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
    CallbackRef callback_ref{
        std::allocate_shared<Callback>(ReboundAllocator<Callback>{}, std::move(callback)),
        owner, owner_generation(owner)};
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
                 bool retain_latest = false,
                 bool retain_replay = true) {
    if (!available() || entity_id.empty() || !callback) return false;
    auto callback_ref =
        std::allocate_shared<Callback>(ReboundAllocator<Callback>{}, std::move(callback));
    size_t channel = find_subscription_channel(entity_id, attribute, true);
    const bool new_channel = channel == subscription_channels_.size();
    if (new_channel) {
      if (next_channel_id_ == 0) return false;
      SubscriptionChannel subscription_channel;
      const uint32_t id = next_channel_id_++;
      subscription_channel.id = id;
      subscription_channel.entity_id = entity_id;
      subscription_channel.attribute = attribute;
      subscription_channels_.push_back(std::move(subscription_channel));
      auto deliver = [this, id](State state) {
        for (size_t index = 0; index < subscription_channels_.size(); ++index) {
          if (subscription_channels_[index].id != id) continue;
          invoke_subscription_channel(index, state);
          return;
        }
      };
      if constexpr (HaTransportOwnsSubscriptions<Transport>::value) {
        const uint32_t token = transport_.subscribe(entity_id, attribute, std::move(deliver));
        if (token == 0) { subscription_channels_.pop_back(); return false; }
        subscription_channels_[channel].transport_registration = token;
      } else {
        transport_.subscribe(entity_id, attribute, std::move(deliver));
      }
    }
    subscriptions_.push_back({callback_ref, scope, owner, channel, retain_latest, false, retain_replay});
    // A grid rebuild re-subscribes on an existing channel; replay the last value
    // so rebuilt cards reflect the live state immediately.
    if (!new_channel && subscription_channels_[channel].has_last_state) {
      const std::string snapshot = subscription_channels_[channel].last_state;
      invoke(callback_ref, State(snapshot.c_str(), snapshot.size()));
    }
    return true;
  }

  bool request_fresh(const std::string &entity_id,
                    const std::string &attribute) {
    if (!state_connected() || entity_id.empty() || attribute.empty()) return false;
    const size_t channel = find_subscription_channel(entity_id, attribute, true);
    if (channel == subscription_channels_.size()) return false;
    bool has_active_callback = false;
    for (const auto &ref : subscriptions_) {
      if (!ref.pending_release && ref.channel == channel && ref.callback && *ref.callback) {
        has_active_callback = true;
        break;
      }
    }
    if (!has_active_callback) return false;
    if (subscription_channels_[channel].fresh_request_pending) return true;
    // Replies use the existing subscription dispatcher, with no additional
    // native callback or borrowed request strings retained after this call.
    subscription_channels_[channel].fresh_request_pending = true;
    if (transport_.request(entity_id, attribute)) return true;
    subscription_channels_[channel].fresh_request_pending = false;
    return false;
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
    release_empty_deferred_storage();
  }

  void reset_deferred() {
    AllocatedVector<DeferredRequest>().swap(deferred_);
  }

  void invalidate_retained_state() {
    ++connection_generation_;
    // Retained values are scoped to one Home Assistant API connection.  In
    // particular, artwork URLs and access tokens may change while the panel is
    // offline, so reads after a reconnect must wait for a fresh announcement.
    for (auto &channel : subscription_channels_) {
      channel.fresh_request_pending = false;
      release_string_storage(channel.last_state);
      channel.has_last_state = false;
      release_string_storage(channel.cached_state);
      channel.has_cached_state = false;
    }
  }

  void reset_subscriptions(uint32_t scope = 0) {
    channels_need_reclamation_ = true;
    cancel_fresh(scope == 0 ? UINT32_MAX : scope);
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
      channel.fresh_request_pending = false;
      release_callback_storage(channel.pending_reads);
      release_string_storage(channel.cached_state);
      channel.has_cached_state = false;
    }
    reset_subscriptions(default_scope);
  }

  void release_owner(void *owner) {
    if (!owner) return;
    channels_need_reclamation_ = true;
    release_owner_generation(owner);
    for (auto &channel : subscription_channels_) {
      channel.pending_reads.erase(
          std::remove_if(channel.pending_reads.begin(), channel.pending_reads.end(),
                         [owner](const CallbackRef &ref) { return ref.owner == owner; }),
          channel.pending_reads.end());
      if (channel.pending_reads.empty()) release_callback_storage(channel.pending_reads);
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
    AllocatedVector<CallbackRef> callbacks;
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
    // Configuration-owned consumers keep state in their own bounded cache.
    // Existing widget consumers retain the legacy rebuild replay behaviour.
    bool retain_replay = true;
  };

  struct SubscriptionChannel {
    uint32_t id = 0;
    uint32_t transport_registration = 0;
    std::string entity_id;
    std::string attribute;
    std::string last_state;
    bool has_last_state = false;
    std::string cached_state;
    AllocatedVector<CallbackRef> pending_reads;
    bool has_cached_state = false;
    bool fresh_request_pending = false;
    uint32_t refresh_scopes = 0;
    uint32_t refresh_due = 0;
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

  void dispatch_many(size_t channel, AllocatedVector<CallbackRef> callbacks) {
    if (channel < subscription_channels_.size() && channel_reuses_reads(channel)) {
      auto &subscription = subscription_channels_[channel];
      if (subscription.has_cached_state) {
        const std::string snapshot = subscription.cached_state;
        State state(snapshot.c_str(), snapshot.size());
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
    struct Guard {
      uint32_t &depth;
      explicit Guard(uint32_t &value) : depth(value) { ++depth; }
      ~Guard() { --depth; }
    } guard(channel_dispatch_depth_);
    SubscriptionChannel &subscription = subscription_channels_[channel];
    subscription.fresh_request_pending = false;
    AllocatedVector<CallbackRef> pending_reads;
    pending_reads.swap(subscription.pending_reads);
    AllocatedVector<std::shared_ptr<Callback>> callbacks;
    size_t matching_callbacks = 0;
    for (const auto &ref : subscriptions_) {
      if (!ref.pending_release && ref.channel == channel &&
          ref.callback && *ref.callback) matching_callbacks++;
    }
    callbacks.reserve(matching_callbacks);
    bool retain_latest = false;
    bool retain_replay = false;
    for (const auto &ref : subscriptions_) {
      if (!ref.pending_release && ref.channel == channel &&
          ref.callback && *ref.callback) {
        callbacks.push_back(ref.callback);
        retain_latest = retain_latest || ref.retain_latest;
        retain_replay = retain_replay || ref.retain_replay;
      }
    }
    if (!retain_replay) {
      release_string_storage(subscription.last_state);
      subscription.has_last_state = false;
    } else {
      retain_replay_state(channel, state);
    }
    if (retain_latest) {
      subscription.cached_state.assign(state.c_str(), state.size());
      subscription.has_cached_state = true;
    } else {
      release_string_storage(subscription.cached_state);
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
      // A callback can retire its owner or invalidate retained values before
      // it finishes reading this view. Keep the delivered bytes alive locally.
      const std::string snapshot = subscription.cached_state;
      State state(snapshot.c_str(), snapshot.size());
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

  bool queue_callback_ref(AllocatedVector<CallbackRef> &callbacks,
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

  uint32_t active_channel_scopes(size_t channel) const {
    uint32_t scopes = 0;
    for (const auto &ref : subscriptions_) {
      if (!ref.pending_release && ref.channel == channel && ref.callback && *ref.callback) scopes |= ref.scope;
    }
    return scopes;
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
          release_string_storage(subscription_channels_[candidate].last_state);
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
      bool active = false, replay = false;
      for (const auto &ref : subscriptions_) {
        if (ref.channel != channel || ref.pending_release || !ref.callback || !*ref.callback) continue;
        active = true;
        replay = replay || ref.retain_replay;
      }
      if (active && !replay) {
        release_string_storage(subscription_channels_[channel].last_state);
        subscription_channels_[channel].has_last_state = false;
      }
      if (channel_reuses_reads(channel)) continue;
      SubscriptionChannel &subscription = subscription_channels_[channel];
      release_string_storage(subscription.cached_state);
      release_callback_storage(subscription.pending_reads);
      subscription.has_cached_state = false;
    }
  }

  static void release_callback_storage(AllocatedVector<CallbackRef> &callbacks) {
    AllocatedVector<CallbackRef>().swap(callbacks);
  }

  static void release_string_storage(std::string &value) {
    std::string().swap(value);
  }

  template<typename T>
  static void trim_container(AllocatedVector<T> &values) {
    if (values.empty()) { AllocatedVector<T>().swap(values); return; }
    if (values.capacity() <= std::max(size_t{8}, values.size() * 2)) return;
    AllocatedVector<T> replacement;
    replacement.reserve(values.size());
    for (auto &value : values) replacement.push_back(std::move(value));
    replacement.swap(values);
  }

  void release_empty_deferred_storage() {
    if (!deferred_.empty() || deferred_.capacity() == 0) return;
    AllocatedVector<DeferredRequest>().swap(deferred_);
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
    if (subscriptions_.empty()) AllocatedVector<SubscriptionRef>().swap(subscriptions_);
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
  AllocatedVector<DeferredRequest> deferred_;
  AllocatedVector<SubscriptionRef> subscriptions_;
  AllocatedVector<SubscriptionChannel> subscription_channels_;
  AllocatedVector<OwnerGeneration> owner_generations_;
  uint32_t generation_ = 1;
  uint32_t connection_generation_ = 1;
  uint32_t next_owner_generation_ = 1;
  uint32_t next_channel_id_ = 1;
  uint32_t channel_dispatch_depth_ = 0;
  bool channels_need_reclamation_ = false;
  bool pending_subscription_compaction_ = false;
  uint8_t callback_depth_ = 0;
};
