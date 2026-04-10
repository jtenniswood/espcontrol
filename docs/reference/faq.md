---
title: FAQ
description:
  Frequently asked questions about the Espcontrol touchscreen panel — WiFi, setup, updates, troubleshooting, and more.
---

# FAQ

## How do I find my device's IP address?

There are several ways:

- **On the display itself** — when no buttons are configured yet, the panel shows its address on screen.
- **In your router** — look at the connected devices list (usually at `192.168.1.1` or similar). The panel will appear with its hostname.
- **In Home Assistant** — go to **Settings > Devices & Services > ESPHome**, click on the device, and look for the IP address.

## The web page looks broken or unstyled

The panel's built-in web page loads some visual resources from the internet. If the page looks plain or broken:

- Make sure your panel has a working **internet connection**, not just local network access. Some routers block IoT devices from reaching the internet.
- Try **clearing your browser cache** and reloading the page.
- Try a different browser (Chrome or Edge recommended).

## My device won't connect to WiFi

- Make sure you're connecting to a **2.4 GHz** network. The panel does not support 5 GHz WiFi.
- Double-check your **WiFi password** — it's easy to mistype on a small screen.
- Move the panel **closer to your router** during initial setup. You can move it to its final location afterwards.
- If the panel previously connected but can't anymore (e.g. you changed your WiFi password), it will create a hotspot so you can enter the new details. Look for a network called **espcontrol-xxxxxx**.

## How do I reset the device?

To start completely fresh, re-flash the firmware using the [install guide](/getting-started/install). Connect the panel to your computer with a USB-C cable and use the web installer. This will reset WiFi settings and the panel will create its setup hotspot again.

Your button configuration is stored separately and will be preserved unless you change it through the web page.

## Can I use this without Home Assistant?

No. The panel is designed to work with Home Assistant. It needs Home Assistant for:

- Controlling your smart home devices (lights, switches, fans, etc.)
- Keeping the clock accurate
- Temperature readings
- Motion sensor data for the screensaver

Without Home Assistant, the panel would have no devices to control and no data to display.

## How do I update the firmware?

If **Auto Update** is turned on (the default), the panel checks for and installs new versions automatically. You don't need to do anything.

To update manually:

1. Open the panel's web page.
2. Go to the **Settings** tab in the [Setup](/features/setup).
3. Under **Firmware**, press **Check for Update**.
4. If a new version is available, the panel will download and install it.

See [Firmware Updates](/features/firmware-updates) for more details.

## What if the icon I need isn't listed?

The panel includes hundreds of icons from the Material Design Icons set. If the one you need isn't there, [open an issue on GitHub](https://github.com/jtenniswood/espcontrol/issues) with the icon name (from [pictogrammers.com/library/mdi](https://pictogrammers.com/library/mdi/)) and what you'd use it for. We'll look into adding it.

## How many buttons can I have?

The home screen has a grid of buttons sized to fill the screen:

- **7-inch JC1060P470** — 20 buttons (4 rows, 5 columns)
- **4.3-inch JC4880P443** — 15 buttons (5 rows, 3 columns)
- **4-inch 4848S040** — 9 buttons (3 rows, 3 columns)

You can have even more using **subpages**. Any button can be turned into a folder that opens a new page of buttons. So you could have 20 buttons on the home screen, and each one could open a subpage with up to 19 more. See [Setup — Subpage buttons](/features/setup#button-types) for details.

## What are subpages?

Subpages are like folders for your buttons. Set any button to the **Subpage** type and it becomes a folder — tapping it on the panel opens a new page with its own set of buttons. This is great for grouping controls by room or device type without filling up the home screen. Each subpage has its own buttons, icons, and labels, set up the same way as the home screen. See [Setup — Subpage buttons](/features/setup#button-types).

## Can I back up my setup?

Yes. In the [Setup](/features/setup) **Settings** tab, under **Backup**, you can **Export** your entire setup (buttons, subpages, colours, and display settings) as a file. To restore it later, use **Import** to load the saved file. You can also use this to copy your setup to a different panel — the import will rearrange buttons automatically if the panels are different sizes. See [Backup](/features/backup) for details.

## Which panels are supported?

Espcontrol currently supports three Guition touchscreen panels:

- **JC1060P470** — 7-inch, 1024x600, landscape orientation (ESP32-P4)
- **JC4880P443** — 4.3-inch, 480x800, portrait orientation (ESP32-P4)
- **4848S040** — 4-inch, 480x480, square (ESP32-S3)

All use the same firmware features, button configuration, and web UI. The grid layout automatically matches each panel's screen size and orientation.

## Does the panel work with other smart home platforms?

Espcontrol is built specifically for Home Assistant. It does not support other platforms like Google Home, Apple HomeKit, or SmartThings directly. However, if those platforms are integrated into your Home Assistant setup, the panel can control devices that are exposed through Home Assistant.

## The display is stuck on the loading screen

- Give it up to **60 seconds** on first boot. It needs time to connect to WiFi and download resources.
- If it stays on the loading screen, **power-cycle** the panel (unplug and re-plug the USB-C cable).
- If the WiFi hotspot appears after restarting, the panel couldn't connect to your network — go through the [WiFi setup](/getting-started/install#connect-to-wifi) again.

## How is my data handled?

Everything stays on your local network. The panel communicates directly with your Home Assistant instance over your home WiFi. No data is sent to external servers, cloud services, or third parties. The only internet connection the panel makes is to check for firmware updates and to load the web page styling.
