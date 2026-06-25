---
title: Calendar Cards
description:
  How to show upcoming or in-progress calendar events on your EspControl panel,
  with a tap-to-open agenda of today's remaining events.
---

# Calendar

A Calendar card shows what's on your schedule from one or more Home Assistant
`calendar.*` entities (Google Calendar, CalDAV, Local Calendar, etc.). The tile
counts down to your next event — or shows how long is left in the one you're in —
and tapping it opens an agenda of today's remaining events.

This is different from the [Date & Time](/card-types/calendar) card, which just
shows the clock and date.

## Setting Up a Calendar Card

1. Select a card and change its type to **Calendar**.
2. In **Entity**, enter one or more `calendar.*` entities
   (comma-separated for multiple — events from all of them are merged).
3. Choose a **Type**: **Time to next event**, **Current event**, or **Next event** (see below).
4. Optionally set a **Label** (defaults to "Calendar").
5. Apply the configuration so the panel restarts with the new card.

## Types

**Next** — answers *"what's my next event, and when?"* The tile counts down to the
start of the next upcoming event (showing `min`, `hr`, or `days`), even while you
are currently in another meeting.

- More than 5 minutes away: muted tile, white countdown + event name.
- 5 minutes or less away: the whole tile turns your **primary colour** to get
  your attention.
- Nothing upcoming but a meeting is on now: shows **Now**.
- Nothing left today: shows a relaxed glyph and **Done for the day**.

**Current** — answers *"how is the meeting I'm in going?"* The tile becomes a
progress bar — primary colour for time remaining, a darker shade for time elapsed,
split at how much is left — and shows the phase:

- **Just started** (first 5 minutes)
- **In progress**
- **About to end** (last 5 minutes — shows the minutes remaining)
- **Free** when no meeting is active.

## The Agenda (tap to open)

Tapping the tile opens today's remaining events, merged from every configured
calendar and sorted by start time. Active events are highlighted, with the event
name on top and the time range beneath it.

## Notes

- **Colours** follow your **primary colour** (Button On Color) from
  [Appearance](/features/appearance), with a darker shade derived from it.
- **Times** follow the 12/24-hour **Clock Format** in [Time Settings](/features/clock).
- The tile updates about every 30 seconds; the agenda refreshes its event list in
  the background so it stays current.
- Long event names shrink and truncate with "…" so they fit the tile and rows.
- Developed and tested on the **Guition ESP32-S3-4848S040** (4-inch, 480×480),
  at the **normal**, **wide**, **tall**, and **large** tile sizes. Other panels
  use the same layout, scaled to their fonts.
