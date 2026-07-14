# Seeed reTerminal D1001 (8")

8-inch 1280x800 landscape touchscreen panel that runs EspControl firmware for Home Assistant. A fixed 5x4 grid of 20 configurable buttons lets you control lights, switches, fans, and other smart home devices with a single tap. The display also shows a live clock, indoor/outdoor temperature, and includes a screensaver with adjustable brightness.

After the initial install, everything is configured through the built-in web page, so no coding or file editing is required.

## Quick links

- **Full documentation:** [jtenniswood.github.io/espcontrol](https://jtenniswood.github.io/espcontrol/)
- **Install guide:** [jtenniswood.github.io/espcontrol/install](https://jtenniswood.github.io/espcontrol/install)
- **Web UI guide:** [jtenniswood.github.io/espcontrol/web-ui](https://jtenniswood.github.io/espcontrol/web-ui)

## Features

- **20 buttons** (5x4 grid) - control any Home Assistant device
- **Drag-and-drop ordering** - rearrange buttons from your browser
- **Automatic icons** - or choose from hundreds of icons manually
- **Custom labels** - name buttons however you like
- **Physical button** exposed to Home Assistant for your own automations
- **Indoor and outdoor temperature** in the top bar
- **Live clock** synced from NTP, with Home Assistant as a fallback
- **Screensaver** with adjustable idle timeout and optional presence sensor to wake
- **Day/night brightness** - adjusts automatically based on sunrise and sunset
- **Over-the-air updates** - automatic or manual
- **WiFi setup** - on-screen guide if the network is unavailable

## Hardware notes

The reTerminal D1001 pairs an **ESP32-P4** application processor with an
**ESP32-C6** networking co-processor (2.4 GHz WiFi). The 8" 800x1280 MIPI-DSI
panel is driven through a JD9365 controller, with display power, reset, and
backlight enable routed through an on-board XL9535/PCA9535 I2C GPIO expander.
Touch is a GSL3670 capacitive controller.

The GSL3670 touch driver (GPLv2, Silead) is vendored in `components/gsl3670`
with its own `LICENSE.md`, the same in-tree pattern used for the 10.1" panel's
`gsl3680` driver.
