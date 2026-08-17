#include <cstdlib>
#include <cstdint>

#include "alarm_delay_audio.h"

int main() {
  if (alarm_delay_audio_mode_for_state("arming") != AlarmDelayAudioMode::EXIT) {
    return EXIT_FAILURE;
  }
  if (alarm_delay_audio_mode_for_state("pending") != AlarmDelayAudioMode::ENTRY) {
    return EXIT_FAILURE;
  }
  if (alarm_delay_audio_mode_for_state("triggered") != AlarmDelayAudioMode::NONE) {
    return EXIT_FAILURE;
  }
  if (!alarm_delay_audio_should_run("arming", 30, true, true) ||
      !alarm_delay_audio_should_run("pending", -1, true, true)) {
    return EXIT_FAILURE;
  }
  if (alarm_delay_audio_should_run("disarmed", 30, true, true) ||
      alarm_delay_audio_should_run("arming", 0, true, true) ||
      alarm_delay_audio_should_run("arming", 30, false, true) ||
      alarm_delay_audio_should_run("arming", 30, true, false)) {
    return EXIT_FAILURE;
  }
  if (alarm_delay_audio_beep_period_ms(11, 10) != 1000U ||
      alarm_delay_audio_beep_period_ms(10, 10) != 700U ||
      alarm_delay_audio_beep_period_ms(1, 10) != 700U ||
      alarm_delay_audio_beep_period_ms(-1, 10) != 1000U) {
    return EXIT_FAILURE;
  }
  if (alarm_delay_audio_should_reset_timer(false, 1000U, 1000U) ||
      !alarm_delay_audio_should_reset_timer(true, 1000U, 1000U) ||
      !alarm_delay_audio_should_reset_timer(false, 1000U, 700U)) {
    return EXIT_FAILURE;
  }
  if (!alarm_delay_audio_waiting_for_announcement(true, false) ||
      alarm_delay_audio_waiting_for_announcement(true, true) ||
      alarm_delay_audio_waiting_for_announcement(false, false)) {
    return EXIT_FAILURE;
  }
  if (alarm_delay_audio_tone_duration_ms(AlarmDelayAudioMode::ENTRY) != 210U ||
      alarm_delay_audio_tone_duration_ms(AlarmDelayAudioMode::EXIT) != 120U ||
      alarm_delay_audio_tone_duration_ms(AlarmDelayAudioMode::NONE) != 0U) {
    return EXIT_FAILURE;
  }
  constexpr uint32_t sample_rate = 48000U;
  if (alarm_delay_audio_tone_sample(
          AlarmDelayAudioMode::ENTRY, 0, sample_rate) != 0 ||
      alarm_delay_audio_tone_sample(
          AlarmDelayAudioMode::EXIT, 0, sample_rate) != 0 ||
      alarm_delay_audio_tone_sample(
          AlarmDelayAudioMode::NONE, 100, sample_rate) != 0) {
    return EXIT_FAILURE;
  }
  const size_t entry_gap_sample = 100U * sample_rate / 1000U;
  const size_t entry_second_pulse_sample = 160U * sample_rate / 1000U;
  const size_t exit_tone_sample = 40U * sample_rate / 1000U;
  if (alarm_delay_audio_tone_sample(
          AlarmDelayAudioMode::ENTRY, entry_gap_sample, sample_rate) != 0 ||
      alarm_delay_audio_tone_sample(
          AlarmDelayAudioMode::ENTRY, entry_second_pulse_sample,
          sample_rate) == 0 ||
      alarm_delay_audio_tone_sample(
          AlarmDelayAudioMode::EXIT, exit_tone_sample, sample_rate) == 0) {
    return EXIT_FAILURE;
  }
  int16_t samples[32] = {};
  alarm_delay_audio_fill_tone(
      AlarmDelayAudioMode::EXIT, samples, exit_tone_sample, 32, sample_rate);
  bool has_audio = false;
  for (int16_t sample : samples) has_audio = has_audio || sample != 0;
  if (!has_audio) return EXIT_FAILURE;
  if (alarm_delay_audio_scale_tone_sample(12000, 0.5f) != 6000 ||
      alarm_delay_audio_scale_tone_sample(-12000, 0.5f) != -6000 ||
      alarm_delay_audio_scale_tone_sample(12000, 0.0f) != 0 ||
      alarm_delay_audio_scale_tone_sample(12000, 1.0f) != 12000 ||
      alarm_delay_audio_scale_tone_sample(12000, 2.0f) != 12000) {
    return EXIT_FAILURE;
  }
  int16_t muted_samples[32] = {};
  alarm_delay_audio_fill_tone(
      AlarmDelayAudioMode::EXIT, muted_samples, exit_tone_sample, 32,
      sample_rate, 0.0f);
  for (int16_t sample : muted_samples) {
    if (sample != 0) return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
