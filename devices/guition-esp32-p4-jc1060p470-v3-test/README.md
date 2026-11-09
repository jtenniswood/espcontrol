# JC1060P470 7-inch V3 experiment

Test package for [issue #2442](https://github.com/jtenniswood/espcontrol/issues/2442).
The hypothesis is that the 7-inch V3 retains the V2 LCD and wiring but uses
production ESP32-P4 v3.x silicon. **This has not been tested on a physical V3.**
Before installing, obtain the chip revision from esptool or the USB boot log and
confirm **v3.x**, plus the exact rear-label model. Do not infer silicon revision
from a date code. Keep the seller's original firmware and recovery instructions.
The pinned toolchain currently produces images supporting **v3.1 through v3.99**;
check the generated bootloader/application header files against the actual chip
revision before flashing. A v3.0 chip is outside this build's revision range.

The package inherits the complete 7-inch V2 configuration: 1024 x 600 JD9165
initialization and timings, GPIO0 panel reset, GT911 touch, backlight and 10MHz
ESP32-C6 SDIO link. It changes `engineering_sample` to `false` and loads this
branch's existing `mipi_dsi` driver, which selects the production P4 PHY clock
and compensates DPI timing for the actual hardware clock divider.

This is a manual experiment, deliberately absent from the public device matrix,
installer and release assets. It reuses V2's web/layout identity, so seeing a V2
label in the editor is expected. The boot warning, device name and firmware
version identify the experiment. P4 and C6 firmware updater packages are omitted;
browser firmware uploads are blocked. Native ESPHome OTA remains available.
The experiment does not flash or update the C6 firmware.

## Build a USB factory image

With Git, Python 3 and Docker available:

```bash
git fetch origin
git switch test-jc1060p470-v3
python3 scripts/build_jc1060p470_v3_test.py
```

On Windows, install Git for Windows, Python 3 and Docker Desktop. Start Docker
Desktop with **Linux containers**, then run from PowerShell or Command Prompt:

```powershell
python scripts/build_jc1060p470_v3_test.py
```

Use `py -3` instead of `python` if that is how Python is installed. Keep the
checkout and any custom `ESPCONTROL_IDF_CACHE` directory accessible to Docker
Desktop. Paths containing spaces are supported. Windows uses Docker's default
container user; macOS/Linux builds retain the host user's file ownership.

The builder requires a clean committed checkout. It uses `.github/esphome.env`
(currently ESPHome 2026.10.2), verifies ESP-IDF 5.5.5 or newer in the 5.5 series,
validates the configuration and performs a clean compile. It checks production
chip image headers, the factory-image flash offsets, the vendored display driver,
the browser-upload guard, native OTA and the absence of both updaters.
The downloaded toolchain is retained in `.esphome/idf-cache/`; optionally set
`ESPCONTROL_IDF_CACHE` to an absolute directory to reuse another local cache.

Output is under
`build/v3-test/guition-esp32-p4-jc1060p470-v3-test/<full-commit>/`:

- `guition-esp32-p4-jc1060p470-v3-test.factory.bin`: merged USB image.
- `guition-esp32-p4-jc1060p470-v3-test.ota.bin`: application for native OTA.
- `SHA256SUMS`, `build-info.json`, `firmware.elf`, `sdkconfig`, resolved
  configuration (`config.log`), compile log, image headers and flash plan.

The factory image embeds the branch editor and no Wi-Fi credentials. Its version
is `dev-7v3-<12-character-commit>`.

## First installation

Use a USB data cable and the board's programming port. Enter download mode with
the board's BOOT/reset controls, then replace the port and image path:

```bash
esptool --chip esp32p4 --port YOUR_SERIAL_PORT --before no-reset write-flash 0x0 /path/to/guition-esp32-p4-jc1060p470-v3-test.factory.bin
```

Use the **merged factory image** at `0x0`; the OTA application is not a substitute.
Do not use a V1/V2 factory or C6 recovery image on this experiment. Release BOOT,
reset, then disconnect and reconnect power. Capture the full USB startup log.

Connect to the `EspControl_XXXX` Wi-Fi setup hotspot, where `XXXX` is the last
four characters of the device's MAC address. Open `http://192.168.4.1` and enter
Wi-Fi credentials. This can be done even if the screen is blank. Once connected,
open the device's web interface and pair with Home Assistant. Allow Home
Assistant actions for a control test.

If it cannot boot, recover over USB using the seller's firmware for this exact
board and chip revision. Returning to the public 7-inch V2 image is not a
production-chip recovery path.

## Manual ESPHome setup and later OTA

Use the pinned ESPHome version. Keep credentials in your local `secrets.yaml`:

```yaml
substitutions:
  name: espctl-7v3-test
  friendly_name: EspControl 7inch V3 Test
  firmware_version: dev-7v3-COMMIT
  # Replace COMMIT below and above with the full commit being tested.
  espcontrol_component_ref: COMMIT

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  api_encryption:
    url: https://github.com/jtenniswood/espcontrol/
    ref: COMMIT
    file: common/addon/api_encryption_dynamic.yaml
    refresh: 1s
  setup:
    url: https://github.com/jtenniswood/espcontrol/
    ref: COMMIT
    file: devices/guition-esp32-p4-jc1060p470-v3-test/test-package.yaml
    refresh: 1s
```

Use the same source commit for all three refs. After the first USB install,
Device Builder's wireless install uses native ESPHome OTA. Manual builds load
the branch editor from jsDelivr, so the browser needs internet access; factory
builds embed the editor. The saved API encryption key must match the device;
retain the adopted configuration when updating an existing installation.

## Hardware acceptance

Record the rear label, chip revision, source commit, ESPHome version and full
startup log without sharing Wi-Fi passwords or API/OTA keys.

1. Confirm production P4 revision, PSRAM detection, stable boot and no repeating
   watchdog resets. Check Wi-Fi, the editor and Home Assistant discovery.
2. Check full 1024 x 600 coverage, correct colours and a stable picture. Tap the
   centre and all four corners, and operate one Home Assistant control.
3. Check brightness, sleep and touch-to-wake repeatedly.
4. Perform several software restarts and at least three full power cycles, then
   leave it operating for at least 30 minutes. Report streaks, corruption or resets.
5. Confirm P4/C6 firmware-update controls are absent. A harmless dummy multipart
   POST to `/update` must return HTTP 403 during Wi-Fi setup and after connection;
   repeat with `/up%64ate` and `/%75pdate?source=test`. Do not upload real firmware
   as a rejection test. Wi-Fi and normal editor saves must work.
6. Install another build of this experiment through native ESPHome OTA, then
   repeat picture, touch, restart and cold-start checks.

If Wi-Fi works but the picture is white or corrupted, collect the log before
changing settings: the next step is to compare manufacturer V3 panel data with
V2 initialization and timings. If display startup crashes, decode it with the
saved ELF and inspect clock setup. If the firmware cannot boot, check chip
targeting, PSRAM and the flash plan first. Change one setting at a time in later
experiments. Leave the issue and PR open until the owner confirms the result.
