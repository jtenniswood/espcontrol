---
title: EspControl FAQ
description: Community questions about EspControl setup, supported screens, Sonos and media, cameras, updates, and troubleshooting.
outline: [2, 3]
---

# FAQ

Answers to questions raised in the EspControl Reddit discussions, with recurring questions and essential setup decisions first, followed by more detailed topics. Repeated questions are combined, and answers reflect the current documentation rather than limitations in older releases.

Start with [Install](/getting-started/install) for a new panel or [Troubleshooting](/getting-started/troubleshooting) for a setup problem.

## Most asked and essential questions {#getting-started}

### What Is EspControl, and Do I Need to Write Code?

EspControl turns a supported ESP32 touchscreen into a dedicated Home Assistant controller. Install with the [browser installer](/getting-started/install), connect it to your network and Home Assistant, then open the panel's web page to choose cards and entities. Normal setup does not require writing YAML or building a user interface.

### Can I Use This Without Home Assistant?

Home Assistant is required for the smart-home controls, entity readings, and media features described here. Add your lights, speakers, thermostats, and other devices to Home Assistant first, then select their entities in EspControl. You do not need ESPHome Device Builder for the normal browser installation.

### Which Panels Are Supported?

Use the [installer's supported-screen list](/getting-started/install) and the matching screen guide. Supported families are the Guition **4848S040** (4-inch S3), **JC4880P443** (4.3-inch P4), **JC1060P470** (7-inch P4, original and V2), **JC8012P4A1** (10.1-inch P4, original, V2, and V3), and the **ESP32-P4 86 Panel**. Exact hardware revisions matter; another screen with the same size or processor is not automatically compatible.

### Should I Choose the S3 or a P4 Screen?

The [4-inch S3](/screens/4848s040) is the lower-cost starting point and supports media controls, speaker groups, and camera snapshots. P4 panels offer more capacity for image cards and a choice of screen sizes. The [4-inch P4 86 Panel](/screens/p4-86) also has a sharper 720×720 display and voice support. Compare the model guides and current seller listings; prices quoted in old posts are not fixed prices.

### Can I Display My Existing Home Assistant Dashboard Cards?

No. The panel uses its own touch interface rather than a browser running a Home Assistant dashboard. You configure EspControl cards that use your Home Assistant entities; Lovelace and custom dashboard cards cannot be imported. See [Setup](/features/setup).

### Can It Control Spotify, Sonos, Apple TV, Plex, or Music Assistant?

EspControl controls the `media_player` entities already available in Home Assistant. Add the relevant integration first, then select its entity in a [Media card](/card-types/media). Playback, volume, seeking, artwork, and other controls depend on what that integration exposes. It does not connect a speaker to Home Assistant for you.

### Does the S3 Now Support Cameras, and Is It Live Video?

Yes, current S3 firmware supports Camera cards. These show still snapshots from `camera.*` or `image.*` entities and refresh on relevant Home Assistant updates. They do not play a live video stream or guarantee a fixed frame rate. Tapping opens a larger image. Older comments saying cameras require a P4 predate S3 support. See [Cameras](/card-types/cameras).

### Why Do Track Details Work but the Album Image Is Missing?

Check that the selected Home Assistant entity provides artwork, then check the HTTP address the panel uses under **Settings > System > Home Assistant Settings**. The native ESPHome connection can work while image downloads fail. Use a reachable protocol/port or **Artwork Base URL** for a reverse proxy, update firmware, and retry. Spotify, radio, Plex, and Apple TV can expose different artwork URLs; capture [USB logs](/reference/collect-usb-logs) if only certain sources fail. See [Media Cover Art](/features/media-cover-art).

### Are There Stands or Wall Mounts, Including a 7-inch Mount?

Published files are listed under [Printable Stands and Mounts](/reference/3d-printable-stands). Match the exact model, rear board, and cable clearance. That list currently includes a 7-inch desk stand; a wall mount mentioned as a prototype in an older comment is not necessarily published. If you do not have a printer, the files can be used with a printing service, but check the model's dimensions first.

### How Do I Open the New Controls or Change Their Tab Order?

For Lights, Covers, Fans, or Media, select **All Controls** in the card's type or interaction settings. Climate cards open their controls when tapped. Use **Visible Tabs** where offered to reorder or hide controls; unsupported entity features remain hidden. If the option is absent, update firmware and reload the setup page. See [Lights](/card-types/lights), [Covers](/card-types/covers), [Fans](/card-types/fans), and [Media](/card-types/media).

## Music, speakers, and cover art

### Can I Group Speakers on the S3, and Can I Mix Sonos with WiiM?

Speaker grouping is available on the S3 as well as P4 panels. It needs compatible Home Assistant players, actions permission, and the discovery sensor described in [Speaker Groups](/features/speaker-groups). Test joining the players in Home Assistant first. Music Assistant can provide another integration route, but EspControl cannot make incompatible Sonos, WiiM, or other players group together.

### Can I Make a Dedicated Album-art Screen or Migrate from the Older Media Controller?

Yes. Use a large Cover Art card, or enable the automatic [Media Cover Art](/features/media-cover-art) screen. On a square 3×3 panel, a 3×3 Cover Art card with the clock bar hidden fills the grid. Follow the [media-controller migration guide](/getting-started/migrate-esphome-media-player). Speaker grouping is now available, although early migration replies described it as unfinished.

### How Do I Get the Speaker and Playback Tabs Shown in the Posts?

Use **Media > All Controls**, or tap a **Cover Art** card. Tabs appear according to the selected player's capabilities; speaker controls also need [speaker discovery setup](/features/speaker-groups). A **Speaker Group** card opens that screen directly.

### Can a Card Start a Playlist, Album, or Radio Station?

Choose **Media > Track, Album or Playlist**, select the player, and provide the content ID or URI accepted by its Home Assistant integration. You can also trigger a Home Assistant script with an Action card for more complex playback. This is a saved shortcut rather than a full music-library browser. See [Media](/card-types/media).

### Which Displays Support Voice or Play Audio Themselves?

[Voice Control](/features/voice-control) is supported on the **4-inch ESP32-P4 86 Panel**, using its microphones and speaker with Home Assistant Assist. Its audio-capable firmware can also play media. Other panels' Media cards control external Home Assistant players; a Media card does not imply that the panel itself has a speaker.

## Cameras and photos

### Why Does a Camera Change from Loading to Unavailable?

Check that the entity has an image in Home Assistant and that the panel can reach its image-download address. In **Settings > System > Home Assistant Settings**, try **Manual** with the reachable protocol and port if automatic discovery selects an unsuitable endpoint. A reverse proxy may need an explicit base URL. Also check the shared image-card limit. See [Setup](/features/setup#device-settings) and [Cameras](/card-types/cameras).

### Can I Show a Doorbell Snapshot When Someone Rings?

Use a Camera card for the doorbell's Home Assistant camera or image entity. A Home Assistant automation can wake the screen with **Screen: Wake**. On P4 panels, the documented **refresh_camera_cards** action refreshes Camera cards on the visible page, subject to its refresh guard. Do not assume that action is available on S3 or that a snapshot card automatically opens on a ring. See [camera refresh](/card-types/cameras#refreshing-cards-from-home-assistant) and [screen wake](/features/screensaver#wake-from-home-assistant).

### How Many Camera and Cover Art Cards Can I Use?

P4 panels have **six shared image slots**. The S3 has **two**, allowing a Camera card alongside a Media Cover Art card. Limits apply across the main page and all subpages combined; moving an image card into a folder does not free a slot. See [camera limits](/card-types/cameras#practical-limits).

### Can I Use Immich or Turn It into a Photo Frame?

A Camera card can display a Home Assistant `image` entity, so a photo integration can supply changing pictures. The [EspControl Immich integration](https://github.com/jtenniswood/espcontrol-immich) was announced for beta testing in the S3-camera discussions. Follow that integration's own setup and status; photo selection happens through the integration, rather than by entering an Immich API key into a Camera card.

## Updates, names, and multiple panels

### How Do I Update the Firmware?

If **Auto Update** is turned on (the default), the panel checks for and installs new versions automatically. You don't need to do anything.

To update manually:

1. Open the panel's web page.
2. Go to the **Settings** tab in the [Setup](/features/setup).
3. Under **Firmware**, press **Check for Update**.
4. If a new version is available, the panel will download and install it.

Advanced Ethernet-only builds may have these built-in update controls disabled. Update those displays through ESPHome OTA or USB instead.

See [Firmware Updates](/features/firmware-updates) for more details.

### Will Automatic Updates Keep My Custom ESPHome YAML?

Built-in updates install the project's released firmware; they do not rebuild your personal YAML. For custom components, pins, or other compiled changes, turn off the panel's built-in **Auto Update** and continue updating through [ESPHome](/getting-started/manual-esphome-setup). Saved cards and supported runtime settings are separate from custom compiled firmware.

### Why Did Home Assistant Say Transport Encryption Was Disabled After an Update?

Older mixed installation paths could replace a custom encrypted build with stock firmware that lacked the personal key. Current [manual setup](/getting-started/manual-esphome-setup) uses a device-stored key and documents how compatible OTA updates preserve it. Keep the recommended encryption package when rebuilding, and manage custom firmware consistently through ESPHome. A full erase or complete reset can require pairing again; do not treat an unexpected encryption warning as something to dismiss automatically.

### Can I Rename a Panel Without Compiling Firmware?

Yes. Open **Settings > System > Device Name**, check the address preview, and choose **Save & Restart**. Current firmware retains the name through normal OTA updates. Entity IDs stay unchanged, but custom ESPHome action names include the hostname, so update automations using those actions. See [Naming Your Panel](/features/setup#naming-your-panel).

### Can I Back Up My Setup?

Yes. In the [Setup](/features/setup) **Settings** tab, under **Backup**, you can **Export** your entire setup (cards, subpages, colours, and display settings) as a file. To restore it later, use **Import** to load the saved file. You can also use this to copy your setup to a different panel — the import will rearrange cards automatically if the panels are different sizes. See [Backup](/features/backup) for details.

### Can I Copy Just a Few Cards to Another Panel?

Use **Copy Code** and **Paste Code** in the setup page to transfer cards and their attached subpages without replacing all the destination's settings. Use Backup Export/Import for a whole setup. Each panel still has its own web page and identity. See [Setup](/features/setup) and [Backup](/features/backup).

### Can I Restart from Home Assistant and Update Without Reconnecting USB?

Yes. Current firmware exposes a **Restart** button in Home Assistant; custom YAML is no longer needed just for that button. Once a panel is online, later manual ESPHome updates can usually use OTA. USB is still needed for initial flashing, recovery, or a documented change of firmware/networking type. See [Restart From Home Assistant](/features/setup#restart-from-home-assistant) and [Manual Setup](/getting-started/manual-esphome-setup).

### How Do I Reset the Device?

Open **Settings > System > Factory Reset** and save a backup first. **Partial reset** clears cards and preferences while keeping WiFi and the Home Assistant encryption key. **Complete reset** also removes those saved credentials and returns the display to first-time setup. Both keep the installed firmware; settings compiled into custom firmware remain. A normal firmware reflash is not a guaranteed reset. See [Reset the display](/features/backup#reset-the-display).

### Can I Stay on Releases While Managing the Panel in ESPHome?

The simplest release-update route is the browser-installed firmware and its built-in updater. Manual ESPHome builds use the sources selected by your package configuration; rebuilding is not the same as installing a published release binary. For a release-specific custom build, use matching tagged sources throughout your configuration and dependencies. See [Manual Setup](/getting-started/manual-esphome-setup).

## Troubleshooting and privacy

### Why Do Cards Show State but Tapping Does Nothing?

Enable **Allow the device to perform Home Assistant actions** for the panel in the ESPHome integration. Verify that the action also works directly in Home Assistant. If a garage-door confirmation still prevents an action, report the entity, card settings, firmware version, and reproduction steps rather than assuming the old report has the same cause. See [Enable Actions](/getting-started/home-assistant-actions).

### The Web Page Looks Broken or Unstyled

The setup page loads web resources from the internet. Check that the browser can reach those resources, then force-refresh the page or try a private window. Update the panel firmware if new card types or **All Controls** options are missing. See [Troubleshooting](/getting-started/troubleshooting).

### My Device Won't Connect to WiFi

- Make sure you're connecting to a **2.4 GHz** network. The panel does not support 5 GHz WiFi.
- Double-check your **WiFi password** — it's easy to mistype on a small screen.
- Move the panel **closer to your router** during initial setup. You can move it to its final location afterwards.
- If the panel previously connected but can't anymore (e.g. you changed your WiFi password), it will first try to reconnect. If that does not work, it will create a hotspot so you can enter the new details. Look for a network called **ESP_xxxxxx**; it can take up to **90 seconds** to appear.
- If you installed an advanced Ethernet-only build, WiFi setup is intentionally disabled. Check the Ethernet cable, switch port, and DHCP/router lease list instead.

### How Do I Find My Device's IP Address?

Tap the connectivity icon in the panel's clock bar to see its IP address and device name. An unconfigured display also shows its address. Alternatively, check your router's connected-device list or the device under **Home Assistant > Settings > Devices & services > ESPHome**.

### The Display Is Stuck on the Loading Screen

- Give it up to **60 seconds** on first boot. It needs time to connect to WiFi and download resources.
- If the display shows a WiFi reconnecting message, wait a little longer. Short WiFi outages can recover by themselves before setup mode starts.
- If it stays on the loading screen, **power-cycle** the panel (unplug and re-plug the USB-C cable).
- If the WiFi hotspot appears after restarting, the panel couldn't connect to your network — go through the [WiFi setup](/getting-started/install#connect-to-wifi) again.

### What Should I Do about Stripes, Haze, or a Halo around the Screen?

Check the correct firmware for the exact panel revision and try a known-good power supply and cable that meet its requirements. A comment reporting improvement with another USB supply does not establish the cause of every display fault. If it persists, include photos, the power setup, model, and firmware in a report; a panel defect may need the seller's help.

### Where Should I Report a Bug or Request a Feature?

Use [GitHub issues](https://github.com/jtenniswood/espcontrol/issues). Include the exact display/revision, firmware version, installation method, steps to reproduce, and relevant Home Assistant entity attributes. Photos help with layout problems; [USB logs](/reference/collect-usb-logs) help with crashes or image downloads. Remove credentials and private details before posting. See [Contributing](/reference/contributing).

### How Is My Data Handled?

Smart-home control normally runs between the panel and Home Assistant on your local network. EspControl does not run a central service collecting your smart-home data. Firmware updates, web assets, network time, artwork URLs, webhooks, and any cloud services chosen in Home Assistant can involve external connections. A local remote does not make Spotify or another cloud-backed integration offline. See [Privacy](/reference/privacy).

## Cards and everyday controls

### What Card Types Are Available?

The [card catalogue](/card-types/) covers lights, switches, climate, fans, covers, locks, alarms, media, cameras, sensors, weather, actions, and more. Choose a simple one-tap card or a supported **All Controls** view when you want several controls behind one card.

### How Many Cards Can I Have?

The standard home grids have **9 slots** on the 4-inch panels, **6** on the 4.3-inch, **15** on the 7-inch, and **20** on the 10.1-inch panels. Larger cards occupy several slots. [Subpages](/features/subpages) add more pages; each reserves one slot for Back. Camera and Cover Art cards also have separate [shared image limits](#how-many-camera-and-cover-art-cards-can-i-use).

### What Is a Subpage?

Subpages are like folders for your cards. Set a home-screen card to the **Subpage** type and it becomes a folder. Tapping it on the panel opens a new page with its own set of cards. This is great for grouping controls by room or device type without filling up the home screen. See [Subpage](/features/subpages).

### Can the Screen Sleep, Wake on Touch, or Wake When Someone Arrives?

Yes. Configure a timer or Home Assistant presence sensor in [Screensaver](/features/screensaver), then choose dimming, a clock, or display off. Touch wakes it, and Home Assistant also exposes a **Screen: Wake** button. Built-in camera face detection is not a supported presence method; use a Home Assistant presence sensor.

### Can It Control Mini-split Fan Speed, Swing, and Heating/Cooling Targets?

A [Climate card](/card-types/climate) exposes temperature, HVAC mode, fan, swing, and preset options when the Home Assistant entity supports them. Range thermostats now show separate low/heating and high/cooling targets; older reports of only one target do not describe the current controls. If an option is missing, compare the entity's attributes in Home Assistant and include them in a bug report.

### Why Does My Weather Card Only Show Cloudy Instead of a Temperature?

**Current Conditions** shows the weather state. Current firmware also offers **Temperatures Today** and **Temperatures Tomorrow** for forecast highs and lows; these need Home Assistant actions permission. For a current measured temperature, use a Sensor card with a temperature sensor entity. If only the old option appears, update firmware and reload the browser. See [Weather](/card-types/weather).

### Can I Choose a Different Background Colour for Every Card?

The current interface uses a shared **Primary** colour for active cards and fixed colours for inactive and information cards. It does not offer arbitrary per-card background colours. Change the accent in [Appearance](/features/appearance).

### Can I Send Blinds to 25, 50, or 75 Percent?

Yes. **Cover > All Controls > Presets** offers 0, 25, 50, 75, and 100 percent when the entity supports position control. A **Set Position** card provides a direct shortcut. Home Assistant uses 0 for closed and 100 for open; the card's fill represents how much is closed. See [Covers](/card-types/covers).

### Is There a Full Fan Control View?

Yes. **Fans > All Controls** combines supported power, speed, preset, oscillation, and direction controls. You can also attach a separate light entity for an on/off Light tab. See [Fans](/card-types/fans).

### Can I Select WLED Presets?

Yes, when the WLED integration exposes them as a `select` entity. Add an **Action** card, choose **Option Select**, and select the preset entity, such as `select.wled_preset`. Use a Light card for the capabilities exposed by its `light` entity. See [Option Select](/card-types/option-select).

### Can One Sensor Card Show Temperature and Humidity Together?

A Sensor card currently uses one source entity. Use separate cards, group them in a subpage, or expose a combined text reading from Home Assistant and display it with a Text sensor card. A dedicated multi-sensor tile was requested in the discussions; it is not a current Sensor card mode. See [Sensors](/card-types/sensors).

### Why Do Light Controls Use Colour Presets Instead of a Colour Wheel?

The light popup uses touch-friendly colour choices. Its available tabs depend on the light's capabilities in Home Assistant, and **Visible Tabs** can put the controls you use most first. See [Lights](/card-types/lights).

### Can the Clock Show Seconds?

There is no seconds option for the normal clock bar or Date & Time card. The clock bar updates by the minute. See [Time Settings](/features/clock) for timezone and 12/24-hour options.

### What If the Icon I Need Isn't Listed?

The panel includes hundreds of icons from the Material Design Icons set. If the one you need isn't there, [open an issue on GitHub](https://github.com/jtenniswood/espcontrol/issues) with the icon name (from [pictogrammers.com/library/mdi](https://pictogrammers.com/library/mdi/)) and what you'd use it for. We'll look into adding it.

## Screens, power, and mounting

### Which 10-inch Revision or 4-inch P4 Variant Should I Buy?

Check the [10.1-inch guide](/screens/jc8012p4a1) before selecting original or V2 firmware, and use the [V3 guide](/screens/jc8012p4a1-v3) when chip information confirms ESP32-P4 v3.x silicon. Seller names alone can be ambiguous. The current [P4 86 guide](/screens/p4-86) targets **ESP32-P4-86-Panel-ETH-2RO**; do not assume a camera-port variant or a different rear board has the same support or fits the same case.

### Can I Use a Cheap Yellow Display or Another ESP32 Screen?

Only the exact supported models have ready-to-install EspControl firmware. The common Cheap Yellow Display is not on that list. A matching processor or resolution is not enough: display drivers, touch, memory, and wiring also matter. For another board, share its exact model and hardware details in a [GitHub request](https://github.com/jtenniswood/espcontrol/issues); community work needs testing on that hardware.

### Will It Fit a Standard Wall Box or Replace a Light Switch?

Check the panel's dimensions and mounting holes against your actual wall box; regional boxes such as Italian 503, North American single-gang, and 86-type boxes are not interchangeable. Some supported variants have [relays](/features/relays), but the screen still needs the correct power supply and room for its rear hardware. Use the manufacturer's wiring instructions and a qualified installer for mains wiring; a neutral may be required depending on the supply.

### Can I Power a Panel over PoE?

An Ethernet port alone does not establish PoE support. Check the exact panel and rear-board specifications. A PoE splitter can supply a panel through its supported power input only when its output voltage, current, connector, and polarity match that panel. See the relevant screen guide before choosing power hardware; do not copy another model's wiring from a comment.

### Can the Panel Also Be a Bluetooth Proxy?

Bluetooth proxy is not part of the current standard panel setup. The screens need their available memory for the interface and image features. The documented Ethernet-only builds also omit Bluetooth proxy and turn off the WiFi/Bluetooth co-processor; changing to Ethernet does not enable it. Use a separate proxy for that role.

### Can It Run on a Battery and Show the Charge Level?

Battery power and battery measurement are separate hardware features. Current [Battery Status](/features/battery) support is optional and limited to compatible 10.1-inch JC8012P4A1 battery hardware; readings are estimates and may need verification. It does not detect charging. The S3 battery experiments in the comments do not establish supported charge reporting or a guaranteed runtime.

### Can I Use a 10-inch Display in Portrait Mode?

Use **Settings > Display > Rotation**. Available rotations depend on the panel and firmware; most supported panels offer 0, 90, 180, and 270 degrees. Check the preview and available card space after rotating. See [Rotation](/features/rotation).

### Does It Have a Room Temperature Sensor?

Do not assume a panel includes a usable ambient temperature sensor. Select a Home Assistant temperature entity, or use a [Local Sensor](/card-types/local-sensors) if your hardware and firmware provide one. A sensor mounted near the processor or backlight can read warmer than the room.

## More about EspControl

### Does the Panel Work with Other Smart Home Platforms?

EspControl is built specifically for Home Assistant. It does not support other platforms like Google Home, Apple HomeKit, or SmartThings directly. However, if those platforms are integrated into your Home Assistant setup, the panel can control devices that are exposed through Home Assistant.

### Where Is the Source Code, and How Can I Help?

The source is on [GitHub](https://github.com/jtenniswood/espcontrol). See [Contributing](/reference/contributing) for feedback and contributions and the repository licence before reusing it. The documentation's support button offers a way to support development.

## Community discussions

The questions above combine the accessible product questions, feature requests, and troubleshooting reports from these discussions. Answers link to the current setup guides; the original replies may describe older firmware.

| Topic | Original discussions |
|---|---|
| S3 camera support | [r/homeassistant](https://www.reddit.com/r/homeassistant/comments/1wkgcnz/espcontrol_added_camera_support_on_the_20_s3/) · [r/Esphome](https://www.reddit.com/r/Esphome/comments/1wkgaz5/espcontrol_added_camera_support_on_the_20_s3/) |
| Device management and WiFi sharing | [r/Esphome](https://www.reddit.com/r/Esphome/comments/1we9zdn/espcontrol_v29_device_management_wifi_sharing/) |
| Speaker groups | [r/sonos](https://www.reddit.com/r/sonos/comments/1viu8wu/sonos_smart_home_control_now_with_multi_speaker/) · [r/espcontrol](https://www.reddit.com/r/espcontrol/comments/1virqf0/multi_speaker_control_added/) · [r/homeassistant](https://www.reddit.com/r/homeassistant/comments/1virpxa/multi_speaker_support_added_to_espcontrol/) · [r/Esphome](https://www.reddit.com/r/Esphome/comments/1virp30/multi_speaker_support_added_to_espcontrol/) |
| Cover art and playlists | [r/homeassistant](https://www.reddit.com/r/homeassistant/comments/1uzsivk/added_cover_art_cards_and_playlists_to_espcontrol/) · [r/Esphome](https://www.reddit.com/r/Esphome/comments/1uzob1p/added_media_cover_art_cards_to_espcontrol/) |
| Media and climate controls | [r/homeassistant](https://www.reddit.com/r/homeassistant/comments/1uti96w/full_media_controls_added_to_espcontrol/) · [r/Esphome](https://www.reddit.com/r/Esphome/comments/1uteclt/media_and_climate_modals_added_to_espcontrol/) |
| Lights and blinds controls | [r/Esphome](https://www.reddit.com/r/Esphome/comments/1ugyd7v/custom_modal_for_lights_and_blinds_in_espcontrol/) · [r/homeassistant](https://www.reddit.com/r/homeassistant/comments/1ugy6nu/espcontrol_improvements_custom_controls_for/) |

Coverage note: Reddit did not make the [r/espcontrol S3-camera discussion](https://www.reddit.com/r/espcontrol/comments/1wknnmf/adding_support_for_camera_cards_to_the_s3_display/) or the [r/homeassistant device-management discussion](https://www.reddit.com/r/homeassistant/comments/1we9xqd/espcontrol_updates_device_names_wifi_sharing_and/) accessible during compilation. Hidden, removed, or subsequently added comments are not guaranteed to be covered.
