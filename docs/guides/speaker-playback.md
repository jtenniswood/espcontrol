---
title: Speaker Playback
description: Use supported ESP32-P4 displays as Home Assistant media players.
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

Music Assistant audio can still be selected through Home Assistant's media
browser when its integration is installed. The panel does not connect to
Music Assistant as a separate player.

Connect a compatible passive speaker to the panel's speaker connector. The P4
audio output uses the onboard ES8311 codec and amplifier; no external DAC is
needed.

Speaker output is included by default only on the ESP32-P4-86. The JC8012P4A1
10.1-inch panel can enable onboard speaker output in a manual ESPHome build.
Other panel profiles do not include local audio output.

### JC8012P4A1 manual builds

The JC8012P4A1 V1, V2, and V3 panels can expose the onboard speaker as a Home
Assistant media player. In the `substitutions` section of the matching manual
ESPHome configuration, set:

```yaml
enable_speaker_output: "true"
```

The output uses the onboard ES8311 codec and NS4150B amplifier. Audio is
configured as mono at 16 kHz with bounded buffers to limit memory use; this is
best suited to speech and basic playback. The amplifier is enabled only during
playback. The option adds speaker playback only: it does not enable the
microphone, voice assistant, or Sendspin.

This flag is available only through manual ESPHome builds. Browser-installed
firmware keeps the audio components excluded.

Audio uses ESPHome's speaker media player, which requires ESP-IDF and Home
Assistant 2024.10 or newer for Home Assistant media transcoding.
