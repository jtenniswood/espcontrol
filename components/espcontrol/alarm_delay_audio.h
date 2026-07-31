#pragma once

#include <cstddef>
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
  std::function<bool()> ready;
  std::function<void(AlarmDelayAudioMode)> play_beep;
  std::function<void(AlarmDelayAudioMode)> announce;
  std::function<void()> stop;
};

inline bool alarm_delay_audio_waiting_for_announcement(
    bool awaiting_announcement, bool ready) {
  return awaiting_announcement && !ready;
}

inline uint32_t alarm_delay_audio_tone_duration_ms(AlarmDelayAudioMode mode) {
  if (mode == AlarmDelayAudioMode::ENTRY) return 210U;
  if (mode == AlarmDelayAudioMode::EXIT) return 120U;
  return 0U;
}

inline int16_t alarm_delay_audio_tone_sample(AlarmDelayAudioMode mode,
                                             size_t sample_index,
                                             uint32_t sample_rate) {
  if (sample_rate == 0 || mode == AlarmDelayAudioMode::NONE) return 0;

  const uint32_t duration_ms = alarm_delay_audio_tone_duration_ms(mode);
  const size_t total_samples =
      static_cast<size_t>(duration_ms) * sample_rate / 1000U;
  if (sample_index >= total_samples) return 0;

  const uint32_t pulse_ms = mode == AlarmDelayAudioMode::ENTRY ? 70U : 120U;
  const uint32_t second_pulse_ms = 140U;
  const uint32_t sample_ms =
      static_cast<uint32_t>(sample_index * 1000U / sample_rate);
  size_t pulse_sample = sample_index;
  if (mode == AlarmDelayAudioMode::ENTRY) {
    if (sample_ms >= pulse_ms && sample_ms < second_pulse_ms) return 0;
    if (sample_ms >= second_pulse_ms) {
      pulse_sample -= static_cast<size_t>(second_pulse_ms) * sample_rate / 1000U;
    }
  }

  const size_t pulse_samples =
      static_cast<size_t>(pulse_ms) * sample_rate / 1000U;
  if (pulse_sample >= pulse_samples) return 0;

  const uint32_t frequency_hz =
      mode == AlarmDelayAudioMode::ENTRY ? 1200U : 800U;
  const uint32_t phase = static_cast<uint32_t>(
      (static_cast<uint64_t>(pulse_sample) * frequency_hz * 65536U /
       sample_rate) &
      0xFFFFU);
  const int32_t triangle = phase < 32768U
      ? static_cast<int32_t>(phase * 2U) - 32768
      : 98303 - static_cast<int32_t>(phase * 2U);

  const size_t fade_samples = sample_rate / 200U;  // 5 ms click-free edge.
  uint32_t envelope = 32767U;
  if (fade_samples > 0 && pulse_sample < fade_samples) {
    envelope = static_cast<uint32_t>(pulse_sample * 32767U / fade_samples);
  }
  const size_t samples_from_end = pulse_samples - 1U - pulse_sample;
  if (fade_samples > 0 && samples_from_end < fade_samples) {
    const uint32_t fade_out =
        static_cast<uint32_t>(samples_from_end * 32767U / fade_samples);
    if (fade_out < envelope) envelope = fade_out;
  }

  return static_cast<int16_t>(
      static_cast<int64_t>(triangle) * 12000 * envelope /
      (32768LL * 32767LL));
}

inline int16_t alarm_delay_audio_scale_tone_sample(int16_t sample,
                                                    float volume) {
  if (!(volume > 0.0f)) return 0;
  if (volume >= 1.0f) return sample;
  const int32_t gain = static_cast<int32_t>(volume * 32767.0f + 0.5f);
  return static_cast<int16_t>(
      static_cast<int32_t>(sample) * gain / 32767);
}

inline void alarm_delay_audio_fill_tone(AlarmDelayAudioMode mode,
                                        int16_t *samples,
                                        size_t sample_offset,
                                        size_t sample_count,
                                        uint32_t sample_rate,
                                        float volume = 1.0f) {
  if (!samples) return;
  for (size_t i = 0; i < sample_count; i++) {
    samples[i] = alarm_delay_audio_scale_tone_sample(
        alarm_delay_audio_tone_sample(
            mode, sample_offset + i, sample_rate),
        volume);
  }
}

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
