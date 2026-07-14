# Add or Change a Supported Device

Use this when adding a display model, changing a device profile, or changing
device-specific ESPHome YAML.

## Edit First

- `devices/catalog.json`
- `devices/<slug>/packages.yaml`
- `devices/<slug>/dev.yaml`
- `devices/<slug>/esphome.yaml`
- `devices/<slug>/device/*.yaml`

Only edit shared firmware or web code after the device profile proves it needs a
shared behavior change.

## Ask Before

- Removing or renaming an existing device slug.
- Changing a published install path or release-facing build name.
- Changing slot count, orientation, or default rotation for an existing device.
- Adding a device-specific exception to shared code instead of manifest data.

## Checklist

- [ ] Add or update the device entry in `devices/catalog.json`.
- [ ] Select profiles for shared capabilities instead of copying those fields
      into the device `config` block.
- [ ] Use explicit `overrides` only when a device must replace an inherited
      profile value.
- [ ] Add or update the per-device ESPHome files under `devices/<slug>/`.
- [ ] Confirm `layout.cols * layout.rows` equals the device slot count.
- [ ] Confirm required `firmware.fonts` roles are mapped.
- [ ] Confirm the public screen docs path and release metadata are correct.
- [ ] Review web preview sizing in the catalog `web` configuration.
- [ ] Use `dev.yaml` for local compile checks and `esphome.yaml` for the
      production package shape.

## Regenerate

```bash
python3 scripts/generate_device_manifest.py
python3 scripts/build.py
python3 scripts/generate_device_slots.py
```

Do not edit generated files or generated YAML blocks directly. The
source-to-generated mapping is in
[Source of Truth Contract](../source-of-truth.md).

Expected generated files commonly include:

- `docs/public/device-profiles.json`
- `docs/generated/screens/*.md`
- generated blocks in `devices/*/packages.yaml`
- generated blocks in `devices/*/device/sensors.yaml`
- `docs/public/webserver/*/www.js` when web profile data changes

## Stop If

- Generated files for unrelated devices change without a manifest reason.
- Release-facing files under `builds/` change unexpectedly.
- A generated block was edited by hand.
- The new device cannot be represented with catalog profiles and configuration fields.

## Verify

| Level | Run | Stop when |
|---|---|---|
| Minimum | `npm run check:device-matrix`<br>`npm run check:device-profiles` | The change only adjusts manifest/profile data or generated screen/profile docs, and the generated files match the expected device scope. |
| Recommended | `npm run check:product` | Most device-support changes can stop here after product schema, generated outputs, device profiles, web smoke, and release-facing metadata checks pass. |
| Release-grade | Compile the affected device firmware with ESPHome.<br>`npm run check:fast` | Use before publishing support, changing ESPHome YAML, changing release-facing build metadata, or touching shared firmware/web behavior for the device. |
