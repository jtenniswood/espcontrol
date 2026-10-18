# Reusable image ownership

The image-ownership foundation of #1825 supplies fixed reusable image leases.
The [cover-art adapter](pooled-cover-artwork-cards.md) now adds presentation,
secondary-source routing and animation. Expanded views, scene integration and
physical qualification remain unfinished. Production selection, nested pages
and saved configuration are unchanged.

`PageImageRuntime<N>` holds one fixed lease per prepared image resource, not per
configured card. Binding keeps the configuration/card/shell binding token and
connection generation by value. A request has a separate monotonic 64-bit ID;
exhaustion fails instead of wrapping into an old request. No configuration,
entity, URL or widget view survives in the lease. The transport copies URL bytes
synchronously when loop maintenance starts a request.

The view must detach its LVGL source before unbinding, suspension or configuration
retirement. Unbind invalidates the lease before cancelling the downloader, so a
cancellation result cannot touch a replacement. Surface release is deferred until
maintenance, outside navigation and outside LVGL refresh/draw callbacks. The
ArtworkImage adapter waits for draw units before releasing detached pixel buffers.
A reconnect cancels readiness/work without freeing a possibly attached surface;
the binding can request fresh content when the connection returns. The view must
remove its old source when the current snapshot stops being ready.

Run image maintenance only after prepared configuration owners have activated.
The origin validator must delegate to the renderer's full binding validation and
current HA connection generation. Transport and origin outlive the image runtime;
YAML image components outlive their transport adapter. Cancellation must revoke
retained callback ownership before those owners are destroyed. A transport may
complete synchronously, including during cancellation, so leases are retired
before calling it and replacements are checked after every external call.

`PageArtworkRequests<N>` claims exactly N existing YAML ArtworkImage components at
initialization. Missing, duplicate, busy, automatically polled, legacy-owned or animation-owned components
reject initialization; partial claims are rolled back. It installs one stable
observer per component instead of adding capturing callbacks for each card.
Those components cannot be shared with the production grid. The existing global
image service and S3/P4 worker pipelines continue to own transfer/decode work and
reject stale worker generations. The new result channel carries the caller's
request ID, consumes it before dispatch and is cleared on cancellation or direct
legacy updates. HTTP 304 without a usable image cannot mark a slot ready.
The adapter also installs stable animation callbacks after claiming components;
a second runtime cannot claim their frame ownership. Successful asset IDs stay
unchanged during animated redraws. A retained refresh keeps completed readiness
and its asset ID through pending/error states, without retaining across bindings.

Image completion now guards the service generation around external notifications.
A callback that cancels/restarts a download cannot cause the completed request's
cleanup to finish its replacement. Legacy finished/error callback APIs continue
to be used for unclaimed components. URL validation, capped URL rewriting,
transfer priority, format decoding and image buffer policies stay in ArtworkImage.
This does not remove download allocations or establish a lower device peak.

Binding/unbinding creates no widget, string, context, subscription or timer. The
fixed leases and observer records add up-front storage. URL strings, service
queue growth, buffer retirement and image decoding can allocate on maintenance;
include their total/peak/largest-block cost in eventual hardware measurements.
This stage does not enlarge the existing YAML image-resource count or promise
that every P4 grid slot can display a simultaneous image. Preparation of the
future view must check its actual bounded resource budget.

Automated tests exercise actual ArtworkImage ownership/notification methods with
I/O replaced, the concrete slot adapter with transfer I/O replaced, and 1,000
zero-C++-allocation lease cycles with four- and twenty-slot banks. They cover late
success/error, cached results, configuration replacement, reconnect, partial claim
rollback, request exhaustion and reentrant cancel/start/cleanup. Sanitizers and an
actual S3 SDK compile supplement those tests. These are not physical image,
whole-screen or RAM qualification; no device is flashed for this boundary stage.
