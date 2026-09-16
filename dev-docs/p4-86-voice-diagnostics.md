# P4-86 voice diagnostics — issue #1701

Temporary diagnostic branch for [issue #1701](https://github.com/jtenniswood/espcontrol/issues/1701).
This is not a confirmed fix and must stay out of stable releases until the
device evidence has been reviewed. It changes logging only; the audio library,
network settings, buffers, and recovery behavior remain as before.

## Build and retain

Use the ESPHome Docker version in `.github/esphome.env` and the entry point
`builds/esp32-p4-86.factory.yaml`. Run `config` before `compile`. When mounting
a Git worktree into Docker as `/config`, also mount its common Git directory
at the same absolute path, read-only, so the local component checkout works.

Keep `firmware.ota.bin`, `firmware.factory.bin`, the matching `firmware.elf`,
and the full compile log together. Record the firmware Git commit, Docker image
and image ID, ESPHome version, audio external-component Git commit, and resolved
ESP-SR and GMF versions/source refs. Retain `dependencies.lock` and the generated
`sdkconfig` with those artifacts. Hash the firmware and ELF to identify the exact
build used for any backtrace. Do not replace the ELF with one from a later build.

## Flash and capture

1. Use the **Waveshare ESP32-P4 86 Panel** only. Upload `firmware.ota.bin` using
   the panel's firmware update flow. The factory image is for a USB installation;
   do not use it for an OTA update. No automatic flashing is part of this change.
2. Connect the panel's USB-C debug port and start a serial capture at 115200 baud
   **before pressing reset**. Save the complete output through the first failure
   and any reboot, including panic text and backtrace. A serial terminal is
   sufficient; ESPHome `logs <config.yaml> --device <serial-port>` also works.
   USB capture is preferred because API logs can disconnect during the failure.
3. Note the firmware commit, time of each action, Voice Services and Mute states,
   whether Home Assistant is connected, and whether the failure starts at boot
   or after a particular command. Preserve the first error, not just repeated
   `Ringbuffer of AFE is empty` warnings.
4. Test cold startup with Voice Services enabled; ten voice commands including
   TTS replies; five minutes of media with voice interruption; a Home Assistant
   restart/reconnect; then Voice Services off/on. Also capture a quiet interval
   and muted/disabled states so silence can be distinguished from a stalled feed.
5. Return the log and build identity. Hardware behavior and crash-loop resolution
   remain unverified until this capture is reviewed. Decode panic addresses with
   the retained ELF and its matching toolchain, for example
   `riscv32-esp-elf-addr2line -pfiaC -e firmware.elf <backtrace-addresses>`.

## Reading the trace

- `voice.diag` reports every five seconds from the **main loop**, independently
  of the audio task. `changed` means the sampled state flags differ from the last
  snapshot (or this is the first snapshot). Short transitions between samples
  may not appear; retain the normal component event logs too.
- `running`, `i2s_error`, and `afe_ready` describe the audio path; `enabled`,
  `muted`, `wake`, `assistant`, and `api` provide context. `api` means any API
  client is connected, not proof that Home Assistant's voice pipeline is ready.
- `total` counters are cumulative within the AFE instance, may reset on rebuild,
  and are read atomically rather than as one synchronized frame. `feed_ok` means
  delivery through the component bridge, **not ESP-SR acceptance or processing**.
  Levels are dBFS and can remain stale when processing stops. Queue values show
  occupancy; memory fields show free bytes and the largest available block.
- The library's `Perf` and `AFE` lines report every 128 audio frames. These may
  stop when audio stops; the main-loop snapshots should continue while that loop
  is responsive. If all snapshots stop, use the serial panic/watchdog output.
- Look before the first stalled counters for `TX completion`, `Persistent I2S`,
  codec errors, or allocation failures. Low memory alone does not prove the
  cause. The extra telemetry can affect timing; report if the diagnostic build
  changes how often the fault occurs.

Success is a trace showing where progress first stops, the preceding error,
and a decoded backtrace when a crash occurs. A compile pass or disappearance of
warnings alone is not evidence that issue #1701 is fixed.
