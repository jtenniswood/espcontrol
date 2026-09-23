# JC8012P4A1 V3 hardware experiment

Test branch for [issue #1939](https://github.com/jtenniswood/espcontrol/issues/1939).
The reported stock firmware identifies an ESP32-P4 **v3.2** chip. This package
changes the inherited V2 target to `engineering_sample: false`; the 360MHz CPU,
200MHz hex PSRAM, display initialization, GSL3680 touch and C6 wiring stay the same.
It also includes a branch-local MIPI DSI driver fix: ESP-IDF selects the PHY
clock appropriate to the silicon (XTAL on V3). The original driver's legacy
PLL_F20M selection caused an abort during display startup. Two testers on
[PR #1954](https://github.com/jtenniswood/espcontrol/pull/1954) reported working
displays with an XTAL override; this branch build still needs the full checklist
below. Remove any temporary third-party `mipi_dsi` override from your local YAML
before rebuilding, so the test uses this branch's component.

This is not a public V3 release. Use only on the new P4 v3.x hardware, identified
by its boot log or esptool chip information; the case date alone is insufficient.
The web interface reuses V2's layout identity, so a V2 label in that interface is
expected. The device name and firmware version identify the experiment.

## Build the USB image

With Git, Python 3 and Docker installed, check out `test-jc8012p4a1-v3` and run:

```bash
python3 scripts/build_jc8012p4a1_v3_test.py
```

The script requires a clean committed checkout, uses `.github/esphome.env`,
checks that ESP-IDF resolves to 5.5.5 or newer in the 5.5 series, then validates
and clean-builds the complete factory image. No Wi-Fi credentials are embedded.
Output is under `build/v3-test/<full-commit>/`, including the factory binary,
`SHA256SUMS`, `build-info.json`, resolved configuration, SDK configuration and
build log. The firmware version is `dev-v3-<12-character-commit>`.

## Install and connect

Keep the working vendor firmware available for returning the panel to stock.
Use command-line esptool, as the browser-flasher problem is not addressed here.
Enter download mode using the panel's BOOT/reset controls, then substitute your
USB serial port and the path printed by the build script:

```bash
esptool --chip esp32p4 --port YOUR_SERIAL_PORT --before no-reset write-flash 0x0 /path/to/guition-esp32-p4-jc8012p4a1-v3-test.factory.bin
```

Use the **merged factory image** at `0x0`. Do not substitute an OTA application
image or a V2 factory/recovery image. Release BOOT and reset/power-cycle the panel
after flashing. Join the `ESP_xxxxxx` hotspot shown on the panel (`xxxxxx` is
the last six MAC-address characters), and open `http://192.168.4.1` to enter
Wi-Fi credentials.
The captive portal can configure Wi-Fi but cannot upload firmware.

Automatic P4 and C6 updates start off, including after a reboot. Leave them off
for this experiment. The P4 manifest points to an unpublished test channel;
update checks may report an unavailable manifest. The reused web UI may list V2
releases, but browser uploads are blocked with HTTP 403, including the fallback
upload route. Use only branch-built USB images or native ESPHome OTA updates.
No C6 recovery image is built or installed.

## Manual ESPHome configuration / native OTA

Use the repository-pinned ESPHome version (currently 2026.9.0). Copy this into your
Device Builder configuration, set your device name, and keep Wi-Fi credentials
in your local `secrets.yaml`:

```yaml
substitutions:
  name: espctl-v3-test
  friendly_name: EspControl 10inch V3 Test
  # Replace COMMIT with the commit you are testing.
  firmware_version: dev-v3-COMMIT

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  api_encryption:
    url: https://github.com/jtenniswood/espcontrol/
    ref: test-jc8012p4a1-v3
    file: common/addon/api_encryption_dynamic.yaml
    refresh: 1s
  setup:
    url: https://github.com/jtenniswood/espcontrol/
    ref: test-jc8012p4a1-v3
    file: devices/guition-esp32-p4-jc8012p4a1-v3-test/test-package.yaml
    refresh: 1s
```

The manual package pulls external components and loads the complete web UI from
this branch through jsDelivr; the browser needs internet access. It bypasses the
public release-manifest loader, which does not recognize `dev-v3-*` versions.
The USB factory image embeds the matching web UI for offline setup.
For exact reproduction, replace both package refs and the
`espcontrol_component_ref` substitution with the same full commit SHA. Validate
and clean build before the first install. Initial installation needs the USB
factory image; once connected, Device Builder's wireless install uses native
ESPHome OTA. The factory image's dashboard adoption link also selects this branch.

If an earlier manual build opens a blank web page, rebuild from the updated branch
and install through native ESPHome OTA, then hard-refresh the page. Its page source
should load `webserver/embedded/www.js`, not `webserver/www.js`. Keep a unique
`dev-v3-<commit>` firmware version so test reports identify the installed build.

## Physical test checklist

- Capture startup logs: P4 v3.2, PSRAM detected, no repeating watchdog reset.
- Connect Wi-Fi, open the web interface and pair with Home Assistant.
- Check display image, brightness and rotation; touch the centre and all corners.
- Test sleep/wake, several cold starts and at least 30 minutes of operation.
- Verify a multipart POST to `/update` is rejected with HTTP 403, both while the
  Wi-Fi setup portal is active and after joining Wi-Fi. Repeat with encoded paths
  such as `/up%64ate` and `/%75pdate?source=test`; these must also return 403.
  Use a harmless dummy payload rather than firmware for these rejection checks.
  Do not use a V2 binary
  as the test payload. Wi-Fi setup and normal configuration saves must still work.
- Install another build of this V3 branch through native ESPHome OTA.
- If display/touch startup fails, provide the full serial log, exact YAML,
  firmware commit and crash backtrace. Retain the build's ELF for decoding.

Compilation is not physical validation. Leave the PR and issue open until the
hardware tests pass. Public installer/release support and browser-flasher repair
are follow-up work.
