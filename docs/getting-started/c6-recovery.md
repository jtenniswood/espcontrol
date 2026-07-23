---
title: Repair the P4 C6 WiFi Processor
description:
  Use USB to reinstall EspControl and repair the ESP32-C6 WiFi processor on a supported ESP32-P4 panel without needing a network connection.
---

# C6 Wi‑Fi Processor Recovery

Supported P4 panels use a separate **ESP32-C6 processor** for Wi‑Fi. Use this
recovery installer if your panel:

- repeatedly disconnects or has an unreliable connection;
- cannot complete its first Wi‑Fi setup;
- disappears from Home Assistant after restarting; or
- reports ESP32-C6 firmware manifest timeouts in its USB log.

The recovery download contains both the latest EspControl firmware and its matching
C6 firmware. The P4 transfers the C6 update internally, so the repair does **not**
need working Wi‑Fi or internet access after the USB download has completed.

::: warning This reinstalls EspControl
Export your configuration from **Settings > Backup** first when the panel is still
accessible. Recovery normally preserves saved configuration, but preservation
cannot be guaranteed after damaged or previously erased flash.
:::

## Run the Recovery

1. Use **Chrome or Edge on a desktop computer**.
2. Connect the panel's normal P4 USB programming port with a data-capable cable.
3. Select the exact panel model and revision below.
4. Confirm the selection, then choose **Repair C6 and reinstall EspControl**.
5. If the installer offers to erase the device, leave that option off to preserve
   saved configuration where possible.
6. Keep USB power connected after the browser finishes. The panel updates the C6
   internally and restarts automatically.

<C6RecoverySelector />

After the restart, reconnect the panel to Wi‑Fi if needed and confirm that it stays
available in Home Assistant.

## If Recovery Cannot Communicate with the C6

The normal USB socket connects to the P4, which then communicates with the C6 over
the board's internal SDIO connection. A completely non-responsive C6 cannot be
repaired through that path.

Some boards expose a separate C6 UART programming header. Direct recovery through
that header requires a compatible 3.3 V USB-to-UART programmer and the exact wiring
for the panel. Collect a [USB startup log](/reference/collect-usb-logs) and open a
[GitHub issue](https://github.com/jtenniswood/espcontrol/issues/new) before attempting
that advanced procedure.

Next: [Troubleshooting](/getting-started/troubleshooting)
