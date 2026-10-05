---
title: Speaker Playback
description: Use the 4-inch ESP32-P4-86 panel as a Home Assistant media player.
---

# Speaker Playback

The 4-inch ESP32-P4-86 panel exposes an ESPHome media player named after the
panel with “Speaker” appended. Home Assistant can send announcements and media to it,
control playback, and adjust its volume. The board amplifier turns on during
playback and switches off when playback ends.

Use **Browse media** on the speaker entity in Home Assistant to choose audio
from the Home Assistant media browser, including sources provided by installed
integrations such as Music Assistant. Home Assistant resolves the selected
item and sends its audio stream to the speaker; the device does not need a
separate account or connection to each music service.

The speaker also connects to Music Assistant as a native Sendspin player. It is
discovered automatically when Music Assistant and the panel are on the same
network; no separate Music Assistant player provider setup is required.

Connect a compatible passive speaker to the panel's speaker connector. The P4
audio output uses the onboard ES8311 codec and amplifier; no external DAC is
needed.

Speaker output and Sendspin are included only on the ESP32-P4-86. The 7-inch,
10.1-inch, 4.3-inch P4, and 4-inch S3 profiles do not include local audio output.

Audio uses ESPHome's speaker media player, which requires ESP-IDF and Home
Assistant 2024.10 or newer for Home Assistant media transcoding.
