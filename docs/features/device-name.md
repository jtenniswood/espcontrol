---
title: "Device Name"
description: Rename your EspControl panel, check its new address, and update affected Home Assistant action names.
---

# Device Name

1. Open **System > Device Name** and enter a name such as **Kitchen**.
2. Check the address preview, then choose **Save & Restart**.
3. Reopen the setup page using the new address or the current IP shown before restart.

The address includes a hardware suffix, for example `kitchen-b2c3.local`. Clearing the name restores the firmware's original name and address. Normal firmware updates keep the custom name.

Home Assistant entity IDs stay unchanged, and a name set manually in Home Assistant takes precedence. ESPHome action names include the hostname: update automations using actions such as `esphome.<old_name>_navigate`. Reload the ESPHome integration if it still lists the old action names. Update router-managed DNS entries separately if needed.
