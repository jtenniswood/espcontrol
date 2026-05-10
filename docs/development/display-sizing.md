---
title: Display Sizing Maintainer Notes
description: Internal notes for updating proportional font, icon, and modal sizing across supported displays.
---

# Display Sizing Maintainer Notes

This page explains how EspControl keeps fonts, icons, and volume modal controls looking similar across screens with different resolutions.

The short version: use the 4-inch S3 screen as the visual reference, then scale sizes by each device's short-side pixel count. Use native font sizes wherever text or icons are involved, because scaling an already-rendered font can look soft.

## Reference Screen

The reference device is:

| Device | Resolution | Short side |
|---|---:|---:|
| `guition-esp32-s3-4848s040` | `480x480` | `480px` |

The proportional calculation is:

```text
new size = reference size * target short side / 480
```

Round to the nearest whole number. For `.5`, round up.

The 4.3-inch P4 portrait screen (`guition-esp32-p4-jc4880p443`) is the exception. It has a `480px` short side, but its 2-column portrait grid creates much larger cards than the 4-inch square S3 grid. For that screen, match the existing `label_font_lg` scale instead of the raw short side. In practice, that means `22 -> 31`, which gives a scale of about `1.41`.

Examples:

| Reference size | 480px target | 600px target | 720px target | 800px target |
|---:|---:|---:|---:|---:|
| `22` | `22` | `28` | `33` | `37` |
| `42` | `42` | `53` | `63` | `70` |
| `44` | `44` | `55` | `66` | `73` |
| `61` | `61` | `76` | `92` | `102` |
| `96` | `96` | `120` | `144` | `160` |

## Current Master Sizes

These are the current reference sizes from the S3 480px screen.

| UI item | Reference size | Notes |
|---|---:|---|
| Button/card icons | `44` | Main card icons, based on `icon_font_sm` |
| Setup screen icons | `61` | WiFi, Ethernet, cog, Home Assistant setup icons |
| Volume modal number | `96` | Large percentage number, using `Roboto@Thin` |
| Volume modal label | `22` | `Volume` label |
| Volume modal icons | `44` | Back, minus, and plus controls using the standard card icon font |
| Volume modal arc stroke | `12` | Runtime layout constant, not a font |

If a master size changes, update each native target font using the same formula.

## Where to Update Fonts and Icons

Native font definitions live in YAML. The device-specific files add sizes that differ from the shared/common size.

| Device | Short side | Font file |
|---|---:|---|
| `guition-esp32-s3-4848s040` | `480` | `devices/guition-esp32-s3-4848s040/device/fonts.yaml` |
| `guition-esp32-p4-jc4880p443` | `480` | `devices/guition-esp32-p4-jc4880p443/device/fonts.yaml` |
| `guition-esp32-p4-jc1060p470` | `600` | `devices/guition-esp32-p4-jc1060p470/device/fonts.yaml` |
| `esp32-p4-86` | `720` | `devices/esp32-p4-86/device/fonts.yaml` |
| `guition-esp32-p4-jc8012p4a1` | `800` | `devices/guition-esp32-p4-jc8012p4a1/device/fonts.yaml` |

Shared/default font definitions live in:

- `common/assets/icons.yaml`
- `common/assets/fonts.yaml`

Device package substitutions select setup-screen and main-card fonts:

- `devices/<device>/packages.yaml`

Generated C++ setup for home and subpage grids comes from:

- `devices/manifest.json`
- `scripts/generate_device_slots.py`
- `devices/<device>/device/sensors.yaml`

Do not hand-edit the generated `device/sensors.yaml` font assignments. Update `devices/manifest.json`, then regenerate.

## Volume Modal Layout

The volume modal layout is in:

- `components/espcontrol/button_grid.h`

Important constants:

| Constant | Purpose |
|---|---|
| `MEDIA_VOLUME_REFERENCE_SIDE_PX` | Reference short side, currently `480` |
| `MEDIA_VOLUME_ARC_STROKE_REF_PX` | Master arc stroke thickness at the 480px reference |
| `MEDIA_VOLUME_BUTTON_REF_PX` | Master plus/minus button size at the 480px reference |
| `MEDIA_VOLUME_BACK_BUTTON_REF_PX` | Master back button touch target at the 480px reference |

The modal uses `media_volume_scaled_px(...)` for layout measurements. That is correct for shapes, spacing, buttons, and arc thickness.

For text and icon glyphs, prefer native font sizes in YAML instead of `transform_zoom`. Native font sizes render more cleanly because the glyphs are generated at the final size.

## How to Change a Master Size

1. Decide the new 480px reference size.
2. Calculate each target size with `reference size * target short side / 480`.
3. Round to the nearest whole number.
4. Update the matching native font entries in `devices/<device>/device/fonts.yaml`.
5. If a font ID changes, update `devices/manifest.json` and the matching `devices/<device>/packages.yaml` substitution.
6. Regenerate generated device config:

```sh
python3 scripts/generate_device_slots.py
```

7. Run the checks:

```sh
npm run check:config
python3 scripts/generate_device_slots.py --check
git diff --check
```

`npm run check:generated` is also useful, but it checks webserver bundles too. If it only reports stale `docs/public/webserver/*/www.js` files, that is separate from display sizing.

## Quick Calculator

You can use this one-line calculator from the repo root:

```sh
python3 -c 'import math; ref=44; short=600; print(math.floor(ref * short / 480 + 0.5))'
```

Change `ref` to the 480px reference size and `short` to the target device's short side.
