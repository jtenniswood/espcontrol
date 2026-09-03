# Change Firmware UI

Use this for LVGL layout, card runtime state, modals, display lifecycle, Home
Assistant bindings, fonts, image handling, or device-specific UI behavior.

## Edit First

- The smallest relevant `components/espcontrol/button_grid_*.h` or compiled
  module.
- `components/espcontrol/button_grid_grid.h` only for shared grid/runtime wiring.
- `components/espcontrol/button_grid_config.h` only when parsing changes.

Use `product/v2/device_catalog.json` for device profile or font-role data. Do
not put a reusable device difference into generated YAML or a one-off C++
condition when the catalog can express it.

## Regenerate

Run only the generators required by the authored inputs. Device profile or font
role changes normally require:

```bash
python3 scripts/generate_device_manifest.py
python3 scripts/generate_device_slots.py
```

## Stop If

- The smallest supported screen or constrained S3 build has not been considered.
- A modal can survive navigation or reconfiguration unexpectedly.
- Home Assistant subscriptions or runtime allocations are not cleaned up.
- A generated device file would need a manual edit.

## Verify

```bash
npm run check:firmware-parser
npm run check:firmware-modals
npm run check:firmware-modal-layouts
npm run check:firmware-card-runtime
npm run check:firmware-display-tokens
npm run check:firmware-ha-bindings
```

Run the focused device/profile checks when layout data changes. Compile the
affected display firmware before publishing, and record physical-device testing
separately from a successful compile.
