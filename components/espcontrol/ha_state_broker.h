#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace espcontrol::ha {

// Reference-counted Home Assistant state fan-out. The transport owns the
// platform-specific subscription mechanics; callers only hold leases and never
// see API reconnects, ESPHome subscription indices, or broker generations.
template <typename Transport, std::size_t MaxChannels,
          std::size_t MaxSubscribers>
class StateBroker {
 public:
  using State = typename Transport::State;
  using Callback = typename Transport::Callback;
  using TransportHandle = typename Transport::Handle;

  class Lease {
   public:
    Lease() = default;
    Lease(const Lease &) = delete;
    Lease &operator=(const Lease &) = delete;

    Lease(Lease &&other) noexcept { move_from(other); }
    Lease &operator=(Lease &&other) noexcept {
      if (this == &other) return *this;
      reset();
      move_from(other);
      return *this;
    }

    ~Lease() { reset(); }

    bool valid() const {
      return broker_ != nullptr &&
             broker_->subscriber_valid(subscriber_, generation_);
    }

    void reset() {
      if (broker_ != nullptr) {
        broker_->release_subscriber(subscriber_, generation_);
      }
      broker_ = nullptr;
      subscriber_ = invalid_index();
      generation_ = 0;
    }

   private:
    friend class StateBroker;
    static constexpr std::size_t invalid_index() { return MaxSubscribers; }

    Lease(StateBroker *broker, std::size_t subscriber, uint32_t generation)
        : broker_(broker), subscriber_(subscriber), generation_(generation) {}

    void move_from(Lease &other) {
      broker_ = other.broker_;
      subscriber_ = other.subscriber_;
      generation_ = other.generation_;
      other.broker_ = nullptr;
      other.subscriber_ = invalid_index();
      other.generation_ = 0;
    }

    StateBroker *broker_ = nullptr;
    std::size_t subscriber_ = invalid_index();
    uint32_t generation_ = 0;
  };

  // Construct the transport directly in the broker. Passing a default
  // transport by value creates a second, potentially very large fixed table
  // on the ESP task stack before it can be moved into place.
  StateBroker() = default;
  explicit StateBroker(Transport transport) : transport_(std::move(transport)) {}

  StateBroker(const StateBroker &) = delete;
  StateBroker &operator=(const StateBroker &) = delete;

  Lease subscribe(const std::string &entity_id, const std::string &attribute,
                  Callback callback) {
    if (entity_id.empty() || !callback || !transport_.available()) return {};
    const std::size_t channel = ensure_channel(entity_id, attribute);
    if (channel == invalid_channel()) return {};
    const std::size_t subscriber = allocate_subscriber(channel, std::move(callback), false);
    if (subscriber == invalid_subscriber()) {
      prune();
      return {};
    }
    Lease lease(this, subscriber, subscribers_[subscriber].generation);
    if (channels_[channel].has_value) {
      subscribers_[subscriber].callback(channels_[channel].value);
    }
    return lease;
  }

  // A get is a one-delivery lease owned by the broker. It shares the same
  // channel and cached value as subscriptions, then releases itself.
  bool get(const std::string &entity_id, const std::string &attribute,
           Callback callback) {
    if (entity_id.empty() || !callback || !transport_.available()) return false;
    const std::size_t channel = ensure_channel(entity_id, attribute);
    if (channel == invalid_channel()) return false;
    const std::size_t subscriber = allocate_subscriber(channel, std::move(callback), true);
    if (subscriber == invalid_subscriber()) {
      prune();
      return false;
    }
    if (channels_[channel].has_value && dispatch_depth_ == 0) {
      const uint32_t generation = subscribers_[subscriber].generation;
      subscribers_[subscriber].callback(channels_[channel].value);
      release_subscriber(subscriber, generation);
    }
    return true;
  }

  void poll() {
    transport_.poll();
    if (dispatch_depth_ == 0) {
      deliver_cached_once();
      sweep(false);
    }
  }

  // Channels deliberately outlive one-shot callbacks and temporarily released
  // leases. This lets a rebuilt card replace its callback without creating a
  // second ESPHome subscription, and lets repeated image reads reuse cached
  // state. The lifecycle owner calls prune() after a generation is committed.
  void prune() { sweep(true); }

  std::size_t channel_count() const {
    std::size_t count = 0;
    for (const auto &channel : channels_) {
      if (channel.active) ++count;
    }
    return count;
  }

  std::size_t subscriber_count() const {
    std::size_t count = 0;
    for (const auto &subscriber : subscribers_) {
      if (subscriber.active) ++count;
    }
    return count;
  }

  static constexpr std::size_t channel_capacity() { return MaxChannels; }
  static constexpr std::size_t subscriber_capacity() { return MaxSubscribers; }

  Transport &transport() { return transport_; }
  const Transport &transport() const { return transport_; }

 private:
  struct Channel {
    std::string entity_id;
    std::string attribute;
    State value{};
    TransportHandle transport_handle{};
    uint32_t generation = 0;
    bool active = false;
    bool has_value = false;
  };

  struct Subscriber {
    Callback callback{};
    std::size_t channel = MaxChannels;
    uint32_t generation = 0;
    bool active = false;
    bool once = false;
  };

  static constexpr std::size_t invalid_channel() { return MaxChannels; }
  static constexpr std::size_t invalid_subscriber() { return MaxSubscribers; }

  std::size_t find_channel(const std::string &entity_id,
                           const std::string &attribute) const {
    for (std::size_t index = 0; index < channels_.size(); ++index) {
      const Channel &channel = channels_[index];
      if (channel.active && channel.entity_id == entity_id &&
          channel.attribute == attribute) {
        return index;
      }
    }
    return invalid_channel();
  }

  std::size_t ensure_channel(const std::string &entity_id,
                             const std::string &attribute) {
    const std::size_t existing = find_channel(entity_id, attribute);
    if (existing != invalid_channel()) return existing;
    std::size_t free_index = invalid_channel();
    for (std::size_t index = 0; index < channels_.size(); ++index) {
      if (!channels_[index].active) {
        free_index = index;
        break;
      }
    }
    if (free_index == invalid_channel()) {
      prune();
      for (std::size_t index = 0; index < channels_.size(); ++index) {
        if (!channels_[index].active) {
          free_index = index;
          break;
        }
      }
    }
    if (free_index != invalid_channel()) {
      Channel &channel = channels_[free_index];
      advance_generation(channel.generation);
      channel.entity_id = entity_id;
      channel.attribute = attribute;
      channel.active = true;
      channel.has_value = false;
      const uint32_t generation = channel.generation;
      if (!transport_.open(
              entity_id, attribute,
              [this, free_index, generation](State state) {
                receive(free_index, generation, std::move(state));
              },
              &channel.transport_handle)) {
        channel = {};
        channel.generation = generation;
        return invalid_channel();
      }
      return free_index;
    }
    return invalid_channel();
  }

  std::size_t allocate_subscriber(std::size_t channel, Callback callback,
                                  bool once) {
    for (std::size_t index = 0; index < subscribers_.size(); ++index) {
      Subscriber &subscriber = subscribers_[index];
      if (subscriber.active) continue;
      advance_generation(subscriber.generation);
      subscriber.callback = std::move(callback);
      subscriber.channel = channel;
      subscriber.active = true;
      subscriber.once = once;
      return index;
    }
    return invalid_subscriber();
  }

  bool subscriber_valid(std::size_t index, uint32_t generation) const {
    return index < subscribers_.size() && subscribers_[index].active &&
           subscribers_[index].generation == generation;
  }

  void release_subscriber(std::size_t index, uint32_t generation) {
    if (!subscriber_valid(index, generation)) return;
    subscribers_[index].active = false;
    subscribers_[index].once = false;
    if (dispatch_depth_ == 0) sweep(false);
  }

  void receive(std::size_t channel_index, uint32_t generation, State state) {
    if (channel_index >= channels_.size()) return;
    Channel &channel = channels_[channel_index];
    if (!channel.active || channel.generation != generation) return;
    channel.value = std::move(state);
    channel.has_value = true;

    ++dispatch_depth_;
    for (std::size_t index = 0; index < subscribers_.size(); ++index) {
      Subscriber &subscriber = subscribers_[index];
      if (!subscriber.active || subscriber.channel != channel_index) continue;
      const uint32_t subscriber_generation = subscriber.generation;
      const bool once = subscriber.once;
      Callback callback = subscriber.callback;
      callback(channel.value);
      if (once) release_subscriber(index, subscriber_generation);
    }
    --dispatch_depth_;
    if (dispatch_depth_ == 0) {
      deliver_cached_once();
      sweep(false);
    }
  }

  void deliver_cached_once() {
    // A callback may request another cached attribute. Deliver after the outer
    // callback returns, using the fixed subscriber table as the queue.
    for (std::size_t pass = 0; pass < subscribers_.size(); ++pass) {
      bool delivered = false;
      for (std::size_t index = 0; index < subscribers_.size(); ++index) {
        Subscriber &subscriber = subscribers_[index];
        if (!subscriber.active || !subscriber.once ||
            subscriber.channel >= channels_.size() ||
            !channels_[subscriber.channel].has_value) continue;
        const uint32_t generation = subscriber.generation;
        Callback callback = subscriber.callback;
        ++dispatch_depth_;
        callback(channels_[subscriber.channel].value);
        --dispatch_depth_;
        release_subscriber(index, generation);
        delivered = true;
      }
      if (!delivered) break;
    }
  }

  void sweep(bool prune_channels) {
    for (auto &subscriber : subscribers_) {
      if (!subscriber.active && subscriber.callback) {
        subscriber.callback = Callback{};
        subscriber.channel = invalid_channel();
      }
    }

    if (!prune_channels) return;
    for (std::size_t channel_index = 0; channel_index < channels_.size();
         ++channel_index) {
      Channel &channel = channels_[channel_index];
      if (!channel.active) continue;
      bool referenced = false;
      for (const auto &subscriber : subscribers_) {
        if (subscriber.active && subscriber.channel == channel_index) {
          referenced = true;
          break;
        }
      }
      if (referenced) continue;
      transport_.close(channel.transport_handle);
      const uint32_t generation = channel.generation;
      channel = {};
      channel.generation = generation;
    }
  }

  static void advance_generation(uint32_t &generation) {
    ++generation;
    if (generation == 0) ++generation;
  }

  Transport transport_;
  std::array<Channel, MaxChannels> channels_{};
  std::array<Subscriber, MaxSubscribers> subscribers_{};
  uint16_t dispatch_depth_ = 0;
};

// Keeps scoped subscription leases in two fixed banks while a card generation
// is prepared. Old leases remain live until every replacement lease has been
// acquired, so a low-memory/capacity failure can discard the candidate without
// leaving the current screen disconnected.
template <typename Broker, std::size_t MaxLeases>
class ScopedStateSubscriptions {
 public:
  using Callback = typename Broker::Callback;
  using Lease = typename Broker::Lease;

  explicit ScopedStateSubscriptions(Broker &broker) : broker_(broker) {}

  uint32_t generation() const { return visible_generation_; }
  uint32_t &generation_ref() { return visible_generation_; }

  void begin_generation(uint32_t replace_scopes) {
    clear_bank(pending_);
    collecting_ = true;
    collection_failed_ = false;
    replace_scopes_ = replace_scopes;
    visible_generation_ = committed_generation_ + 1;
    if (visible_generation_ == 0) visible_generation_ = 1;
  }

  bool subscribe(const std::string &entity_id, const std::string &attribute,
                 Callback callback, uint32_t scope) {
    if (collecting_ && retained_active_count() + bank_size(pending_) >= MaxLeases) {
      collection_failed_ = true;
      return false;
    }
    Lease lease = broker_.subscribe(entity_id, attribute, std::move(callback));
    if (!lease.valid()) {
      if (collecting_) collection_failed_ = true;
      return false;
    }
    Bank &target = collecting_ ? pending_ : active_;
    if (!store(target, std::move(lease), scope)) {
      if (collecting_) collection_failed_ = true;
      return false;
    }
    return true;
  }

  bool commit_generation() {
    if (!collecting_) return true;
    if (collection_failed_) {
      clear_bank(pending_);
      collecting_ = false;
      visible_generation_ = committed_generation_;
      broker_.prune();
      return false;
    }
    release_matching(active_, replace_scopes_);
    for (auto &entry : pending_) {
      if (!entry.active) continue;
      if (!store(active_, std::move(entry.lease), entry.scope)) {
        // retained_active_count() prevents this path; keep the guard so a
        // future capacity change still fails closed.
        clear_bank(pending_);
        collecting_ = false;
        visible_generation_ = committed_generation_;
        broker_.prune();
        return false;
      }
      entry.active = false;
      entry.scope = 0;
    }
    committed_generation_ = visible_generation_;
    collecting_ = false;
    replace_scopes_ = 0;
    broker_.prune();
    return true;
  }

  void reset(uint32_t scopes = 0) {
    release_matching(active_, scopes);
    release_matching(pending_, scopes);
    broker_.prune();
  }

  std::size_t active_count() const { return bank_size(active_); }
  std::size_t pending_count() const { return bank_size(pending_); }
  bool collecting() const { return collecting_; }
  bool collection_failed() const { return collection_failed_; }

 private:
  struct Entry {
    Lease lease{};
    uint32_t scope = 0;
    bool active = false;
  };
  using Bank = std::array<Entry, MaxLeases>;

  static bool scope_matches(uint32_t entry_scope, uint32_t scopes) {
    return scopes == 0 || (entry_scope & scopes) != 0;
  }

  static std::size_t bank_size(const Bank &bank) {
    std::size_t count = 0;
    for (const auto &entry : bank) {
      if (entry.active) ++count;
    }
    return count;
  }

  std::size_t retained_active_count() const {
    std::size_t count = 0;
    for (const auto &entry : active_) {
      if (entry.active && !scope_matches(entry.scope, replace_scopes_)) ++count;
    }
    return count;
  }

  static bool store(Bank &bank, Lease lease, uint32_t scope) {
    for (auto &entry : bank) {
      if (entry.active) continue;
      entry.lease = std::move(lease);
      entry.scope = scope;
      entry.active = true;
      return true;
    }
    return false;
  }

  static void release_matching(Bank &bank, uint32_t scopes) {
    for (auto &entry : bank) {
      if (!entry.active || !scope_matches(entry.scope, scopes)) continue;
      entry.lease.reset();
      entry.scope = 0;
      entry.active = false;
    }
  }

  static void clear_bank(Bank &bank) { release_matching(bank, 0); }

  Broker &broker_;
  Bank active_{};
  Bank pending_{};
  uint32_t committed_generation_ = 1;
  uint32_t visible_generation_ = 1;
  uint32_t replace_scopes_ = 0;
  bool collecting_ = false;
  bool collection_failed_ = false;
};

}  // namespace espcontrol::ha
