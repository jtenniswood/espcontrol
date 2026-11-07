# Pooled media cards

This part of #1825 supplies the common icon/label views for Play/Pause, Previous,
Next, Playlist, Position, Now Playing, Volume, All Controls and Speaker Group. It remains
unselected in production. Cover artwork has its own
[adapter](pooled-cover-artwork-cards.md). Demand-created
[volume](pooled-volume-panels.md) and [media-control](pooled-media-control-panels.md)
panels are available for scene integration. Unsupported active modes reject preparation.
There is no change to saved configuration or navigation depth.

The shared entity graph owns raw state. These views create no card contexts or
subscriptions when navigating. Play/Pause follows the existing exact `playing`
predicate and optional translated state label. Previous and Next dispatch on
press and suppress the later click; other actions use the validated press/click
binding token. [Deferred grid dispatch](pooled-media-commands.md) now consumes
these intents in maintenance and installs seek feedback after helper acceptance. Playlist highlighting preserves the existing URI, suffix, case,
leaf-ID and content-type matching rules without allocating temporary strings.
Identity metadata keeps its existing HTML decoding and byte limits; display-only
transliteration is not applied to playlist identifiers.

`PageMediaStateRuntime` owns bounded freshness flags per distinct configured media
entity, including eligible cover-art secondary entities. Prepared per-card source
links restrict queries to that card's primary and secondary targets. It invalidates old metadata on transitions to idle/off/unavailable and
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
Now Playing configuration with the cover-art option uses the
[cover-art port](pooled-cover-artwork-cards.md) and requires its image provider
and media-control hook.

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
secondary-source selection is available to the
[cover-art view](pooled-cover-artwork-cards.md). Its demanded control-panel adapter
is described in [pooled-media-control-panels.md](pooled-media-control-panels.md).

Capturing/reusing label styles adds fixed per-shell storage and preinstalled LVGL
style properties, even in configurations without Now Playing. Include that cost
in the integrated device comparison; retained host counts are not device RAM
savings. Temporary grid/style work is measured separately from retained storage.

The fixed request/slot boundary is described in [reusable image ownership](pooled-image-ownership.md).
The cover-art view is now described in
[pooled-cover-artwork-cards.md](pooled-cover-artwork-cards.md). Scene wiring and
production selection remain unfinished.

Media/volume/speaker panel adapters now have separate demanded owners. Scene
wiring, production selection and the S3 hardware canary remain required, including
artwork and panel peak measurements.

The model's cover route selects the secondary only while the primary uses an
external input and the distinct secondary has a known, available, usable state
and current content. The returned entity must be used consistently for text,
progress and controls. If the secondary is ineligible, the primary keeps its
external-source fallback. Source names in a route are borrowed from the active
media arena: validate the originating binding and dispatch synchronously.

Artwork content has separate remote/local validity flags. A URL received before
state or during idle cannot become current solely because playback later starts.
`None`, `unknown` and `unavailable` are absent pictures; picture URL bytes are not
HTML-decoded. A fresh remote picture can clear a stale omitted TV/HDMI source,
while a local-picture observation does not. Use `effective_source` with decoded
cache text to honour synthesized/cleared input semantics. Raw retained cache
values alone cannot determine the fallback label.

Cover dependency declarations reserve progress for both sources even on the
small-device policy, matching the current route's unconditional progress
subscriptions. Ordinary low-heap control cards keep their existing policy.
Power/group/discovery dependencies are still prepared ahead of navigation; the
visible media panel adapter must preserve the installed S3 discovery fix.

Preparation decodes secondary names once using bounded temporary arena scratch,
then deduplicates them into the final source arena and adds compact per-media-card
links. The temporary allocation can fail without replacing the active owner, and
is freed before preparation returns. `peak_preparation_bytes` reports that scratch
arena; the media arena peak includes simultaneous active, staged and scratch
storage. Parser string temporaries and whole-device peaks remain separate costs.

Host tests compare routing with the unchanged cover-art policy helpers across
state/content combinations, exercise remote/local freshness and primary/secondary
input roles, and use the actual shared entity graph for 1,000 source switches with
stable allocations/channels. Replacement tests reject late results and revoke a
removed secondary even when another card still owns that same source. These
checks do not establish physical control, image or RAM parity.

A reused source that gains detail or progress responsibilities queues a fresh
snapshot even when it already has a current content ID. Newly required progress
clears old duration/position while preserving playback state. Retained coordinator
replay cannot supply those newly required fields safely. The request waits until
the source is usable (or its state is not yet known), consolidates with other
refreshes and is cleared before external dispatch. Preparation/activation never
start network work, and idle sources retain the pending request until playback
returns.


Volume and All Controls reuse the common number/icon/label bank. Volume remains a
numeric button requiring its dedicated volume-panel hook. All Controls keeps its
icon/volume-number choice, default status label, explicit configured label and
playing highlight, and requires the media-control hook. Missing hooks or the
shared media owner reject preparation. Neither card creates an inline slider or
a per-card modal context. The normal V1 reader canonicalizes an empty volume label
to translated Volume before dependency enumeration, matching existing parsing.

The graph supplies volume and capability values without retaining another copy.
Absolute controls clamp their displayed value to the configured maximum; step
and read-only controls display the reported percentage. Non-finite values stay
unknown, finite extremes clamp before integer conversion, and only the relevant
bits are extracted from a wide capability mask. Cache exhaustion is distinguished
from an unknown capability and prevents an absolute-volume preview.

A panel may pass an accepted absolute-volume attempt to `preview_volume` with its
original full binding token. The card shows that bounded target for the existing
1.5-second acknowledgement window. A matching report, timeout, capability change,
unavailable source, disconnect, navigation, takeover or configuration replacement
retires it. Preview changes no HA state and sends no service call. Media panels
validate and dispatch their actions independently; scene wiring remains
unfinished. The scene must request a consolidated fresh snapshot
when configuration introduces dependencies already registered by another owner;
these card readers neither replay HA state nor create subscriptions on navigation.

The native card test exercises modal hooks, numeric/icon/status options, late
clicks, delayed acknowledgements, reconnect/replacement, reused sensor-container
alignment and 1,000 navigation cycles. Capturing that original alignment adds
fixed per-shell storage and preinstalled style properties. Include it in device
measurements; retained host counters do not establish a lower device heap peak.

Speaker Group cards retain their icon/configured label and open the group-only
panel through the validated media-control hook. Their checked state means two
distinct valid players, including the primary, while that primary is available;
playback state alone does not check them. Numeric/status options remain limited
to All Controls, matching the current firmware. A borrowed entity-list cursor
preserves the existing trimming, quoting, escaping and deduplication rules
without allocating a member vector during paint. Tests compare it with the
production legacy parser, including invalid primaries and unusually quoted IDs.


An accepted grouping response can pass its boolean checked state through
`PageLegacyModalStyle::state_confirmed`. The scene must match the modal opening
to the original full card binding before calling `preview_group`; other media
card modes reject this feedback. It changes no Home Assistant cache value and
sends no command. The existing shell feedback storage holds the confirmation
until a newer membership report, unavailable/exhausted data, reconnect,
navigation, takeover or configuration replacement retires that binding's hint.
It has no clock timeout, so unrelated playback/volume updates cannot erase it.

The shared media owner counts live membership reports for each unique source,
including reports with unchanged values. Copied preparation state is not an
acknowledgement. This counter lets a later report override both the panel and
card confirmation without changing the raw graph's value-deduplication policy.
Include the per-source field/padding and fixed modal callback observer storage
in device memory qualification. The card shell and configured-card metadata
gain no fields for this feedback. Scene wiring remains a cutover gate.
