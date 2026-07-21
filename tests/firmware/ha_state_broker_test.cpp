#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "ha_state_broker.h"

namespace {

struct FakeTransport {
  using State = std::string;
  using Callback = std::function<void(State)>;
  using Handle = std::size_t;

  struct Channel {
    std::string entity;
    std::string attribute;
    Callback callback;
    bool open = true;
  };

  bool available() const { return online; }
  bool open(const std::string &entity, const std::string &attribute,
            Callback callback, Handle *handle) {
    if (!online || handle == nullptr) return false;
    *handle = channels.size();
    channels.push_back({entity, attribute, std::move(callback), true});
    ++open_calls;
    return true;
  }
  void close(Handle handle) {
    if (handle < channels.size() && channels[handle].open) {
      channels[handle].open = false;
      ++close_calls;
    }
  }
  void poll() { ++poll_calls; }
  void publish(Handle handle, const std::string &value,
               bool include_closed = false) {
    if (handle < channels.size() &&
        (channels[handle].open || include_closed)) {
      channels[handle].callback(value);
    }
  }

  bool online = true;
  std::size_t open_calls = 0;
  std::size_t close_calls = 0;
  std::size_t poll_calls = 0;
  std::vector<Channel> channels;
};

using Broker = espcontrol::ha::StateBroker<FakeTransport, 3, 5>;

bool leases_share_one_transport_channel() {
  Broker broker;
  int first = 0;
  int second = 0;
  auto first_lease = broker.subscribe("sensor.room", "", [&](std::string) { ++first; });
  auto second_lease = broker.subscribe("sensor.room", "", [&](std::string) { ++second; });
  if (!first_lease.valid() || !second_lease.valid() ||
      broker.channel_count() != 1 || broker.subscriber_count() != 2 ||
      broker.transport().open_calls != 1) {
    return false;
  }
  broker.transport().publish(0, "ready");
  if (first != 1 || second != 1) return false;
  first_lease.reset();
  if (broker.channel_count() != 1 || broker.transport().close_calls != 0) {
    return false;
  }
  second_lease.reset();
  if (broker.channel_count() != 1 || broker.transport().close_calls != 0) {
    return false;
  }
  broker.prune();
  return broker.channel_count() == 0 && broker.subscriber_count() == 0 &&
         broker.transport().close_calls == 1;
}

bool cached_state_replays_to_a_new_lease() {
  Broker broker;
  std::string first;
  auto persistent = broker.subscribe(
      "media_player.room", "media_title",
      [&](std::string value) { first = std::move(value); });
  broker.transport().publish(0, "Track one");
  std::string replayed;
  auto later = broker.subscribe(
      "media_player.room", "media_title",
      [&](std::string value) { replayed = std::move(value); });
  return persistent.valid() && later.valid() && first == "Track one" &&
         replayed == "Track one" && broker.transport().open_calls == 1;
}

bool repeated_gets_are_one_shot_and_bounded() {
  Broker broker;
  int persistent_calls = 0;
  auto persistent = broker.subscribe("camera.room", "entity_picture",
                                     [&](std::string) { ++persistent_calls; });
  broker.transport().publish(0, "/cover.jpg");
  for (int index = 0; index < 100; ++index) {
    int calls = 0;
    if (!broker.get("camera.room", "entity_picture",
                    [&](std::string) { ++calls; }) ||
        calls != 1 || broker.subscriber_count() != 1 ||
        broker.channel_count() != 1) {
      return false;
    }
  }
  return persistent.valid() && persistent_calls == 1 &&
         broker.transport().open_calls == 1;
}

bool callbacks_after_close_are_ignored() {
  Broker broker;
  int calls = 0;
  auto lease = broker.subscribe("sensor.old", "", [&](std::string) { ++calls; });
  lease.reset();
  broker.transport().publish(0, "late", true);
  if (calls != 0) return false;

  auto replacement = broker.subscribe("sensor.new", "", [&](std::string) { ++calls; });
  broker.transport().publish(0, "later still", true);
  broker.transport().publish(1, "current");
  return replacement.valid() && calls == 1;
}

bool churn_returns_to_zero_active_storage() {
  Broker broker;
  for (int index = 0; index < 100; ++index) {
    auto lease = broker.subscribe("sensor." + std::to_string(index), "",
                                  [](std::string) {});
    if (!lease.valid() || broker.channel_count() != 1) return false;
    lease.reset();
    broker.prune();
    if (broker.channel_count() != 0 || broker.subscriber_count() != 0) {
      return false;
    }
  }
  return Broker::channel_capacity() == 3 && broker.transport().open_calls == 100 &&
         broker.transport().close_calls == 100;
}

bool one_shot_get_channel_survives_until_pruned() {
  Broker broker;
  int calls = 0;
  if (!broker.get("camera.room", "entity_picture",
                  [&](std::string) { ++calls; })) {
    return false;
  }
  broker.transport().publish(0, "/cover.jpg");
  if (calls != 1 || broker.subscriber_count() != 0 ||
      broker.channel_count() != 1) {
    return false;
  }
  for (int index = 0; index < 100; ++index) {
    int replayed = 0;
    if (!broker.get("camera.room", "entity_picture",
                    [&](std::string) { ++replayed; }) ||
        replayed != 1 || broker.transport().open_calls != 1) {
      return false;
    }
  }
  broker.prune();
  return broker.channel_count() == 0 && broker.transport().close_calls == 1;
}

bool generations_swap_leases_atomically() {
  using Scoped = espcontrol::ha::ScopedStateSubscriptions<Broker, 4>;
  Broker broker;
  Scoped scoped(broker);
  int old_calls = 0;
  if (!scoped.subscribe("sensor.same", "", [&](std::string) { ++old_calls; }, 1)) {
    return false;
  }
  const uint32_t first_generation = scoped.generation();
  scoped.begin_generation(1);
  int new_calls = 0;
  if (!scoped.subscribe("sensor.same", "", [&](std::string) { ++new_calls; }, 1) ||
      !scoped.commit_generation()) {
    return false;
  }
  broker.transport().publish(0, "on");
  return scoped.generation() != first_generation && old_calls == 0 &&
         new_calls == 1 && broker.transport().open_calls == 1 &&
         broker.channel_count() == 1 && scoped.active_count() == 1;
}

bool failed_generation_keeps_previous_leases() {
  using SmallBroker = espcontrol::ha::StateBroker<FakeTransport, 2, 1>;
  using Scoped = espcontrol::ha::ScopedStateSubscriptions<SmallBroker, 1>;
  SmallBroker broker;
  Scoped scoped(broker);
  int old_calls = 0;
  if (!scoped.subscribe("sensor.stable", "", [&](std::string) { ++old_calls; }, 1)) {
    return false;
  }
  const uint32_t stable_generation = scoped.generation();
  scoped.begin_generation(1);
  if (scoped.subscribe("sensor.replacement", "", [](std::string) {}, 1) ||
      scoped.commit_generation()) {
    return false;
  }
  broker.transport().publish(0, "still active");
  return scoped.generation() == stable_generation && old_calls == 1 &&
         scoped.active_count() == 1 && scoped.pending_count() == 0;
}

bool cached_get_requested_in_callback_is_delivered_afterwards() {
  Broker broker;
  std::string nested;
  auto metadata = broker.subscribe(
      "media_player.room", "media_title", [](std::string) {});
  broker.transport().publish(0, "Song");
  auto state = broker.subscribe(
      "media_player.room", "",
      [&](std::string) {
        broker.get("media_player.room", "media_title",
                   [&](std::string value) { nested = std::move(value); });
        if (!nested.empty()) nested = "delivered recursively";
      });
  broker.transport().publish(1, "playing");
  return metadata.valid() && state.valid() && nested == "Song" &&
         broker.subscriber_count() == 2;
}

bool capacity_failure_does_not_disturb_active_leases() {
  espcontrol::ha::StateBroker<FakeTransport, 1, 1> broker;
  int calls = 0;
  auto active = broker.subscribe("sensor.one", "", [&](std::string) { ++calls; });
  auto no_subscriber = broker.subscribe("sensor.one", "", [](std::string) {});
  auto no_channel = broker.subscribe("sensor.two", "", [](std::string) {});
  broker.transport().publish(0, "ok");
  return active.valid() && !no_subscriber.valid() && !no_channel.valid() &&
         calls == 1 && broker.channel_count() == 1 &&
         broker.subscriber_count() == 1;
}

}  // namespace

int main() {
  return leases_share_one_transport_channel() &&
                 cached_state_replays_to_a_new_lease() &&
                 repeated_gets_are_one_shot_and_bounded() &&
                 callbacks_after_close_are_ignored() &&
                 churn_returns_to_zero_active_storage() &&
                 capacity_failure_does_not_disturb_active_leases() &&
                 one_shot_get_channel_survives_until_pruned() &&
                 generations_swap_leases_atomically() &&
                 failed_generation_keeps_previous_leases() &&
                 cached_get_requested_in_callback_is_delivered_afterwards()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
