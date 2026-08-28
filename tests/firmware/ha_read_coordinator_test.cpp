#include "ha_read_coordinator.h"
#include "espcontrol_app_core.h"
#include "home_assistant_binding_service.h"

#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

struct FakeTransport {
  using State = std::string;
  using Callback = std::function<void(State)>;

  struct Request {
    std::string entity_id;
    std::string attribute;
    Callback callback;
  };

  bool api_available = true;
  bool connected = true;
  std::vector<Request> subscriptions;

  bool available() const { return api_available; }
  bool state_connected() const { return connected; }

  void subscribe(const std::string &entity_id,
                 const std::string &attribute,
                 Callback callback) {
    subscriptions.push_back({entity_id, attribute, std::move(callback)});
  }

  void publish(size_t index, const std::string &state) {
    Callback callback = subscriptions.at(index).callback;
    callback(state);
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
using BindingService = HomeAssistantBindingService<FakeTransport, FakeHeapProbe>;

[[noreturn]] void fail(const char *message) {
  (void) message;
  std::abort();
}

void require(bool condition, const char *message) {
  if (!condition) fail(message);
}


void disconnected_read_flushes_after_reconnect() {
  Coordinator coordinator;
  require(coordinator.subscribe("sensor.room", "", [](std::string) {}, 1u, nullptr, true),
          "retained state subscription should register");
  coordinator.transport().connected = false;
  std::string received;
  require(coordinator.read_retained(
              "sensor.room", "", [&](std::string value) { received = value; }, false, 10, 5),
          "disconnected retained read should wait");
  require(coordinator.deferred_count() == 0 && coordinator.pending_read_count() == 1,
          "disconnected read did not wait on its retained channel");
  coordinator.transport().connected = true;
  coordinator.transport().publish(0, "ready");
  require(received == "ready", "reconnected callback was not invoked");
}


void low_memory_rejects_retained_read_without_pending_work() {
  Coordinator coordinator;
  require(coordinator.subscribe("sensor.heap", "", [](std::string) {}, 1u, nullptr, true),
          "retained state subscription should register");
  coordinator.heap_probe().enough = false;
  int calls = 0;
  require(!coordinator.read_retained(
              "sensor.heap", "", [&](std::string) { calls++; }, false, 10, 5),
          "low-memory retained read should fail safely");
  require(calls == 0 && coordinator.pending_read_count() == 0,
          "low-memory retained read left pending work");
}


void duplicate_reads_fan_out_once() {
  Coordinator coordinator;
  require(coordinator.subscribe("sensor.same", "", [](std::string) {}, 1u, nullptr, true),
          "retained state subscription should register");
  int first = 0;
  int second = 0;
  require(coordinator.read_retained(
              "sensor.same", "", [&](std::string) { first++; }, false, 10, 5),
          "first duplicate read should wait");
  require(coordinator.read_retained(
              "sensor.same", "", [&](std::string) { second++; }, false, 10, 5),
          "second duplicate read should wait");
  require(coordinator.pending_read_count() == 2, "independent reads were not retained");
  coordinator.transport().publish(0, "on");
  require(first == 1 && second == 1, "duplicate callbacks did not fan out");
}


void reentrant_reads_are_deferred() {
  Coordinator coordinator;
  require(coordinator.subscribe("sensor.outer", "", [](std::string) {}, 1u, nullptr, true),
          "outer retained subscription should register");
  require(coordinator.subscribe("sensor.inner", "", [](std::string) {}, 1u, nullptr, true),
          "inner retained subscription should register");
  int nested = 0;
  require(coordinator.read_retained(
              "sensor.outer", "",
              [&](std::string) {
                require(coordinator.read_retained(
                            "sensor.inner", "", [&](std::string) { nested++; }, false, 10, 5),
                        "nested read should queue");
              },
              false, 10, 5),
          "outer read should wait");
  coordinator.transport().publish(0, "outer");
  require(coordinator.deferred_count() == 1 && nested == 0,
          "reentrant read was invoked inside callback");
  coordinator.flush(8, 10, 5);
  require(coordinator.pending_read_count() == 1,
          "reentrant read did not flush to retained channel");
  coordinator.transport().publish(1, "inner");
  require(nested == 1, "reentrant callback did not run");
}

void cancellation_is_safe_during_callback() {
  Coordinator coordinator;
  constexpr uint32_t scope = 1u << 2;
  int calls = 0;
  require(coordinator.subscribe(
              "sensor.cancel", "",
              [&](std::string) {
                calls++;
                coordinator.reset_subscriptions(scope);
              },
              scope),
          "subscription should register");
  coordinator.transport().publish(0, "first");
  coordinator.transport().publish(0, "second");
  require(calls == 1 && coordinator.subscription_count() == 0,
          "callback cancellation was not deferred safely");
}

void rebuilt_subscriptions_share_one_transport_channel() {
  Coordinator coordinator;
  constexpr uint32_t scope = 1u;
  int old_calls = 0;
  int new_calls = 0;
  require(coordinator.subscribe("media_player.room", "media_title",
                                [&](std::string) { old_calls++; }, scope),
          "initial subscription should register");
  coordinator.reset_subscriptions(scope);
  require(coordinator.subscribe("media_player.room", "media_title",
                                [&](std::string) { new_calls++; }, scope),
          "rebuilt subscription should register");
  require(coordinator.transport().subscriptions.size() == 1,
          "rebuilt subscription created a duplicate transport channel");
  coordinator.transport().publish(0, "Track");
  require(old_calls == 0 && new_calls == 1,
          "shared transport channel did not invoke only the current callback");
}

void rebuilt_subscription_replays_last_value() {
  Coordinator coordinator;
  constexpr uint32_t scope = 1u;
  require(coordinator.subscribe("cover.blind", "current_position",
                                [](std::string) {}, scope),
          "initial cover subscription should register");
  coordinator.transport().publish(0, "42");
  coordinator.reset_subscriptions(scope);

  std::string received;
  require(coordinator.subscribe("cover.blind", "current_position",
                                [&](std::string value) { received = value; }, scope),
          "rebuilt cover subscription should register");
  require(received == "42", "rebuilt subscription did not replay the latest value");
}

void reconnect_does_not_replay_previous_connection_state() {
  Coordinator coordinator;
  constexpr uint32_t scope = 1u;
  require(coordinator.subscribe("cover.blind", "current_position",
                                [](std::string) {}, scope),
          "initial cover subscription should register");
  coordinator.transport().publish(0, "42");
  coordinator.reset_subscriptions(scope);
  coordinator.invalidate_retained_state();

  int calls = 0;
  require(coordinator.subscribe("cover.blind", "current_position",
                                [&](std::string) { calls++; }, scope),
          "reconnected cover subscription should register");
  require(calls == 0, "reconnected subscription replayed stale state");
  coordinator.transport().publish(0, "43");
  require(calls == 1, "reconnected subscription missed the fresh state");
}

void inactive_channels_do_not_retain_new_replay_values() {
  Coordinator coordinator;
  constexpr uint32_t scope = 1u;
  require(coordinator.subscribe("cover.old", "current_position",
                                [](std::string) {}, scope),
          "initial cover subscription should register");
  coordinator.transport().publish(0, "old");
  coordinator.reset_subscriptions(scope);
  coordinator.transport().publish(0, "inactive-update");

  int calls = 0;
  require(coordinator.subscribe("cover.old", "current_position",
                                [&](std::string) { calls++; }, scope),
          "inactive cover subscription should rebuild");
  require(calls == 0, "inactive channel retained a new replay value");
}

void replay_state_cache_is_bounded() {
  Coordinator coordinator;
  constexpr uint32_t scope = 1u;
  for (size_t i = 0; i < 65; i++) {
    require(coordinator.subscribe("sensor.channel_" + std::to_string(i), "state",
                                  [](std::string) {}, scope),
            "bounded replay subscription should register");
    coordinator.transport().publish(i, "value_" + std::to_string(i));
  }
  coordinator.reset_subscriptions(scope);

  int oldest_calls = 0;
  require(coordinator.subscribe("sensor.channel_0", "state",
                                [&](std::string) { oldest_calls++; }, scope),
          "oldest bounded replay subscription should rebuild");
  require(oldest_calls == 0, "replay cache retained more than its bounded capacity");

  std::string newest;
  require(coordinator.subscribe("sensor.channel_64", "state",
                                [&](std::string value) { newest = value; }, scope),
          "newest bounded replay subscription should rebuild");
  require(newest == "value_64", "bounded replay cache discarded the newest value");
}

void subscription_backed_reads_reuse_the_live_channel() {
  Coordinator coordinator;
  int subscription_calls = 0;
  require(coordinator.subscribe(
              "media_player.room", "entity_picture",
              [&](std::string) { subscription_calls++; }, 1u, nullptr, true),
          "artwork subscription should register");

  std::string first_read;
  require(coordinator.read_retained(
              "media_player.room", "entity_picture",
              [&](std::string value) { first_read = value; }, true, 10, 5),
          "first artwork read should wait on its live subscription");
  require(coordinator.transport().subscriptions.size() == 1 &&
              coordinator.pending_read_count() == 1,
          "subscription-backed artwork read created extra transport activity");

  coordinator.transport().publish(0, "/api/media_player_proxy/room");
  require(first_read == "/api/media_player_proxy/room" && subscription_calls == 1,
          "live artwork response did not satisfy the pending read");

  std::string cached_read;
  require(coordinator.read_retained(
              "media_player.room", "entity_picture",
              [&](std::string value) { cached_read = value; }, true, 10, 5),
          "later artwork read should reuse the live channel");
  require(cached_read == "/api/media_player_proxy/room" &&
              coordinator.transport().subscriptions.size() == 1 &&
              coordinator.pending_read_count() == 0,
          "later artwork read did not reuse the latest live value");
}


void retained_reads_never_accumulate_transport_callbacks() {
  Coordinator coordinator;
  int subscription_calls = 0;
  int read_calls = 0;
  require(coordinator.subscribe(
              "media_player.room", "entity_picture",
              [&](std::string) { subscription_calls++; }, 1u, nullptr, true),
          "retained artwork subscription should register");
  coordinator.transport().publish(0, "cached");

  for (size_t i = 0; i < 1000; i++) {
    require(coordinator.read_retained(
                "media_player.room", "entity_picture",
                [&](std::string value) {
                  require(value == "cached", "retained read received the wrong cached value");
                  read_calls++;
                },
                true, 10, 5),
            "cached retained read should succeed");
  }

  require(read_calls == 1000 && coordinator.subscription_channel_count() == 1 &&
              coordinator.pending_read_count() == 0,
          "repeated retained reads accumulated transport or pending callbacks");
  coordinator.transport().publish(0, "new");
  require(read_calls == 1000 && subscription_calls == 2,
          "later state update invoked completed retained-read callbacks");
}

void missing_retained_channel_fails_closed() {
  Coordinator coordinator;
  int calls = 0;
  require(!coordinator.read_retained(
              "media_player.room", "entity_picture",
              [&](std::string) { calls++; }, true, 10, 5),
          "missing retained channel should fail");
  require(calls == 0 && coordinator.subscription_channel_count() == 0 &&
              coordinator.pending_read_count() == 0,
          "missing retained channel created coordinator state");
}

void pending_unowned_reads_are_globally_capped() {
  Coordinator coordinator;
  int calls = 0;
  size_t accepted = 0;
  require(coordinator.subscribe(
              "media_player.room", "entity_picture",
              [](std::string) {}, 1u, nullptr, true),
          "retained remote artwork subscription should register");
  require(coordinator.subscribe(
              "media_player.room", "entity_picture_local",
              [](std::string) {}, 1u, nullptr, true),
          "retained local artwork subscription should register");
  for (size_t i = 0; i < 1000; i++) {
    const char *attribute = (i % 2 == 0) ? "entity_picture" : "entity_picture_local";
    if (coordinator.read_retained(
            "media_player.room", attribute,
            [&](std::string) { calls++; }, true, 10, 5)) {
      accepted++;
    }
  }
  require(accepted == 64 && coordinator.pending_read_count() == 64,
          "pending retained reads exceeded the global cap");
  coordinator.transport().publish(0, "remote");
  coordinator.transport().publish(1, "local");
  require(calls == 64 && coordinator.pending_read_count() == 0,
          "globally capped pending retained reads were not released");
}

void generation_change_discards_cached_and_pending_channel_reads() {
  Coordinator coordinator;
  require(coordinator.subscribe("media_player.room", "entity_picture",
                                [](std::string) {}, 1u, nullptr, true),
          "artwork subscription should register");
  coordinator.transport().publish(0, "old");
  coordinator.bump_generation(1u);
  require(coordinator.subscribe("media_player.room", "entity_picture",
                                [](std::string) {}, 1u, nullptr, true),
          "new generation artwork subscription should register");

  int calls = 0;
  require(coordinator.read_retained("media_player.room", "entity_picture",
                          [&](std::string) { calls++; }, true, 10, 5),
          "new generation artwork read should wait for a current value");
  require(calls == 0, "new generation consumed stale cached artwork");
  coordinator.transport().publish(0, "new");
  require(calls == 1, "new generation artwork read did not receive the current value");
}

void reconnect_discards_retained_channel_state() {
  Coordinator coordinator;
  require(coordinator.subscribe("media_player.room", "entity_picture",
                                [](std::string) {}, 1u, nullptr, true),
          "artwork subscription should register");
  coordinator.transport().publish(0, "old-token");
  coordinator.invalidate_retained_state();

  std::string received;
  require(coordinator.read_retained("media_player.room", "entity_picture",
                          [&](std::string value) { received = value; }, true, 10, 5),
          "reconnected artwork read should wait for a current value");
  require(received.empty(), "reconnected artwork read reused stale credentials");
  coordinator.transport().publish(0, "new-token");
  require(received == "new-token",
          "reconnected artwork read did not receive the current credentials");
}


void ordinary_subscriptions_fail_closed_for_retained_reads() {
  Coordinator coordinator;
  require(coordinator.subscribe("sensor.room", "friendly_name",
                                [](std::string) {}, 1u),
          "ordinary subscription should register");
  coordinator.transport().publish(0, "Room");

  int calls = 0;
  require(!coordinator.read_retained(
              "sensor.room", "friendly_name", [&](std::string) { calls++; }, true, 10, 5),
          "ordinary subscription should reject retained reads");
  require(calls == 0 && coordinator.pending_read_count() == 0,
          "ordinary subscription retained an unsafe read");
}

void inactive_reusable_channels_release_cached_state() {
  Coordinator coordinator;
  require(coordinator.subscribe("media_player.room", "entity_picture",
                                [](std::string) {}, 1u, nullptr, true),
          "reusable artwork subscription should register");
  coordinator.transport().publish(0, "old");
  coordinator.reset_subscriptions();
  require(coordinator.subscribe("media_player.room", "entity_picture",
                                [](std::string) {}, 1u, nullptr, true),
          "artwork subscription should be reusable after reset");

  int calls = 0;
  require(coordinator.read_retained("media_player.room", "entity_picture",
                          [&](std::string) { calls++; }, true, 10, 5),
          "recreated artwork read should wait for reannouncement");
  require(calls == 0, "inactive artwork channel retained stale cached state");
  coordinator.transport().publish(0, "new");
  require(calls == 1, "reannounced artwork did not satisfy the recreated read");
}

void stalled_reusable_channel_coalesces_reads_per_owner() {
  Coordinator coordinator;
  int owner = 0;
  require(coordinator.subscribe("media_player.room", "entity_picture_local",
                                [](std::string) {}, 1u, &owner, true),
          "reusable local artwork subscription should register");

  int stale_calls = 0;
  int current_calls = 0;
  require(coordinator.read_retained("media_player.room", "entity_picture_local",
                          [&](std::string) { stale_calls++; }, true, 10, 5, &owner),
          "first local artwork read should wait");
  require(coordinator.read_retained("media_player.room", "entity_picture_local",
                          [&](std::string) { current_calls++; }, true, 10, 5, &owner),
          "replacement local artwork read should wait");
  coordinator.transport().publish(0, "new");
  require(stale_calls == 0 && current_calls == 1,
          "stalled artwork retries retained superseded callbacks for one owner");
}

void unowned_reusable_channel_keeps_independent_reads() {
  Coordinator coordinator;
  require(coordinator.subscribe("media_player.room", "entity_picture",
                                [](std::string) {}, 1u, nullptr, true),
          "reusable artwork subscription should register");

  int first_calls = 0;
  int second_calls = 0;
  require(coordinator.read_retained("media_player.room", "entity_picture",
                          [&](std::string) { first_calls++; }, true, 10, 5),
          "first unowned artwork read should wait");
  require(coordinator.read_retained("media_player.room", "entity_picture",
                          [&](std::string) { second_calls++; }, true, 10, 5),
          "second unowned artwork read should wait independently");
  coordinator.transport().publish(0, "new");
  require(first_calls == 1 && second_calls == 1,
          "duplicate cards did not both receive the first artwork update");
}


void generation_change_drops_pending_retained_reads() {
  Coordinator coordinator;
  require(coordinator.subscribe("sensor.stale", "", [](std::string) {}, 1u, nullptr, true),
          "retained state subscription should register");
  int calls = 0;
  require(coordinator.read_retained(
              "sensor.stale", "", [&](std::string) { calls++; }, false, 10, 5),
          "pending retained read should register");
  coordinator.bump_generation(1u);
  require(coordinator.pending_read_count() == 0,
          "generation cleanup retained pending work");
  require(coordinator.subscribe("sensor.stale", "", [](std::string) {}, 1u, nullptr, true),
          "replacement retained state subscription should register");
  coordinator.transport().publish(0, "late");
  require(calls == 0, "stale retained callback was delivered");
}


void retained_subscription_preserves_attribute() {
  Coordinator coordinator;
  require(coordinator.subscribe(
              "media_player.room", "media_title", [](std::string) {}, 1u, nullptr, true),
          "retained metadata subscription should register");
  require(coordinator.transport().subscriptions.size() == 1 &&
              coordinator.transport().subscriptions[0].attribute == "media_title",
          "retained subscription lost its attribute");
}


void released_owner_drops_pending_reads_even_if_its_address_is_reused() {
  Coordinator coordinator;
  int owner = 0;
  int old_calls = 0;
  int replacement_calls = 0;
  require(coordinator.subscribe("sensor.old", "", [](std::string) {}, 1u, &owner, true),
          "old retained subscription should register");
  require(coordinator.read_retained(
              "sensor.old", "", [&](std::string) { old_calls++; }, false, 10, 5, &owner),
          "old owned read should wait");
  coordinator.release_owner(&owner);
  require(coordinator.subscribe("sensor.replacement", "", [](std::string) {}, 1u, &owner, true),
          "replacement retained subscription should register");
  require(coordinator.read_retained(
              "sensor.replacement", "", [&](std::string) { replacement_calls++; },
              false, 10, 5, &owner),
          "replacement owned read should wait");
  coordinator.transport().publish(0, "late");
  coordinator.transport().publish(1, "current");
  require(old_calls == 0 && replacement_calls == 1,
          "released owner delivered a callback after its address was reused");
}

void callback_owner_scope_restores_the_previous_owner() {
  BindingService service;
  int first = 0;
  int second = 0;
  require(service.callback_owner() == nullptr, "new binding service has no callback owner");
  {
    auto first_scope = service.callback_owner_scope(&first);
    require(service.callback_owner() == &first, "first callback scope was not applied");
    {
      auto second_scope = service.callback_owner_scope(&second);
      require(service.callback_owner() == &second, "nested callback scope was not applied");
    }
    require(service.callback_owner() == &first, "nested callback scope was not restored");
  }
  require(service.callback_owner() == nullptr, "callback scope leaked after destruction");
}

void app_owned_callback_owner_is_used_when_bound() {
  BindingService service;
  HomeAssistantCallbackOwnerService app_owner;
  int owner = 0;
  set_home_assistant_callback_owner_service(&app_owner);
  {
    auto scope = service.callback_owner_scope(&owner);
    require(app_owner.callback_owner() == &owner,
            "binding service did not use app-owned callback state");
  }
  set_home_assistant_callback_owner_service(nullptr);
  require(app_owner.callback_owner() == nullptr,
          "callback scope did not restore app-owned state");
}

void core_owns_binding_service_lifetime() {
  espcontrol::EspControlAppCore app;
  require(app.start(), "application core did not start");
  BindingService &service = app.home_assistant_binding_service<BindingService>();
  int owner = 0;
  {
    auto scope = service.callback_owner_scope(&owner);
    require(app.home_assistant_callback_owner().callback_owner() == &owner,
            "core-owned binding did not use the app callback state");
  }
  require(app.stop(), "application core did not stop");
}

}  // namespace

int main() {
  disconnected_read_flushes_after_reconnect();
  low_memory_rejects_retained_read_without_pending_work();
  duplicate_reads_fan_out_once();
  reentrant_reads_are_deferred();
  cancellation_is_safe_during_callback();
  rebuilt_subscriptions_share_one_transport_channel();
  rebuilt_subscription_replays_last_value();
  reconnect_does_not_replay_previous_connection_state();
  inactive_channels_do_not_retain_new_replay_values();
  replay_state_cache_is_bounded();
  subscription_backed_reads_reuse_the_live_channel();
  retained_reads_never_accumulate_transport_callbacks();
  missing_retained_channel_fails_closed();
  pending_unowned_reads_are_globally_capped();
  generation_change_discards_cached_and_pending_channel_reads();
  reconnect_discards_retained_channel_state();
  ordinary_subscriptions_fail_closed_for_retained_reads();
  inactive_reusable_channels_release_cached_state();
  stalled_reusable_channel_coalesces_reads_per_owner();
  unowned_reusable_channel_keeps_independent_reads();
  generation_change_drops_pending_retained_reads();
  retained_subscription_preserves_attribute();
  released_owner_drops_pending_reads_even_if_its_address_is_reused();
  callback_owner_scope_restores_the_previous_owner();
  app_owned_callback_owner_is_used_when_bound();
  core_owns_binding_service_lifetime();
  return EXIT_SUCCESS;
}
