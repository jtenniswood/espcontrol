# Font Guidelines

Firmware fonts are expensive on embedded displays. Each new font size, weight,
or glyph set increases firmware size and memory pressure, and it must be kept
consistent across every supported device. Prefer reusing existing font roles.

## Main Rule

Do not create a new font size for a one-off layout adjustment.

Use the existing functional font roles first:

- body text
- title text
- card icons
- status/subpage icons
- normal sensor numbers
- large sensor numbers
- modal numbers
- modal/supporting labels
- cover-art title, artist, and time text

If text does not fit, fix the layout before adding a font size:

- constrain the label width or height
- use `LV_LABEL_LONG_DOT` for ellipsis
- allow the existing label to wrap where the design supports it
- reduce padding or gaps
- choose a shorter label
- reuse a smaller existing role such as `font_text_small` where that role already
  exists on the target devices

## Why New Sizes Are Avoided

Every new physical font entry must be compiled into each affected firmware
image. That has several costs:

- larger firmware builds
- more RAM/flash pressure
- longer compile time
- more glyph-set maintenance
- more per-device tuning
- higher risk that one screen looks correct while another breaks

The project supports different display sizes, orientations, and pixel densities.
A physical size that works on one panel is rarely the right abstraction for all
panels.

## Functional Roles vs Physical Sizes

Device font files define physical sizes behind stable role-like IDs:

```text
devices/<slug>/device/fonts.yaml
```

Examples:

- `font_text_body`
- `font_text_title`
- `font_text_small`
- `font_icon_main`
- `font_icon_card`
- `font_icon_status`
- `font_number_value`
- `font_number_value_large`
- `font_number_modal`

The same ID can have a different physical size on each device. Card and feature
code should depend on the role, not a hardcoded size.

Font weight is chosen by the font source, not by a runtime LVGL bold toggle. For
Google Fonts, use suffixes such as:

- `gfonts://Roboto`
- `gfonts://Roboto@Light`
- `gfonts://Roboto@Medium`
- `gfonts://Roboto@Bold`

A heavier weight is a separate compiled font entry, so treat it with the same
care as a new size.

## Firmware Font Role Map

The cross-device role map lives in `devices/manifest.json` under each device's
`firmware.fonts` section.

Those roles are read by `scripts/device_profiles.py` and written into generated
slot setup by `scripts/generate_device_slots.py`. Firmware then receives the
resolved `lv_font_t *` values through grid config fields such as:

- `icon_font`
- `sp_sensor_font`
- `sp_large_sensor_font`
- `media_title_font`
- `volume_number_font`
- `volume_label_font`
- `climate_card_icon_font`
- `climate_option_title_font`
- `climate_option_value_font`
- `subpage_chevron_font`

When adding or changing card UI, prefer one of these existing pointers.

## Glyph Sets Matter

Fonts only include the glyphs explicitly listed for that font.

- Text fonts usually include `common/assets/text_glyphs.yaml`.
- Icon fonts use Material Design Icon glyph sets such as
  `common/assets/icon_glyphs.yaml`.
- Number fonts intentionally include only digits and a few symbols such as
  `0-9 . - ° / % :`.

Do not use a number font for normal text. Do not use an icon font for plain text.
Missing glyphs appear as boxes or incorrect symbols on the device.

In firmware, resolve an icon name to its glyph string with `find_icon("Some Name")`
from `components/espcontrol/icons.h`, or embed a known codepoint directly as a
UTF-8 escape. Only glyphs present in the compiled icon set will render.

To make a new icon available, add it to `common/assets/icons.json` when it should
appear in the setup page, add the needed glyph to the relevant glyph set, and
run:

```bash
python3 scripts/build.py icons
```

The web configurator references icons by Material Design Icon name and renders
them with the MDI webfont/CSS.

## Images

The firmware UI convention is to use icon fonts, not bitmap images. This keeps
the display UI smaller, themeable, and easier to scale across devices.

If a raster image is genuinely required, add an ESPHome `image:` or
`online_image:` entry and render it with an LVGL image widget. Prefer an icon
glyph first unless the card specifically needs bitmap content.

## When a New Font Role Is Acceptable

A new font role is reasonable only when all of these are true:

- an existing role cannot meet the need with layout changes
- the need is reusable, not tied to one card's local spacing issue
- the role has a clear purpose and name
- every supported device gets an appropriate physical font mapping
- generated slot/config code is updated
- firmware checks and at least one real compile confirm the result

Good role names describe purpose, not size:

- `alarmKeypadLabel`
- `coverArtSubtitle`
- `compactListValue`

Avoid names like:

- `font_18`
- `tiny_fix`
- `card_special_small`

## Adding a New Font Role

Only use this path after the reuse options above have been exhausted.

1. Add the physical font entry to each relevant
   `devices/<slug>/device/fonts.yaml`.
2. Add the role mapping under each device's `firmware.fonts` in
   `devices/manifest.json`.
3. Read the role in `scripts/device_profiles.py`.
4. Emit it in `scripts/generate_device_slots.py`.
5. Add the corresponding field to the firmware grid/config structure.
6. Pass the font pointer to the card or modal that needs it.
7. Regenerate device slots.

```bash
python3 scripts/generate_device_slots.py
npm run check:device-profiles
npm run check:device-matrix
npm run check:product
```

## Review Checklist

Before accepting a font change, confirm:

- no new physical size was added for a single label
- existing roles were considered first
- all supported devices still have valid font mappings
- text still fits on the smallest relevant screen
- number-only fonts are not used for words
- icon glyphs are present in the compiled icon glyph set
- generated files were rebuilt when role mappings changed
