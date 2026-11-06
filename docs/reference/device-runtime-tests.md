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
