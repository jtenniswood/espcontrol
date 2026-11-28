---
title: Device Runtime Tests
description: Validate a flashed EspControl panel and collect crash, restart, connection, and memory evidence.
---

# Device Runtime Tests

Use this suite after flashing a test build. It checks the device identity and web
API, watches the native ESPHome connection and crash logs, detects a boot-clock
reset, and records free heap and PSRAM diagnostics. It does not flash firmware or
change saved configuration.

## Set up

Use the pinned Python environment from [Contributing](./contributing#preparing-a-branch).
ESPHome provides the `aioesphomeapi` dependency. Python 3.12 or newer is recommended.
Run commands from the repository root with that environment active.

Start with a passive two-minute check, replacing the example address:

```sh
npm run test:device-runtime -- --host 192.168.6.105 --duration 120
```

The target is always explicit; the runner does not discover or test other panels.
It needs the device's HTTP API (port 80) and native ESPHome API (port 6053).
Use `--http-port` or `--api-port` for other ports. It checks HTTP every ten seconds
and listens continuously for native disconnects and crash signatures.

## Exercise a card

To repeatedly open and close the media panel, name an entity already configured
on that device:

```sh
npm run test:device-runtime -- --host 192.168.6.105 --duration 180 \
  --modal-entity media_player.office --modal-cycles 30 --modal-dwell 1
```

This visibly wakes the display and changes the open panel. It requires the
`open_modal` and `close_modal` native actions. An actual device log confirming the
panel opened is required for each cycle. The runner attempts to close it after
each opening and on interruption; it cannot restore the previous screen or sleep
state. It sends no playback, grouping, volume, or Home Assistant service commands.
Use the physical panel for those functional checks.

Choose a duration long enough for all cycles: each takes at least twice the dwell
time, plus API and rendering time. Incomplete cycles fail the run. For a longer
observation, increase `--duration`, for example to `1800` for 30 minutes. Memory
sensors normally update once per minute, so use several minutes to compare them.
Recorded memory changes are evidence to investigate, not an automatic leak diagnosis.

Optionally require an exact ESPHome version and build timestamp, as reported by
the device's native API:

```sh
npm run test:device-runtime -- --host 192.168.6.105 \
  --expect-version 2026.9.1 --expect-build '2026-10-17 19:13:43 +0100'
```

For protected devices, set `ESPCONTROL_API_PASSWORD` or
`ESPCONTROL_API_NOISE_PSK` for the native API, and `ESPCONTROL_HTTP_USERNAME` /
`ESPCONTROL_HTTP_PASSWORD` for HTTP authentication. Credentials are read from the
environment and are not included in reports.

## Read the result

Each run creates a fresh directory under `build/device-tests/` containing:

- `report.md`: a readable result and its limitations.
- `report.json`: identity, build, failures, counts, and memory ranges.
- `events.jsonl`: timestamped health samples and relevant failure evidence.

Use `--output path/to/new-directory` to select the location. Existing directories
are refused so earlier evidence is preserved. Other device error logs are retained
for investigation without automatically classifying them as crashes. Reports contain the target address,
device identity, and relevant sanitized logs; review them before sharing.

Exit status `0` means the observed window passed. Status `1` means a failure or
incomplete coverage, including missing heap diagnostics, interrupted testing, or
an HTTP/native connection failure that later recovered. Status `2` indicates
invalid arguments or a missing dependency. A run needs at least two successful
HTTP samples and a native connection; very short durations may fail coverage.
`--min-heap-percent` changes the default 5% minimum free-heap threshold.

A restart is detected from the device's boot-relative event clock, allowing its
normal 32-bit wrap. A connection failure alone is not classified as a confirmed
crash, and a restart does not identify its cause. A pass covers only the observed
window. Panics before Wi-Fi or log delivery can require [USB logs](./collect-usb-logs).
Touch responsiveness, visual layout, speaker grouping, and playback still need
physical device testing.

The runner's own offline tests are included in the fast/CI checks. Run them alone
with `npm run check:device-runtime-tests`; they use mocked APIs and a local HTTP
fixture, without contacting hardware.

## 10-inch V3 display performance

The local `builds/guition-esp32-p4-jc8012p4a1-v3-benchmark.yaml` build adds
touch, LVGL, heap and FreeRTOS timing counters to the normal V3 factory
configuration. Use the same instrumented build settings for baseline and
candidate firmware. Capture one workload per run and avoid rebooting during a
capture. Start the runner before the workload so the first complete sample
precedes the measured actions. Add the existing `--modal-entity` option for
repeatable remote modal cycles.

For the buffer-size comparison, use
`builds/guition-esp32-p4-jc8012p4a1-v3-benchmark-quarter-buffer.yaml` as the
candidate. It sets the LVGL draw and rotation buffer size to 25% of a frame.
Compare it with the benchmark build above, which uses the normal V3 allocation
policy. Check full-screen redraws and all supported orientations as well as
the repeated modal workload. Smaller buffers make LVGL split large redraws
into more transfers, so check frame timing alongside available memory. The
summary records the allocated LVGL draw buffer size, so a baseline that fell
back to a smaller allocation remains distinguishable.

For the DSI transfer comparison, use
`builds/guition-esp32-p4-jc8012p4a1-v3-benchmark-staged-dsi.yaml` against the
quarter-buffer build. It keeps ESPHome's LVGL and rotation path and gives the
DSI driver one PSRAM buffer for transfers. This lets the next region render
while DMA reads the previous region. The refresh waits for its final transfer.
Confirm the summary reports both allocated `dsi_staging_bytes` and nonzero
`dsi_staged_submissions` for the workload before comparing CPU, frame timing,
and memory. The staging path adds a copy and uses additional PSRAM, so include
those costs in the comparison.

```sh
npm run test:device-runtime -- --host 192.168.6.103 --capture-display --output build/display-baseline
python3 scripts/summarize_display_benchmark.py build/display-baseline/display-benchmark.log
```

Use equal-duration idle, slider/navigation and image-heavy workloads, and
alternate baseline and candidate runs on the same panel. Record the firmware
commit, orientation, dashboard, and action count alongside each capture. The
summary reports elapsed time and combined busy time for both CPU cores,
minimum heap availability, touch counts, and LVGL timings. Refresh intervals
include idle gaps, so compare them within the same continuous interaction.
The timer spans instrumented firmware work. Use high-speed video to measure
touch-to-visible feedback and check scanout for tearing. Repeat the normal
installer, OTA, and existing Home Assistant automation checks before rollout.
