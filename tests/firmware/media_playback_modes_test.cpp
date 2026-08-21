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
  assert(three.total_width <= 400 && three.start_x >= 0);
  assert(four.total_width <= 400 && four.start_x >= 0);
  assert(five.total_width <= 400 && five.start_x >= 0);
  assert(three.primary_size > 0 && three.mode_size < three.primary_size);
  assert(four.mode_size < four.primary_size);
  assert(five.mode_size < five.primary_size);

  const auto portrait = media_transport_layout(400, 480, true, true);
  const auto landscape = media_transport_layout(720, 800, true, true);
  assert(portrait.total_width <= 400);
  assert(landscape.total_width <= 720);

  return 0;
}
