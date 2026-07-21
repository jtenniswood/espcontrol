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

  explicit StateBroker(Transport transport = Transport())
      : transport_(std::move(transport)) {}

  StateBroker(const StateBroker &) = delete;
  StateBroker &operator=(const StateBroker &) = delete;

  Lease subscribe(const std::string &entity_id, const std::string &attribute,
                  Callback callback) {
    if (entity_id.empty() || !callback || !transport_.available()) return {};
    const std::size_t channel = ensure_channel(entity_id, attribute);
    if (channel == invalid_channel()) return {};
    const std::size_t subscriber = allocate_subscriber(channel, std::move(callback), false);
    if (subscriber == invalid_subscriber()) {
      sweep();
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
      sweep();
      return false;
    }
    if (channels_[channel].has_value) {
      const uint32_t generation = subscribers_[subscriber].generation;
      subscribers_[subscriber].callback(channels_[channel].value);
      release_subscriber(subscriber, generation);
    }
    return true;
  }

  void poll() {
    transport_.poll();
    if (dispatch_depth_ == 0) sweep();
  }

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
    for (std::size_t index = 0; index < channels_.size(); ++index) {
      Channel &channel = channels_[index];
      if (channel.active) continue;
      advance_generation(channel.generation);
      channel.entity_id = entity_id;
      channel.attribute = attribute;
      channel.active = true;
      channel.has_value = false;
      const uint32_t generation = channel.generation;
      if (!transport_.open(
              entity_id, attribute,
              [this, index, generation](State state) {
                receive(index, generation, std::move(state));
              },
              &channel.transport_handle)) {
        channel = {};
        channel.generation = generation;
        return invalid_channel();
      }
      return index;
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
    if (dispatch_depth_ == 0) sweep();
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
    if (dispatch_depth_ == 0) sweep();
  }

  void sweep() {
    for (auto &subscriber : subscribers_) {
      if (!subscriber.active && subscriber.callback) {
        subscriber.callback = Callback{};
        subscriber.channel = invalid_channel();
      }
    }

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

}  // namespace espcontrol::ha
