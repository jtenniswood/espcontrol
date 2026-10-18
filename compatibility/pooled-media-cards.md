# Pooled media cards

This part of #1825 supplies the common icon/label views for Play/Pause, Previous,
Next and Playlist. It remains unselected in production. Other media modes reject
preparation until their views, modal resources and image ownership are ported.
There is no change to saved configuration or navigation depth.

The shared entity graph owns raw state. These views create no card contexts or
subscriptions when navigating. Play/Pause follows the existing exact `playing`
predicate and optional translated state label. Previous and Next dispatch on
press and suppress the later click; other actions use the validated press/click
binding token. Playlist highlighting preserves the existing URI, suffix, case,
leaf-ID and content-type matching rules without allocating temporary strings.
Identity metadata keeps its existing HTML decoding and byte limits; display-only
transliteration is not applied to playlist identifiers.

`PageMediaStateRuntime` owns bounded freshness flags per distinct primary media
entity. It invalidates old metadata on transitions to idle/off/unavailable and
preserves new attributes received after that transition. Returning to a usable
state requests a fresh snapshot on loop maintenance when content is missing.
Only sources used by content-reading cards request metadata; plain playback
buttons do not trigger a refresh for attributes they do not use.
Identical fresh attributes still notify the view even if their raw cache bytes
have not changed. Configuration replacement copies validity independently of raw
cache values, and reconnect generations invalidate both prepared and active data.
Raw metadata remains in the shared cache rather than being copied into this owner.

Prepare the media owner before the entity graph, pass it as the graph's observer,
and activate the prepared owners together. The observer records bounded state
synchronously; it must not dispatch actions or replace configuration from that
callback. Copied cache values are distinguished from transport observations.
The coordinator adapter skips initial coordinator replay because the page cache
explicitly copies live values. This prevents unrelated retained consumers from
reviving invalidated metadata. Ordinary coordinator consumers keep their existing
initial replay behaviour.

Automated checks compare playlist matching with the actual existing helpers,
including 10,000 varied pairs, and check bounded HTML decoding, reconnects,
configuration replacement and repeated navigation. The native grid fixtures
exercise all nine profiles and track retained resources separately from drawing
and style scratch allocations. These checks do not establish physical RAM savings.

Remaining media work includes secondary entities, progress timing, metadata
identity changes, cover-art loading/cancellation and the media/volume/speaker
panels. Those paths must preserve the existing behaviour before production
selection and the S3 hardware canary.
