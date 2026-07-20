#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Coordinates every EspControl Home Assistant read through one persistent
// transport channel per entity/attribute. Generations describe the channels
// used by the current card tree; replacing a generation can therefore recycle
// stale ESPHome API subscriptions instead of retaining them for the life of the
// device.
template<typename Transport, typename HeapProbe>
class HaReadCoordinator {
 public:
  using State = typename Transport::State;
  using Callback = typename Transport::Callback;
  using Key = std::pair<std::string, std::string>;

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
  size_t channel_count() const { return channels_.size(); }
  bool transport_reset_pending() const { return transport_reset_pending_; }

  bool get(const std::string &entity_id,
           const std::string &attribute,
           Callback callback,
           bool has_attribute,
           size_t min_free,
           size_t min_largest) {
    if (!available() || entity_id.empty() || !callback) return false;
    if (!heap_probe_.available("Home Assistant state request", min_free, min_largest)) return false;
    auto callback_ref = std::make_shared<Callback>(std::move(callback));
    if (callback_depth_ != 0) {
      return queue(entity_id, attribute, std::move(callback_ref), has_attribute);
    }
    return attach_get(entity_id, has_attribute ? attribute : std::string(), std::move(callback_ref));
  }

  bool subscribe(const std::string &entity_id,
                 const std::string &attribute,
                 Callback callback,
                 uint32_t scope,
                 size_t min_free,
                 size_t min_largest) {
    if (!available() || entity_id.empty() || !callback) return false;
    if (!heap_probe_.available("Home Assistant state subscription", min_free, min_largest)) return false;
    auto callback_ref = std::make_shared<Callback>(std::move(callback));
    subscriptions_.push_back({callback_ref, scope, false});
    std::shared_ptr<Channel> channel = ensure_channel(entity_id, attribute);
    if (!channel) {
      *callback_ref = nullptr;
      release_null_subscriptions();
      return false;
    }
    release_expired_channel_callbacks(channel);
    channel->callbacks.push_back({callback_ref, false});
    transport_.replay(
        entity_id, attribute,
        [this, callback_ref](State state) { invoke(callback_ref, state); });
    return true;
  }

  void flush(size_t max_requests,
             size_t min_free,
             size_t min_largest) {
    poll_transport_reset();
    if (callback_depth_ != 0) return;
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
      const std::string attribute = request.has_attribute ? request.attribute : std::string();
      for (auto &callback : request.callbacks) {
        attach_get(request.entity_id, attribute, std::move(callback));
      }
      processed++;
    }
    release_empty_deferred_storage();
  }

  void reset_deferred() {
    std::vector<DeferredRequest>().swap(deferred_);
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
    collecting_generation_ = true;
    reset_deferred();
    reset_subscriptions(default_scope);
    for (const auto &channel : channels_) {
      if (!channel) continue;
      release_expired_channel_callbacks(channel);
      channel->seen_generation = channel->callbacks.empty() ? 0 : generation_;
    }
  }

  void commit_generation() {
    collecting_generation_ = false;
    bool key_set_changed = false;
    for (const auto &channel : channels_) {
      if (!channel) continue;
      release_expired_channel_callbacks(channel);
      const bool desired = channel_desired(channel);
      if (desired != channel->transport_active) key_set_changed = true;
    }
    if (key_set_changed) start_transport_reset();
    poll_transport_reset();
  }

  bool poll_transport_reset() {
    if (!transport_reset_pending_) return true;
    if (!transport_.poll_reset()) return false;

    transport_.remove_owned_subscriptions();
    for (const auto &channel : channels_) {
      if (channel) channel->transport_active = false;
    }
    channels_.erase(
        std::remove_if(channels_.begin(), channels_.end(),
                       [this](const std::shared_ptr<Channel> &channel) {
                         return !channel || !channel_desired(channel);
                       }),
        channels_.end());

    std::vector<Key> active_keys;
    active_keys.reserve(channels_.size());
    for (const auto &channel : channels_) {
      active_keys.push_back({channel->entity_id, channel->attribute});
      transport_.subscribe(
          channel->entity_id, channel->attribute,
          [this, channel](State state) { invoke_channel(channel, state); });
      channel->transport_active = true;
    }
    transport_.prune_cache(active_keys);
    transport_reset_pending_ = false;
    return true;
  }

 private:
  struct DeferredRequest {
    std::string entity_id;
    std::string attribute;
    std::vector<std::shared_ptr<Callback>> callbacks;
    uint32_t generation = 0;
    bool has_attribute = false;
  };

  struct SubscriptionRef {
    std::shared_ptr<Callback> callback;
    uint32_t scope = 0;
    bool once = false;
  };

  struct ChannelCallback {
    std::weak_ptr<Callback> callback;
    bool once = false;
  };

  struct Channel {
    std::string entity_id;
    std::string attribute;
    std::vector<ChannelCallback> callbacks;
    uint32_t seen_generation = 0;
    bool transport_active = false;
  };

  static constexpr size_t MAX_DEFERRED_REQUESTS = 64;

  bool attach_get(const std::string &entity_id,
                  const std::string &attribute,
                  std::shared_ptr<Callback> callback_ref) {
    std::shared_ptr<Channel> channel = ensure_channel(entity_id, attribute);
    if (!channel) return false;

    bool replayed = false;
    transport_.replay(
        entity_id, attribute,
        [this, callback_ref, &replayed](State state) {
          replayed = true;
          invoke(callback_ref, state);
        });
    if (replayed) return true;

    subscriptions_.push_back({callback_ref, 0, true});
    release_expired_channel_callbacks(channel);
    channel->callbacks.push_back({callback_ref, true});
    return true;
  }

  std::shared_ptr<Channel> ensure_channel(const std::string &entity_id,
                                          const std::string &attribute) {
    std::shared_ptr<Channel> channel = find_channel(entity_id, attribute);
    if (!channel) {
      channel = std::make_shared<Channel>();
      channel->entity_id = entity_id;
      channel->attribute = attribute;
      channel->seen_generation = generation_;
      channels_.push_back(channel);
      if (!collecting_generation_) start_transport_reset();
    } else {
      channel->seen_generation = generation_;
    }
    return channel;
  }

  bool queue(const std::string &entity_id,
             const std::string &attribute,
             std::shared_ptr<Callback> callback,
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
  }

  std::shared_ptr<Channel> find_channel(const std::string &entity_id,
                                        const std::string &attribute) {
    for (const auto &channel : channels_) {
      if (channel && channel->entity_id == entity_id && channel->attribute == attribute) {
        return channel;
      }
    }
    return nullptr;
  }

  bool channel_desired(const std::shared_ptr<Channel> &channel) const {
    return channel && (channel->seen_generation == generation_ || !channel->callbacks.empty());
  }

  void release_expired_channel_callbacks(const std::shared_ptr<Channel> &channel) {
    if (!channel) return;
    channel->callbacks.erase(
        std::remove_if(channel->callbacks.begin(), channel->callbacks.end(),
                       [](const ChannelCallback &entry) {
                         std::shared_ptr<Callback> callback = entry.callback.lock();
                         return !callback || !*callback;
                       }),
        channel->callbacks.end());
    if (channel->callbacks.empty()) std::vector<ChannelCallback>().swap(channel->callbacks);
  }

  void invoke_channel(const std::shared_ptr<Channel> &channel, State state) {
    if (!channel) return;
    const size_t callback_count = channel->callbacks.size();
    for (size_t index = 0; index < callback_count; index++) {
      const ChannelCallback entry = channel->callbacks[index];
      std::shared_ptr<Callback> callback = entry.callback.lock();
      if (!callback || !*callback) continue;
      invoke(callback, state);
      if (entry.once && callback && *callback) *callback = nullptr;
    }
    release_expired_channel_callbacks(channel);
    release_null_subscriptions();
  }

  void release_subscriptions(uint32_t scope) {
    for (auto &ref : subscriptions_) {
      if (!ref.callback || !*ref.callback) continue;
      if (scope == 0 || (ref.scope & scope) != 0) *ref.callback = nullptr;
    }
    release_null_subscriptions();
    for (const auto &channel : channels_) release_expired_channel_callbacks(channel);
  }

  void release_null_subscriptions() {
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                       [](const SubscriptionRef &ref) {
                         return !ref.callback || !*ref.callback;
                       }),
        subscriptions_.end());
    if (subscriptions_.empty()) std::vector<SubscriptionRef>().swap(subscriptions_);
  }

  void start_transport_reset() {
    if (transport_reset_pending_) return;
    transport_.begin_reset();
    transport_reset_pending_ = true;
  }

  void release_empty_deferred_storage() {
    if (!deferred_.empty() || deferred_.capacity() == 0) return;
    std::vector<DeferredRequest>().swap(deferred_);
  }

  Transport transport_;
  HeapProbe heap_probe_;
  std::vector<DeferredRequest> deferred_;
  std::vector<SubscriptionRef> subscriptions_;
  std::vector<std::shared_ptr<Channel>> channels_;
  uint32_t generation_ = 1;
  uint32_t pending_reset_mask_ = 0;
  uint8_t callback_depth_ = 0;
  bool collecting_generation_ = false;
  bool transport_reset_pending_ = false;
};
