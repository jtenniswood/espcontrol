#include <cstdlib>
#include <string>

#include "button_grid_limits.h"
#include "button_grid_card_runtime.h"
#include "button_grid_string.h"

int main() {
  static_assert(MAX_GRID_SLOTS == ESPCONTROL_MAX_GRID_SLOTS);
  static_assert(MAX_SUBPAGE_ITEMS == MAX_GRID_SLOTS * MAX_GRID_SLOTS);

  if (string_ref_limited(esphome::StringRef("calendar"), 4) != "cale") return EXIT_FAILURE;
  if (string_ref_limited(esphome::StringRef("clock"), 32) != "clock") return EXIT_FAILURE;

  using espcontrol::cards::Family;
  const auto media = card_runtime_registration("media");
  if (media.version != 1 || !media.known || !media.allow_in_subpage ||
      media.family != Family::MEDIA) return EXIT_FAILURE;
  if (card_runtime_context("climate_control").family != Family::CLIMATE ||
      card_runtime_context("light_brightness").family != Family::SLIDER ||
      card_runtime_context("fan_speed").family != Family::FAN ||
      card_runtime_context("not_a_card").family != Family::UNKNOWN) {
    return EXIT_FAILURE;
  }
  if (!card_runtime_uses_slider_visual(card_runtime_context("light_brightness")) ||
      !card_runtime_uses_slider_visual(card_runtime_context("cover")) ||
      !card_runtime_uses_slider_visual(card_runtime_context("fan_speed")) ||
      card_runtime_uses_slider_visual(card_runtime_context("fan_switch"))) {
    return EXIT_FAILURE;
  }
  const auto door = card_runtime_context("door_window");
  const auto presence = card_runtime_context("presence");
  const auto image = card_runtime_context("image");
  const auto clock = card_runtime_context("clock");
  const auto timezone = card_runtime_context("timezone");
  const auto calendar = card_runtime_context("calendar");
  const auto sensor = card_runtime_context("sensor");
  const auto local_sensor = card_runtime_context("local_sensor");
  const auto legacy_text_sensor = card_runtime_context("text_sensor");
  const auto weather = card_runtime_context("weather");
  const auto weather_forecast = card_runtime_context("weather_forecast");
  const auto toggle = card_runtime_context("");
  const auto action = card_runtime_context("action");
  const auto alarm_action = card_runtime_context("alarm_action");
  const auto fan_switch = card_runtime_context("fan_switch");
  const auto internal = card_runtime_context("internal");
  const auto light_switch = card_runtime_context("light_switch");
  const auto local_action = card_runtime_context("local");
  const auto push = card_runtime_context("push");
  const auto screen_lock = card_runtime_context("screen_lock");
  const auto webhook = card_runtime_context("webhook");
  if (!card_runtime_information_only(door) || !card_runtime_passive(door) ||
      door.legacy_dispatch || presence.legacy_dispatch ||
      clock.runtime.driver != espcontrol::card_runtime::CardDriverId::DATE_TIME ||
      timezone.runtime.driver != espcontrol::card_runtime::CardDriverId::DATE_TIME ||
      calendar.runtime.driver != espcontrol::card_runtime::CardDriverId::DATE_TIME ||
      clock.legacy_dispatch || timezone.legacy_dispatch || calendar.legacy_dispatch ||
      sensor.runtime.driver != espcontrol::card_runtime::CardDriverId::SENSOR ||
      local_sensor.runtime.driver != espcontrol::card_runtime::CardDriverId::SENSOR ||
      legacy_text_sensor.runtime.driver != espcontrol::card_runtime::CardDriverId::SENSOR ||
      sensor.legacy_dispatch || local_sensor.legacy_dispatch ||
      legacy_text_sensor.legacy_dispatch ||
      weather.runtime.driver != espcontrol::card_runtime::CardDriverId::WEATHER ||
      weather_forecast.runtime.driver != espcontrol::card_runtime::CardDriverId::WEATHER ||
      weather.legacy_dispatch || weather_forecast.legacy_dispatch ||
      toggle.legacy_dispatch || action.legacy_dispatch ||
      alarm_action.legacy_dispatch || fan_switch.legacy_dispatch ||
      internal.legacy_dispatch || light_switch.legacy_dispatch ||
      local_action.legacy_dispatch || push.legacy_dispatch ||
      screen_lock.legacy_dispatch || webhook.legacy_dispatch ||
      !card_runtime_information_only(image) || card_runtime_passive(image) ||
      !image.legacy_dispatch) {
    return EXIT_FAILURE;
  }
  struct TestConfig {
    std::string type;
    std::string sensor;
  };
  const auto cover = card_runtime_context(
      TestConfig{"cover", "tilt"}, espcontrol::cards::Surface::SUBPAGE);
  const auto option_select_compatibility = card_runtime_context(
      TestConfig{"action", card_runtime_option_select_canonical_action()});
  if (cover.runtime.type != espcontrol::card_runtime::CardTypeId::COVER ||
      cover.runtime.driver != espcontrol::card_runtime::CardDriverId::COVER_TILT ||
      cover.surface != espcontrol::cards::Surface::SUBPAGE ||
      !cover.allow_in_subpage) {
    return EXIT_FAILURE;
  }
  if (!option_select_compatibility.legacy_dispatch ||
      option_select_compatibility.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::ACTION) {
    return EXIT_FAILURE;
  }
  const auto todo = card_runtime_context("todo");
  if (!todo.known || todo.family != Family::TODO || !todo.allow_in_subpage ||
      !card_runtime_has_capability(
          todo, espcontrol::card_runtime::CAPABILITY_ACTIONS)) {
    return EXIT_FAILURE;
  }
  const char *contract_card_types[] = {
    "", "action", "alarm", "alarm_action", "calendar", "climate",
    "climate_control", "clock", "cover", "door_window", "fan_control",
    "fan_direction", "fan_oscillate", "fan_preset", "fan_speed", "fan_switch",
    "garage", "gate", "image", "internal", "lawn_mower", "light_brightness",
    "light_control", "light_switch", "light_temperature", "local_sensor", "lock",
    "media", "option_select", "presence", "push", "screen_lock", "sensor",
    "slider", "subpage", "timezone", "vacuum", "weather", "weather_forecast",
    "webhook",
  };
  for (const char *type : contract_card_types) {
    const auto registration = card_runtime_registration(type);
    if (!registration.known ||
        registration.allow_in_subpage != card_contract_allow_in_subpage(type)) {
      return EXIT_FAILURE;
    }
  }

  const char embedded_null[] = {'a', '\0', 'b'};
  const std::string copied = string_ref_limited(esphome::StringRef(embedded_null, 3), 3);
  if (copied.size() != 3 || copied[0] != 'a' || copied[1] != '\0' || copied[2] != 'b') {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
