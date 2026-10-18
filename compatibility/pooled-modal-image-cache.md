# Bounded expanded-image cache ownership

This #1825 prerequisite adds decoded-image reuse to the
[demanded expanded source](pooled-expanded-image-source.md). Production still
selects the existing renderer. Expanded widgets, application scene integration,
compiled legacy retirement and physical memory/latency qualification remain.
Nothing was flashed and no memory-saving claim follows from these model tests.

The one-slot modal image runtime opts into a parked pixel lease. Ordinary grid
and cover-art runtimes retain their default slot layout and behavior. Parking
revokes the full modal opening and request before cancellation, publishes a new
monotonic nonzero identity and leaves completed pixels inactive. Neither old
views nor animation callbacks can draw parked pixels. A new opening can bind
without exposing them, then explicitly adopt the exact parked identity only
after its source owner checks the key. Reuse is marked cached without pretending
a new network download completed or resetting refresh cadence. Request and
parking identities share the same non-reusing counter; exhaustion fails closed.

Discard, expiry, connection changes and a new request invalidate parked data.
Pixel release remains deferred to maintenance through the existing concrete
adapter, including its LVGL draw-completion fence. Cancellation may synchronously
bind/adopt a replacement; the prior call cannot revoke that replacement. Retire
purges parked ownership, and a source retirement guard rejects reentrant binding.

The source owns one cache key: entity and selected source URL strings, image
revision, decode dimensions, fit/background, endpoint and connection. Metadata
is copied only from a clean completed result during maintenance. Closing parks
only an eligible result; no key allocation occurs during modal retirement. The
already-registered observer invalidates a hidden cache on unavailable, image
revision or source/auth changes without new HA channels or I/O. In particular,
unavailable/recovery while closed cannot revive old cached pixels. Geometry,
fit/background, entity, endpoint, connection and URL are rechecked before reuse.
A close with pending, queued or otherwise unsettled work conservatively releases
the cache rather than attaching bytes to an uncertain key.

The device environment must explicitly supply retention policy; the default is
disabled. The concrete scene must delegate to existing image-pipeline PSRAM
headroom/fragmentation checks. Use a 15-second wrap-safe lifetime after close and
skip the initial open refresh on a matching constrained S3 cache. Standard P4
policy may retain indefinitely (zero TTL) but still refresh on opening while
showing the cached preview. Memory pressure and display/screensaver takeover
must purge the cache; keep pumping source/raw maintenance while the modal is
closed. Cache hit adoption runs in maintenance after context geometry is known.
No new LVGL timer or per-configured-card mutable cache is introduced.

Host tests cover cached reopen, role isolation, wrong/stale identities, last
nonce exhaustion, close after token invalidation, synchronous replacement during
parking, deferred release, wrap/15-second expiry, persistent P4 preview plus
refresh, memory pressure, changed credentials/revisions/entity/geometry/fit/
background/endpoint/connection, unavailable/recovery while closed and explicit
purge. All nine profiles execute 1,000 cache-hit reopen/close cycles without
additional transfers or registrations, alongside 1,000 fresh demanded downloads
and the existing navigation/handoff workloads. Concrete adapter tests verify
paused parked animation, ignored stale transport generations, draw fences on
release and rejection of a cached success with no pixels. Sanitizer, GCC, SDK
and native grid regressions complement these tests; they are not hardware tests
or current factory firmware builds.

After scene integration, qualify the Guition 4848S040 S3 and a working twenty-slot
JC8012P4A1 P4 with repeated cache hits/misses, actual low memory/fragmentation,
reconnect and takeover. Compare open latency, idle heap/PSRAM, minimum heap,
largest block and download/decode peaks against matched pre-cutover firmware.
The P4-86 remains reserved for its USB boot diagnosis. Configuration/backup
formats and supported nesting depth remain unchanged.
