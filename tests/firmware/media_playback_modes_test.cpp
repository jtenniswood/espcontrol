#include <cassert>

#include "media_playback_modes.h"

int main() {
  using espcontrol::media::RepeatMode;
  using espcontrol::media::SUPPORT_REPEAT_SET;
  using espcontrol::media::SUPPORT_SHUFFLE_SET;
  using espcontrol::media::media_transport_layout;
  using espcontrol::media::next_repeat_mode;
  using espcontrol::media::parse_repeat_mode;
  using espcontrol::media::parse_shuffle_state;
  using espcontrol::media::repeat_mode_value;
  using espcontrol::media::repeat_supported;
  using espcontrol::media::shuffle_supported;

  assert(!shuffle_supported(false, SUPPORT_SHUFFLE_SET));
  assert(!shuffle_supported(true, 0));
  assert(shuffle_supported(true, SUPPORT_SHUFFLE_SET));
  assert(!repeat_supported(false, SUPPORT_REPEAT_SET));
  assert(!repeat_supported(true, 0));
  assert(repeat_supported(true, SUPPORT_REPEAT_SET));
  assert(shuffle_supported(true, SUPPORT_SHUFFLE_SET | SUPPORT_REPEAT_SET));
  assert(repeat_supported(true, SUPPORT_SHUFFLE_SET | SUPPORT_REPEAT_SET));

  bool shuffle = false;
  assert(parse_shuffle_state("true", shuffle) && shuffle);
  assert(parse_shuffle_state(" OFF ", shuffle) && !shuffle);
  assert(parse_shuffle_state("1", shuffle) && shuffle);
  assert(parse_shuffle_state("no", shuffle) && !shuffle);
  assert(!parse_shuffle_state("unknown", shuffle));
  assert(!parse_shuffle_state("", shuffle));

  assert(parse_repeat_mode("off") == RepeatMode::OFF);
  assert(parse_repeat_mode(" ALL ") == RepeatMode::ALL);
  assert(parse_repeat_mode("One") == RepeatMode::ONE);
  assert(parse_repeat_mode("unknown") == RepeatMode::UNKNOWN);
  assert(next_repeat_mode(RepeatMode::OFF) == RepeatMode::ALL);
  assert(next_repeat_mode(RepeatMode::ALL) == RepeatMode::ONE);
  assert(next_repeat_mode(RepeatMode::ONE) == RepeatMode::OFF);
  assert(next_repeat_mode(RepeatMode::UNKNOWN) == RepeatMode::UNKNOWN);
  assert(std::string(repeat_mode_value(RepeatMode::OFF)) == "off");
  assert(std::string(repeat_mode_value(RepeatMode::ALL)) == "all");
  assert(std::string(repeat_mode_value(RepeatMode::ONE)) == "one");
  assert(repeat_mode_value(RepeatMode::UNKNOWN) == nullptr);

  const auto three = media_transport_layout(400, 480, false, false);
  const auto four = media_transport_layout(400, 480, true, false);
  const auto five = media_transport_layout(400, 480, true, true);
  assert(three.total_width <= 400 && three.first_row_start_x >= 0);
  assert(four.total_width <= 400 && four.first_row_start_x >= 0);
  assert(five.total_width <= 400 && five.first_row_start_x >= 0);
  assert(three.button_size > 0);
  assert(four.button_size > 0);
  assert(five.button_size > 0);
  assert(!three.modes_on_second_row);
  assert(!four.modes_on_second_row);
  assert(!five.modes_on_second_row);
  assert(three.first_row_width == three.button_size * 3 + three.gap * 2);
  assert(four.first_row_width == four.button_size * 4 + four.gap * 3);
  assert(five.first_row_width == five.button_size * 5 + five.gap * 4);

  const auto portrait = media_transport_layout(400, 480, true, true, true);
  const auto portrait_shuffle_only =
    media_transport_layout(400, 480, true, false, true);
  const auto landscape = media_transport_layout(720, 800, true, true);
  assert(portrait.total_width <= 400);
  assert(portrait.modes_on_second_row);
  assert(portrait.first_row_width == portrait.button_size * 3 + portrait.gap * 2);
  assert(portrait.second_row_width == portrait.button_size * 2 + portrait.gap);
  assert(portrait.total_height == portrait.button_size * 2 + portrait.row_gap);
  assert(portrait.first_row_start_x >= 0 && portrait.second_row_start_x >= 0);
  assert(portrait_shuffle_only.modes_on_second_row);
  assert(portrait_shuffle_only.second_row_width == portrait_shuffle_only.button_size);
  assert(landscape.total_width <= 720);
  assert(!landscape.modes_on_second_row);

  return 0;
}
