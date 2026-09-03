# Devices and Builds

Device support is authored in `product/v2/device_catalog.json` plus per-device ESPHome YAML
under `devices/<slug>/`. `devices/manifest.json` is a generated, committed
compatibility copy for existing tools.

## Device Catalog and Compatibility Manifest

`product/v2/device_catalog.json` defines:

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

The helper injects `dev` as the `firmware_version`. That version appears in
ESPHome logs, Home Assistant diagnostics, and the firmware version sensor.
Running `esphome run dev.yaml` directly still works, and it uses the same static
fallback version from `devices/<slug>/packages.yaml`.

OTA upload only works after the display is already running EspControl firmware
and is connected to the network. First flash is over USB.

The [supported-device playbook](playbooks/add-supported-device.md) owns local
compile/upload commands and explicit target selection.

## Generated Device Outputs

Device-profile changes can regenerate:

- `docs/public/device-profiles.json`
- `docs/generated/screens/*.md`
- generated blocks in `devices/*/packages.yaml`
- generated blocks in `devices/*/device/sensors.yaml`
- files under `docs/public/webserver/` when web profile data changes

The supported-device playbook owns the generator order and verification checks.

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
pipeline instead of reading the committed files. The web configurator playbook
owns normal regeneration; the generator also supports isolated temporary output
for tests and diagnostics.

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
The exact flag/device mapping below is generated from every device YAML and its
local shared-package include graph, including `build_flags` and
`build_src_flags` lists. Maintainers own the purpose and removal criteria in
`dev-docs/build-flag-notes.json`.

Run `python3 scripts/check_dev_docs.py --update` after a device flag or its notes
change. `npm run check:dev-docs` fails for a stale mapping, an undocumented flag,
or notes for a flag that no device uses.

<!-- BEGIN GENERATED DEVICE BUILD FLAGS -->
| Flag | Devices from YAML | Purpose | Remove when |
| --- | --- | --- | --- |
| `-Wno-complain-wrong-lang` | `esp32-p4-86`<br>`guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-p4-jc4880p443`<br>`guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Suppresses an ESP-IDF warning option that is invalid for C sources in the P4 build. | The upstream headers/build no longer pass the C++-only warning option to C sources. |
| `-Wno-deprecated-declarations` | `esp32-p4-86`<br>`guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-p4-jc4880p443`<br>`guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Suppresses deprecation warnings emitted by current ESPHome/ESP-IDF dependencies in P4 builds. | Supported dependencies compile these device profiles without the deprecated declarations. |
| `-Wno-literal-suffix` | `esp32-p4-86`<br>`guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-p4-jc4880p443`<br>`guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Suppresses the ESP-IDF header literal-suffix warning in P4 C++ builds. | The upstream headers compile cleanly without the suppression. |
| `-mtext-section-literals` | `guition-esp32-s3-4848s040` | Keeps Xtensa literal pools close enough for the large generated S3 firmware translation unit to link. | The S3 firmware is split into smaller translation units or the toolchain makes the flag unnecessary. |
| `ESPCONTROL_DEVICE_SLUG="${device_slug}"` | `esp32-p4-86`<br>`guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-p4-jc4880p443`<br>`guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Injects the selected device profile slug into the compiled firmware for diagnostics and runtime identification. | The device slug is supplied through generated component configuration instead of a preprocessor definition. |
| `ESPCONTROL_DISABLE_WEATHER_FORECAST=1` | `guition-esp32-s3-4848s040` | Excludes the weather-forecast card from the constrained S3 firmware. | The card compiles and runs with safe flash and heap headroom on that panel. |
| `ESPCONTROL_ESPHOME_2026_5_REBUILD=1` | `esp32-p4-86`<br>`guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-p4-jc4880p443`<br>`guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Forces PlatformIO to rebuild objects after ESPHome 2026.5 scheduler and watchdog changes. | Stale 2026.4 objects are no longer present in supported caches, or a later marker supersedes it. |
| `ESPCONTROL_JC1060P470_BOOTFIX_20260522=1` | `guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the JC1060P470 boot-loop fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC1060P470_OTA_WDT_20260526=1` | `guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the JC1060P470 OTA flash erase watchdog increase. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC4880P443_BOOTFIX_20260522=1` | `guition-esp32-p4-jc4880p443`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the JC4880P443 boot-loop fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC4880P443_OTA_WDT_20260522=1` | `guition-esp32-p4-jc4880p443`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the JC4880P443 OTA flash erase watchdog increase. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_BOOTFIX_20260526=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the JC8012P4A1 boot-loop fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_GSL3680_TOUCH_SCALE_20260626=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for restoring native display-space GSL3680 touch scaling. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_GSL3680_WAKE_TAP_20260625=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the GSL3680 wake-tap handling change. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_IMAGE_CARD_BOOTFIX_20260611=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for an image-card boot fix. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_OTA_PREP_20260528=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for JC8012P4A1 OTA preparation changes. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_RESTORE_CRASH_RECOVERY_20260611=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for restore crash recovery. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_USB_LOGGER_UART0_20260528=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the JC8012P4A1 USB logger and UART0 change. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_JC8012P4A1_WDT_20260526=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Cache-busting marker for the JC8012P4A1 watchdog increase. | A later required rebuild marker supersedes it. |
| `ESPCONTROL_KEEP_LVGL_ACTIVE_ON_DISPLAY_OFF=1` | `guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Keeps LVGL processing active while the panel is electrically off so scheduled and touch wake paths remain available. | ESPHome/LVGL can suspend and resume these panels without losing wake behavior. |
| `ESPCONTROL_LOW_HEAP_COVER_ART=1` | `guition-esp32-s3-4848s040` | Uses the reduced-memory cover-art path on the constrained S3 panel. | The full cover-art path fits with safe runtime heap headroom. |
| `ESPCONTROL_LOW_HEAP_MEDIA_CONTROL=1` | `guition-esp32-s3-4848s040` | Uses the reduced-memory media modal path on the constrained S3 panel. | The full media path fits with safe runtime heap headroom. |
| `ESPCONTROL_MAX_GRID_SLOTS=6` | `guition-esp32-p4-jc4880p443`<br>`guition-esp32-s3-4848s040` | Caps runtime grid allocation to six slots. | Grid slot capacity is generated from device profile data. |
| `ESPCONTROL_MAX_GRID_SLOTS=9` | `esp32-p4-86`<br>`guition-esp32-s3-4848s040` | Caps runtime grid allocation to nine slots. | Grid slot capacity is generated from device profile data. |
| `LV_USE_BIDI=1` | `esp32-p4-86`<br>`guition-esp32-p4-jc1060p470`<br>`guition-esp32-p4-jc1060p470-v2`<br>`guition-esp32-p4-jc4880p443`<br>`guition-esp32-p4-jc8012p4a1`<br>`guition-esp32-p4-jc8012p4a1-v2`<br>`guition-esp32-s3-4848s040` | Enables LVGL bidirectional text ordering for right-to-left scripts such as Hebrew and Arabic. | ESPHome enables LVGL bidirectional text for every supported display without this compile definition. |
<!-- END GENERATED DEVICE BUILD FLAGS -->

## Changing Device Support

Use [Add or Change a Supported Device](playbooks/add-supported-device.md) for the
complete checklist, generator order, stop conditions, and checks.
