# ADR 0004: Generated Device Slots

## Status

Accepted.

## Context

Each supported display has repeated ESPHome text entities, subpage chunks,
layout values, and font role assignments. Hand-maintaining these repeated YAML
blocks across devices is error-prone.

## Decision

Keep repeated device slot and sensor blocks generated from the compatibility
manifest at `devices/manifest.json`. The manifest itself is generated from the
authored `product/v2/device_catalog.json`.

## Why

- Slot counts, layout, and font roles stay consistent across devices.
- Adding hardware becomes a device-catalog-first change.
- Generated diffs reveal exactly which device outputs changed.

## Consequences

- Do not hand-edit generated blocks in `devices/*/packages.yaml` or
  `devices/*/device/sensors.yaml`.
- Device catalog changes must regenerate `devices/manifest.json` with
  `python3 scripts/generate_device_manifest.py` before regenerating device slots.
- `python3 scripts/generate_device_slots.py --check` is the guard for stale
  slot output.
