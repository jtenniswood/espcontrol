#pragma once

// Include after button_grid.h's native UI helpers. One bank owns every demanded
// family and all deferred confirmation/alarm/guest commands for the scene.
#include "page_alarm_modal.h"
#include "page_climate_modal.h"
#include "page_confirmation_modal.h"
#include "page_expanded_image_modal.h"
#include "page_fan_preset_modal.h"
#include "page_light_cover_modal.h"
#include "page_media_control_modal.h"
#include "page_media_volume_modal.h"
#include "page_modal_router.h"
#include "page_native_modal_commands.h"
#include "page_network_status_modal.h"
#include "page_option_select_modal.h"
#include "page_weather_modal.h"
#include "page_wifi_qr_modal.h"

namespace espcontrol::pages {
struct PageNativeModalStyles {
  PageClimateModalStyle climate;
  PageLegacyModalStyle<PageLightModalFamily> light;
  PageLegacyModalStyle<PageCoverModalFamily> cover;
  PageLegacyModalStyle<PageFanControlModalFamily> fan;
  PageLegacyModalStyle<PageFanPresetModalFamily> fan_preset;
  PageLegacyModalStyle<PageOptionSelectModalFamily> options;
  PageLegacyModalStyle<PageMediaVolumeModalFamily> volume;
  PageLegacyModalStyle<PageMediaControlModalFamily> media;
  PageWeatherModalStyle weather;
  PageConfirmationModalStyle confirmation;
  PageAlarmModalStyle alarm;
  PageExpandedImageModalStyle image;
  PageWifiQrModalStyle wifi;
  PageNetworkStatusStyle network;
  bool ready() const {
    return climate.resolve && light.resolve && cover.resolve && fan.resolve && fan_preset.resolve &&
           options.resolve && volume.resolve && media.resolve && weather.resolve && confirmation.resolve &&
           alarm.resolve && image.resolve && wifi.resolve && network.resolve;
  }
};
struct PageNativeModalSources {
  PageArenaAllocator& allocator;
  PageEntityStateRuntime& graph;
  PageForecastRuntime& forecasts;
  PageAlarmStateRuntime& alarms;
  PageAlarmAckRuntime& acknowledgements;
  PageQrEncoder& qr;
  PageExpandedImageView& images;
  PageNetworkStatusSource& network;
  PageGuestWifiRuntime::Now now;
  size_t profile_slots;
};
struct PageNativeModalOrigins {
  void* owner{nullptr};
  bool (*grid)(void*, PageBindingToken){nullptr};
  bool (*local)(void*, PageLocalModalOrigin){nullptr};
  bool (*interactive)(void*){nullptr};
};

// Borrowed sources/styles/validators belong to the application scene. No card
// creates a family owner; all fifteen kinds share exactly one demanded opening.
// Construct the complete bank before initialize: constructors only store the
// reference to runtime_ declared last, and never query it before construction.
class PageNativeModalBank final : public PageRuntimeResources {
 public:
  PageNativeModalBank(PageNativeModalSources sources, const PageNativeModalStyles& styles,
                      PageNativeModalOrigins origins)
      : configured_(styles.ready() && origins.grid && origins.local && sources.now &&
                    sources.profile_slots > 0 && sources.profile_slots <= 255),
        confirmation_port_(runtime_, origins.owner, origins.grid),
        alarm_port_(runtime_, this, grid_origin, selected_pin),
        guest_port_(runtime_),
        confirmation_(sources.graph, confirmation_port_),
        alarm_commands_(sources.graph, sources.alarms, sources.acknowledgements, alarm_port_),
        guest_(sources.graph, guest_port_, sources.now),
        climate_(sources.allocator, sources.graph, styles.climate, origins.owner, origins.grid),
        light_(sources.allocator, sources.graph, styles.light, origins.owner, origins.grid),
        cover_(sources.allocator, sources.graph, styles.cover, origins.owner, origins.grid),
        fan_(sources.allocator, sources.graph, styles.fan, origins.owner, origins.grid),
        fan_preset_(sources.allocator, sources.graph, styles.fan_preset, origins.owner, origins.grid),
        options_(sources.allocator, sources.graph, styles.options, origins.owner, origins.grid),
        volume_(sources.allocator, sources.graph, styles.volume, origins.owner, origins.grid),
        media_(sources.allocator, sources.graph, styles.media, origins.owner, origins.grid),
        weather_(sources.allocator, sources.forecasts, styles.weather, origins.owner, origins.grid),
        confirmation_modal_(sources.allocator, sources.graph, styles.confirmation, origins.owner,
                            origins.grid),
        alarm_modal_(sources.allocator, sources.graph, sources.alarms, alarm_commands_, styles.alarm,
                     origins.owner, origins.grid),
        image_(sources.allocator, sources.graph, sources.images, styles.image, origins.owner, origins.grid),
        wifi_(sources.allocator, sources.graph, sources.qr, guest_, styles.wifi, origins.owner, origins.grid),
        network_(sources.allocator, sources.network, sources.profile_slots, styles.network, origins.owner,
                 origins.local),
        origins_(origins),
        router_({{{ControlModalKind::CLIMATE, &climate_},
                  {ControlModalKind::LIGHT_CONTROL, &light_},
                  {ControlModalKind::COVER_CONTROL, &cover_},
                  {ControlModalKind::FAN_CONTROL, &fan_},
                  {ControlModalKind::FAN_PRESET, &fan_preset_},
                  {ControlModalKind::OPTION_SELECT, &options_},
                  {ControlModalKind::MEDIA_VOLUME, &volume_},
                  {ControlModalKind::MEDIA_CONTROL, &media_},
                  {ControlModalKind::WEATHER_FORECAST, &weather_},
                  {ControlModalKind::SWITCH_CONFIRMATION, &confirmation_modal_},
                  {ControlModalKind::ALARM_PIN, &alarm_modal_},
                  {ControlModalKind::ALARM_CONTROL, &alarm_modal_},
                  {ControlModalKind::IMAGE_CARD, &image_},
                  {ControlModalKind::WIFI_QR, &wifi_},
                  {ControlModalKind::NETWORK_STATUS, &network_}}},
                origins.owner, origins.grid),
        runtime_(router_, 1, origins.owner, origins.interactive) {}
  PageNativeModalBank(const PageNativeModalBank&) = delete;
  PageNativeModalBank& operator=(const PageNativeModalBank&) = delete;

  bool initialize(bool legacy_renderer_retired) {
    if (stopped_) return false;
    if (initialized_) return true;
    if (!configured_ || !legacy_renderer_retired || !page_native_command_ready()) return false;
    // Failed setup is retryable before configuration activation. No family may
    // admit work through the bank until every native lifetime hook is installed.
    initialized_ = climate_.install(runtime_) && light_.install(runtime_, true) &&
                   cover_.install(runtime_, true) && fan_.install(runtime_, true) &&
                   fan_preset_.install(runtime_, true) && options_.install(runtime_, true) &&
                   volume_.install(runtime_, true) && media_.install(runtime_, true) &&
                   weather_.install(runtime_) && confirmation_modal_.install(runtime_, confirmation_, true) &&
                   alarm_modal_.install(runtime_, true) && image_.install(runtime_, true) &&
                   wifi_.install(runtime_, true) && network_.install(runtime_, true);
    return initialized_;
  }
  bool ready() const { return initialized_ && !stopped_; }
  void refresh_temperature_unit() { weather_.refresh_temperature_unit(); }
  bool maintenance_ready() const {
    return ready() && !running_ && page_native_command_ready() && router_.ready();
  }
  bool prepare(const PageStore& store) override {
    discard_prepared();
    if (!ready() || running_ || !page_native_command_ready() || !runtime_.prepare(store) || !confirmation_.prepare(store) ||
        !alarm_commands_.prepare(store)) {
      discard_prepared();
      return false;
    }
    prepared_ = true;
    return true;
  }
  void discard_prepared() override {
    runtime_.discard_prepared();
    confirmation_.discard_prepared();
    alarm_commands_.discard_prepared();
    prepared_ = false;
  }
  void activate_prepared(const PageStore& store) override {
    if (!prepared_ || stopped_) return;
    // Retire demanded contexts before their command or shared-state storage.
    runtime_.activate_prepared(store);
    confirmation_.activate_prepared(store);
    alarm_commands_.activate_prepared(store);
    guest_.retire();
    prepared_ = false;
  }
  void retire_active() override {
    runtime_.retire_active();
    confirmation_.retire_active();
    alarm_commands_.retire_active();
    guest_.retire();
  }
  void loop() {
    if (!initialized_ || running_ || !page_native_command_ready() || !router_.ready()) return;
    running_ = true;
    if (!runtime_.input_allowed()) {
      confirmation_.cancel_queued();
      alarm_commands_.cancel();
      guest_.cancel_queued();
    }
    // Commands run before modal collection. Accepted effects can use the source
    // binding retained under an overlay; a closed/replaced token cancels first.
    if (!stopped_) confirmation_.loop();
    if (!stopped_) alarm_commands_.loop();
    if (!stopped_) guest_.loop();
    runtime_.loop();
    running_ = false;
  }
  void stop() {
    stopped_ = true;
    runtime_.stop();
    confirmation_.cancel();
    alarm_commands_.cancel();
    guest_.retire();
  }
  PageModalRuntime& runtime() { return runtime_; }
  const PageModalRuntime& runtime() const { return runtime_; }
  PageConfirmationRuntime& confirmations() { return confirmation_; }
  PageAlarmCommands& alarm_commands() { return alarm_commands_; }

 private:
  static bool grid_origin(void* owner, PageBindingToken token) {
    const auto& origins = static_cast<PageNativeModalBank*>(owner)->origins_;
    return origins.grid && origins.grid(origins.owner, token);
  }
  static bool selected_pin(void* owner, PageModalToken token, alarm::Action action) {
    return static_cast<PageNativeModalBank*>(owner)->alarm_modal_.selected_pin(token, action);
  }
  bool configured_{false}, initialized_{false}, prepared_{false}, running_{false}, stopped_{false};
  PageNativeConfirmationPort confirmation_port_;
  PageNativeAlarmCommandPort alarm_port_;
  PageNativeGuestWifiPort guest_port_;
  PageConfirmationRuntime confirmation_;
  PageAlarmCommands alarm_commands_;
  PageGuestWifiRuntime guest_;
  PageClimateModalPort climate_;
  PageLegacyModalPort<PageLightModalFamily> light_;
  PageLegacyModalPort<PageCoverModalFamily> cover_;
  PageLegacyModalPort<PageFanControlModalFamily> fan_;
  PageLegacyModalPort<PageFanPresetModalFamily> fan_preset_;
  PageLegacyModalPort<PageOptionSelectModalFamily> options_;
  PageLegacyModalPort<PageMediaVolumeModalFamily> volume_;
  PageLegacyModalPort<PageMediaControlModalFamily> media_;
  PageWeatherModalPort weather_;
  PageConfirmationModalPort confirmation_modal_;
  PageAlarmModalPort alarm_modal_;
  PageExpandedImageModalPort image_;
  PageWifiQrModalPort wifi_;
  PageNetworkStatusModalPort network_;
  PageNativeModalOrigins origins_;
  PageModalRouter<15> router_;
  PageModalRuntime runtime_;
};
}  // namespace espcontrol::pages
