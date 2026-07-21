#include <cstdlib>
#include <string>

#include "button_grid_limits.h"
#include "button_grid_card_runtime.h"
#include "button_grid_string.h"
#include "ha_state_capacity.h"

int main() {
  static_assert(MAX_GRID_SLOTS == ESPCONTROL_MAX_GRID_SLOTS);
  static_assert(MAX_SUBPAGE_ITEMS == MAX_GRID_SLOTS * MAX_GRID_SLOTS);
  static_assert(
      HA_CONFIGURED_CARD_CAPACITY == MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS);
  static_assert(HA_STATE_CHANNEL_CAPACITY >=
                HA_CONFIGURED_CARD_CAPACITY * 8 + 32);
  static_assert(HA_SCOPED_LEASE_CAPACITY >=
                HA_CONFIGURED_CARD_CAPACITY * 12 + 64);

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
  struct FullTestConfig {
    std::string entity;
    std::string label;
    std::string icon;
    std::string icon_on;
    std::string sensor;
    std::string unit;
    std::string type;
    std::string precision;
    std::string options;
  };
  FullTestConfig base{"media_player.room", "Music", "Auto", "Auto",
                      "cover_art", "", "media", "", ""};
  const auto address = espcontrol::cards::CardAddress{
    espcontrol::cards::CardSurface::MAIN_GRID, 0, 1};
  const auto base_node = espcontrol::cards::node_for(base, address, 1);
  FullTestConfig relabelled = base;
  relabelled.label = "Now playing";
  const auto relabelled_node = espcontrol::cards::node_for(relabelled, address, 1);
  const uint8_t relabelled_domains =
    espcontrol::cards::changed_domains(base_node, relabelled_node);
  if (espcontrol::cards::mutation_for(relabelled_domains) !=
        espcontrol::cards::CardMutation::UPDATE_VISUAL ||
      (relabelled_domains & espcontrol::cards::CHANGE_BINDINGS) != 0) {
    return EXIT_FAILURE;
  }
  FullTestConfig rebound = base;
  rebound.entity = "media_player.kitchen";
  const uint8_t rebound_domains = espcontrol::cards::changed_domains(
    base_node, espcontrol::cards::node_for(rebound, address, 1));
  if (espcontrol::cards::mutation_for(rebound_domains) !=
        espcontrol::cards::CardMutation::REBIND ||
      (rebound_domains & espcontrol::cards::CHANGE_VISUAL) != 0) {
    return EXIT_FAILURE;
  }
  const auto door = card_runtime_context("door_window");
  const auto presence = card_runtime_context("presence");
  const auto image = card_runtime_context("image");
  const auto light_control = card_runtime_context("light_control");
  const auto fan_control = card_runtime_context("fan_control");
  const auto climate = card_runtime_context("climate");
  const auto climate_control = card_runtime_context("climate_control");
  const auto alarm = card_runtime_context("alarm");
  const auto clock = card_runtime_context("clock");
  const auto timezone = card_runtime_context("timezone");
  const auto calendar = card_runtime_context("calendar");
  const auto sensor = card_runtime_context("sensor");
  const auto local_sensor = card_runtime_context("local_sensor");
  const auto raw_text_sensor_alias = card_runtime_context("text_sensor");
  const auto weather = card_runtime_context("weather");
  const auto weather_forecast = card_runtime_context("weather_forecast");
  const auto toggle = card_runtime_context("");
  const auto action = card_runtime_context("action");
  const auto alarm_action = card_runtime_context("alarm_action");
  const auto fan_switch = card_runtime_context("fan_switch");
  const auto internal = card_runtime_context("internal");
  const auto light_switch = card_runtime_context("light_switch");
  const auto raw_local_action_alias = card_runtime_context("local");
  const auto push = card_runtime_context("push");
  const auto screen_lock = card_runtime_context("screen_lock");
  const auto webhook = card_runtime_context("webhook");
  const auto slider = card_runtime_context("slider");
  const auto light_brightness = card_runtime_context("light_brightness");
  const auto light_temperature = card_runtime_context("light_temperature");
  const auto fan_speed = card_runtime_context("fan_speed");
  const auto fan_oscillate = card_runtime_context("fan_oscillate");
  const auto fan_direction = card_runtime_context("fan_direction");
  const auto fan_preset = card_runtime_context("fan_preset");
  const auto option_select = card_runtime_context("option_select");
  const auto vacuum = card_runtime_context("vacuum");
  const auto mower = card_runtime_context("lawn_mower");
  const auto garage = card_runtime_context("garage");
  const auto gate = card_runtime_context("gate");
  const auto lock = card_runtime_context("lock");
  const auto subpage = card_runtime_context("subpage");
  const auto todo_compatibility = card_runtime_context("todo");
  const auto unsupported = card_runtime_context("not_a_card");
  if (!card_runtime_information_only(door) || !card_runtime_passive(door) ||
      door.legacy_dispatch || presence.legacy_dispatch ||
      clock.runtime.driver != espcontrol::card_runtime::CardDriverId::DATE_TIME ||
      timezone.runtime.driver != espcontrol::card_runtime::CardDriverId::DATE_TIME ||
      calendar.runtime.driver != espcontrol::card_runtime::CardDriverId::DATE_TIME ||
      clock.legacy_dispatch || timezone.legacy_dispatch || calendar.legacy_dispatch ||
      sensor.runtime.driver != espcontrol::card_runtime::CardDriverId::SENSOR ||
      local_sensor.runtime.driver != espcontrol::card_runtime::CardDriverId::SENSOR ||
      sensor.legacy_dispatch || local_sensor.legacy_dispatch ||
      raw_text_sensor_alias.known || raw_text_sensor_alias.legacy_dispatch ||
      weather.runtime.driver != espcontrol::card_runtime::CardDriverId::WEATHER ||
      weather_forecast.runtime.driver != espcontrol::card_runtime::CardDriverId::WEATHER ||
      weather.legacy_dispatch || weather_forecast.legacy_dispatch ||
      toggle.legacy_dispatch || action.legacy_dispatch ||
      alarm_action.legacy_dispatch || fan_switch.legacy_dispatch ||
      internal.legacy_dispatch || light_switch.legacy_dispatch ||
      raw_local_action_alias.known || raw_local_action_alias.legacy_dispatch ||
      push.legacy_dispatch ||
      screen_lock.legacy_dispatch || webhook.legacy_dispatch ||
      slider.runtime.driver != espcontrol::card_runtime::CardDriverId::NUMERIC ||
      light_brightness.runtime.driver != espcontrol::card_runtime::CardDriverId::NUMERIC ||
      light_temperature.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::LIGHT_TEMPERATURE ||
      fan_speed.runtime.driver != espcontrol::card_runtime::CardDriverId::FAN ||
      fan_oscillate.runtime.driver != espcontrol::card_runtime::CardDriverId::FAN ||
      fan_direction.runtime.driver != espcontrol::card_runtime::CardDriverId::FAN ||
      fan_preset.runtime.driver != espcontrol::card_runtime::CardDriverId::FAN ||
      option_select.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::OPTION_SELECT ||
      slider.legacy_dispatch || light_brightness.legacy_dispatch ||
      light_temperature.legacy_dispatch || fan_speed.legacy_dispatch ||
      fan_oscillate.legacy_dispatch || fan_direction.legacy_dispatch ||
      fan_preset.legacy_dispatch || option_select.legacy_dispatch ||
      vacuum.runtime.driver != espcontrol::card_runtime::CardDriverId::VACUUM ||
      mower.runtime.driver != espcontrol::card_runtime::CardDriverId::LAWN_MOWER ||
      vacuum.legacy_dispatch || mower.legacy_dispatch ||
      garage.runtime.driver != espcontrol::card_runtime::CardDriverId::ACCESS ||
      gate.runtime.driver != espcontrol::card_runtime::CardDriverId::ACCESS ||
      lock.runtime.driver != espcontrol::card_runtime::CardDriverId::ACCESS ||
      garage.legacy_dispatch || gate.legacy_dispatch || lock.legacy_dispatch ||
      subpage.runtime.driver != espcontrol::card_runtime::CardDriverId::SUBPAGE ||
      subpage.legacy_dispatch || subpage.allow_in_subpage ||
      !card_runtime_has_capability(
        subpage, espcontrol::card_runtime::CAPABILITY_ACTIONS) ||
      !card_runtime_information_only(image) || card_runtime_passive(image) ||
      image.legacy_dispatch ||
      light_control.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::LIGHT_CONTROL ||
      light_control.legacy_dispatch ||
      fan_control.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::FAN_CONTROL ||
      fan_control.legacy_dispatch ||
      climate.runtime.driver != espcontrol::card_runtime::CardDriverId::CLIMATE ||
      climate_control.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::CLIMATE ||
      climate.legacy_dispatch || climate_control.legacy_dispatch ||
      alarm.runtime.driver != espcontrol::card_runtime::CardDriverId::ALARM ||
      alarm.legacy_dispatch || !todo_compatibility.known ||
      todo_compatibility.family != Family::TODO ||
      !todo_compatibility.legacy_dispatch || unsupported.known ||
      unsupported.legacy_dispatch) {
    return EXIT_FAILURE;
  }
  struct TestConfig {
    std::string type;
    std::string sensor;
  };
  const auto cover = card_runtime_context(
      TestConfig{"cover", "tilt"}, espcontrol::cards::Surface::SUBPAGE);
  const auto cover_position = card_runtime_context(
      TestConfig{"cover", ""});
  const auto cover_toggle = card_runtime_context(
      TestConfig{"cover", "toggle"});
  const auto cover_command = card_runtime_context(
      TestConfig{"cover", "open"});
  const auto cover_modal = card_runtime_context(
      TestConfig{"cover", "modal"});
  const auto media_control = card_runtime_context(
      TestConfig{"media", "control_modal"});
  const auto media_play_pause = card_runtime_context(
      TestConfig{"media", "play_pause"});
  const auto media_transport = card_runtime_context(
      TestConfig{"media", "next"}, espcontrol::cards::Surface::SUBPAGE);
  const auto media_volume = card_runtime_context(
      TestConfig{"media", "volume"});
  const auto media_position = card_runtime_context(
      TestConfig{"media", "position"});
  const auto media_now_playing = card_runtime_context(
      TestConfig{"media", "now_playing"});
  const auto media_cover_art = card_runtime_context(
      TestConfig{"media", "cover_art"});
  const auto media_playlist = card_runtime_context(
      TestConfig{"media", "playlist"});
  const auto option_select_compatibility = card_runtime_context(
      TestConfig{"action", card_runtime_option_select_canonical_action()});
  if (cover.runtime.type != espcontrol::card_runtime::CardTypeId::COVER ||
      cover.runtime.driver != espcontrol::card_runtime::CardDriverId::COVER_TILT ||
      cover.surface != espcontrol::cards::Surface::SUBPAGE ||
      !cover.allow_in_subpage || cover.legacy_dispatch ||
      cover_position.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::COVER_POSITION ||
      cover_toggle.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::COVER_TOGGLE ||
      cover_command.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::COVER_COMMAND ||
      cover_position.legacy_dispatch || cover_toggle.legacy_dispatch ||
      cover_command.legacy_dispatch ||
      cover_modal.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::COVER_MODAL ||
      cover_modal.legacy_dispatch) {
    return EXIT_FAILURE;
  }
  if (media_control.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_CONTROL ||
      media_play_pause.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_PLAY_PAUSE ||
      media_transport.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_TRANSPORT ||
      media_transport.surface != espcontrol::cards::Surface::SUBPAGE ||
      media_volume.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_VOLUME ||
      media_position.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_POSITION ||
      media_now_playing.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_NOW_PLAYING ||
      media_cover_art.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_COVER_ART ||
      media_playlist.runtime.driver !=
        espcontrol::card_runtime::CardDriverId::MEDIA_PLAYLIST ||
      media_control.legacy_dispatch || media_play_pause.legacy_dispatch ||
      media_transport.legacy_dispatch || media_volume.legacy_dispatch ||
      media_position.legacy_dispatch || media_now_playing.legacy_dispatch ||
      media_cover_art.legacy_dispatch || media_playlist.legacy_dispatch) {
    return EXIT_FAILURE;
  }
  if (option_select_compatibility.legacy_dispatch ||
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
