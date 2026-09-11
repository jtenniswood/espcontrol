// Expanded by generate_climate_subscription_runtime_test.py at build time.
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "esphome/core/string_ref.h"
#include "climate_subscription_policy.h"
#include "climate_target_logic.h"
#include "ha_read_coordinator.h"

using ClimateTargetKind = espcontrol::climate::TargetKind;
struct lv_obj_t {};
struct lv_font_t {};
struct lv_timer_t { void (*callback)(lv_timer_t *); };
constexpr int MAX_GRID_SLOTS = 32, MAX_SUBPAGE_ITEMS = 64;
constexpr uint32_t DEFAULT_SLIDER_COLOR = 0, SECONDARY_GREY = 0, TERTIARY_GREY = 0;
constexpr size_t HA_SHORT_STATE_MAX_LEN = 64, HA_FRIENDLY_NAME_MAX_LEN = 128;
constexpr size_t HA_TEXT_SENSOR_STATE_MAX_LEN = 256;

std::vector<lv_timer_t *> timers;
lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), uint32_t delay, void *) {
  assert(delay == 250);
  auto *timer = new lv_timer_t{callback};
  timers.push_back(timer);
  return timer;
}
void lv_timer_del(lv_timer_t *timer) {
  timers.erase(std::remove(timers.begin(), timers.end(), timer), timers.end());
  delete timer;
}
void run_maintenance(bool expect_retry = false) {
  assert(timers.size() == 1);
  auto *timer = timers.front();
  timer->callback(timer);
  assert(timers.size() == (expect_retry ? 1u : 0u));
}
std::string string_ref_limited(esphome::StringRef value, size_t limit) {
  return std::string(value.c_str(), std::min(value.size(), limit));
}
bool parse_float_ref(esphome::StringRef value, float &out) {
  std::string text(value.c_str(), value.size());
  char *end = nullptr;
  out = std::strtof(text.c_str(), &end);
  return end != text.c_str();
}

// @production-context

struct FakeTransport {
  using State = esphome::StringRef;
  using Callback = std::function<void(State)>;
  struct Channel { std::string entity, attribute; Callback callback; };
  std::vector<Channel> channels;
  bool connected = true;
  bool dispatching = false;
  bool api_available = true;
  bool available() const { return api_available; }
  bool state_connected() const { return connected; }
  void subscribe(const std::string &entity, const std::string &attribute, Callback callback) {
    assert(!dispatching);  // Capability delivery must defer new registrations.
    channels.push_back({entity, attribute, std::move(callback)});
  }
  bool has(const std::string &entity, const std::string &attribute) const {
    for (const auto &channel : channels)
      if (channel.entity == entity && channel.attribute == attribute) return true;
    return false;
  }
  void publish(const std::string &entity, const std::string &attribute, const char *value) {
    for (const auto &channel : channels) {
      if (channel.entity != entity || channel.attribute != attribute) continue;
      auto callback = channel.callback;
      dispatching = true;
      callback(State(value));
      dispatching = false;
      return;
    }
    assert(false && "Publishing an unregistered attribute");
  }
};
struct FakeHeapProbe { bool available(const char *, size_t, size_t) { return true; } };
using Coordinator = HaReadCoordinator<FakeTransport, FakeHeapProbe>;
Coordinator coordinator;
void *callback_owner = nullptr;
size_t reannouncements = 0;
std::function<void()> on_reannounce;
uint32_t ha_subscription_generation() { return coordinator.generation(); }
void ha_release_callbacks_for_owner(void *owner) { coordinator.release_owner(owner); }
struct HaCallbackOwnerScope {
  void *previous;
  explicit HaCallbackOwnerScope(void *owner) : previous(callback_owner) { callback_owner = owner; }
  ~HaCallbackOwnerScope() { callback_owner = previous; }
};
bool ha_subscribe_attribute(const std::string &entity, const std::string &attribute,
                            FakeTransport::Callback callback) {
  assert(!coordinator.transport().dispatching);
  return coordinator.subscribe(entity, attribute, std::move(callback), 1, callback_owner);
}
bool ha_subscribe_state(const std::string &entity, FakeTransport::Callback callback) {
  return ha_subscribe_attribute(entity, "", std::move(callback));
}
bool ha_api_state_connected() { return coordinator.state_connected(); }
void ha_reannounce_state_subscriptions() {
  assert(!coordinator.transport().dispatching);
  ++reannouncements;
  if (on_reannounce) on_reannounce();
}

// Rendering is outside this test. The production callbacks still update the
// real context fields before calling these UI boundaries.
void climate_update_card(ClimateControlCtx *) {}
void climate_control_set_modal_value(ClimateControlCtx *) {}
void climate_select_target_for_mode(ClimateControlCtx *, const std::string &) {}
void climate_cancel_temperature_send(ClimateControlCtx *) {}
void climate_normalize_range(ClimateControlCtx *) {}
struct ModalUi { ClimateControlCtx *active = nullptr; } modal;
ModalUi &climate_control_modal_ui() { return modal; }
void climate_control_hide_modal() { modal.active = nullptr; }

// @production-runtime

struct FixtureCard { const char *entity; const char *tabs; };
// @production-fixture

ClimateControlCtx *add_card(const std::string &entity, const char *tabs) {
  auto *ctx = new ClimateControlCtx;
  ctx->entity_id = entity;
  ctx->configured_tab_mask = espcontrol::climate::configured_climate_tab_mask(tabs);
  climate_control_refs()[climate_control_ref_count()++] = ctx;
  subscribe_climate_control_state(ctx);
  return ctx;
}
void remove_card(ClimateControlCtx *ctx) {
  delete_climate_control_context(ctx);
}
void reset() {
  while (climate_control_ref_count()) remove_card(climate_control_refs()[0]);
  if (!timers.empty()) run_maintenance();
  coordinator = Coordinator{};
  callback_owner = nullptr;
  reannouncements = 0;
  on_reannounce = {};
  assert(climate_optional_subscription_timer() == nullptr);
}

void fixture_registers_only_needed_attributes() {
  reset();
  for (size_t i = 0; i < fixture_other_channels; ++i)
    coordinator.subscribe("sensor.other_" + std::to_string(i), "", [](auto) {}, 1);
  for (const auto &card : fixture_cards) {
    add_card(card.entity, card.tabs);
    for (const char *attribute : {"supported_features", "hvac_modes", "preset_modes", "fan_modes", "swing_modes"})
      assert(coordinator.transport().has(card.entity, attribute));
    for (const char *attribute : {"preset_mode", "fan_mode", "swing_mode"})
      assert(!coordinator.transport().has(card.entity, attribute));
  }
  assert(coordinator.subscription_count() == fixture_expected_channels);
  assert(coordinator.transport().channels.size() == fixture_expected_channels);
  assert(timers.empty());
}

void configured_values_are_registered_immediately() {
  reset();
  auto *ctx = add_card("climate.all", "temperature|mode|preset|fan|swing");
  assert(coordinator.subscription_count() == 18);
  coordinator.transport().publish(ctx->entity_id, "preset_mode", "eco");
  coordinator.transport().publish(ctx->entity_id, "fan_mode", "auto");
  coordinator.transport().publish(ctx->entity_id, "swing_mode", "vertical");
  assert(ctx->preset_mode == "eco" && ctx->fan_mode == "auto" && ctx->swing_mode == "vertical");
  assert(timers.empty());
}

void fallback_delivery_reannounces_historical_channels() {
  reset();
  auto *old = add_card("climate.room", "preset");
  remove_card(old);
  // HA can deliver the historical channel while it has no active listener.
  coordinator.transport().publish("climate.room", "preset_mode", "eco");
  auto *ctx = add_card("climate.room", "temperature");
  const size_t channels = coordinator.subscription_channel_count();
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  coordinator.transport().publish(ctx->entity_id, "preset_modes", "['eco', 'boost']");
  assert(ctx->preset_mode.empty());
  assert(coordinator.subscription_count() == 15);
  on_reannounce = [&] { coordinator.transport().publish(ctx->entity_id, "preset_mode", "eco"); };
  run_maintenance();
  assert(coordinator.subscription_channel_count() == channels);
  assert(coordinator.subscription_count() == 16);
  assert(reannouncements == 1 && ctx->preset_mode == "eco");
  coordinator.transport().publish(ctx->entity_id, "preset_modes", "['eco', 'boost']");
  assert(timers.empty());  // Repeated capabilities do not add duplicate callbacks.
  remove_card(ctx);
  assert(coordinator.subscription_count() == 0);  // Delayed callback kept its owner.
}

void deleted_card_does_not_receive_delayed_work() {
  reset();
  auto *ctx = add_card("climate.deleted", "temperature");
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  coordinator.transport().publish(ctx->entity_id, "fan_modes", "['auto']");
  remove_card(ctx);
  auto *replacement = add_card("climate.replacement", "temperature");
  run_maintenance();
  assert(!coordinator.transport().has("climate.deleted", "fan_mode"));
  assert(!coordinator.transport().has(replacement->entity_id, "fan_mode"));
  assert(reannouncements == 0);
}

void maintenance_rechecks_capabilities_and_batches_cards() {
  reset();
  auto *first = add_card("climate.first", "temperature");
  auto *second = add_card("climate.second", "temperature");
  auto *third = add_card("climate.third", "temperature");
  for (auto *ctx : {first, second, third}) {
    coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
    coordinator.transport().publish(ctx->entity_id, "fan_modes", "['auto']");
  }
  // Temperature becomes available before maintenance; its fallback is obsolete.
  coordinator.transport().publish(first->entity_id, "supported_features", "1");
  assert(timers.size() == 1);
  run_maintenance();
  assert(!coordinator.transport().has(first->entity_id, "fan_mode"));
  assert(coordinator.transport().has(second->entity_id, "fan_mode"));
  assert(coordinator.transport().has(third->entity_id, "fan_mode"));
  assert(reannouncements == 1);
  coordinator.transport().publish(second->entity_id, "fan_mode", "auto");
  assert(second->fan_mode == "auto");
}

void disconnected_maintenance_waits_for_connection_replay() {
  reset();
  auto *ctx = add_card("climate.offline", "temperature");
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  coordinator.transport().publish(ctx->entity_id, "swing_modes", "['vertical']");
  coordinator.transport().connected = false;
  run_maintenance();
  assert(reannouncements == 0 && ctx->swing_mode.empty());
  assert(coordinator.transport().has(ctx->entity_id, "swing_mode"));
  coordinator.transport().connected = true;
  // The next connection's initial state replay supplies the delayed value.
  coordinator.transport().publish(ctx->entity_id, "swing_mode", "vertical");
  assert(ctx->swing_mode == "vertical");
}

void temperature_fallback_does_not_subscribe_to_preset() {
  reset();
  auto *ctx = add_card("climate.temperature", "fan");
  coordinator.transport().publish(ctx->entity_id, "supported_features", "1");
  coordinator.transport().publish(ctx->entity_id, "preset_modes", "['eco']");
  assert(timers.empty());
  assert(!coordinator.transport().has(ctx->entity_id, "preset_mode"));
}

void failed_registration_retries_without_new_capabilities() {
  reset();
  auto *ctx = add_card("climate.retry", "temperature");
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  coordinator.transport().publish(ctx->entity_id, "fan_modes", "['auto']");
  auto *timer = timers.front();
  coordinator.transport().api_available = false;
  run_maintenance(true);
  run_maintenance(true);
  assert(timers.front() == timer && reannouncements == 0);
  assert(ctx->optional_subscriptions.pending == espcontrol::climate::OPTIONAL_SUBSCRIPTION_FAN);
  coordinator.transport().api_available = true;
  // No further capability updates: retry alone must finish registration.
  run_maintenance();
  assert(coordinator.subscription_count() == 16 && reannouncements == 1);
  coordinator.transport().publish(ctx->entity_id, "fan_mode", "auto");
  assert(ctx->fan_mode == "auto");
  coordinator.transport().publish(ctx->entity_id, "fan_modes", "['auto']");
  assert(timers.empty());
}

void failed_registration_is_cancelled_by_deletion_or_capability_change() {
  reset();
  auto *ctx = add_card("climate.obsolete", "temperature");
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  coordinator.transport().publish(ctx->entity_id, "fan_modes", "['auto']");
  coordinator.transport().api_available = false;
  run_maintenance(true);
  coordinator.transport().publish(ctx->entity_id, "supported_features", "1");
  run_maintenance();
  assert(ctx->optional_subscriptions.pending == 0 && reannouncements == 0);
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  run_maintenance(true);
  remove_card(ctx);
  run_maintenance();
  assert(coordinator.subscription_count() == 0 && reannouncements == 0);
}

void context_teardown_preserves_other_cards_on_the_same_page() {
  reset();
  int page = 0;
  HaCallbackOwnerScope page_scope(&page);
  auto *first = add_card("climate.same", "fan");
  auto *second = add_card("climate.same", "fan");
  assert(callback_owner == &page);
  // The first channel listener destroys its context during dispatch. The
  // second card must still receive this update, with no generation change.
  bool removed = false;
  coordinator.subscribe("climate.same", "preset_mode", [&](auto) {
    remove_card(first);
    removed = true;
  }, 1, &page);
  climate_subscribe_optional_fields(first, espcontrol::climate::OPTIONAL_SUBSCRIPTION_PRESET);
  climate_subscribe_optional_fields(second, espcontrol::climate::OPTIONAL_SUBSCRIPTION_PRESET);
  coordinator.transport().publish("climate.same", "preset_mode", "eco");
  assert(removed && second->preset_mode == "eco");
  assert(callback_owner == &page);
  coordinator.transport().publish("climate.same", "fan_mode", "auto");
  assert(second->fan_mode == "auto");
  remove_card(second);
  assert(coordinator.subscription_count() == 1); // Only the page callback remains.
  coordinator.release_owner(&page);
}

void reannouncement_can_queue_the_next_fallback() {
  reset();
  auto *ctx = add_card("climate.next_fallback", "temperature");
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  coordinator.transport().publish(ctx->entity_id, "fan_modes", "['auto']");
  bool first = true;
  on_reannounce = [&] {
    if (!first) return;
    first = false;
    coordinator.transport().publish(ctx->entity_id, "fan_modes", "[]");
    coordinator.transport().publish(ctx->entity_id, "swing_modes", "['vertical']");
  };
  run_maintenance(true);
  run_maintenance();
  assert(coordinator.subscription_count() == 17 && reannouncements == 2);
}

void generation_reset_discards_stale_pending_work() {
  reset();
  auto *ctx = add_card("climate.generation", "temperature");
  coordinator.transport().publish(ctx->entity_id, "supported_features", "0");
  coordinator.transport().publish(ctx->entity_id, "fan_modes", "['auto']");
  coordinator.bump_generation(1);
  run_maintenance();
  assert(coordinator.subscription_count() == 0 && reannouncements == 0);
  assert(!coordinator.transport().has(ctx->entity_id, "fan_mode"));
}

void configured_optional_failures_are_retried() {
  reset();
  coordinator.transport().api_available = false;
  auto *ctx = add_card("climate.configured_retry", "fan");
  run_maintenance(true);
  coordinator.transport().api_available = true;
  run_maintenance();
  assert(coordinator.transport().has(ctx->entity_id, "fan_mode"));
  assert(reannouncements == 1);
}

int main() {
  fixture_registers_only_needed_attributes();
  configured_values_are_registered_immediately();
  fallback_delivery_reannounces_historical_channels();
  deleted_card_does_not_receive_delayed_work();
  maintenance_rechecks_capabilities_and_batches_cards();
  disconnected_maintenance_waits_for_connection_replay();
  temperature_fallback_does_not_subscribe_to_preset();
  failed_registration_retries_without_new_capabilities();
  failed_registration_is_cancelled_by_deletion_or_capability_change();
  context_teardown_preserves_other_cards_on_the_same_page();
  reannouncement_can_queue_the_next_fallback();
  generation_reset_discards_stale_pending_work();
  configured_optional_failures_are_retried();
  reset();
  std::puts("Climate subscription runtime tests passed");
}
