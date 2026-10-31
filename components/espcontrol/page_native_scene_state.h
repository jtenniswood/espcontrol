#pragma once

// Include after button_grid.h and the clock/date producer helpers. This is the
// concrete scene's configuration-owned state, independent of visible shells.
#include <cassert>
#include <ctime>
#include <type_traits>

#include "page_access_state_runtime.h"
#include "page_alarm_ack_runtime.h"
#include "page_calendar_events.h"
#include "page_card_dependencies.h"
#include "page_cover_toggle_runtime.h"
#include "page_entity_coordinator_adapter.h"
#include "page_entity_observer_set.h"
#include "page_fan_status_runtime.h"
#include "page_light_temperature_runtime.h"
#include "page_local_relay_runtime.h"
#include "page_local_sensor_runtime.h"
#include "page_number_slider_runtime.h"
#include "page_percent_slider_runtime.h"
#include "page_runtime_resources_group.h"
#include "page_timer_runtime.h"
#include "page_timezone_events.h"

namespace espcontrol::pages {
struct PageNativeStateBudget {
  size_t cards{0}, keys{0};
  PageEntityBudget ha{}, local{};
};

class PageNativeSceneState final : public PageRuntimeResources {
public:
  static constexpr uint32_t SUBSCRIPTION_SCOPE = 1u << 4;
  PageNativeSceneState(PageArenaAllocator &allocator,
                       PageNativeStateBudget budget,
                       PageTimezoneResolver timezone_resolver,
                       PageStatusCaptions captions,
                       PageAlarmStateRuntime::Translate translate)
      : subscriptions(ha_read_coordinator(), SUBSCRIPTION_SCOPE),
        graph(dependencies, allocator, subscriptions, budget.ha, &observers),
        media(allocator, media_transport),
        forecasts(allocator, forecast_requests),
        local(allocator, local_provider, budget.local),
        relays(allocator, relay_provider, budget.cards, budget.keys),
        fans(allocator, subscriptions, now, budget.cards, budget.keys),
        percent(allocator, subscriptions, budget.cards, budget.cards,
                budget.keys),
        number(allocator, subscriptions, budget.cards, budget.cards,
               budget.keys),
        temperature(allocator, subscriptions, budget.cards, budget.cards,
                    budget.keys),
        timers(allocator, subscriptions, now, epoch, budget.cards, budget.cards,
               budget.keys),
        covers(allocator, subscriptions, now, budget.cards, budget.keys,
               captions),
        access(allocator, subscriptions, now, budget.cards, budget.keys,
               captions),
        timezones(timezone_resolver),
        alarms(allocator, subscriptions, now, budget.cards, budget.keys,
               translate),
        acknowledgements(allocator, subscriptions, alarms, now, budget.cards),
        calendar_events(calendar), timezone_events(timezones),
        resources({{&media, &forecasts, &local, &relays, &fans, &percent,
                    &number, &temperature, &timers, &covers, &access, &calendar,
                    &timezones, &alarms, &acknowledgements, &graph}}) {
    configured_ = budget.cards && budget.keys && timezone_resolver.resolve &&
                  observers.add(media) && observers.add(fans) &&
                  observers.add(percent) && observers.add(number) &&
                  observers.add(temperature) && observers.add(timers) &&
                  observers.add(covers) && observers.add(access) &&
                  observers.add(calendar) && observers.add(alarms) &&
                  observers.add(acknowledgements);
  }
  ~PageNativeSceneState() override {
    assert(!running_);
    stop();
    resources.discard_prepared();
    resources.retire_active();
  }
  PageNativeSceneState(const PageNativeSceneState &) = delete;
  PageNativeSceneState &operator=(const PageNativeSceneState &) = delete;
  bool initialize(bool legacy_renderer_retired) {
    if (!configured_ || stopped_ || !legacy_renderer_retired ||
        !page_native_command_ready())
      return false;
    if (!local.install_manual_feed(true) || !relays.install(true))
      return false;
    if (!calendar_installed_)
      calendar_installed_ = calendar_events.install(true);
    if (!timezone_installed_)
      timezone_installed_ = timezone_events.install(true);
    initialized_ = calendar_installed_ && timezone_installed_;
    return initialized_;
  }
  bool ready() const { return initialized_ && !stopped_; }
  bool prepare(const PageStore &store) override {
    return ready() && !running_ && page_native_command_ready() &&
           resources.prepare(store);
  }
  void discard_prepared() override { resources.discard_prepared(); }
  void activate_prepared(const PageStore &store) override {
    if (!ready() || running_)
      return;
    resources.activate_prepared(store);
  }
  void retire_active() override { resources.retire_active(); }
  void loop() {
    if (!ready() || running_ || !page_native_command_ready())
      return;
    running_ = true;
    (void)graph
        .revision(); // Synchronize only when the HA connection epoch changed.
    local.loop();
    media.loop();
    fans.loop();
    percent.loop();
    number.loop();
    temperature.loop();
    timers.loop();
    covers.loop();
    access.loop();
    alarms.loop();
    acknowledgements.loop();
    if (!stopped_)
      forecasts.loop();
    running_ = false;
  }
  // Scene shutdown keeps backing storage alive until its application owner can
  // retire it safely; revoke producer facades immediately without dispatching.
  void stop() {
    stopped_ = true;
    calendar_events.uninstall();
    timezone_events.uninstall();
    if (page_local_sensor_feed().owner == &local)
      page_local_sensor_feed() = {};
    if (page_local_relay_feed().owner == &relays)
      page_local_relay_feed() = {};
  }
  static uint32_t now() { return esphome::millis(); }
  static int64_t epoch() { return static_cast<int64_t>(std::time(nullptr)); }

  using Coordinator = std::remove_reference_t<decltype(ha_read_coordinator())>;
  CoordinatorPageEntitySubscriptions<Coordinator> subscriptions;
  PageCardDependencies dependencies;
  // Eleven model observers plus three fixed artwork/tile/expanded observers.
  // The complete scene registers/removes the latter while those owners live.
  PageEntityObserverSet<14> observers;
  PageEntityStateRuntime graph;
  PageHaMediaTransport media_transport;
  PageHaForecastRequests forecast_requests;
  PageESPHomeLocalSensorProvider<LOCAL_NUMERIC_PRODUCERS, LOCAL_TEXT_PRODUCERS>
      local_provider;
  PageInternalRelayProvider relay_provider;
  PageMediaStateRuntime media;
  PageForecastRuntime forecasts;
  PageLocalSensorRuntime local;
  PageLocalRelayRuntime relays;
  PageFanStatusRuntime fans;
  PagePercentSliderRuntime percent;
  PageNumberSliderRuntime number;
  PageLightTemperatureRuntime temperature;
  PageTimerRuntime timers;
  PageCoverToggleRuntime covers;
  PageAccessStateRuntime access;
  PageCalendarRuntime calendar;
  PageTimezoneRuntime timezones;
  PageAlarmStateRuntime alarms;
  PageAlarmAckRuntime acknowledgements;
  PageCalendarEvents calendar_events;
  PageTimezoneEvents timezone_events;

private:
  PageRuntimeResourcesGroup<16> resources;
  bool configured_{false}, initialized_{false}, stopped_{false},
      running_{false}, calendar_installed_{false}, timezone_installed_{false};
};
} // namespace espcontrol::pages
