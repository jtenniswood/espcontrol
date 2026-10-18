#include "media_playback_clock.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
using namespace espcontrol::media;
void playback_timing() {
  PlaybackClock clock;
  clock.playback(true, 100);
  clock.duration_value("300");
  clock.position_value("10.5", 100);
  assert(clock.seconds(1100) == 11.5f && clock.percent(1100) == 4);
  clock.playback(false, 2100);
  assert(clock.seconds(12100) == 12.5f);
  clock.playback(true, 12100);
  assert(clock.seconds(13100) == 13.5f);
  clock.position_value("20tail", 14100);
  assert(clock.seconds(15100) == 21);
  clock.timestamp_value("1970-01-01T00:16:40Z", 1003, 15100);
  assert(clock.seconds(15100) == 23);
  clock.playback(false, 16100);
  assert(clock.seconds(17100) == 24);
  clock.playback(true, 30000);
  assert(clock.seconds(31000) == 25);
  clock.timestamp_value("bad", 1003, 31000);
  assert(!clock.has_reported_time && clock.seconds(32000) == 25);
  clock.position_value("1000", 32000);
  assert(clock.seconds(33000) == 300 && clock.percent(33000) == 100);
  const char bounded[] = {'1', '.', '5', '9'};
  clock.position_value({bounded, 3}, 33000);
  assert(clock.position == 1.5f);
  clock.duration_value("0");
  assert(!clock.has_duration && clock.percent(33000) == 0);
  for (const auto* bad : {"nan", "inf", "-inf", "-1", "unknown", ""}) {
    clock.position_value(bad, 33000);
    assert(clock.position == 0 && clock.seconds(33000) == 0);
    clock.duration_value(bad);
    assert(!clock.has_duration);
  }
  PlaybackClock wrap;
  wrap.playback(true, UINT32_MAX - 500);
  wrap.position_value("1", UINT32_MAX - 500);
  assert(wrap.seconds(499) == 2);
  PlaybackClock zero;
  zero.playback(true, 0);
  zero.position_value("1", 0);
  assert(zero.seconds(1000) == 2);
  PlaybackClock early;
  early.position_value("10", 100);
  early.timestamp_value("1970-01-01T00:16:40Z", 1003, 100);
  early.playback(true, 100);
  assert(early.seconds(100) == 13);
  clock.duration = clock.position = std::numeric_limits<float>::max();
  clock.has_duration = clock.has_position = true;
  clock.playing = false;
  assert(clock.percent(0) == 100);
}
void timestamps_and_formatting() {
  int64_t epoch;
  assert(parse_timestamp("1970-01-01T00:00:01Z", epoch) && epoch == 1);
  assert(parse_timestamp("1970-01-01T01:00:01+01:00", epoch) && epoch == 1);
  assert(parse_timestamp("1970-01-01T00:00:01-01:00", epoch) && epoch == 3601);
  uint32_t stamp;
  assert(timestamp_milliseconds("1970-01-01T00:16:40Z", 1003, 100, stamp) && stamp == uint32_t(100 - 3000));
  assert(!timestamp_milliseconds("1970-01-01T00:16:40Z", 999, 100, stamp));
  assert(timestamp_milliseconds("1970-01-01T00:00:01Z", std::numeric_limits<int64_t>::max(), 100, stamp) &&
         stamp == 101);
  assert(!parse_timestamp("bad", epoch) && !parse_timestamp("2026-13-01T00:00:00Z", epoch));
  char text[16];
  format_playback_time(3661, text, sizeof(text));
  assert(std::strcmp(text, "1:01:01") == 0);
  format_playback_time(std::numeric_limits<float>::max(), text, sizeof(text));
  assert(std::strcmp(text, "--:--") == 0);
  format_playback_time(NAN, text, sizeof(text));
  assert(std::strcmp(text, "0:00") == 0);
}
int main() {
  playback_timing();
  timestamps_and_formatting();
}
