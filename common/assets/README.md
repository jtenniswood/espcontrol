# How to add an icon

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
- `src/webserver/www.js` — web UI icon picker names and domain defaults

Run `python3 scripts/build.py` to also rebuild the generated per-device web UI bundles under `docs/public/webserver/.../www.js`.

## 4. Verify

```sh
python3 scripts/build.py icons --check
```

Exits 0 if everything is in sync. The check also compares each `icons.json` codepoint with the pinned Material Design Icons release, so the browser preview and device font cannot silently drift apart.

## Domain defaults

To change which icon is used when a button's icon is set to "Auto", edit the `"domain_defaults"` object in `icons.json`. Values must reference an icon `name` from the `"icons"` array.
