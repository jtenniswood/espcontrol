#pragma once

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
  size_t subscription_count() const {
    size_t count = 0;
    for (const SubscriptionRef &ref : subscriptions_) {
      if (ref.callback && *ref.callback) count++;
    }
    return count;
  }
  size_t retained_subscription_count() const { return subscriptions_.size(); }

  bool get(const std::string &entity_id,
           const std::string &attribute,
           Callback callback,
           bool has_attribute,
           size_t min_free,
           size_t min_largest) {
    if (!available() || entity_id.empty() || !callback) return false;
    if (!heap_probe_.available("Home Assistant state request", min_free, min_largest)) return false;
    auto callback_ref = std::make_shared<Callback>(std::move(callback));
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
                 bool reusable = false) {
    if (!available() || entity_id.empty() || !callback) return false;
    if (reusable) {
      for (SubscriptionRef &ref : subscriptions_) {
        if (!ref.reuse_key || ref.reuse_key->entity_id != entity_id ||
            ref.reuse_key->attribute != attribute || ref.scope != scope ||
            !ref.callback || *ref.callback) {
          continue;
        }
        std::shared_ptr<Callback> callback_ref = ref.callback;
        *callback_ref = std::move(callback);
        bool has_attribute = !attribute.empty();
        if (callback_depth_ != 0 || !state_connected()) {
          queue(entity_id, attribute, callback_ref, has_attribute);
        } else {
          dispatch_one(entity_id, attribute, callback_ref, has_attribute, generation_);
        }
        return true;
      }
    }
    auto callback_ref = std::make_shared<Callback>(std::move(callback));
    std::unique_ptr<ReusableSubscriptionKey> reuse_key;
    if (reusable) {
      reuse_key = std::make_unique<ReusableSubscriptionKey>(
          ReusableSubscriptionKey{entity_id, attribute});
    }
    subscriptions_.push_back({callback_ref, scope, std::move(reuse_key)});
    transport_.subscribe(
        entity_id, attribute,
        [this, callback_ref](State state) { invoke(callback_ref, state); });
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
    reset_subscriptions(default_scope);
  }

 private:
  struct DeferredRequest {
    std::string entity_id;
    std::string attribute;
    std::vector<std::shared_ptr<Callback>> callbacks;
    uint32_t generation = 0;
    bool has_attribute = false;
  };

  struct ReusableSubscriptionKey {
    std::string entity_id;
    std::string attribute;
  };

  struct SubscriptionRef {
    std::shared_ptr<Callback> callback;
    uint32_t scope = 0;
    // Normal grid subscriptions deliberately carry no copied entity strings.
    // Only modal subscriptions that can be rebound need this lookup key.
    std::unique_ptr<ReusableSubscriptionKey> reuse_key;
  };

  static constexpr size_t MAX_DEFERRED_REQUESTS = 64;

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

  void dispatch_one(std::string entity_id,
                    std::string attribute,
                    std::shared_ptr<Callback> callback,
                    bool has_attribute,
                    uint32_t generation) {
    transport_.get(
        std::move(entity_id), has_attribute ? std::move(attribute) : std::string(),
        [this, callback, generation](State state) {
          if (generation == generation_) invoke(callback, state);
        });
  }

  void dispatch_many(std::string entity_id,
                     std::string attribute,
                     std::vector<std::shared_ptr<Callback>> callbacks,
                     bool has_attribute,
                     uint32_t generation) {
    auto callback_refs =
        std::make_shared<std::vector<std::shared_ptr<Callback>>>(std::move(callbacks));
    transport_.get(
        std::move(entity_id), has_attribute ? std::move(attribute) : std::string(),
        [this, callback_refs, generation](State state) {
          if (generation != generation_) return;
          for (const auto &callback : *callback_refs) invoke(callback, state);
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
  }

  void release_subscriptions(uint32_t scope) {
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < subscriptions_.size(); read_index++) {
      SubscriptionRef &ref = subscriptions_[read_index];
      if (scope == 0 || (ref.scope & scope) != 0) {
        if (ref.callback && *ref.callback) *ref.callback = nullptr;
        if (!ref.reuse_key) continue;
      }
      if (write_index != read_index) subscriptions_[write_index] = std::move(ref);
      write_index++;
    }
    subscriptions_.resize(write_index);
    if (subscriptions_.empty()) std::vector<SubscriptionRef>().swap(subscriptions_);
  }

  void release_empty_deferred_storage() {
    if (!deferred_.empty() || deferred_.capacity() == 0) return;
    std::vector<DeferredRequest>().swap(deferred_);
  }

  Transport transport_;
  HeapProbe heap_probe_;
  std::vector<DeferredRequest> deferred_;
  std::vector<SubscriptionRef> subscriptions_;
  uint32_t generation_ = 1;
  uint32_t pending_reset_mask_ = 0;
  uint8_t callback_depth_ = 0;
};
