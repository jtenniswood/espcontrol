#pragma once

#include "page_modal_admission.h"
// Include after the existing weather modal/HA/shell helpers.
#include <cassert>
#include <new>

#include "page_forecast_runtime.h"
#include "page_lvgl_dispatch.h"
#include "page_modal_gestures.h"
namespace espcontrol::pages {
struct PageWeatherModalStyle {
  const lv_font_t *font{nullptr}, *icon_font{nullptr}, *temperature_font{nullptr};
  int radius{12};
  size_t gesture_capacity{128}, internal_free_min{24 * 1024}, internal_largest_min{8 * 1024};
  void* owner{nullptr};
  // Resolve by the opening's original configuration/card identity, including
  // hidden-page fonts; never infer the source from the current visible grid.
  bool (*resolve)(void*, PageModalToken, const ParsedCfg&, PageWeatherModalStyle&){nullptr};
};
class PageWeatherModalPort final : public PageModalPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageWeatherModalPort(PageArenaAllocator& allocator, PageForecastRuntime& forecasts,
                       PageWeatherModalStyle style, void* owner, OriginValid valid)
      : allocator_(allocator), forecasts_(forecasts), style_(style), owner_(owner), valid_(valid) {}
  ~PageWeatherModalPort() override {
    assert(!record_);
    if (weather_modal_lifetime().owner == this) weather_modal_lifetime() = {};
  }
  PageWeatherModalPort(const PageWeatherModalPort&) = delete;
  PageWeatherModalPort& operator=(const PageWeatherModalPort&) = delete;
  bool install(PageModalRuntime& runtime) {
    if (!lv_is_initialized() || record_ || weather_daily_modal().overlay || !weather_daily_cards().empty() ||
        !weather_daily_sources().empty() ||
        (weather_modal_lifetime().owner && weather_modal_lifetime().owner != this))
      return false;
    runtime_ = &runtime;
    weather_modal_lifetime() = {this, managed, current, current, closed, widgets};
    return true;
  }
  bool valid_origin(PageBindingToken token) const override {
    return valid_ && valid_(owner_, token);
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle handle) const override {
    if (!runtime_ || !style_.resolve || weather_modal_lifetime().owner != this) return ControlModalKind::NONE;
    const auto* card = store.card(handle);
    if (!card) return ControlModalKind::NONE;
    const auto type = store.field(*card, CardField::TYPE),
               precision = store.field(*card, CardField::PRECISION);
    if (!card_runtime_weather_forecast_supported() ||
        (type != "weather_forecast" && (type != "weather" || !admission::forecast_precision(precision))))
      return ControlModalKind::NONE;
    PageForecastSnapshot snapshot;
    if (!forecasts_.snapshot(handle, store.field(*card, CardField::ENTITY), snapshot))
      return ControlModalKind::NONE;
    const auto view = precision == "today"                                    ? weather::View::TODAY
                      : precision == "tomorrow" || type == "weather_forecast" ? weather::View::TOMORROW
                                                                              : weather::View::DAILY;
    return view == weather::View::DAILY ||
                   weather::selected_day(snapshot.forecast, view) < snapshot.forecast.count
               ? ControlModalKind::WEATHER_FORECAST
               : ControlModalKind::NONE;
  }
  bool ready() const override {
    return lv_is_initialized() && !page_lvgl_dispatching() && !ha_read_coordinator().dispatching();
  }
  // This is a read-only view of owned cached data. It remains usable offline;
  // the forecast owner marks stale data and rejects late network responses.
  bool connected() const override {
    return true;
  }
  uint32_t connection_generation() const override {
    return 0;
  }
  void* create(const PageStore& store, ConfigurationHandle handle, PageModalToken token) override {
    if (record_ || kind(store, handle) != ControlModalKind::WEATHER_FORECAST ||
        !ha_internal_heap_available("forecast panel metadata"))
      return nullptr;
    auto environment = style_;
    const auto* card = store.card(handle);
    ParsedCfg config;
    config.entity = std::string(store.field(*card, CardField::ENTITY));
    config.type = std::string(store.field(*card, CardField::TYPE));
    config.precision = std::string(store.field(*card, CardField::PRECISION));
    config.options = std::string(store.field(*card, CardField::OPTIONS));
    if (!style_.resolve(style_.owner, token, config, environment) || !environment.font ||
        !environment.icon_font || !environment.temperature_font ||
        !ha_internal_heap_available("forecast panel setup", environment.internal_free_min,
                                    environment.internal_largest_min))
      return nullptr;
    auto* memory = allocator_.allocate(sizeof(Record));
    if (!memory) return nullptr;
    auto* record = new (memory) Record(*this, token, handle, environment.gesture_capacity);
    if (!record->gestures.available()) {
      release(record);
      return nullptr;
    }
    record->source.entity_id = std::move(config.entity);
    record->card.source = &record->source;
    record->card.view = config.precision == "today" ? weather::View::TODAY
                        : config.precision == "tomorrow" || config.type == "weather_forecast"
                            ? weather::View::TOMORROW
                            : weather::View::DAILY;
    record->card.detail_font = environment.font;
    record->card.font = environment.font;
    record->card.icon_font = environment.icon_font;
    record->card.temperature_font = environment.temperature_font;
    record->radius = environment.radius;
    record_ = record;
    return record;
  }
  bool initialize(void* pointer, PageModalToken) override {
    auto& record = *static_cast<Record*>(pointer);
    if (control_modal_active().kind != ControlModalKind::NONE || !refresh(record)) return false;
    // Radius comes from the profile, not from a pointer into a reusable grid.
    record.card.owner = lv_obj_create(lv_layer_top());
    if (!record.card.owner) return false;
    lv_obj_add_flag(record.card.owner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(record.card.owner, 1, 1);
    lv_obj_set_style_radius(record.card.owner, record.radius, LV_PART_MAIN);
    weather_daily_open_modal(&record.card);
    return visible(pointer);
  }
  bool visible(void* pointer) const override {
    const auto& record = *static_cast<Record*>(pointer);
    return !record.retired && weather_daily_modal().source == &record.source && weather_daily_modal().overlay;
  }
  void update(void* pointer, PageModalToken) override {
    auto& record = *static_cast<Record*>(pointer);
    if (record.retired || (record.seen == forecasts_.revision() && record.palette == &current_theme()))
      return;
    if (!refresh(record)) {
      runtime_->notify_closed(record.token, PageModalClose::REPLACED);
      return;
    }
    weather_daily_refresh_modal();
  }
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return ControlModalDismissPolicy::DISMISS;
  }
  void retire(void* pointer, PageModalClose reason) override {
    auto& record = *static_cast<Record*>(pointer);
    record.retired = true;
    record.gestures.invalidate();
    if (weather_daily_modal().source != &record.source) return;
    if (reason == PageModalClose::REPLACED || reason == PageModalClose::SHUTDOWN) {
      auto* overlay = weather_daily_modal().overlay;
      if (overlay) lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
      if (control_modal_active().overlay == overlay) control_modal_service().reset_active();
    } else
      weather_daily_hide_modal();
  }
  void destroy(void* pointer) override {
    auto* record = static_cast<Record*>(pointer);
    assert(record == record_ && record->retired);
    if (weather_daily_modal().source == &record->source) weather_daily_hide_modal();
    assert(record->source.users == 0 && record->gestures.live() == 0);
    if (record->card.owner) lv_obj_delete(record->card.owner);
    record_ = nullptr;
    release(record);
  }

 private:
  struct Record {
    Record(PageWeatherModalPort& port, PageModalToken token, ConfigurationHandle handle, size_t capacity)
        : token(token), handle(handle), gestures(port.allocator_, capacity, token, &port, gesture_valid) {}
    WeatherDailySource source;
    WeatherDailyCard card;
    PageModalToken token;
    ConfigurationHandle handle;
    PageModalGestures gestures;
    uint64_t seen{UINT64_MAX};
    const void* palette{nullptr};
    int radius{12};
    bool retired{false};
  };
  void release(Record* record) {
    record->~Record();
    allocator_.release(record, sizeof(Record));
  }
  bool refresh(Record& record) {
    PageForecastSnapshot snapshot;
    if (!forecasts_.snapshot(record.handle, record.source.entity_id, snapshot)) return false;
    record.source.forecast = snapshot.forecast;
    record.source.loading = snapshot.loading;
    record.source.stale = snapshot.stale;
    record.seen = forecasts_.revision();
    record.palette = &current_theme();
    return true;
  }
  static bool managed(void* owner, const WeatherDailySource* source) {
    const auto& port = *static_cast<PageWeatherModalPort*>(owner);
    return port.record_ && &port.record_->source == source;
  }
  static bool current(void* owner, const WeatherDailySource* source) {
    const auto& port = *static_cast<PageWeatherModalPort*>(owner);
    return managed(owner, source) && !port.record_->retired && port.runtime_->valid(port.record_->token);
  }
  static bool gesture_valid(void* owner, PageModalToken token) {
    const auto& port = *static_cast<PageWeatherModalPort*>(owner);
    return port.record_ && !port.record_->retired && port.runtime_->interactive(token);
  }
  static void closed(void* owner, WeatherDailySource* source) {
    auto& port = *static_cast<PageWeatherModalPort*>(owner);
    if (managed(owner, source) && !port.record_->retired) port.runtime_->notify_closed(port.record_->token);
  }
  static bool widgets(void* owner, void* root) {
    auto& port = *static_cast<PageWeatherModalPort*>(owner);
    return port.record_ && !port.record_->retired &&
           port.record_->gestures.attach(static_cast<lv_obj_t*>(root));
  }
  PageArenaAllocator& allocator_;
  PageForecastRuntime& forecasts_;
  PageWeatherModalStyle style_;
  void* owner_{nullptr};
  OriginValid valid_{nullptr};
  PageModalRuntime* runtime_{nullptr};
  Record* record_{nullptr};
};
}  // namespace espcontrol::pages
