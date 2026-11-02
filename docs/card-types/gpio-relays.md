---
title: "GPIO Relay Cards"
description: "Configure opt-in GPIO controls for external relay modules on the 7-inch JC1060P470."
---

# GPIO Relay

GPIO Relay cards control external relay modules locally, without Home Assistant. They are separate from [Internal Switches](/card-types/internal-relays), which operate a panel's built-in relays.

This feature supports the **7-inch JC1060P470 original and V2 panels**. GPIO relay support is disabled by default and no pins are assigned automatically.

## Set up the controls

1. Open **Device Settings → Advanced → GPIO Relays**.
2. Enable **GPIO relay controls**.
3. Choose a **GPIO pin** for each control you need and optionally give it a name. GPIO **1, 2, and 3** are supported; each pin can belong to only one control. Leave unused controls **Not configured**.
4. Enable **Active low** only if the connected relay module requires a low signal to turn on.
5. Select **Save GPIO settings**. Saving turns all GPIO controls off.
6. Add a **GPIO Relay** card and choose the configured control. Select **Switch** to toggle it or **Push Button** for a 200 ms pulse.

Use a compatible external relay driver or module. GPIO pins provide control signals and cannot power a relay coil directly. Check your exact board's connector pinout and relay module requirements before connecting hardware. These pins cannot simultaneously serve another custom ESPHome peripheral.

Outputs start off after a restart. Disabling the feature turns outputs off and releases the pins. Unassigned controls do nothing. The card becomes available after at least one pin is configured and support is enabled.

GPIO assignments and polarity settings stay on the physical display and are not included in card configuration backups. A restored GPIO Relay card refers to the destination display's control number; check its pin assignment before using it. Factory/customization resets clear GPIO settings.

Test the assigned pin, polarity, switch behavior, and pulse timing with a disconnected load before relying on a control.
