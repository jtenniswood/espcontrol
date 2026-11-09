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
  using espcontrol::media::SUPPORT_NEXT_TRACK;
  using espcontrol::media::SUPPORT_PAUSE;
  using espcontrol::media::SUPPORT_PLAY;
  using espcontrol::media::SUPPORT_PREVIOUS_TRACK;
  using espcontrol::media::next_track_supported;
  using espcontrol::media::play_pause_supported;
  using espcontrol::media::previous_track_supported;
  using espcontrol::media::transport_button_count;

  // Until Home Assistant reports features, keep the full transport row.
  assert(previous_track_supported(false, 0));
  assert(play_pause_supported(false, 0));
  assert(next_track_supported(false, 0));
  assert(transport_button_count(false, 0) == 3);
  // A player like an amp zone with volume/source only has no transport.
  assert(!previous_track_supported(true, 659340));
  assert(!play_pause_supported(true, 659340));
  assert(!next_track_supported(true, 659340));
  assert(transport_button_count(true, 659340) == 0);
  assert(previous_track_supported(true, SUPPORT_PREVIOUS_TRACK));
  assert(next_track_supported(true, SUPPORT_NEXT_TRACK));
  assert(play_pause_supported(true, SUPPORT_PAUSE));
  assert(play_pause_supported(true, SUPPORT_PLAY));
  assert(!play_pause_supported(true, SUPPORT_PREVIOUS_TRACK | SUPPORT_NEXT_TRACK));
  assert(transport_button_count(true, SUPPORT_PLAY | SUPPORT_NEXT_TRACK) == 2);

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
  const auto seven_inch = media_transport_layout(900, 600, true, true);
  const auto ten_inch = media_transport_layout(1100, 800, true, true);
  const auto ten_inch_compact =
    media_transport_layout(1100, 800, true, true, false, 77);
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
  assert(seven_inch.button_size == 110);
  assert(ten_inch.button_size == 146);
  assert(ten_inch_compact.button_size == 112);
  assert(ten_inch_compact.total_width < ten_inch.total_width);

  // Fewer transport buttons narrow and re-centre the first row.
  const auto one_transport = media_transport_layout(400, 480, false, false, false, 100, 1);
  assert(one_transport.first_row_width == one_transport.button_size);
  assert(one_transport.first_row_start_x == (400 - one_transport.button_size) / 2);
  const auto modes_only = media_transport_layout(400, 480, true, true, false, 100, 0);
  assert(modes_only.first_row_width == modes_only.button_size * 2 + modes_only.gap);
  assert(!modes_only.modes_on_second_row);
  // Stacked portrait keeps modes on the first row when there is no transport.
  const auto stacked_modes_only = media_transport_layout(400, 480, true, true, true, 100, 0);
  assert(!stacked_modes_only.modes_on_second_row);
  assert(stacked_modes_only.total_height == stacked_modes_only.button_size);
  const auto stacked_two = media_transport_layout(400, 480, true, false, true, 100, 2);
  assert(stacked_two.modes_on_second_row);
  assert(stacked_two.first_row_width == stacked_two.button_size * 2 + stacked_two.gap);
  // No buttons at all: the row takes no height.
  const auto empty = media_transport_layout(400, 480, false, false, false, 100, 0);
  assert(empty.total_height == 0 && empty.first_row_width == 0);

  return 0;
}
