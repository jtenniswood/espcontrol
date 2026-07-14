---
title: ESP32-P4 86 Voice Control
description:
  How to set up and use Voice Assistant, wake words, timers, mute, volume, media playback, and troubleshooting on the ESP32-P4 86 Panel.
---

# Voice Control

Voice Control turns the ESP32-P4 86 Panel into a Home Assistant Assist satellite. You can speak to Home Assistant from the panel, hear responses through the built-in speaker, set timers, control smart home devices, and interrupt voice or media playback with barge-in.

Voice Control is available on the **4-inch ESP32-P4 86 Panel** firmware. It uses the panel's built-in microphones and speaker, and is designed to behave closely to the official Home Assistant Voice Preview Edition while staying integrated with EspControl's screen, clock bar, media controls, and cover-art display.

## What You Can Do

- Ask Home Assistant questions using a wake word.
- Choose the active wake word from Home Assistant.
- Set, update, cancel, and stop voice timers.
- Hear a wake sound when the device starts listening.
- Hear a repeating timer alert when a timer finishes.
- Say **stop** to dismiss a timer alert without first saying the wake word.
- Interrupt an active voice response with another wake word command.
- Use voice while media is playing on the device.
- Pause media during a voice turn, then resume it afterwards.
- Stop local media playback with common voice phrases.
- Opt in to Voice Services when you want the panel to listen for wake words.
- Mute and unmute wake-word response from Home Assistant or the device screen.
- Open the local device volume control from the clock bar or the physical BOOT/Key2 button on WiFi firmware.

## Requirements

Before using Voice Control, make sure:

- The ESP32-P4 86 Panel is flashed with firmware that includes Voice Control.
- The panel has been added to Home Assistant through the ESPHome integration.
- Home Assistant Assist is configured with a pipeline, speech-to-text, text-to-speech, and a conversation agent.
- The panel is allowed to perform Home Assistant actions. See [Enable Actions](/getting-started/home-assistant-actions).

Voice Control depends on Home Assistant. Wake-word detection runs on the device, but spoken commands are sent to Home Assistant for speech recognition, intent handling, and voice responses.

## First-Time Setup

After flashing and connecting the panel to WiFi, Home Assistant should discover it as an ESPHome device.

1. Open **Home Assistant**.
2. Go to **Settings > Devices & services**.
3. Add the discovered ESPControl device from the **ESPHome** integration.
4. Follow the Home Assistant prompts to set up the voice satellite.
5. Choose the Assistant pipeline and voice options Home Assistant offers.
6. Finish the setup and wait for the device to settle for a short time.

Once setup is complete, the panel should appear in **Settings > Voice assistants > Assist devices**.

::: tip
If setup fails the first time, press **Retry**. In testing, an initial failure has usually been Home Assistant trying to configure the voice satellite before the device had fully finished starting. Retrying once the device is ready normally completes setup.
:::

## Upgrading an Existing Device

If the panel was already added to Home Assistant before Voice Control was added, Home Assistant may not immediately show the new voice satellite options.

Try reconfiguring the existing ESPHome device before deleting it:

1. Open **Settings > Devices & services**.
2. Open the **ESPHome** integration card. Use the ESPHome integration itself, not the numbered device count shortcut.
3. Find the ESP32-P4 86 Panel.
4. Click the three-dot menu beside the device.
5. Choose **Reconfigure**.
6. Run through the Voice Assistant setup again.

After reconfiguration, **Set up voice assistant** should work, the **Assistant**, **Assistant 2**, and voice options should appear where Home Assistant supports them, and the panel should be listed under **Settings > Voice assistants > Assist devices**.

If reconfigure still does not work, remove the device from Home Assistant and add it again. This is usually only needed when Home Assistant has kept old device metadata from a previous non-voice firmware.

## Turn On Voice Services

Voice Services is off by default. This makes voice an opt-in feature: the device will not listen for wake words until you enable it.

To enable it:

1. Open the device in Home Assistant.
2. Turn on **Voice Services**.
3. Wait a few seconds for the wake-word engine to start.

When **Voice Services** is off, wake-word listening is stopped and the clock-bar microphone shortcut is hidden. Other normal EspControl screen features continue to work.

## Wake Words

The panel can listen for one active wake word at a time.

Available wake words:

- **Alexa**
- **Okay Nabu**
- **Hey Jarvis**
- **Hey Mycroft**

To change the wake word:

1. Open the device in Home Assistant.
2. Find **Voice Wake Word**.
3. Choose the wake word you want.

The new wake word is applied immediately. A reboot should not be needed.

## Wake Word Sensitivity

The device exposes **Wake word sensitivity** in Home Assistant. Use this if the panel hears the wake word too easily or not easily enough.

- **Slightly sensitive** - best if the device triggers too often.
- **Moderately sensitive** - a balanced option.
- **Very sensitive** - best if the device misses wake words.

Start with the default and only adjust it if you notice false detections or missed wake words in your room.

## Wake Sound

The **Wake sound** switch controls whether the panel plays a short sound when it starts listening.

- **On** - the panel plays a wake sound after the wake word is detected.
- **Off** - the panel starts listening silently.

This setting is useful if the device is in a bedroom or another quiet space.

## Asking for Things

Use the device like a normal Home Assistant Assist satellite:

- "Hey Jarvis, turn on the kitchen lights."
- "Okay Nabu, what is the weather?"
- "Alexa, set the living room heating to 20 degrees."
- "Hey Mycroft, open the blinds."

The exact commands available depend on your Home Assistant setup, exposed entities, Assist pipeline, and conversation agent.

## Timers

Voice timers are supported.

Examples:

- "Hey Jarvis, set a timer for 10 minutes."
- "Okay Nabu, set a pasta timer for 12 minutes."
- "Alexa, cancel my timer."
- "Hey Mycroft, how much time is left?"

When a timer finishes, the panel plays a repeating timer alert. The alert continues until it is dismissed or until the built-in timeout stops it.

To stop a ringing timer, simply say:

```text
stop
```

You do not need to say the normal wake word first. This standalone **stop** handling is intended for timers and active on-device audio.

## Stop Commands

The panel has two kinds of stop handling.

### Standalone Stop

Saying only:

```text
stop
```

works without the normal wake word. It is used to stop a timer alert, interrupt a voice response, or stop other active on-device audio.

When media is playing and a timer alert is ringing, **stop** is intended to dismiss the timer alert without also stopping the underlying music.

### Media Stop Phrases

To stop local media playback during a normal voice command, use your selected wake word first:

- "Hey Jarvis, stop playing."
- "Hey Jarvis, stop playback."
- "Hey Jarvis, stop the music."
- "Hey Jarvis, stop music."
- "Hey Jarvis, stop the song."
- "Hey Jarvis, stop song."
- "Hey Jarvis, stop audio."
- "Hey Jarvis, stop the audio."

These phrases are handled specially when the device has paused media for a voice interaction. They stop the local playback instead of incorrectly resuming it after the assistant response.

## Barge-In

Barge-in lets you interrupt an active voice response by saying the wake word again.

For example:

1. Say "Hey Jarvis, what is the weather?"
2. While the device is still speaking, say "Hey Jarvis, turn off the hallway light."

The device stops the current response and starts listening for the new command.

Barge-in also works while media is playing. The device pauses or ducks audio as needed, listens to your command, responds, and then restores media playback where possible.

## Media Playback

The panel can be used as a Home Assistant media player.

During a voice interaction:

- Normal voice turns pause media while the device listens and responds.
- Home Assistant announcements may duck media volume instead of fully pausing it.
- Volume should be restored after the voice interaction or announcement finishes.
- Cover art is hidden while the device is listening or responding.
- The configured media artwork source is restored afterwards.

If you manually dismiss the cover-art screen, the panel should respect that and return to the normal screen timeout behaviour instead of immediately reopening cover art.

## Volume Control

The panel exposes **Voice Media Player** in Home Assistant. This controls the device speaker used for voice responses, announcements, chimes, and media playback.

You can adjust volume in three places:

- From the Home Assistant media player entity.
- By tapping the speaker or microphone shortcut in the panel clock bar.
- By pressing the physical BOOT/Key2 button on WiFi firmware.

The physical BOOT/Key2 button is the button furthest from the USB-C ports. It opens the same local volume control shown from the clock bar.

::: warning
The physical BOOT/Key2 volume shortcut is only available in WiFi firmware. Ethernet firmware uses the same GPIO line for Ethernet hardware, so it cannot also be used as a runtime button.
:::

## Microphone Mute

The device exposes a **Mute** switch in Home Assistant.

When muted, the panel continues to run the audio path, but wake-word detections are ignored and are not sent to Home Assistant. This is intentional: it keeps the audio pipeline stable while still preventing voice commands from being acted on.

You can toggle mute from:

- The **Mute** switch on the Home Assistant device page.
- The microphone button shown on the device volume control.

When the clock bar is enabled, the icon indicates the state:

- Normal speaker or microphone icon - listening is enabled.
- Microphone-off icon - wake-word response is muted.
- Speaker-off icon - speaker output is muted in Home Assistant.

## Clock Bar Controls

Voice Control uses the clock bar for the local speaker and microphone shortcuts.

Enable it from the panel web page:

1. Open the panel web page in a browser.
2. Go to **Settings > Display > Clock Bar**.
3. Turn on **Show Clock Bar**.

If the clock bar is disabled, the speaker and microphone shortcuts are not shown. You can still use the Home Assistant device page, and on WiFi firmware you can still press the physical BOOT/Key2 button to open local volume control.

## Home Assistant Entities

The exact entity names depend on your Home Assistant naming, but the device exposes controls similar to:

- **Voice Wake Word** - choose Alexa, Okay Nabu, Hey Jarvis, or Hey Mycroft.
- **Voice Services** - turn wake-word listening on or off.
- **Wake word sensitivity** - tune wake-word detection sensitivity.
- **Wake sound** - enable or disable the wake chime.
- **Mute** - ignore wake-word detections until unmuted.
- **Voice Media Player** - speaker output for voice, alerts, announcements, and media.
- **Voice Assistant Stage** - diagnostic state showing the current voice pipeline stage.
- **Voice Assistant Last STT** - diagnostic text last heard by speech-to-text.
- **Voice Assistant Last TTS** - diagnostic text last spoken by text-to-speech.

Some older entities can remain visible as unavailable after an upgrade. Home Assistant sometimes keeps removed ESPHome entities in the device registry instead of deleting them automatically. They are safe to ignore, or you can remove stale disabled entities from Home Assistant if you are sure they are no longer used.

## Troubleshooting

### Voice Satellite Setup Failed

If Home Assistant shows that initial Voice Satellite setup failed, press **Retry**.

This has been seen when Home Assistant tries to complete setup before the panel has fully finished initialising. Retrying after the device is ready normally succeeds.

If retry does not work, use **Settings > Devices & services > ESPHome > three-dot menu > Reconfigure** for the device.

### The Device Does Not Appear Under Assist Devices

If the panel was already added before Voice Control was installed, reconfigure it:

1. Open **Settings > Devices & services**.
2. Open **ESPHome**.
3. Use the three-dot menu beside the panel.
4. Choose **Reconfigure**.

After reconfiguration, check **Settings > Voice assistants > Assist devices** again. If you have more than one voice-capable device, both should be visible on that Assist devices screen.

### Assistant and Voice Options Are Missing

Use the same **Reconfigure** flow above. Once Home Assistant has refreshed the ESPHome voice satellite setup, the Assistant and voice options should appear.

### Wake Word Is Not Detected

Check the following:

- The **Mute** switch is off.
- **Voice Services** is on.
- The selected **Voice Wake Word** matches what you are saying.
- **Wake word sensitivity** is not set too low for the room.
- The device has been online for a short time after boot.
- Home Assistant is connected to the ESPHome device.

If the device has only just been factory reset or added to Home Assistant, give it a minute to settle and try again.

### Wake Word Is Detected While Muted

This is expected. The device may still log that it heard a wake word, but it ignores the detection while muted and does not start a Home Assistant voice command.

### No Wake Sound Plays

Check that **Wake sound** is enabled on the Home Assistant device page.

If the wake word is detected very soon after boot or setup, the device may still be finishing its Home Assistant connection. Wait briefly and try again.

### Timer Finishes but There Is No Alert Sound

Make sure the device volume is not muted and the **Voice Media Player** volume is high enough. The timer alert uses the same speaker path as voice responses and media playback.

### Music Does Not Resume Correctly

Media resume depends on the Home Assistant media player source as well as the panel. Music Assistant and streaming services can behave differently when playback is paused, interrupted, or resumed.

If a track restarts from the beginning, check whether the same thing happens with another Music Assistant provider or another media source.

### Cover Art Appears at the Wrong Time

Voice interactions temporarily hide cover art while listening or responding. If cover art returns after the response, it usually means the media player state says media is playing again.

If you manually dismiss the cover-art screen, the device should return to the normal controls and then follow the configured screen timeout.

### Clock Bar Icons Are Missing

The local speaker and microphone icons only appear when the clock bar is enabled. Turn on **Settings > Display > Clock Bar > Show Clock Bar** from the panel web page.

### ESP32-C6 Firmware Entities Appear

Some ESP32-P4 86 firmware builds expose ESP32-C6 firmware update entities because the panel uses an ESP32-C6 co-processor for WiFi and Bluetooth. These are separate from Voice Control and are not voice diagnostics.

## Good Things to Test After Setup

After installing or updating Voice Control, test:

1. Say the selected wake word and ask a simple question.
2. Change **Voice Wake Word** in Home Assistant and confirm the new word works.
3. Turn **Wake sound** off and on.
4. Set a one-minute timer and stop the alert with **stop**.
5. Play music to the panel, then ask a voice command and confirm playback resumes.
6. While music is playing, say the wake word and "stop music" to confirm local playback stops.
7. Turn **Voice Services** off and on, then confirm wake-word listening starts again.
8. Mute and unmute the microphone from Home Assistant.
9. Open the local volume control from the clock bar or BOOT/Key2 button.
10. If using cover art, confirm it hides during a voice turn and restores afterwards.
