#pragma once

#include <cstdint>
#include <functional>
#include <string>

enum class AlarmDelayAudioMode : uint8_t {
  NONE = 0,
  EXIT = 1,
  ENTRY = 2,
};

struct AlarmDelayAudioHooks {
  std::function<bool()> enabled;
  std::function<bool()> tts_enabled;
  std::function<int()> final_countdown_seconds;
  std::function<void(AlarmDelayAudioMode)> play_beep;
  std::function<void(AlarmDelayAudioMode)> announce;
  std::function<void()> stop;
};

inline AlarmDelayAudioMode alarm_delay_audio_mode_for_state(
    const std::string &state) {
  if (state == "arming") return AlarmDelayAudioMode::EXIT;
  if (state == "pending") return AlarmDelayAudioMode::ENTRY;
  return AlarmDelayAudioMode::NONE;
}

inline bool alarm_delay_audio_should_run(const std::string &state,
                                         int remaining_seconds,
                                         bool available,
                                         bool enabled) {
  return available && enabled && remaining_seconds != 0 &&
         alarm_delay_audio_mode_for_state(state) != AlarmDelayAudioMode::NONE;
}

inline uint32_t alarm_delay_audio_beep_period_ms(int remaining_seconds,
                                                 int final_countdown_seconds) {
  if (final_countdown_seconds < 0) final_countdown_seconds = 0;
  return remaining_seconds > 0 && remaining_seconds <= final_countdown_seconds
    ? 700U : 1000U;
}

inline bool alarm_delay_audio_should_reset_timer(bool starting,
                                                 uint32_t current_period_ms,
                                                 uint32_t next_period_ms) {
  return starting || current_period_ms != next_period_ms;
}
