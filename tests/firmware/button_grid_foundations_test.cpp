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
  if (espcontrol::cards::family_for_type("climate_control") != Family::CLIMATE ||
      espcontrol::cards::family_for_type("light_brightness") != Family::SLIDER ||
      espcontrol::cards::family_for_type("fan_speed") != Family::FAN ||
      espcontrol::cards::family_for_type("not_a_card") != Family::UNKNOWN) {
    return EXIT_FAILURE;
  }
  if (!espcontrol::cards::uses_slider_visual("light_brightness") ||
      !espcontrol::cards::uses_slider_visual("cover") ||
      !espcontrol::cards::uses_slider_visual("fan_speed") ||
      espcontrol::cards::uses_slider_visual("fan_switch")) {
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
