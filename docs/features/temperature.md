---
title: EspControl Temperature Display
description:
  How to show temperature sensors on your EspControl panel's top bar.
---

# Temperature Display

The top bar of your panel can show one temperature sensor from Home Assistant.

- **Temperature** — select the temperature item in the screen preview and choose **Edit** to pick the sensor.
- **Show Degree Symbol** — edit the temperature item and turn this off if you want the clock bar to show only the temperature numbers, with no `°C` or `°F` suffix.
- **Temperature Unit** — choose **Auto**, **°C**, or **°F** in the **Temperature** section for the temperature labels. Auto uses the timezone setting and selects °F for US and US-territory timezones, otherwise °C.

Older configurations that saved more than one clock-bar temperature are still accepted, but current firmware only displays the first one.

If you hide the temperature item in the preview, the device screen hides it but keeps the chosen sensor so you can show it again later. If you turn off **Show Clock Bar** in the **Clock Bar** settings, the temperature display is hidden along with the top bar.
