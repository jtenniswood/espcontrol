---
title: EspControl Battery Status
description:
  How to enable the experimental battery status icon on supported panels.
---

# Battery Status (Experimental)

Some Guition JC8012P4A1 (10.1") units ship with an IP5306-based battery add-on board. EspControl can show a battery icon in the top bar for these units, but the wiring (GPIO52, post-divider voltage range) is based on community and third-party BSP reports and has **not** been confirmed by the firmware maintainer. It may not work, or may read incorrectly, on every unit.

Because of this, the feature is off by default and only appears in **Settings > Display > Battery** on JC8012P4A1 panels.

- **Show Battery Icon (Experimental)** - turns the battery icon in the top bar on or off. The icon reflects the "Battery" percentage sensor (a diagnostic entity in Home Assistant), which is estimated from the raw ADC voltage using a linear approximation between an empty and full voltage.

If the icon shows the wrong level, or your unit does not have the battery add-on, leave this setting off. The "Battery Voltage Raw" and "Battery Voltage" diagnostic sensors in Home Assistant can help you verify or recalibrate the readings against a multimeter. There is currently no way to detect charging state from firmware alone, so the icon only ever shows a charge level, never a "charging" state.
