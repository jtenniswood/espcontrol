---
title: Media Cards
description:
  How to use media cards on your EspControl panel to control Home Assistant media player entities.
---

# Media

A Media card controls a Home Assistant `media_player` entity. It can work as a simple playback button, a volume control, a track position control, a now-playing display, or a cover-art tile.

![Wide media card showing now-playing title and artist](/images/card-media.png)

## Setting Up a Media Card

1. Select a card and change its type to **Media**.
2. Choose the media **Type**:
   - **All Controls**
   - **Speaker Group**
   - **Play/Pause Button**
   - **Previous Button**
   - **Next Button**
   - **Volume Button**
   - **Track Position**
   - **Now Playing**
   - **Cover Art**
   - **Media Content**
3. Enter the media player entity, for example `media_player.living_room`.
4. Set a label or icon if the selected type shows those fields.

If Home Assistant reports the media player as unavailable, the card keeps its normal appearance. A card set to show the live state can still say **Unavailable**, and controls will work again when Home Assistant can accept actions for the player.

## Playback Buttons

**Play/Pause Button**, **Previous Button**, and **Next Button** send the matching Home Assistant media player action when tapped.

For Play/Pause, you can choose whether the card shows its fixed label or the live state, such as **Playing** or **Paused**.

## Volume Button

The Volume Button shows the current volume percentage. Tapping it opens a volume control popup on the panel, where you can adjust the volume without leaving the current page.

For media players that support setting an exact volume, the popup dial is draggable. Some integrations, including Android TV Remote, only support volume-up and volume-down commands. For those players, the dial remains a live volume indicator and the **−** and **+** buttons adjust the volume one device-defined step at a time.

Set **Maximum Volume** to cap the panel control below 100%. The popup dial rescales to that maximum, so a 40% cap makes 40% the end of the arc.

For step-only players, the maximum is best-effort because the device decides the size of each volume step. EspControl disables further volume-up commands once Home Assistant reports that the configured maximum has been reached.

The card watches the media player's `volume_level` attribute, so it also updates when volume changes elsewhere.

## Track Position

Track Position shows playback progress and elapsed time.

- Drag the progress bar to seek within the current track.
- The card uses Home Assistant's `media_duration`, `media_position`, and `media_position_updated_at` attributes when they are available.
- You can show a fixed label or the live playback state.

Seeking depends on the media player integration. Some players expose progress but do not support seeking.

## Now Playing

Now Playing shows the media title and artist from Home Assistant. For TV or Line-in sources that do not provide artist data, the artist line shows **Source** instead of reusing the previous track's artist.

You can choose optional controls:

- **None** shows only the current title and artist.
- **Track Position** adds a progress background and lets you seek.
- **Play/Pause** makes the card tappable so it toggles playback.

Now Playing works best on wider or larger cards because it has more room for track text.

## Cover Art

Cover Art shows the current artwork reported by the selected media player. Choose a square card size: **1x1**, **2x2**, or **3x3**. EspControl crops the image to fill the tile without stretching it.

If the main player is a soundbar or speaker that reports an external `TV`, `Line-in`, or `HDMI` source, you can set **External Source Media Entity** to the connected player, such as an Apple TV. While that external source is active, the card takes its artwork, track details, progress, and Play/Pause or **All Controls** actions from the connected player. If the connected player is unavailable or has no current media, the card hides cached artwork and shows the main player's source instead. It switches back to the main player automatically for normal playback.

Enable **Show Track Details** to place the current title and artist over the artwork. EspControl adds an artwork-derived dark tint so the text remains readable. If artwork is unavailable, the title and artist remain visible on the card's normal background. The setting is off by default, so existing Cover Art cards remain image-only.

Choose what happens when the artwork is tapped:

- **Play/Pause** toggles playback.
- **All Controls** opens the full media controls popup.

Cover Art uses one of the display's shared image download slots. If all slots are already used by image or cover-art cards, remove one before adding another. Artwork availability and update speed depend on the media player integration and the `entity_picture` information it supplies to Home Assistant.

## All Controls

All Controls opens playback controls and volume in a popup. The parent card uses the play/pause icon, and can show either its fixed label or the current media player state. Its top-left area can show either the icon or the current volume number.

Add a **Compatible Speakers Group** to include a fourth **Speakers** tab. The tab is shown only when the main player reports Home Assistant's grouping capability. Leaving this field empty keeps the existing three-tab popup unchanged.

## Speaker Groups

Speaker Group opens the same speaker panel directly, without the playback, progress, and single-player volume tabs. It lets you join and unjoin compatible speakers and control the volume of speakers that are currently grouped.

### Create the Compatible Speakers Group

EspControl needs a Home Assistant media-player Group helper as its list of possible speakers:

1. In Home Assistant, open **Settings > Devices & Services > Helpers**.
2. Create a **Group** helper containing the media players that can play together.
3. Put only mutually compatible players in the helper. For example, keep Sonos players with other Sonos players supported by the same Home Assistant integration.
4. In EspControl, select the main **Entity** that grouping actions should use.
5. Enter the helper's `media_player` entity in **Compatible Speakers Group**, for example `media_player.downstairs_speakers`.

Home Assistant does not expose enough integration-registry information through EspControl's device connection to prove that two players are compatible. The panel checks that the main player advertises grouping support, but the helper is the source of truth for which speakers to offer. Speakers already in the live group are also shown even when they are missing from the helper, so they can still be controlled or removed.

The main speaker is always selected. Selecting another speaker sends `media_player.join` with the complete selected group; clearing one sends `media_player.unjoin` to that speaker. A row shows a pending state while Home Assistant handles the request. If an integration rejects an incompatible request, the selection is restored and the panel shows an error.

### Group and Individual Volume

Individual volume controls appear only for speakers in a multi-speaker group. **Group Volume** is the arithmetic mean of the available members' current levels. Moving it applies the same difference to each speaker instead of making every speaker equally loud.

For example, `10%`, `25%`, and `40%` has a group level of `25%`. Moving Group Volume to `35%` sends `20%`, `35%`, and `50%`. Each result is limited to `0%` and the card's **Maximum Volume** setting. Group Volume remains disabled until every available member has reported a volume, which avoids losing the existing balance.

### Permissions and Device Testing

The panel must be allowed to run `media_player.join`, `media_player.unjoin`, and `media_player.volume_set`; see [Enable Actions](/getting-started/home-assistant-actions). Confirm grouping works in Home Assistant first because integration support varies.

Before relying on the card, test with at least three compatible speakers:

- Join and unjoin from both the All Controls tab and a standalone Speaker Group card.
- Change membership and volume in Home Assistant and confirm the panel updates.
- Confirm individual controls disappear after a speaker leaves the group.
- Check relative group volume near both volume limits.
- Try an unavailable speaker and, if practical, an intentionally incompatible join to confirm the error state recovers.
- Reconnect or restart a speaker and confirm its state returns without reopening the card.

## Media Content

Media Content is a shortcut for anything Home Assistant can play with the `media_player.play_media` action. It is not tied to Spotify, Music Assistant, Plex, Jellyfin, Sonos, or any other specific music system.

Use it for playlists, radio stations, albums, saved favorites, channels, podcasts, or other playable media items.

Enter:

- **Speaker Entity** - the Home Assistant `media_player` that should play the media, for example `media_player.living_room`.
- **Source** - choose the source preset, such as **Spotify**, **Apple Music**, **YouTube Music**, **Plex**, **Jellyfin**, **Home Assistant Media Source**, or **Custom / full URI**.
- **Media Type** - choose the type Home Assistant expects, such as `playlist`, `music`, `album`, `track`, or `channel`. Use **Custom** if your integration needs a different value.
- **ID** - enter only the playlist, station, album, track, or favorite ID when using a source preset. Use **Custom / full URI** if your integration needs the full media content ID exactly as Home Assistant shows it.
- **Label** - the name shown on the button, for example `Morning Playlist`.
- **Icon** - the icon shown on the button.

EspControl sends the media content type as `playlist` automatically. The important part is that the media content ID works in Home Assistant first.

When Home Assistant reports the currently playing `media_content_id`, the Playlist Button highlights only while that configured playlist or media item is playing. Some integrations do not report this value reliably; in that case EspControl can start the playlist, but it may not be able to confirm that the specific playlist is active.

### Find the Media Content ID

The easiest way is to test the media item in Home Assistant first, then copy the working values into EspControl.

1. In Home Assistant, open **Media** from the sidebar.
2. Browse to the playlist, station, album, or favorite you want to play.
3. Play it once from Home Assistant to confirm the speaker and media source work.
4. Open **Developer Tools**.
5. Open the **Actions** tab.
6. Choose the action **Media player: Play media**.
7. Select the same speaker entity you want EspControl to use.
8. Enter the media content ID you want to test and use `playlist` as the media content type.
9. Press **Perform action**.
10. If the speaker starts the right media, copy those same values into EspControl.

The YAML view in Home Assistant will look similar to this:

```yaml
action: media_player.play_media
target:
  entity_id: media_player.living_room
data:
  media_content_id: "spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO"
  media_content_type: "playlist"
```

In EspControl, that becomes:

| Home Assistant value | EspControl field |
| --- | --- |
| `entity_id: media_player.living_room` | **Speaker Entity** |
| `media_content_id: "spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO"` | **Source:** Spotify, **Media Type:** Playlist, **ID:** `1LG2Lnt9EDQS1DqoE8E2uO` |
| `media_content_type: "playlist"` | **Media Type:** Playlist |

If the value does not match one of the source presets, choose **Custom / full URI** and paste the full `media_content_id` into **ID**.

### If You Do Not Know the ID Format

Different Home Assistant integrations use different ID formats. There is no single format that works for every music system.

Common examples look like this:

| System or source | Source | ID | Media Type |
| --- | --- | --- | --- |
| Spotify playlist | Spotify | `1LG2Lnt9EDQS1DqoE8E2uO` | `playlist` |
| Spotify track | Spotify | `0KIhLAkHfL9fvgn0yy1qsU` | `music` or `track` |
| Home Assistant media source | Home Assistant Media Source | `media_source/local/Morning.mp3` | `music` |
| Integration favorite | Custom / full URI | `favorite_id_or_uri` | often `music`, `playlist`, or `favorite` |

These examples are starting points only. The value that matters is the one your Home Assistant integration accepts when you test `media_player.play_media`.

### Using a Shared Playlist URL

Many music services give you a web sharing URL. EspControl usually needs the media ID or URI, not the full website URL.

For example, this Spotify playlist URL:

```text
https://open.spotify.com/playlist/1LG2Lnt9EDQS1DqoE8E2uO?si=1Jho2boIRDGE4PQ9Q0COXA
```

contains this playlist ID:

```text
1LG2Lnt9EDQS1DqoE8E2uO
```

For Spotify, the Home Assistant media content ID is commonly:

```text
spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO
```

In EspControl, choose **Spotify**, choose **Playlist**, and paste only this ID into **ID**:

```text
1LG2Lnt9EDQS1DqoE8E2uO
```

The same idea applies to other services: copy the playlist, album, station, or favorite ID from the shared URL, then turn it into the URI format your Home Assistant integration expects.

Examples:

| Shared link or ID | Source | Media Type | ID |
| --- | --- | --- | --- |
| `https://open.spotify.com/playlist/1LG2Lnt9EDQS1DqoE8E2uO?si=...` | Spotify | Playlist | `1LG2Lnt9EDQS1DqoE8E2uO` |
| `https://open.spotify.com/track/0KIhLAkHfL9fvgn0yy1qsU?si=...` | Spotify | Track | `0KIhLAkHfL9fvgn0yy1qsU` |

Always test the result in Home Assistant first:

```yaml
action: media_player.play_media
target:
  entity_id: media_player.living_room
data:
  media_content_id: "spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO"
  media_content_type: "playlist"
```

If the Home Assistant test starts the right playlist, use the same `media_content_id` in EspControl.

### Troubleshooting

If tapping the Media Content button does not start playback:

- Make sure the button has an **ID**. EspControl requires this before saving.
- Test the same values with `media_player.play_media` in Home Assistant.
- Check that the selected **Speaker Entity** can play that source from Home Assistant.
- Try a different **Media Type**. Some integrations use `playlist`, while others use `music`, `album`, `track`, `channel`, or a custom value.
- If your integration gives you a full sharing URL, convert it to the ID or URI format that Home Assistant expects.
- If the preset source builds the wrong value for your integration, choose **Custom / full URI** and paste the exact `media_content_id` that works in Home Assistant.
- If the button starts the playlist but does not stay highlighted, check whether the media player entity reports a matching `media_content_id` while playing.
- If the speaker needs repeat or shuffle, create a Home Assistant script that sets those options and starts playback, then trigger that script from an EspControl Action card.

::: info Requires Home Assistant actions
Media cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
