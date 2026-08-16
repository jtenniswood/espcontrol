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
  std::vector<Request> reads;
  std::vector<Request> subscriptions;

  bool available() const { return api_available; }
  bool state_connected() const { return connected; }

  void get(std::string entity_id, std::string attribute, Callback callback) {
    reads.push_back({std::move(entity_id), std::move(attribute), std::move(callback)});
  }

  void subscribe(const std::string &entity_id,
                 const std::string &attribute,
                 Callback callback) {
    subscriptions.push_back({entity_id, attribute, std::move(callback)});
  }

  void deliver_read(size_t index, const std::string &state) {
    Callback callback = reads.at(index).callback;
    callback(state);
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
  coordinator.transport().connected = false;
  std::string received;
  require(coordinator.get("sensor.room", "", [&](std::string value) { received = value; },
                          false, 10, 5),
          "disconnected read should queue");
  require(coordinator.deferred_count() == 1 && coordinator.transport().reads.empty(),
          "disconnected read was not deferred");
  coordinator.transport().connected = true;
  coordinator.flush(8, 10, 5);
  require(coordinator.transport().reads.size() == 1, "reconnected read was not sent");
  coordinator.transport().deliver_read(0, "ready");
  require(received == "ready", "reconnected callback was not invoked");
}

void low_memory_preserves_deferred_work() {
  Coordinator coordinator;
  coordinator.transport().connected = false;
  int calls = 0;
  require(coordinator.get("sensor.heap", "", [&](std::string) { calls++; }, false, 10, 5),
          "read should queue before heap pressure");
  coordinator.transport().connected = true;
  coordinator.heap_probe().enough = false;
  coordinator.flush(8, 10, 5);
  require(coordinator.deferred_count() == 1 && coordinator.transport().reads.empty(),
          "low-memory flush should retain work");
  coordinator.heap_probe().enough = true;
  coordinator.flush(8, 10, 5);
  coordinator.transport().deliver_read(0, "ok");
  require(calls == 1, "deferred low-memory read did not recover");
}

void duplicate_reads_fan_out_once() {
  Coordinator coordinator;
  coordinator.transport().connected = false;
  int first = 0;
  int second = 0;
  require(coordinator.get("sensor.same", "", [&](std::string) { first++; }, false, 10, 5),
          "first duplicate read should queue");
  require(coordinator.get("sensor.same", "", [&](std::string) { second++; }, false, 10, 5),
          "second duplicate read should join");
  require(coordinator.deferred_count() == 1, "duplicate reads were not coalesced");
  coordinator.transport().connected = true;
  coordinator.flush(8, 10, 5);
  require(coordinator.transport().reads.size() == 1, "duplicate reads sent more than once");
  coordinator.transport().deliver_read(0, "on");
  require(first == 1 && second == 1, "duplicate callbacks did not fan out");
}

void reentrant_reads_are_deferred() {
  Coordinator coordinator;
  int nested = 0;
  require(coordinator.get(
              "sensor.outer", "",
              [&](std::string) {
                require(coordinator.get("sensor.inner", "", [&](std::string) { nested++; },
                                        false, 10, 5),
                        "nested read should queue");
              },
              false, 10, 5),
          "outer read should send");
  coordinator.transport().deliver_read(0, "outer");
  require(coordinator.deferred_count() == 1 && coordinator.transport().reads.size() == 1,
          "reentrant read was sent inside callback");
  coordinator.flush(8, 10, 5);
  require(coordinator.transport().reads.size() == 2, "reentrant read did not flush");
  coordinator.transport().deliver_read(1, "inner");
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

void subscription_backed_reads_reuse_the_live_channel() {
  Coordinator coordinator;
  int subscription_calls = 0;
  require(coordinator.subscribe(
              "media_player.room", "entity_picture",
              [&](std::string) { subscription_calls++; }, 1u, nullptr, true),
          "artwork subscription should register");

  std::string first_read;
  require(coordinator.get(
              "media_player.room", "entity_picture",
              [&](std::string value) { first_read = value; }, true, 10, 5),
          "first artwork read should wait on its live subscription");
  require(coordinator.transport().reads.empty(),
          "subscription-backed artwork read created an unbounded one-shot request");

  coordinator.transport().publish(0, "/api/media_player_proxy/room");
  require(first_read == "/api/media_player_proxy/room" && subscription_calls == 1,
          "live artwork response did not satisfy the pending read");

  std::string cached_read;
  require(coordinator.get(
              "media_player.room", "entity_picture",
              [&](std::string value) { cached_read = value; }, true, 10, 5),
          "later artwork read should reuse the live channel");
  require(cached_read == "/api/media_player_proxy/room" &&
              coordinator.transport().reads.empty(),
          "later artwork read did not reuse the latest live value");
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
  require(coordinator.get("media_player.room", "entity_picture",
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
  require(coordinator.get("media_player.room", "entity_picture",
                          [&](std::string value) { received = value; }, true, 10, 5),
          "reconnected artwork read should wait for a current value");
  require(received.empty(), "reconnected artwork read reused stale credentials");
  coordinator.transport().publish(0, "new-token");
  require(received == "new-token",
          "reconnected artwork read did not receive the current credentials");
}

void ordinary_subscriptions_do_not_retain_state_for_reads() {
  Coordinator coordinator;
  require(coordinator.subscribe("sensor.room", "friendly_name",
                                [](std::string) {}, 1u),
          "ordinary subscription should register");
  coordinator.transport().publish(0, "A deliberately retained label");

  int calls = 0;
  require(coordinator.get("sensor.room", "friendly_name",
                          [&](std::string) { calls++; }, true, 10, 5),
          "ordinary one-shot read should be dispatched");
  require(calls == 0 && coordinator.transport().reads.size() == 1,
          "ordinary subscription unexpectedly retained a persistent state copy");
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
  require(coordinator.get("media_player.room", "entity_picture",
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
  require(coordinator.get("media_player.room", "entity_picture_local",
                          [&](std::string) { stale_calls++; }, true, 10, 5, &owner),
          "first local artwork read should wait");
  require(coordinator.get("media_player.room", "entity_picture_local",
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
  require(coordinator.get("media_player.room", "entity_picture",
                          [&](std::string) { first_calls++; }, true, 10, 5),
          "first unowned artwork read should wait");
  require(coordinator.get("media_player.room", "entity_picture",
                          [&](std::string) { second_calls++; }, true, 10, 5),
          "second unowned artwork read should wait independently");
  coordinator.transport().publish(0, "new");
  require(first_calls == 1 && second_calls == 1,
          "duplicate cards did not both receive the first artwork update");
}

void stale_generations_do_not_deliver() {
  Coordinator coordinator;
  int calls = 0;
  require(coordinator.get("sensor.stale", "", [&](std::string) { calls++; }, false, 10, 5),
          "stale read should send");
  uint32_t old_generation = coordinator.generation();
  coordinator.bump_generation(1u);
  require(coordinator.generation() != old_generation, "generation did not advance");
  coordinator.transport().deliver_read(0, "late");
  require(calls == 0, "stale in-flight callback was delivered");

  coordinator.transport().connected = false;
  require(coordinator.get("sensor.queued", "", [&](std::string) { calls++; }, false, 10, 5),
          "queued stale read should be accepted");
  coordinator.bump_generation(1u);
  require(coordinator.deferred_count() == 0, "generation cleanup retained deferred work");
}

void attribute_requests_preserve_attribute() {
  Coordinator coordinator;
  require(coordinator.get("media_player.room", "media_title", [](std::string) {}, true, 10, 5),
          "attribute read should send");
  require(coordinator.transport().reads.size() == 1 &&
              coordinator.transport().reads[0].attribute == "media_title",
          "attribute read lost its attribute");
}

void released_owner_drops_pending_reads_even_if_its_address_is_reused() {
  Coordinator coordinator;
  int old_screen = 0;
  int old_calls = 0;
  int replacement_calls = 0;
  require(coordinator.get("sensor.old", "", [&](std::string) { old_calls++; }, false, 10, 5,
                          &old_screen),
          "owned read should send");
  coordinator.release_owner(&old_screen);
  require(coordinator.get("sensor.replacement", "", [&](std::string) { replacement_calls++; },
                          false, 10, 5, &old_screen),
          "replacement owner should receive a fresh token");
  coordinator.transport().deliver_read(0, "late");
  coordinator.transport().deliver_read(1, "current");
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
  low_memory_preserves_deferred_work();
  duplicate_reads_fan_out_once();
  reentrant_reads_are_deferred();
  cancellation_is_safe_during_callback();
  rebuilt_subscriptions_share_one_transport_channel();
  subscription_backed_reads_reuse_the_live_channel();
  generation_change_discards_cached_and_pending_channel_reads();
  reconnect_discards_retained_channel_state();
  ordinary_subscriptions_do_not_retain_state_for_reads();
  inactive_reusable_channels_release_cached_state();
  stalled_reusable_channel_coalesces_reads_per_owner();
  unowned_reusable_channel_keeps_independent_reads();
  stale_generations_do_not_deliver();
  attribute_requests_preserve_attribute();
  released_owner_drops_pending_reads_even_if_its_address_is_reused();
  callback_owner_scope_restores_the_previous_owner();
  app_owned_callback_owner_is_used_when_bound();
  core_owns_binding_service_lifetime();
  return EXIT_SUCCESS;
}
