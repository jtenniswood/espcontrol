# Shared demanded-panel routing

`PageModalRouter` connects the individual panel adapters to one
`PageModalRuntime`. Production still selects the existing renderer. The
application scene, action routing, compiled legacy retirement and physical
qualification remain unfinished; this does not enable nested pages.

The application supplies a fixed table of panel kinds and existing family ports,
plus the scene's grid-origin validator. Install every family on the same runtime.
Families and the router must outlive that runtime. The router forwards the actual
context pointer without allocating a wrapper or adding per-card records. It keeps
the selected family until safe destruction returns; a delayed QR/image decoder
lease blocks collection and the next opening.

Admission and maintenance ask the selected family for its connection policy.
Wi-Fi sharing, Settings and cached weather retain their stable local epoch and
can open offline; HA-backed panels retain their captured connection epoch.
Explicit-purpose requests are still admitted by that family's existing policy.
Duplicate kind entries, null ports and ambiguous default panel choices fail
closed. Table order cannot choose which panel opens.

`PageModalRuntime::request(binding, kind)` adds explicit-purpose grid requests
while retaining the original full binding check. A scene can request a specific
panel without converting touch input into an authenticated hidden-card request.
The original binding and family connection are rechecked before opening. Opened
panels then use the independent configuration/card/kind/opening identity.

The router keeps readiness queries usable during safe destruction so a pending
climate edit can finish through the runtime's guarded closing-edit check.
Reentrant creation remains blocked until destruction returns, and the runtime
continues to serialize cleanup and replacement.

The host model switches through all 15 panel kinds 1,000 times per profile across
all nine profiles. It covers failed creation/initialization, reentrant close,
stale bindings, reconnects, offline local panels, a retained decoder/draw lease,
alarm takeover preservation and final climate-edit eligibility. Native real-LVGL
fixtures route every existing panel family through the same boundary, including
both alarm kinds sharing one family port and light/cover switching between two
real family ports. Existing bounded-memory, stale-input and draw-retirement
checks remain in those fixtures. These checks do not establish device heap,
touch/layout parity or complete application-scene ownership.
