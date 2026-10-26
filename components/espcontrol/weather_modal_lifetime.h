#pragma once
#include "modal_context_lifetime.h"
struct WeatherDailySource;
using WeatherModalLifetime = espcontrol::ModalContextLifetime<WeatherDailySource>;
inline WeatherModalLifetime& weather_modal_lifetime() {
  static WeatherModalLifetime observer;
  return observer;
}
inline bool weather_modal_managed(const WeatherDailySource* source) {
  return weather_modal_lifetime().managed_context(source);
}
inline bool weather_modal_context_current(const WeatherDailySource* source) {
  return weather_modal_lifetime().current_context(source);
}
inline void weather_modal_notify_closed(WeatherDailySource* source) {
  weather_modal_lifetime().notify_closed(source);
}
inline bool weather_modal_widgets_ready(void* root) {
  return weather_modal_lifetime().widgets_ready(root);
}
