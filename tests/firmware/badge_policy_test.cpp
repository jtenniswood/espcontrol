#include "badge_policy.h"
#include <cassert>
#include <cmath>

int main() {
  using namespace espcontrol::badge;
  assert(slider_value_text(21.4) == "21");
  assert(slider_value_text(21.5) == "22");
  assert(slider_value_text(-21.5) == "-22");
  assert(slider_value_text(-0.4) == "0");
  assert(slider_value_text(35) == "35");
  assert(slider_value_text(std::numeric_limits<double>::infinity()) == "--");
  assert(slider_value_text(std::numeric_limits<double>::max()).size() == 309);
  assert(slider_value_text(-std::numeric_limits<double>::max()).size() == 310);
  assert(content_left_padding(24) < content_right_padding(24));
  assert(width_for_text(24, 20, false) == 108);
  assert(width_for_text(24, 20, true) == 88);
  assert(width_for_text(24, 40, true) == 108);
  assert(width_for_text(24, 150, false) == 218);
  assert(width_for_text(24, 150, true) == 218);
  assert(display_text("Energy", "3.98 kW solar", false) == "3.98 kW solar");
  assert(display_text("Sonos", "35%", true) == "Sonos");
  assert(display_text("Sonos", "Sonos", false) == "Sonos");
  assert(display_text("", "Unavailable", false) == "Unavailable");
  assert(display_text("Energy", "", false) == "Energy");
  assert(display_text("Sonos", "Track\nArtist", false) == "Track Artist");
  assert(progress_percent(35, 0, 100) == 35);
  assert(progress_percent(-5, -10, 10) == 25);
  assert(progress_percent(1, 1, 1) == 0);
  assert(progress_percent(200, 0, 100) == 100);
  assert(progress_percent(-200, 0, 100) == 0);
  assert(progress_percent(INT_MAX, INT_MIN, INT_MAX) == 100);
  assert(toggle_checked(true, true));
  assert(!toggle_checked(true, false));
  assert(!toggle_checked(false, true));
  assert(action("erase") == "display");
  assert(action("button") == "button");
  assert(action("card") == "card");
  assert(subpage_unsupported("subpage", ""));
  assert(subpage_unsupported("subpage", "subpage"));
  assert(subpage_unsupported("card", "subpage"));
  assert(!subpage_unsupported("card", "media"));
  assert(!subpage_unsupported("modal", ""));
  assert(card_options("badge_card_type=media,badge_action=card,media_tabs=progress|volume,active_color,card_off_color=112233") ==
         "media_tabs=progress|volume,active_color,card_off_color=112233");
  assert(card_options("badge_page=8,ssid64=YWJjLGQ=,,hidden,badge_attribute=x") == "ssid64=YWJjLGQ=,hidden");
  assert(button_supported("button.restart"));
  assert(button_supported("input_button.scene"));
  assert(!button_supported("switch.lights"));
  assert(!button_supported("button."));
  assert(std::string(button_action("button.restart")) == "button.press");
  assert(std::string(button_action("input_button.scene")) == "input_button.press");
  assert(toggle_supported("light.kitchen"));
  assert(!toggle_supported("lock.front_door"));
  assert(!toggle_supported("light."));
  assert(slider_supported("input_number.target"));
  assert(!slider_supported("sensor.target"));
  assert(std::fabs(level_percent("light.kitchen", 128) - 50.196078) < 0.001);
  assert(level_percent("light.kitchen", 255) == 100);
  assert(level_percent("media_player.kitchen", 0.35) == 35);
  assert(level_percent("fan.kitchen", 40) == 40);
  assert(level_percent("media_player.kitchen", 1.5) == 100);
  assert(level_percent("light.kitchen", -1) == 0);
  assert(std::string(level_attribute("media_player.kitchen")) == "volume_level");
  assert(std::string(level_attribute("number.target")).empty());
}
