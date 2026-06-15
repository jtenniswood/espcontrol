---
title: Collect USB Logs
description:
  How to collect EspControl logs over USB with the web installer and share them in a GitHub issue.
---

# Collect USB Logs

If something is not working and you need help, USB logs are often the most useful information you can share. They show what the display is doing while it starts, connects to WiFi, talks to Home Assistant, or loads the web interface.

You do not need to install developer tools. You can collect the logs from the same browser-based installer used to flash EspControl.

## What You Need

- A desktop or laptop running **Chrome or Edge**.
- A **USB-C data cable**. Charge-only cables will power the display but will not show logs.
- The display connected directly to your computer, or through a reliable USB hub.
- A GitHub account if you want to open or comment on an issue.

## Open the USB Log Viewer

1. Connect the display to your computer with the USB-C cable.
2. Open the [EspControl install page](/getting-started/install).
3. Choose your panel model.
4. Click **Install EspControl**.
5. When the browser asks for a serial port, choose the port that appeared when you connected the display.
6. If the installer gives you a choice, open **Logs & Console** instead of reinstalling the firmware.

::: warning Do not erase or reinstall unless you mean to
For collecting logs, you only need the serial log view. Do not choose erase, reset, or install unless you are intentionally re-flashing the display.
:::

If the browser cannot see the display, try another USB-C cable or USB port. On Windows, you may also need to allow the driver install when prompted.

## Capture Useful Logs

After the log window opens, restart the display so the log includes a full startup:

1. Leave the log window open.
2. Unplug and reconnect the display, or press the display's reset button if it has one.
3. Wait until the problem happens again.
4. Copy the log text from the log window.

For startup, WiFi, loading screen, and Home Assistant connection problems, try to capture at least the first **60 to 90 seconds** after the display restarts.

## Check Before Sharing

Before you paste logs into GitHub, quickly scan them for private information. Remove or replace anything you do not want public, such as:

- WiFi names if you consider them private.
- Home Assistant URLs, tokens, or long keys.
- Local IP addresses if you do not want to share your network range.

Please keep the surrounding log lines if possible. Replacing a value with something like `[removed]` is better than deleting whole sections.

## Share the Logs in GitHub

1. Open the [EspControl issues page](https://github.com/jtenniswood/espcontrol/issues).
2. Search for an existing issue that matches your problem.
3. If one exists, add a comment. If not, open a new issue.
4. Include the display model, what you expected to happen, what actually happened, and the USB log.

When pasting logs, wrap them in triple backticks so they stay readable:

````md
```text
Paste the USB log here
```
````

If the log is very long, attach it as a `.txt` file instead.

## What to Include

A useful issue usually includes:

- The display model, for example `JC1060P470`, `JC4880P443`, `JC8012P4A1`, `4848S040`, or `ESP32-P4 86 Panel`.
- Whether the problem happens after a fresh flash, after a firmware update, or after changing settings.
- The firmware version if you can see it.
- A short description of the problem.
- The USB log from startup through the problem happening.

Related: [Troubleshooting](/getting-started/troubleshooting)
