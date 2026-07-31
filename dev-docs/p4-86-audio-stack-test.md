# ESP32-P4-86 Audio Stack Test Notes

Last updated: 2026-07-11

## Current Status

The latest test build appears stable in user testing after reducing hosted WiFi/SDIO memory pressure as well as the normal ESP WiFi settings.

`logs (5).txt` did not show the earlier `sdio_rx_get_buffer` assert, panic, Guru Meditation, reboot, or backtrace markers. Media and TTS decoding reached the points that previously triggered the crash.

Follow-up test on 2026-07-11:

- Re-enabled normal update behaviour.
- Re-enabled cover art downloads.
- Removed the temporary `ESPCONTROL_DISABLE_IMAGE_CARD_DOWNLOADS` build flag.
- Build completed successfully.
- First USB upload to `COM3` was blocked by Windows with `Access is denied`; `COM3` was present as `USB-Enhanced-SERIAL CH343 (COM3)`, so another process likely had the port open.
- Retried after the port was released and flashed successfully on `COM3`.
- Restored the ESP32-C6 hosted SDIO clock from `10MHz` to `20MHz`, then built and flashed successfully on `COM3`.
- Restored speaker, mixer input, and resampler `buffer_duration` values from `100ms` to `500ms`, then built and flashed successfully on `COM3`.
- Restored `media_player.buffer_size` from `32768` to `1000000` and moved the normal media pipeline back to upstream-style `format: NONE` at `48000Hz`, then built and flashed successfully on `COM3`.
- Removed the remaining temporary telemetry, heap logger, cover-art download gate, and image-card download gate, then built and flashed successfully on `COM3`.
- Changed the AFE test profile to `mode: high_perf` with `input_format: MMNR`, then built and flashed successfully on `COM3`.
- Changed the AFE test profile to `mode: low_cost` while keeping `input_format: MMNR` and `aec_nlp_level: normal`, then built and flashed successfully on `COM3`.
- Locally switched the AFE test profile back to `mode: high_perf` with `input_format: MMNR` and `aec_nlp_level: normal`, then built and flashed successfully on `COM3`.
- Set `memory_alloc_mode: more_psram`, kept `task_priority: 5`, and moved the AFE feed/fetch frame buffers back to internal RAM while leaving the feed ring in PSRAM, then built and flashed successfully on `COM3`.
- Tried `feed_ring_in_psram: false`, but the device segfaulted because there was not enough internal memory. Reverted it to `true` and flashed successfully on `COM3`.
- Changed the AFE diagnostic `input_volume` and `output_rms` update intervals from `500ms` to `250ms`; ES7210 input gain remained at `30.0`, then built and flashed successfully on `COM3`.
- Raised ES7210 input gain from `30.0` to `33.0`, then to `36.0`; built and flashed successfully on `COM3`.
- Returned the AFE diagnostic `input_volume` and `output_rms` update intervals to `500ms` and made them internal so they are not published back to Home Assistant, then built and flashed successfully on `COM3`.

## Current Working Candidate

Audio configuration:

- External audio stack from `github://n-IA-hane/esphome-audio-stack@3c30b96ad2540f676eadd30bdbb93104b105dee0`.
- `esp_afe` uses `type: sr`, `mode: high_perf`.
- Dual microphone speech enhancement is enabled with `mic_num: 2`, `input_format: MMNR`, and `se_enabled: true`.
- AEC remains enabled.
- NS, VAD gating, VAD mute playback, and AGC are disabled.
- `memory_alloc_mode: more_psram`, `feed_buf_in_psram: false`, `feed_ring_in_psram: true`, and `fetch_ring_in_psram: false`.
- The feed and fetch frame buffers are kept internal to reduce latency and improve responsiveness, but the feed ring stays in PSRAM because moving it internal caused a memory-pressure segfault.
- AFE diagnostic `input_volume` and `output_rms` sensors are internal and update every `500ms`.
- ES7210 input gain is currently `36.0`.
- TDM remapping is enabled with four slots:
  - slot 0: microphone 1
  - slot 1: hardware speaker/AEC reference
  - slot 2: microphone 2
  - slot 3: unused
- `tdm_mic_slots: [0, 2]`
- `tdm_ref_slot: 1`
- Audio telemetry and the 1-second heap logger have been removed after testing.

Network and memory workaround:

- `network.enable_high_performance: false`
- ESP hosted transport mempool placed in PSRAM.
- Hosted task stack placed in PSRAM.
- WiFi/LWIP allocations are biased towards PSRAM.
- Normal ESP WiFi and hosted WiFi remote buffer counts are reduced.
- Hosted WiFi remote AMPDU RX/TX is disabled.
- LWIP TCP windows and receive mailboxes are reduced.
- ESP32-C6 hosted SDIO clock is currently `20MHz`.

The key discovery was that setting only `CONFIG_ESP_WIFI_*` was not enough for this board. The final SDK config was still being driven by the hosted WiFi remote `CONFIG_WIFI_RMT_*` options, so those had to be set too.

## Why This Seems To Help

The crash was happening in the hosted WiFi/SDIO receive path while dual-mic SE/AEC and media playback were competing for internal/DMA-capable memory.

Earlier logs showed the failure as:

- `assert failed: sdio_rx_get_buffer`
- crash shortly after media playback started
- low or falling DMA-capable heap

The working candidate reduces network burst pressure and hosted WiFi buffer demand, leaving the audio stack less likely to collide with the SDIO receive path.

## Remaining Risk

DMA headroom is still tight during playback. In `logs (5).txt`, DMA free memory dropped to around 4.8 KB with the largest block around 1.5 KB during sustained audio. That is much better than crashing immediately, but it means this still needs longer soak testing with:

- repeated voice commands
- long TTS responses
- normal media playback
- barge-in during playback

## Files Currently Involved

- `devices/esp32-p4-86/device/voice_assistant.yaml`
- `devices/esp32-p4-86/device/device.yaml`
- `dev-docs/p4-86-audio-stack-test.md`

The temporary cover-art and image-card download gates have been removed again, so this branch is back to normal media artwork behaviour while keeping the audio/TDM and hosted-WiFi stability changes under test.
