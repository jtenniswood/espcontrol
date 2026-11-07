# Pooled camera and image policy

This stage of #1825 adds a camera/static-image policy owner over the existing
[shared image leases](pooled-image-ownership.md). It does not select the pooled
renderer in production, change saved configuration or enable nested pages.
The [image tile view](pooled-image-tile-views.md) now adds bounded presentation;
the expanded modal, AppScene integration and physical memory/image qualification
are still required.

`PageImageTileRuntime<N>` keeps one fixed policy record per downloader: two on
S3, six on P4, sharing the same `PageImageRuntime<N>` as media cover art. The
renderer passes immutable prepared entity/trigger metadata; it must unbind the
lease before that metadata's arena retires. Bind, unbind, navigation and graph
notifications allocate no strings, widgets, timers or subscriptions and start
no transfers. URL construction, retry decisions and image requests run only
in loop maintenance, after configuration owners have activated.

Camera and image proxy requests use the current graph access token. A picture
attribute event can select its reported URL; canonical proxy selection resumes
on primary/token updates. A public picture is a fallback when a proxy cannot
be constructed. Invalid/missing sources retry at two-second intervals, with the
existing 45-second initial loading grace. Tokens/pictures are bounded to
512/4096 raw bytes and never logged by this owner. Image timestamps and activity
baselines use fixed 80-byte storage, preserving embedded NULs and the existing
invalid-state, connection and revision-wrap rules.

Image revisions coalesce during an in-flight transfer. Completing that transfer
acknowledges its requested revision, leaving any newer revision pending. Camera
refresh reuses the existing periodic/activity schedule, including its separate
initial fetch, activity baseline and five-second cadence. Hidden activity is
not replayed. Expanded state defers tile work; this is an admission hook, not
an implementation of the expanded image modal. Ordinary failures back off at
2/4/8/16/30 seconds and hide the old frame; scheduled refresh failures retain a
healthy completed image. Memory admission has a two-second cooldown even when
HA continues reporting source changes.

Unavailable primary state cancels current work while retaining pixel ownership,
but the policy hides that image until a successful recovery request. Geometry
or HTTP endpoint changes invalidate readiness before cancelling/rebinding.
Reconnect and full configuration/binding tokens reject stale results; request
ID exhaustion fails closed. Cancellation can synchronously deliver or replace
the scene, so records are invalidated before external calls and checked after
reentry. Pixel release remains deferred to image maintenance after detachment.
The future view must validate current binding, policy readiness and asset ID
before dereferencing a draw descriptor.

Camera requests carry TILE priority and an explicit maximum source dimension.
Proxy request dimensions preserve aspect ratio within the device's configured
long-side cap; decode dimensions remain the tile's actual size. Fit requests
carry the current background and invalidate on its change. Existing cover-art
callers retain COVER_ART priority and their previous dimension defaults.

Read-only image ownership must use `PageRendererRuntime::bound`, which checks
full identity during bind/paint as well as ordinary maintenance. User input and
navigation continue to use `valid`, which also rejects renderer reentry. The
future view's draw validation needs the same read-only ownership gate; using
the input gate during painting would suppress legitimate image drawing.

Host tests compare 600,000 activity/revision operations with the existing camera
policy, exercise the actual entity graph and renderer, and navigate 1,000 times
on each of the nine profiles without C++ allocation or channel growth. They
cover shared cover-art lease exhaustion, URL/token selection, revision
coalescing, memory admission, refresh/retry timing, late and synchronous results,
reconnect, fit/size/endpoint changes, request exhaustion and reentrant
configuration replacement (private generation UINT32_MAX to published zero).
Sanitizers, strict Linux GCC and actual S3/P4 SDK source checks supplement these
models. These are not factory builds, physical UI tests or measured RAM savings;
no display is flashed for this policy stage.
