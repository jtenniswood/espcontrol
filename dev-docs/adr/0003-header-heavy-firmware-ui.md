# ADR 0003: Header-Heavy Firmware UI

## Status

Accepted.

## Context

Firmware UI behavior lives mostly in `components/espcontrol/*.h`. The project
uses ESPHome and LVGL, with many card types sharing grid setup, runtime state,
modal handling, and Home Assistant subscriptions.

## Decision

Keep the firmware UI organized as header-heavy component code with explicit
includes and generated contract headers.

## Why

- ESPHome custom components integrate cleanly with this structure.
- Card behavior can stay close to its LVGL setup and runtime callbacks.
- Generated headers let firmware share card metadata with the web setup page.

## Consequences

- Include order matters and must be reviewed when adding helpers.
- Shared behavior should still move to common headers when duplication becomes
  risky.
- Firmware checks are required because C++ structure and runtime behavior are
  not fully protected by web tests.
