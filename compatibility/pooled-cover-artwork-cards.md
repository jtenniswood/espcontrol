# Pooled cover artwork cards

This stage of #1825 adds cover-art presentation to the reusable grid. It remains
**unselected in production**, with no configuration migration or navigation-depth
change. Media panels, the remaining drivers, application scene wiring, document
storage and physical qualification still precede nested pages.

`PageBasicLvglPort` reuses its generic drawing surface and two labels for artwork,
tint, title and artist. Idle uses those same labels for Music and translated Idle.
Image-only, details, external-source fallback, custom backgrounds, compensated
image width and single/large/compact-portrait fonts follow the existing helpers.
Cover clicks require a media-control hook; they do not create an inline slider.
Unsupported or missing providers/hooks reject preparation.

The shared image pool has the existing YAML resource count: two images on the S3
and six on the P4. An image slot is independent of the physical grid shell index.
Excess visible image cards keep their ordinary fallback; the pool does not grow
with configured pages or promise simultaneous artwork in every P4 cell.

`PageCoverArtRuntime` reads remote/local URLs and metadata from the shared entity
graph. Register it after the media owner in `PageEntityObserverSet`, and remove
it before destruction. Observers compare borrowed old/new URL bytes, mark fixed
records and start no work. Each leased image keeps one selected job URL and a
bounded refresh signature. Configuration/entity names and candidate URLs are not
copied into a per-card artwork context.

The remote picture remains authoritative. Source selection shares the legacy
local-first/changed-remote policy. Adjacent deliveries settle for 75 ms; a missing
local companion receives a further 300 ms window. A nonempty content ID owns the
refresh signature; otherwise title/artist, then playback state supply it. Raw
metadata is limited before HTML decoding. Missing proxy tokens wait for the
graph's retained access token, bounded to 512 bytes. Token query parameters precede
URL fragments. URL and token contents must not be logged.

Refresh keeps a healthy completed image visible through a pending or failed
transfer. Retries use the selected URL after two seconds. Initial failures get
at most ten retries inside the 45-second startup window; successful images can
continue refreshing after an error. The environment supplies the current base
URL, endpoint generation and existing modal/headroom admission policy. Changing
HTTP settings or HA generation invalidates old work before a fresh request.
The application must arrange the initial shared-dependency snapshot before
selecting this scene; navigation itself requests no HA replay or subscription.

Run image maintenance, cover maintenance, then renderer update after all prepared
owners activate. A drawing frame must be cleared before retiring its image lease.
The image adapter fences outstanding draw work before freeing pixels. Every draw
validates the complete binding and successful asset ID, including between image
and tint tasks. Completed asset IDs change only on successful downloads. Animated
frames advance the redraw revision while retaining the asset ID; the component
refreshes its descriptor on each pull, and accent sampling runs once per asset.
One runtime exclusively owns the pool's stable animation callbacks. Its origin
must supply actual image visibility and active-screen eligibility, including
modal/takeover behavior.

Host tests cover the shared graph, both pool sizes, retries, missing companions,
clock wrap, source changes, authentication, reconnect, endpoint replacement and
late/reentrant retirement. Native UI tests exercise all nine profiles, legacy
label geometry, source/idle fallbacks, asset replacement during drawing and 1,000
navigation cycles with stable retained C++/arena/LVGL resources. These navigation
checks still include temporary LVGL layout allocations. The frame test compares
13,824 full-invalidation pixel cases against the existing image plus overlay;
it does not establish arbitrary dirty-region, whole-screen or hardware parity.
SDK and sanitizer checks do not establish device RAM savings. Qualify actual
heap, PSRAM, largest blocks, image peaks and latency on the S3 and working P4
hardware before production selection.
