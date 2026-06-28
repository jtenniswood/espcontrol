# Change Fonts or Icons

Use this when adding icon names, glyphs, or firmware font mappings.

## Edit First

- `common/assets/icons.json`
- `common/assets/*glyphs.yaml`
- `devices/<slug>/device/fonts.yaml`

Only edit manifest font roles or firmware structs after confirming an existing
font role cannot solve the layout need.

## Ask Before

- Adding a new firmware font role.
- Adding a new physical font size for a one-off layout issue.
- Removing glyphs that may already be used by saved configs or firmware screens.
- Using a number-only font for text or an icon font for plain text.

## Checklist

- [ ] Reuse existing font roles before adding any new size.
- [ ] Add user-selectable icons to `common/assets/icons.json`.
- [ ] Add required icon glyphs to the relevant `common/assets/*glyphs.yaml`.
- [ ] Add required text glyphs when labels use new characters.
- [ ] Update `devices/<slug>/device/fonts.yaml` only for affected devices.
- [ ] If a new reusable font role is approved, update:
      `devices/manifest.json`, `scripts/device_profiles.py`,
      `scripts/generate_device_slots.py`, and the consuming firmware config
      structure.

## Regenerate

For icons:

```bash
python3 scripts/build.py icons
```

For font roles or slot/profile data:

```bash
python3 scripts/generate_device_slots.py
```

Do not edit generated font, icon, or device-slot outputs directly. The
source-to-generated mapping is in
[Source of Truth Contract](../source-of-truth.md).

Expected generated files commonly include:

- generated icon outputs from `scripts/build.py icons`
- generated blocks in `devices/*/device/sensors.yaml`
- `docs/public/device-profiles.json` when profile data changes

## Stop If

- A font change affects every device for a single-card spacing issue.
- Generated slot files change for devices that should not be affected.
- Text would rely on missing glyphs.
- The smallest supported screen has not been considered.

## Verify

| Level | Run | Stop when |
|---|---|---|
| Minimum | `python3 scripts/check_icon_groups.py`<br>`npm run check:device-profiles`<br>`npm run check:device-matrix` | The change only adds icons, glyphs, or profile font mappings, and generated slot/profile files changed only for the intended devices. |
| Recommended | `npm run check:product` | Most font or icon changes can stop here after icon/profile consistency, generated outputs, web smoke, and release-facing metadata checks pass. |
| Release-grade | Compile an affected device firmware with ESPHome.<br>`npm run check:fast` | Use before release, when a firmware-visible font role changes, when the smallest screen layout may be affected, or when multiple devices inherit the change. |
