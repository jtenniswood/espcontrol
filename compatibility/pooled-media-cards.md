# Pooled media cards

This part of #1825 supplies the common icon/label views for Play/Pause, Previous,
Next, Playlist, Position and Now Playing. It remains unselected in production. Other media modes reject
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

Detail consumers also invalidate title/artist validity when the established item
identity changes. A temporary empty ID retains the previous identity for the
next comparison. A title-only change clears the old artist and queues one
consolidated reannouncement on safe loop maintenance. The graph exposes its
previous packed value synchronously before mutation, so the owner compares
bounded decoded title bytes without retaining another title string or hash.
An expected title refresh keeps an artist that arrived first in the same
snapshot; its two-second window follows the existing policy. Configuration
replacement cannot repopulate invalidated text from a copied cache value.
Playlist-only sources do not request unused title/artist data.

The old string presentation and item-kind APIs remain available. Shared views
can normalize display text and classify borrowed URI/type views without heap
allocation. Presentation strips unsupported emoji and preserves complete UTF-8,
including in-place conversion of a fixed decoded buffer. It must never be used
for stored identifiers or HA actions.

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

Position views use one configuration-owned playback clock per distinct source.
The application calls the media owner’s `loop()`; a single one-second revision
refreshes playing sources that have progress consumers. Pausing freezes elapsed
time, copied cache values do not reset the clock, and reconnects invalidate the
clock before fresh state arrives. Timestamp, rounding and ordinary time formatting
match the existing helpers. The clock and snapshots allocate no UI objects,
per-card timers or temporary strings.

Position cards require a fixed slider and fill in every reusable shell, a media
owner and a synchronous seek hook. Missing resources reject preparation. The
transparent slider keeps the existing horizontal geometry and minimum fill handle.
A preprocessing guard captures the binding before slider processing; a separate
callback reads the final value after LVGL handles release. Page changes, takeovers,
reconnects and lost input cancel the captured gesture. A held pointer must release
before it can operate a newly bound card. The view keeps a seek preview for at most
three seconds while waiting for an acknowledgement; it never changes shared HA
state. The action hook must validate its origin, send synchronously and avoid
retaining the duration or configuration views beyond dispatch. Non-finite or
out-of-range seek conversions are rejected.

Native tests compare actual pointer results and slider/fill geometry with the
existing production helpers, then exercise reconnects, reentrant navigation,
paused seek expiry and 1,000 page cycles per profile. Retained resource counts are
checked separately from temporary LVGL layout/style work. These are host pointer
tests, not physical touch, whole-firmware RAM or full-screen pixel parity results.

Now Playing reuses the root icon label for its title and the caption for its
artist. It reserves longer fixed text buffers during preparation and snapshots
original label fonts, geometry, spacing and scaling at bank initialization.
Binding/retirement restore that baseline, so a media label cannot become a
misplaced or incorrectly styled toggle icon. Plain, Play/Pause-background and
progress-background modes keep their distinct input behaviour. A legacy
Now Playing configuration with the cover-art option still rejects preparation
until the cover-art port is ready.

The theme supplies the media title font and current text scales; generic icon/
caption fonts can be supplied when they change. `PageRendererPort::finish_update`
is called after all visible shells update. The LVGL port resolves the grid,
arranges all media labels and measures them together. Button resize events mark
the bank dirty instead of starting recursive per-label layout passes.

Native LVGL does not support DOTS mode with static text. `write_media_label`
uses its public text/position APIs to find the same cut, then stores the rendered
ellipsis in the reserved buffer and keeps the label in WRAP mode. A temporary
text pointer is used only between synchronous getters, with no layout/refresh
in that interval, and is replaced before returning. The measured full-text height
is retained, matching native DOTS when truncation changes word wrapping.
A standalone native fixture compares 6,480 rendered cases across fonts, alignment,
scales, spacing, widths and line limits, and checks 1,000 updates with no added
label allocations/descriptors. This is label-level pixel parity, not complete
screen or hardware qualification.

Input-source state uses compact flags plus synthesized TV/Line-in labels; raw
source strings remain in the shared cache. It preserves source/content snapshot
ordering and omitted-source cleanup, and lets Now Playing display the translated
Source label without retaining another string. Primary cover consumers can
invalidate old detail/progress state on an external-input transition; full
secondary-source and artwork ownership is still unfinished.

Capturing/reusing label styles adds fixed per-shell storage and preinstalled LVGL
style properties, even in configurations without Now Playing. Include that cost
in the integrated device comparison; retained host counts are not device RAM
savings. Temporary grid/style work is measured separately from retained storage.

The fixed request/slot boundary is described in [reusable image ownership](pooled-image-ownership.md).
It remains unselected and still needs its cover-art view and scene wiring.

Remaining media work includes secondary entities, full cover-art routing and
loading/cancellation and the media/volume/speaker
panels. Those paths must preserve the existing behaviour before production
selection and the S3 hardware canary.
