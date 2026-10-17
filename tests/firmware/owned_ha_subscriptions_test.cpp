#include "ha_read_coordinator.h"
#include "owned_ha_subscriptions.h"
#include "esphome/core/string_ref.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
struct Backend {
  using State = esphome::StringRef;
  using Callback = std::function<void(State)>;
  struct Entry {
    const char *entity;
    const char *attribute;
    Callback callback;
    bool once;
  };
  static inline std::vector<Entry> native;
  static inline bool online = true;
  static inline size_t announcements = 0;
  static bool available() { return online; }
  static void subscribe(const char *entity, const char *attribute, Callback callback) {
    native.push_back({entity, attribute, std::move(callback), false});
  }
  template<typename Predicate>
  static bool remove_if(Predicate predicate) {
    const size_t before = native.size();
    native.erase(std::remove_if(native.begin(), native.end(),
        [&](const Entry &entry) { return predicate(entry.entity, entry.attribute, entry.once); }), native.end());
    return before != native.size();
  }
  static void reannounce() { ++announcements; }
  static void publish(const std::string &entity, const std::string &value) {
    // Match the SDK's range iteration: mutating native here would invalidate it.
    for (const auto &entry : native) {
      if (entity == entry.entity) entry.callback(State(value));
    }
  }
};
using Registry = OwnedHaSubscriptions<Backend>;
struct Transport {
  using State = Backend::State;
  using Callback = Backend::Callback;
  static constexpr bool RECLAIMABLE_SUBSCRIPTIONS = true;
  bool available() const { return Backend::online; }
  bool state_connected() const { return Backend::online; }
  uint32_t subscribe(const std::string &entity, const std::string &attribute, Callback callback) {
    return Registry::instance().subscribe(entity, attribute, std::move(callback));
  }
  void unsubscribe(uint32_t token) { Registry::instance().unsubscribe(token); }
  bool request(const std::string &, const std::string &) { return Backend::online; }
};
struct Heap {
  bool available(const char *, size_t, size_t) { return true; }
};
using Coordinator = HaReadCoordinator<Transport, Heap>;
void maintain(Coordinator &coordinator) {
  coordinator.reclaim_unused_channels();
  Registry::instance().flush();
}
void empty_registry() {
  Registry::instance().flush();
  require(Registry::instance().size() == 0, "owned callbacks survived coordinator destruction");
  require(Registry::instance().capacity() == 0, "empty registry retained history capacity");
  require(Backend::native.empty(), "native callbacks survived registry maintenance");
}

void disjoint_configurations_release_both_layers() {
  size_t foreign_calls = 0;
  Backend::native.push_back({"sensor.foreign", "unit", [&](auto) { ++foreign_calls; }, true});
  Backend::Callback stale;
  size_t calls = 0;
  {
    Coordinator coordinator;
    coordinator.subscribe("sensor.shared", "", [](auto) {}, 2u, nullptr, true);
    for (size_t generation = 0; generation < 1000; ++generation) {
      coordinator.reset_subscriptions(1u);
      for (size_t card = 0; card < 20; ++card) {
        const std::string entity = "sensor.g" + std::to_string(generation) + "_" + std::to_string(card);
        require(coordinator.subscribe(entity, "", [&](auto state) {
          require(std::string(state.c_str(), state.size()) == "live", "stale state reached replacement");
          ++calls;
        }, 1u), "owned subscribe failed");
      }
      maintain(coordinator);
      require(coordinator.subscription_channel_count() == 21, "coordinator retained old entities");
      require(coordinator.subscription_channel_capacity() <= 42, "channel capacity grew with history");
      require(Registry::instance().size() == 21, "registry retained old entities");
      require(Registry::instance().capacity() <= 42, "registry capacity grew with history");
      require(Backend::native.size() == 22, "native callbacks grew with configuration history");
      require(Backend::native[0].once && std::string(Backend::native[0].attribute) == "unit",
              "foreign native metadata was modified");
      if (generation == 0) stale = Backend::native.back().callback;
      else stale(Backend::State("stale"));
      Backend::publish("sensor.g" + std::to_string(generation) + "_19", "live");
      require(calls == generation + 1, "old callback operated a replacement card");
    }
    Backend::publish("sensor.foreign", "outside");
    require(foreign_calls == 1, "foreign callback was removed");
    coordinator.reset_subscriptions();
    maintain(coordinator);
    require(coordinator.subscription_channel_count() == 0, "empty coordinator retained channels");
    require(coordinator.subscription_channel_capacity() == 0, "empty coordinator retained storage");
  }
  Registry::instance().flush();
  require(Backend::native.size() == 1, "foreign callback lost during final cleanup");
  stale(Backend::State("stale"));
  require(calls == 1000, "destroyed coordinator was invoked");
  Backend::native.clear();
  empty_registry();
}

void callback_mutations_wait_until_dispatch_returns() {
  Backend::Callback stale;
  {
    Coordinator coordinator;
    int owner = 0;
    size_t new_calls = 0;
    coordinator.subscribe("sensor.old", "", [&](auto) {
      coordinator.release_owner(&owner);
      for (int i = 0; i < 100; ++i)
        coordinator.subscribe("sensor.new_" + std::to_string(i), "", [&](auto) { ++new_calls; }, 1u);
      require(coordinator.reclaim_unused_channels() == 0, "compacted channel during dispatch");
      require(!Registry::instance().flush(), "mutated SDK vector during dispatch");
      require(Backend::native.size() == 1, "new native callbacks were added during dispatch");
    }, 1u, &owner);
    maintain(coordinator);
    stale = Backend::native[0].callback;
    Backend::publish("sensor.old", "start");
    maintain(coordinator);
    require(Backend::native.size() == 100, "deferred native changes were not published");
    stale(Backend::State("late"));
    Backend::publish("sensor.new_99", "live");
    require(new_calls == 1, "late callback collided with a reused index");
    // Names are borrowed by the backend, and must survive all record moves.
    for (size_t i = 0; i < Backend::native.size(); ++i)
      require(std::string(Backend::native[i].entity) == "sensor.new_" + std::to_string(i),
              "borrowed names moved or were freed before native removal");
  }
  stale(Backend::State("after destruction"));
  empty_registry();
}

void compaction_preserves_pending_reads_and_reconnect_rules() {
  {
    Coordinator coordinator;
    int old_owner = 0;
    size_t reads = 0;
    coordinator.subscribe("sensor.retired", "", [](auto) {}, 1u, &old_owner);
    coordinator.subscribe("sensor.live", "unit", [](auto) {}, 2u, nullptr, true);
    maintain(coordinator);
    require(coordinator.read_retained("sensor.live", "unit", [&](auto state) {
      require(std::string(state.c_str(), state.size()) == "fresh", "replayed pre-reconnect data");
      ++reads;
    }, true, 0, 0), "could not queue retained read");
    coordinator.release_owner(&old_owner);
    maintain(coordinator);
    require(coordinator.subscription_channel_count() == 1, "did not compact retired leading channel");
    auto active = Backend::native[0].callback;
    active(Backend::State("fresh"));
    require(reads == 1, "pending read was lost or remapped to the wrong channel");
    coordinator.invalidate_retained_state();
    require(coordinator.read_retained("sensor.live", "unit", [&](auto state) {
      require(std::string(state.c_str(), state.size()) == "fresh", "replayed stale retained cache");
      ++reads;
    }, true, 0, 0), "could not queue after reconnect");
    require(reads == 1, "old state was replayed after reconnect invalidation");
    active(Backend::State("fresh"));
    require(reads == 2, "fresh state was not delivered after reconnect");
    coordinator.reset_subscriptions(2u);
    coordinator.subscribe("sensor.live", "unit", [](auto) {}, 2u, nullptr, true);
    maintain(coordinator);
    require(Backend::native.size() == 1, "same-entity rebuild allocated another native callback");
  }
  empty_registry();
}

void offline_retirement_keeps_borrowed_names_until_native_removal() {
  Backend::Callback stale;
  {
    Coordinator coordinator;
    coordinator.subscribe("sensor.offline", "", [](auto) {}, 1u);
    maintain(coordinator);
    stale = Backend::native[0].callback;
    Backend::online = false;
    coordinator.reset_subscriptions();
    coordinator.reclaim_unused_channels();
    require(!Registry::instance().flush(), "modified unavailable backend");
    require(std::string(Backend::native[0].entity) == "sensor.offline", "freed borrowed names too early");
    stale(Backend::State("late"));
    Backend::online = true;
    maintain(coordinator);
    require(Backend::native.empty(), "offline retirement was not eventually reclaimed");
  }
  empty_registry();
}

void foreign_subscription_to_the_same_pair_is_preserved() {
  size_t calls = 0;
  Backend::native.push_back({"sensor.same", "unit", [&](auto) { ++calls; }, false});
  {
    Coordinator coordinator;
    coordinator.subscribe("sensor.same", "unit", [](auto) {}, 1u);
    maintain(coordinator);
    require(Backend::native.size() == 2, "test requires identical foreign and owned pairs");
    coordinator.reset_subscriptions();
    maintain(coordinator);
    require(Backend::native.size() == 1, "erased a foreign registration with identical text");
    Backend::publish("sensor.same", "live");
    require(calls == 1, "foreign registration no longer dispatches");
  }
  Backend::native.clear();
  empty_registry();
}

void cached_string_views_survive_owner_retirement() {
  {
    Coordinator coordinator;
    int owner = 0;
    const std::string value(256, 'x');
    coordinator.subscribe("sensor.cached", "", [](auto) {}, 1u, &owner, true);
    maintain(coordinator);
    Backend::publish("sensor.cached", value);
    bool called = false;
    require(coordinator.read_retained("sensor.cached", "", [&](auto state) {
      coordinator.release_owner(&owner);
      coordinator.invalidate_retained_state();
      require(std::string(state.c_str(), state.size()) == value,
              "retiring a widget invalidated the state view it was consuming");
      called = true;
    }, true, 0, 0), "cached read failed");
    require(called, "cached read did not replay immediately");
    maintain(coordinator);
  }
  empty_registry();
}
}  // namespace

int main() {
  try {
    disjoint_configurations_release_both_layers();
    callback_mutations_wait_until_dispatch_returns();
    compaction_preserves_pending_reads_and_reconnect_rules();
    offline_retirement_keeps_borrowed_names_until_native_removal();
    foreign_subscription_to_the_same_pair_is_preserved();
    cached_string_views_survive_owner_retirement();
  } catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
