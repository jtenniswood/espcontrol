---
title: Request Support for Another ESP32 Touchscreen
description: Check supported EspControl displays and provide the hardware details needed to assess another ESP32 touchscreen for Home Assistant.
---

# Request Support for Another ESP32 Touchscreen

EspControl firmware is built for specific display boards. A matching processor, screen size, or resolution does not make another board compatible. Check [supported screens](/screens/) before buying or flashing a device.

If your board is not listed, [open a device-support request](https://github.com/jtenniswood/espcontrol/issues/new) with:

- The exact manufacturer, model, revision, and a product or documentation link.
- Display resolution, processor, memory, display driver, and touch controller where documented.
- Pin diagrams or an existing working ESPHome configuration, if available.
- Whether you own the hardware and can test a development build.

A request does not guarantee support. Contributors need reliable hardware information and testing on the actual device before a ready-to-install image can be offered. Do not flash a different model's firmware merely because its name or screen looks similar.
