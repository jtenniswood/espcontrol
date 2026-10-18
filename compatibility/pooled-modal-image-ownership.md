# Image ownership for a modal opening

This prerequisite for #1825 gives expanded image work its own typed request
lifetime. The [image tiles](pooled-image-tile-views.md) still use full grid
bindings; `PageModalImageRuntime` instead accepts `PageModalToken` with the
current configuration, opening nonce, card and IMAGE_CARD kind. It never
constructs a grid token for an overlay or borrows its old shell identity.
Production remains on the existing renderer, and expanded source/refresh/UI,
application-scene integration and physical qualification are not complete.

The existing image lease algorithm now has default token/origin/identity template
parameters. Grid callers keep `PageImageRuntime<N>` and the same fixed storage;
the modal specialization has exactly one slot. This reuses cancellation,
connection/request guards, retained completion, animation callbacks, exhaustion
and deferred pixel release rather than implementing another downloader. No
configuration strings, URL, context or widget pointers are added to a lease.

The modal origin delegates `valid` to `PageModalRuntime::valid`, which permits
binding during initialization before the overlay is visible. Animation
visibility additionally delegates to `interactive`; screen activity can retain
the admitted opening while hidden, with the scene supplying actual display
activity. Origin/transport must outlive the owner. The owner persists across
openings so its request nonce does not reset whenever a modal closes.

Use a separate `PageArtworkRequests<1>` over the existing modal image component;
the S3 two-/P4 six-component tile pool is unchanged. Component claiming rejects
accidental overlap. The expanded source adapter must explicitly use MODAL
priority and a bounded long-side dimension. Closing/replacing the modal detaches
its draw frame before unbinding. Unbind revokes the opening/request before any
external cancel can deliver, while draw-fenced surface release stays in safe
maintenance. New openings, including those reusing a context address, cannot
accept an old opening's result. Configuration generation zero and final nonces
remain valid; counters fail closed after exhaustion.

Host sanitizer tests exercise the real modal lifecycle and one-image owner,
including hidden-card admission, initialization before visible UI, independent
grid retirement, close/reconnect/configuration replacement, synchronous results,
reentrant cancellation/rebinding, final opening/request nonces and 1,000
open/close cycles per profile. A concrete downloader-adapter test uses separate
tile/modal components, checks MODAL priority, old component generations, animation
visibility, component overlap and deferred draw fences. Existing tile/cover-art
lease regressions, strict Linux GCC and actual S3/P4 SDK source checks supplement
these models. Focused native S3 and twenty-slot P4 image tests preserve the grid
path. Transport I/O and modal UI are doubles, not physical expanded-image or
memory qualification.

The [refresh handoff](pooled-image-refresh-handoff.md) now preserves fixed
activity/revision/schedule/cooldown state across visible tile and modal ownership.
The next source/UI stage must own its configuration strings,
apply modal sizing/headroom and guard all draw/UI work with the exact opening.
It must also handle a direct request for an image on a hidden page without a tile
lease. No new capacity, nesting, saved-document or backup behavior is enabled by
this ownership prerequisite.
