---
title: EspControl FAQ
description:
  Frequently asked questions about the EspControl touchscreen panel — WiFi, setup, updates, troubleshooting, and more.
---

# FAQ

## How Do I Find My Device's IP Address?

There are several ways:

- **On the display itself** — when no cards are configured yet, the panel shows its address on screen.
- **In your router** — look at the connected devices list (usually at `192.168.1.1` or similar). The panel will appear with its hostname.
- **In Home Assistant** — go to **Settings > Devices & Services > ESPHome**, click on the device, and look for the IP address.

## The Web Page Looks Broken or Unstyled

The panel's built-in web page loads some visual resources from the internet. If the page looks plain or broken:

- Make sure your panel has a working **internet connection**, not just local network access. Some routers block IoT devices from reaching the internet.
- Try **clearing your browser cache** and reloading the page.
- Try a different browser (Chrome or Edge recommended).

## My Device Won't Connect to WiFi

- Make sure you're connecting to a **2.4 GHz** network. The panel does not support 5 GHz WiFi.
- Double-check your **WiFi password** — it's easy to mistype on a small screen.
- Move the panel **closer to your router** during initial setup. You can move it to its final location afterwards.
- If the panel previously connected but can't anymore (e.g. you changed your WiFi password), it will first try to reconnect. If that does not work, it will create a hotspot so you can enter the new details. Look for a network called **ESP_xxxxxx**; it can take up to **90 seconds** to appear.

## How Do I Reset the Device?

To start completely fresh, re-flash the firmware using the [install guide](/getting-started/install). Connect the panel to your computer with a USB-C cable and use the web installer. This will reset WiFi settings and the panel will create its setup hotspot again.

Your card configuration is stored separately and will be preserved unless you change it through the web page.

## Can I Use This Without Home Assistant?

No. The panel is designed to work with Home Assistant. It needs Home Assistant for:

- Controlling your smart home devices (lights, switches, fans, etc.)
- Keeping the clock accurate
- Temperature readings
- Motion sensor data for the screensaver

Without Home Assistant, the panel would have no devices to control and no data to display.

## How Do I Update the Firmware?

If **Auto Update** is turned on (the default), the panel checks for and installs new versions automatically. You don't need to do anything.

To update manually:

1. Open the panel's web page.
2. Go to the **Settings** tab in the [Setup](/features/setup).
3. Under **Firmware**, press **Check for Update**.
4. If a new version is available, the panel will download and install it.

See [Firmware Updates](/features/firmware-updates) for more details.

## What If the Icon I Need Isn't Listed?

The panel includes hundreds of icons from the Material Design Icons set. If the one you need isn't there, [open an issue on GitHub](https://github.com/jtenniswood/espcontrol/issues) with the icon name (from [pictogrammers.com/library/mdi](https://pictogrammers.com/library/mdi/)) and what you'd use it for. We'll look into adding it.

## What Card Types Are Available?

The setup page currently includes Switch, Lights, Action, Trigger, Sensor, Slider, Cover, Garage Door, Lock, Date & Time, World Clock, Weather, Media, Climate, Internal Switches, and Subpage cards.

## How Many Cards Can I Have?

The home screen has a grid of card slots sized to fill the screen:

- **10.1-inch JC8012P4A1** — 20 cards (4 rows, 5 columns)
- **7-inch JC1060P470** — 15 cards (3 rows, 5 columns)
- **4.3-inch JC4880P443** — 6 cards (3 rows, 2 columns)
- **4-inch ESP32-P4 86 Panel** — 9 cards (3 rows, 3 columns)
- **4-inch 4848S040** — 9 cards (3 rows, 3 columns)

You can have even more using **Subpage** cards. Any home-screen card can be turned into a folder that opens a new page of cards. Each subpage has one fewer usable slot than the home screen because it includes a Back card. See [Subpage](/features/subpages) for details.

## What Is a Subpage?

Subpages are like folders for your cards. Set a home-screen card to the **Subpage** type and it becomes a folder. Tapping it on the panel opens a new page with its own set of cards. This is great for grouping controls by room or device type without filling up the home screen. See [Subpage](/features/subpages).

## Can I Back Up My Setup?

Yes. In the [Setup](/features/setup) **Settings** tab, under **Backup**, you can **Export** your entire setup (cards, subpages, colours, and display settings) as a file. To restore it later, use **Import** to load the saved file. You can also use this to copy your setup to a different panel — the import will rearrange cards automatically if the panels are different sizes. See [Backup](/features/backup) for details.

## Which Panels Are Supported?

EspControl currently supports these touchscreen panels:

- **JC8012P4A1** — 10.1-inch, 1280x800 landscape orientation (ESP32-P4)
- **JC1060P470** — 7-inch, 1024x600, landscape orientation (ESP32-P4)
- **JC4880P443** — 4.3-inch, 480x800, portrait orientation (ESP32-P4)
- **ESP32-P4 86 Panel** — 4-inch, 720x720, square (ESP32-P4)
- **4848S040** — 4-inch, 480x480, square (ESP32-S3)

All use the same firmware features, card configuration, and web UI. The grid layout automatically matches each panel's screen size and orientation.

## Does the Panel Work with Other Smart Home Platforms?

EspControl is built specifically for Home Assistant. It does not support other platforms like Google Home, Apple HomeKit, or SmartThings directly. However, if those platforms are integrated into your Home Assistant setup, the panel can control devices that are exposed through Home Assistant.

## The Display Is Stuck on the Loading Screen

- Give it up to **60 seconds** on first boot. It needs time to connect to WiFi and download resources.
- If the display shows a WiFi reconnecting message, wait a little longer. Short WiFi outages can recover by themselves before setup mode starts.
- If it stays on the loading screen, **power-cycle** the panel (unplug and re-plug the USB-C cable).
- If the WiFi hotspot appears after restarting, the panel couldn't connect to your network — go through the [WiFi setup](/getting-started/install#connect-to-wifi) again.

## How Is My Data Handled?

Everything stays on your local network. The panel communicates directly with your Home Assistant instance over your home WiFi. No data is sent to external servers, cloud services, or third parties. The only internet connection the panel makes is to check for firmware updates and to load the web page styling.
