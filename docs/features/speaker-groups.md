---
title: "Group Home Assistant Speakers from Your Touchscreen"
description: Set up Home Assistant speaker discovery, join compatible speakers, and control group volume from EspControl.
---

# Speaker Groups

Speaker Group opens the speaker panel directly, without the playback, progress, and single-player volume tabs. It lets you join and unjoin compatible speakers and control the volume of speakers that are currently grouped.

## Before You Start

Speaker grouping needs at least two speakers from a platform that supports Home Assistant's `media_player.join` and `media_player.unjoin` actions. Support depends on the integration and player; listing a speaker in the discovery sensor does not make it compatible. See Home Assistant’s [media player actions](https://www.home-assistant.io/integrations/media_player/#list-of-actions).

First, try joining the same speakers in Home Assistant. Home Assistant can reject a request to join speakers from different platforms, such as a Sonos speaker and a Google Cast speaker.

The panel must also be allowed to run `media_player.join`, `media_player.unjoin`, and `media_player.volume_set`; see [Enable Actions](/getting-started/home-assistant-actions).

## Create the Speaker Discovery Sensor

EspControl needs a Home Assistant template sensor that supplies the list of speakers, their names, availability, and current volumes. Choose **one** of the templates below and add it to `configuration.yaml`. If you already have a `template:` section, add the `- sensor:` entry beneath it instead of creating a second `template:` key. If you use `template: !include templates.yaml`, put the `- sensor:` entry in that file without the top-level `template:` line.

Check your configuration in Home Assistant, then restart Home Assistant to load the sensor. This is Home Assistant configuration, not part of the panel’s ESPHome YAML.

### Standard Integrations

This example lists Sonos players. For another integration that supports joining and unjoining your speakers, replace both instances of `sonos` with its Home Assistant integration name, such as `heos`. Include the main speaker and the speakers you want to join.

```yaml
template:
  - sensor:
      - name: "Speaker Group"
        unique_id: speaker_group
        state: >
          {%- set s = integration_entities("sonos") | select("match", "media_player") | list -%}
          {{ s | count }}
        attributes:
          data: >
            {%- set s = integration_entities("sonos") | select("match", "media_player") | list -%}
            {%- set ns = namespace(items=[]) -%}
            {%- for entity_id in s -%}
              {%- set available = states(entity_id) not in ["unknown", "unavailable"] -%}
              {%- set ns.items = ns.items + [[entity_id, state_attr(entity_id, "friendly_name") or entity_id, state_attr(entity_id, "volume_level"), available]] -%}
            {%- endfor -%}
            v2|{{ ns.items | to_json }}
```

### Music Assistant 2.8 and Later

[Music Assistant 2.8 introduced player merging](https://www.music-assistant.io/blog/2026/03/25/music-assistant-2-8/), which combines multiple protocols for the same physical speaker into one player. Use this template to list its media players. Only include players that can be grouped together. To restrict the list, replace the `set s = ...` expression in **both** `state` and `data` with an explicit list, for example `{%- set s = ["media_player.living_room", "media_player.kitchen"] -%}`. Use the Music Assistant entity IDs for those players.

```yaml
template:
  - sensor:
      - name: "Speaker Group"
        unique_id: speaker_group
        state: >
          {%- set s = integration_entities("music_assistant") | select("match", "media_player") | list -%}
          {{ s | count }}
        attributes:
          data: >
            {%- set s = integration_entities("music_assistant") | select("match", "media_player") | list -%}
            {%- set ns = namespace(items=[]) -%}
            {%- for entity_id in s -%}
              {%- set available = states(entity_id) not in ["unknown", "unavailable"] -%}
              {%- set ns.items = ns.items + [[entity_id, state_attr(entity_id, "friendly_name") or entity_id, state_attr(entity_id, "volume_level"), available]] -%}
            {%- endfor -%}
            v2|{{ ns.items | to_json }}
```

## Verify the Sensor

After Home Assistant restarts, open **Developer Tools** > **States** and search for `sensor.speaker_group`. Its state should be the number of speakers found, and its `data` attribute should list the speaker names. If it is missing, check the YAML indentation and Home Assistant logs. If Home Assistant assigned another entity ID, such as `sensor.speaker_group_2`, use that exact ID in **Speaker Discovery Entity** below. If its state is `0`, check that the integration name matches Home Assistant and that it has `media_player` entities.

The versioned JSON format above is recommended because speaker names can safely contain commas and it reports availability explicitly. The earlier comma-separated ESPHome Media Player format remains supported for existing installations.

## Add the Card in EspControl

1. Select a card and change its type to **Media**.
2. Choose **Speaker Group** as the media type.
3. Enter the main speaker entity, for example `media_player.living_room`.
4. Leave **Speaker Discovery Entity** empty to use `sensor.speaker_group`, or enter a different discovery sensor if you created one.
5. Save the card.

You can also use **All Controls**. When the selected player supports grouping and the discovery sensor has speakers, its popup includes a **Speakers** tab.

The main speaker is always selected. Selecting another speaker sends `media_player.join` with the complete selected group; clearing one sends `media_player.unjoin` to that speaker. A row shows a pending state while Home Assistant handles the request. If an integration rejects an incompatible request, the selection is restored and the panel shows an error.

The optional **Speaker Discovery Entity** card setting can point to another compatible discovery sensor. A Home Assistant media-player Group helper can provide a manually maintained list for joining and removing speakers, but it exposes only entity IDs. Group and per-speaker volume controls therefore require the discovery sensor format above, which also supplies each member's volume.

## Group and Individual Volume

Individual volume controls appear only for speakers in a multi-speaker group. While grouped, the main Volume tab shows **Group** and its arc represents the arithmetic mean of the available members' current levels. Moving it applies the same difference to each speaker instead of making every speaker equally loud.

For example, `10%`, `25%`, and `40%` has a group level of `25%`. Moving Group Volume to `35%` sends `20%`, `35%`, and `50%`. Volume increases respect the card’s **Maximum Volume** setting, and levels cannot go below `0%`. Lowering the group volume preserves the balance even if a speaker was already above the cap. Group Volume remains disabled until every available member has reported a volume, which avoids losing the existing balance.

## Troubleshooting and Device Testing

- If the **Speakers** tab does not appear, check that `sensor.speaker_group` exists and its `data` attribute is not empty.
- If a speaker does not appear, make sure it is a `media_player` from the integration named in the template. If its row is unavailable, check its state in Home Assistant.
- If adding a speaker fails, try the same join in Home Assistant first. The speakers may not be compatible, or the integration may not support grouping.
- If the panel can show speakers but cannot change them, check the panel's Home Assistant action permissions.
- If volume controls are missing, join at least one other speaker and check that each group member reports a `volume_level` value.
- Change membership and volume in Home Assistant and confirm the panel updates.
- Reconnect or restart a speaker and confirm its state returns without reopening the card.
