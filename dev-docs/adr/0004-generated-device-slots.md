# ADR 0004: Generated Device Slots

## Status

Accepted.

## Context

Each supported display has repeated ESPHome text entities, subpage chunks,
layout values, and font role assignments. Hand-maintaining these repeated YAML
blocks across devices is error-prone.

## Decision

Keep repeated device slot and sensor blocks generated from `devices/manifest.json`.

## Why

- Slot counts, layout, and font roles stay consistent across devices.
- Adding hardware becomes a manifest-first change.
- Generated diffs reveal exactly which device outputs changed.

## Consequences

- Do not hand-edit generated blocks in `devices/*/packages.yaml` or
  `devices/*/device/sensors.yaml`.
- Manifest changes must run `python3 scripts/generate_device_slots.py`.
- `python3 scripts/generate_device_slots.py --check` is the guard for stale
  slot output.
