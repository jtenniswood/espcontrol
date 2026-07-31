# Assets

This directory contains internal firmware assets for icons and glyph sets. It is
repository documentation for maintainers and is not part of the public docs site.

The Material Design Icons font is committed under `common/assets/fonts/` so
firmware builds do not depend on downloading it during ESPHome configuration.
Keep its filename version aligned with `MDI_VERSION` in `scripts/build.py`.

## Font style names

Device fonts use functional style IDs instead of physical names such as
`font_roboto_regular_22_text` or `font_mdi_48_icons`. New firmware features
should reference the style IDs directly so the same feature YAML can render at
the right size on every device.

| Style ID | Intended use |
| --- | --- |
| `font_icon_main` | Main action icons and setup icons |
| `font_icon_card` | Smaller fixed climate option chip icons |
| `font_icon_status` | Status, network, and subpage indicator icons |
| `font_text_body` | Labels, setup body copy, and normal UI text |
| `font_text_small` | Compact supporting text on small displays, only where needed |
| `font_text_title` | Headings and media titles |
| `font_text_large` | Larger supporting labels, only where needed |
| `font_number_value` | Normal sensor/statistic values |
| `font_number_value_large` | Large sensor/statistic values |
| `font_number_modal` | Large modal numeric values |
| `font_number_clock` | Screensaver clock digits |

Do not add alias fonts just to keep an old role name working. If two roles are
intended to look the same, point them at the same style ID.

The old shared common font package was removed from normal device builds. Each
device defines the style IDs it needs in `devices/*/device/fonts.yaml`, using
device-specific sizes behind the same generic names. This avoids loading setup
fonts that duplicate fonts already available on the device.

Some icon roles intentionally use smaller glyph sets. `font_icon_main` carries
the user-selectable icon picker glyphs. `font_icon_card` carries only the fixed
climate option chip icons; user-selected climate card icons still render through
`font_icon_main`.

### Shared ratios

There are repeated ratios that should be preserved when adding or adjusting a
device unless the layout clearly needs an exception:

| Relationship | Current rule |
| --- | --- |
| Large sensor numbers | About `2.5x` the normal sensor value font |
| Card icons | About `75%` of the main card icon font |
| 720x720 square panel | Mostly `1.5x` the 480x480 square panel |

Examples of the current ratios:

| Device class | Main icon | Card icon | Sensor | Large sensor |
| --- | ---: | ---: | ---: | ---: |
| 480x480 square | 44 | 33 | 44 | 110 |
| 720x720 square | 66 | 50 | 66 | 165 |
| 1024x600 landscape | 55 | 41 | 55 | 138 |
| 480x800 portrait | 62 | 47 | 62 | 155 |
| 1280x800 landscape | 56 | 42 | 52 | 130 |

### Role assignments

The firmware role names in `devices/manifest.json` remain usage-based because
they describe generated UI wiring. They should map to the generic style IDs:

| Role | Typical style ID |
| --- | --- |
| `icon` | `font_icon_main` |
| `climateCardIcon` | `font_icon_card` |
| `subpageChevron` | `font_icon_status` |
| `sensor` | `font_number_value` |
| `largeSensor` | `font_number_value_large` |
| `mediaTitle` | `font_text_title` |
| `volumeNumber` | `font_number_modal` |
| `volumeLabel` | `font_text_body` or `font_text_large` |
| `climateOptionTitle` | `font_text_body` |
| `climateOptionValue` | `font_text_body` or `font_text_small` |

When changing the ramp, update the device font file and manifest together, then
run:

```sh
python3 scripts/generate_device_slots.py
npm run check:all
```

## How to add an icon

All button icons are defined once in [`icons.json`](icons.json) and synced to the device font list, firmware lookup table, and web UI by a script. Never edit generated icon lists directly.

## 1. Find the icon on MDI

Browse [Material Design Icons](https://materialdesignicons.com/) and note three things:

| Field | Example | Where to find it |
|-------|---------|-------------------|
| **name** | `Ceiling Fan` | Choose a user-friendly display name |
| **codepoint** | `F1797` | Shown on the icon detail page (hex, no `0x` prefix) |
| **mdi** | `ceiling-fan` | The MDI class name (used as `mdi-ceiling-fan` in CSS) |

## 2. Add the entry to `icons.json`

Open `common/assets/icons.json` and add an object to the `"icons"` array:

```json
{ "name": "Ceiling Fan", "codepoint": "F1797", "mdi": "ceiling-fan" }
```

The array order determines display order in the LVGL font glyph list and the C++ lookup table. The YAML select options and JS picker sort alphabetically, so position doesn't matter for those.

## 3. Run the sync script

```sh
python3 scripts/build.py icons
```

This patches the generated icon sections in:

- `common/assets/icon_glyphs.yaml` — LVGL font glyph codepoints
- `components/espcontrol/icons.h` — C++ icon lookup table and domain defaults
- `src/webserver/generated/icons.ts` — web UI icon picker names and domain defaults

Run `python3 scripts/build.py` to also rebuild the shared web UI bundle at `docs/public/webserver/www.js`.

## 4. Verify

```sh
python3 scripts/build.py icons --check
```

Exits 0 if everything is in sync. The check also compares each `icons.json` codepoint with the pinned Material Design Icons release, so the browser preview and device font cannot silently drift apart.

## Domain defaults

To change which icon is used when a button's icon is set to "Auto", edit the `"domain_defaults"` object in `icons.json`. Values must reference an icon `name` from the `"icons"` array.
