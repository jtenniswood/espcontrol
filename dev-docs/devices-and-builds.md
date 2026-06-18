# Devices and Builds

Device support is driven by `devices/manifest.json` plus per-device ESPHome YAML
under `devices/<slug>/`.

## Device Manifest

`devices/manifest.json` defines:

- public name and docs path
- screen size, resolution, and orientation
- slot count and grid layout
- web preview sizing and drag behavior
- supported rotation values
- firmware chip family
- firmware font roles
- display-specific options
- package substitutions and release metadata

Validators read the manifest before generated files are accepted.

## Per-Device Folder Shape

Each supported display normally has:

```text
devices/<slug>/
  esphome.yaml
  dev.yaml
  packages.yaml
  device/
    device.yaml
    fonts.yaml
    lvgl.yaml
    sensors.yaml
```

Some devices also include extra files for network coprocessors, ethernet, or
touchscreen variants.

## Production vs Local Development

Use `esphome.yaml` for the production package shape. It is the path end users
install.

Use `dev.yaml` for local work. It points ESPHome at local component sources under
`components/`, so firmware changes can be compiled before they are published.

`dev.yaml` does that with a local `external_components` override:

```yaml
external_components:
  - source:
      type: local
      path: ../../components
    components: [espcontrol, web_server_idf]
    refresh: 1s
```

Build and upload local firmware from the device folder:

```bash
cd devices/<slug>
esphome run dev.yaml
```

If both USB and over-the-air upload targets are available, ESPHome prompts for a
choice. In scripts or background runs, that prompt can stop the upload, so pass
the target explicitly:

```bash
esphome run dev.yaml --device 192.168.x.x
esphome run dev.yaml --device /dev/cu.usbserial-...
esphome run dev.yaml --device <ip> --no-logs
```

OTA upload only works after the display is already running EspControl firmware
and is connected to the network. First flash is over USB.

## Generated Device Outputs

Device-profile changes can regenerate:

- `docs/public/device-profiles.json`
- `docs/generated/screens/*.md`
- generated blocks in `devices/*/packages.yaml`
- generated blocks in `devices/*/device/sensors.yaml`
- `docs/public/webserver/*/www.js` when web profile data changes

Run:

```bash
python3 scripts/build.py
python3 scripts/generate_device_slots.py
```

Then verify:

```bash
npm run check:device-matrix
npm run check:device-profiles
npm run check:product
```

## Web Bundle Output

Each device gets a bundle at:

```text
docs/public/webserver/<slug>/www.js
```

Production firmware points the browser setup page at the GitHub Pages copy of
that bundle. Local testing can override `web_server.js_url` to load a bundle
served from a development machine.

## Firmware Build Artifacts

Release-facing firmware YAML lives in `builds/`:

```text
builds/<slug>.yaml
builds/<slug>.factory.yaml
```

Release checks validate that these outputs stay aligned with device profiles and
public firmware expectations.

## Adding a Device Checklist

1. Add manifest entry in `devices/manifest.json`.
2. Add per-device YAML under `devices/<slug>/`.
3. Confirm all required font roles exist.
4. Confirm slot count equals `layout.cols * layout.rows`.
5. Add public screen docs inputs if the device should appear on the public site.
6. Generate outputs.
7. Run product and device checks.
8. Compile the device firmware before publishing support.
