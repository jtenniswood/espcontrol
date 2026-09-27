---
title: Speaker Playback
description: Use supported EspControl panels as Home Assistant media players.
---

# Speaker Playback

Supported panels expose an ESPHome media player named after the panel with
“Speaker” appended. Home Assistant can send announcements and media to it,
control playback, and adjust its volume. The board amplifier turns on during
playback and switches off when playback ends.

The speaker also connects to Music Assistant as a native Sendspin player. It is
discovered automatically when Music Assistant and the panel are on the same
network; no separate Music Assistant player provider setup is required.

Connect a compatible passive speaker to the panel's speaker connector. The P4
audio output uses the onboard ES8311 codec and amplifier; no external DAC is
needed.

## S3 4848S040

The 4848S040 uses its relay GPIOs for the onboard I2S amplifier, so this setup
keeps the relays available and uses an external I2S amplifier instead. Connect
the external amplifier's BCLK to GPIO41, LRCLK to GPIO42, DOUT to GPIO47, and
connect its power and ground as required by the amplifier. Connect a speaker
to the amplifier output.

The onboard amplifier path requires hardware changes and reassigning the relay
GPIOs. The default firmware does not make that change.

Audio uses ESPHome's speaker media player, which requires ESP-IDF and Home
Assistant 2024.10 or newer for Home Assistant media transcoding.
