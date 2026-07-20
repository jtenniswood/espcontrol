#include "ha_read_coordinator.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

struct FakeTransport {
  using State = std::string;
  using Callback = std::function<void(State)>;
  using Key = std::pair<std::string, std::string>;

  struct Request {
    std::string entity_id;
    std::string attribute;
    Callback callback;
  };

  struct CachedValue {
    Key key;
    std::string value;
  };

  bool api_available = true;
  bool connected = true;
  bool reset_ready = true;
  size_t reset_requests = 0;
  size_t owned_removals = 0;
  size_t external_subscriptions = 2;
  std::vector<Request> subscriptions;
  std::vector<CachedValue> cache;

  bool available() const { return api_available; }
  bool state_connected() const { return connected; }

  void subscribe(const std::string &entity_id,
                 const std::string &attribute,
                 Callback callback) {
    subscriptions.push_back({entity_id, attribute, std::move(callback)});
  }

  void replay(const std::string &entity_id,
              const std::string &attribute,
              Callback callback) {
    for (const auto &entry : cache) {
      if (entry.key == Key(entity_id, attribute)) {
        callback(entry.value);
        return;
      }
    }
  }

  void begin_reset() {
    reset_requests++;
    connected = false;
  }

  bool poll_reset() const { return reset_ready && !connected; }

  void remove_owned_subscriptions() {
    subscriptions.clear();
    owned_removals++;
  }

  void prune_cache(const std::vector<Key> &active_keys) {
    cache.erase(
        std::remove_if(cache.begin(), cache.end(),
                       [&active_keys](const CachedValue &entry) {
                         return std::find(active_keys.begin(), active_keys.end(), entry.key) ==
                                active_keys.end();
                       }),
        cache.end());
  }

  void set_cache(const std::string &entity_id,
                 const std::string &attribute,
                 const std::string &value) {
    cache.push_back({{entity_id, attribute}, value});
  }

  void publish(const std::string &entity_id,
               const std::string &attribute,
               const std::string &state) {
    for (const auto &request : subscriptions) {
      if (request.entity_id == entity_id && request.attribute == attribute) {
        request.callback(state);
      }
    }
  }
};

struct FakeHeapProbe {
  bool enough = true;
  size_t checks = 0;

  bool available(const char *, size_t, size_t) {
    checks++;
    return enough;
  }
};

using Coordinator = HaReadCoordinator<FakeTransport, FakeHeapProbe>;

[[noreturn]] void fail(const char *message) {
  (void) message;
  std::abort();
}

void require(bool condition, const char *message) {
  if (!condition) fail(message);
}

void build_generation(Coordinator &coordinator,
                      const std::string &entity_id,
                      std::function<void(std::string)> callback) {
  coordinator.bump_generation(1u);
  require(coordinator.subscribe(entity_id, "", std::move(callback), 1u, 10, 5),
          "generation subscription should register");
  coordinator.commit_generation();
}

void same_keys_reuse_transport_channel() {
  Coordinator coordinator;
  int first_calls = 0;
  build_generation(coordinator, "sensor.same", [&](std::string) { first_calls++; });
  require(coordinator.transport().subscriptions.size() == 1,
          "first generation should create one transport channel");
  const size_t resets = coordinator.transport().reset_requests;
  coordinator.transport().connected = true;

  int second_calls = 0;
  build_generation(coordinator, "sensor.same", [&](std::string) { second_calls++; });
  require(coordinator.transport().reset_requests == resets,
          "same key set should not recycle the API transport");
  coordinator.transport().publish("sensor.same", "", "on");
  require(first_calls == 0 && second_calls == 1,
          "replacement generation should exclusively receive state");
}

void new_entity_rebuilds_transport_and_receives_state() {
  Coordinator coordinator;
  build_generation(coordinator, "sensor.old", [](std::string) {});
  const size_t resets = coordinator.transport().reset_requests;
  coordinator.transport().connected = true;

  std::string received;
  build_generation(coordinator, "sensor.new", [&](std::string value) { received = value; });
  require(coordinator.transport().reset_requests == resets + 1,
          "new entity should recycle the transport");
  require(coordinator.transport().subscriptions.size() == 1 &&
              coordinator.transport().subscriptions[0].entity_id == "sensor.new",
          "stale entity remained after transport rebuild");
  coordinator.transport().connected = true;
  coordinator.transport().publish("sensor.new", "", "ready");
  require(received == "ready", "new entity did not receive future state");
}

void unrelated_transport_subscriptions_are_preserved() {
  Coordinator coordinator;
  const size_t external = coordinator.transport().external_subscriptions;
  build_generation(coordinator, "sensor.owned", [](std::string) {});
  build_generation(coordinator, "sensor.replacement", [](std::string) {});
  require(coordinator.transport().external_subscriptions == external,
          "transport reset removed unrelated subscriptions");
}

void repeated_gets_share_one_persistent_channel() {
  Coordinator coordinator;
  int first = 0;
  int second = 0;
  require(coordinator.get("camera.room", "entity_picture",
                          [&](std::string) { first++; }, true, 10, 5),
          "first read should register");
  require(coordinator.get("camera.room", "entity_picture",
                          [&](std::string) { second++; }, true, 10, 5),
          "second read should join the channel");
  coordinator.poll_transport_reset();
  require(coordinator.transport().subscriptions.size() == 1,
          "duplicate reads created duplicate transport entries");
  coordinator.transport().connected = true;
  coordinator.transport().publish("camera.room", "entity_picture", "/first.jpg");
  coordinator.transport().publish("camera.room", "entity_picture", "/second.jpg");
  require(first == 1 && second == 1, "one-shot read callbacks were not consumed once");
}

void cached_get_replays_without_retaining_callback() {
  Coordinator coordinator;
  coordinator.transport().set_cache("sensor.cached", "", "warm");
  std::string received;
  require(coordinator.get("sensor.cached", "", [&](std::string value) { received = value; },
                          false, 10, 5),
          "cached read should register");
  require(received == "warm" && coordinator.subscription_count() == 0,
          "cached read was not replayed immediately");
}

void churn_stays_bounded_to_active_generation() {
  Coordinator coordinator;
  for (int index = 0; index < 100; index++) {
    coordinator.transport().connected = true;
    build_generation(
        coordinator, "sensor.churn_" + std::to_string(index), [](std::string) {});
    require(coordinator.channel_count() == 1,
            "channel storage grew beyond the active generation");
    require(coordinator.transport().subscriptions.size() == 1,
            "transport storage grew beyond the active generation");
  }
}

void latest_generation_wins_during_pending_reset() {
  Coordinator coordinator;
  coordinator.transport().reset_ready = false;
  build_generation(coordinator, "sensor.first", [](std::string) {});
  require(coordinator.transport_reset_pending(), "reset should remain pending");

  coordinator.bump_generation(1u);
  require(coordinator.subscribe("sensor.latest", "", [](std::string) {}, 1u, 10, 5),
          "latest generation should register while reset is pending");
  coordinator.commit_generation();
  coordinator.transport().reset_ready = true;
  coordinator.poll_transport_reset();
  require(coordinator.transport().subscriptions.size() == 1 &&
              coordinator.transport().subscriptions[0].entity_id == "sensor.latest",
          "pending reset did not coalesce to the latest generation");
}

void low_heap_rejects_new_work_without_losing_existing_channels() {
  Coordinator coordinator;
  build_generation(coordinator, "sensor.stable", [](std::string) {});
  coordinator.heap_probe().enough = false;
  require(!coordinator.subscribe("sensor.pressure", "", [](std::string) {}, 1u, 10, 5),
          "low heap should defer a new subscription");
  require(coordinator.channel_count() == 1,
          "low-heap rejection damaged the active channel set");
}

void cancellation_is_safe_during_callback() {
  Coordinator coordinator;
  constexpr uint32_t scope = 1u << 2;
  int calls = 0;
  coordinator.bump_generation(scope);
  require(coordinator.subscribe(
              "sensor.cancel", "",
              [&](std::string) {
                calls++;
                coordinator.reset_subscriptions(scope);
              },
              scope, 10, 5),
          "subscription should register");
  coordinator.commit_generation();
  coordinator.transport().connected = true;
  coordinator.transport().publish("sensor.cancel", "", "first");
  coordinator.transport().publish("sensor.cancel", "", "second");
  require(calls == 1 && coordinator.subscription_count() == 0,
          "callback cancellation was not deferred safely");
}

}  // namespace

int main() {
  same_keys_reuse_transport_channel();
  new_entity_rebuilds_transport_and_receives_state();
  unrelated_transport_subscriptions_are_preserved();
  repeated_gets_share_one_persistent_channel();
  cached_get_replays_without_retaining_callback();
  churn_stays_bounded_to_active_generation();
  latest_generation_wins_during_pending_reset();
  low_heap_rejects_new_work_without_losing_existing_channels();
  cancellation_is_safe_during_callback();
  return EXIT_SUCCESS;
}
