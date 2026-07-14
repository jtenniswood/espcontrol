# Devices and Builds

Device support is authored in `devices/catalog.json` plus per-device ESPHome YAML
under `devices/<slug>/`. `devices/manifest.json` is a generated, committed
compatibility copy for existing tools.

## Device Catalog and Compatibility Manifest

`devices/catalog.json` defines:

- public name and docs path
- screen size, resolution, and orientation
- slot count and grid layout
- web preview sizing and drag behavior
- supported rotation values
- firmware chip family
- firmware font roles
- display-specific options
- package substitutions and release metadata

The catalog contains reusable profile categories and ordered device entries. Run
`python3 scripts/generate_device_manifest.py` after editing it. Validators check
that the expanded manifest is current before generated files are accepted.

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

Build and upload local firmware from the repo root with the local ESPHome helper:

```bash
python3 scripts/local_esphome.py devices/<slug>/dev.yaml run
```

The helper injects `dev` as the `firmware_version`. That version appears in
ESPHome logs, Home Assistant diagnostics, and the firmware version sensor.
Running `esphome run dev.yaml` directly still works, and it uses the same static
fallback version from `devices/<slug>/packages.yaml`.

If both USB and over-the-air upload targets are available, ESPHome prompts for a
choice. In scripts or background runs, that prompt can stop the upload, so pass
the target explicitly:

```bash
python3 scripts/local_esphome.py devices/<slug>/dev.yaml run --device 192.168.x.x
python3 scripts/local_esphome.py devices/<slug>/dev.yaml run --device /dev/cu.usbserial-...
python3 scripts/local_esphome.py devices/<slug>/dev.yaml run --device <ip> --no-logs
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

Generated bundles are committed even when firmware bundles them locally. Older
installed firmware can still point at the GitHub Pages copy of this path, while
new `builds/*.yaml` entry points use `web_server.js_include` so the setup page
matches the firmware branch being flashed. Local testing can still override
`web_server.js_url` to load a bundle served from a development machine.

`scripts/build.py` derives each device profile and passes it to the Node bundle
builder. That builder uses esbuild's API to produce a minified browser IIFE with
an ES2020 target. VM and browser smoke tests build fresh copies through the same
pipeline instead of reading the committed files. For an isolated build, run:

```bash
python3 scripts/build.py www --temporary-output /tmp/espcontrol-www
```

## Firmware Build Artifacts

Release-facing firmware YAML lives in `builds/`:

```text
builds/<slug>.yaml
builds/<slug>.factory.yaml
```

Release checks validate that these outputs stay aligned with device profiles and
public firmware expectations.

## Device Build Flags

Per-device `platformio_options.build_flags` are escape hatches. Prefer manifest
data, shared packages, or generated device slots for normal device differences.
When a flag is needed, keep it documented here so it can be reviewed and removed
deliberately.

| Flag | Devices | Purpose | Remove when |
|---|---|---|---|
| `ESPCONTROL_DISABLE_TODO=1` | All current production devices | Keeps the todo card firmware out of memory-constrained builds. | Todo memory use is reduced enough to compile and run reliably on every supported panel. |
| `ESPCONTROL_MAX_GRID_SLOTS=6` | `guition-esp32-p4-jc4880p443` | Caps runtime grid allocation to the device's 6 slots. | Grid slot capacity is generated from device profile data. |
| `ESPCONTROL_MAX_GRID_SLOTS=9` | `esp32-p4-86`, `guition-esp32-s3-4848s040` | Caps runtime grid allocation to the device's 9 slots. | Grid slot capacity is generated from device profile data. |
| `ESPCONTROL_ESPHOME_2026_5_REBUILD=1` | P4 devices | Forces PlatformIO to rebuild objects after ESPHome 2026.5 scheduler/watchdog changes. | Local and CI caches no longer contain stale 2026.4 objects, or the next deliberate cache-busting marker replaces it. |
| `ESPCONTROL_JC1060P470_BOOTFIX_20260522=1` | `guition-esp32-p4-jc1060p470` | Cache-busting marker for a JC1060P470 boot-loop fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC1060P470_OTA_WDT_20260526=1` | `guition-esp32-p4-jc1060p470` | Cache-busting marker for the OTA flash erase watchdog increase. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC4880P443_BOOTFIX_20260522=1` | `guition-esp32-p4-jc4880p443` | Cache-busting marker for a JC4880P443 boot-loop fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC4880P443_OTA_WDT_20260522=1` | `guition-esp32-p4-jc4880p443` | Cache-busting marker for the JC4880P443 OTA flash erase watchdog increase. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_BOOTFIX_20260526=1` | `guition-esp32-p4-jc8012p4a1` | Cache-busting marker for a JC8012P4A1 boot-loop fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_WDT_20260526=1` | `guition-esp32-p4-jc8012p4a1` | Cache-busting marker for the JC8012P4A1 watchdog increase. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_USB_LOGGER_UART0_20260528=1` | `guition-esp32-p4-jc8012p4a1` | Cache-busting marker for the JC8012P4A1 USB logger/UART0 change. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_OTA_PREP_20260528=1` | `guition-esp32-p4-jc8012p4a1` | Cache-busting marker for JC8012P4A1 OTA preparation changes. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_IMAGE_CARD_BOOTFIX_20260611=1` | `guition-esp32-p4-jc8012p4a1` | Cache-busting marker for an image-card boot fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_RESTORE_CRASH_RECOVERY_20260611=1` | `guition-esp32-p4-jc8012p4a1` | Cache-busting marker for restore crash recovery. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_RETERMINAL_D1001_BOOTFIX_20260607=1` | `seeed-esp32-p4-reterminal-d1001` | Cache-busting marker for the initial reTerminal D1001 bring-up. | A later required rebuild marker supersedes it. |

Per-device `platformio_options.build_src_flags` should stay even narrower:

| Flag | Devices | Purpose | Remove when |
|---|---|---|---|
| `-mtext-section-literals` | `guition-esp32-s3-4848s040` | Keeps Xtensa literal pools close enough for the large generated S3 firmware translation unit to link. | The S3 firmware is split into smaller translation units or ESPHome/toolchain changes make the flag unnecessary. |

## Adding a Device Checklist

1. Add the device entry in `devices/catalog.json` and regenerate `devices/manifest.json`.
2. Add per-device YAML under `devices/<slug>/`.
3. Confirm all required font roles exist.
4. Confirm slot count equals `layout.cols * layout.rows`.
5. Add public screen docs inputs if the device should appear on the public site.
6. Generate outputs.
7. Run product and device checks.
8. Compile the device firmware before publishing support.
