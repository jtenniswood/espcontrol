# Pooled camera and image tiles

This stage of #1825 presents camera/static-image cards through the fixed grid
bank over the [shared image policy](pooled-image-tiles.md). Production still
uses the existing renderer. [Expanded-image request ownership](pooled-modal-image-ownership.md) now has a
typed lifetime; its source/UI, actual application scene,
compiled legacy retirement and physical qualification remain prerequisites for
cutover; configuration/backup formats and navigation depth are unchanged.

Image preparation resolves glyphs and decodes activity metadata once. Its five
option bytes fit the existing card option union; the decoded trigger shares the
otherwise unused duration-unit text view. The host card definition remains
264 bytes. Each visible card borrows the existing icon, label and generic draw
surface. No image widget, shadow label, per-card context, timer or subscription
is created during binding/navigation. Two S3 or six P4 leases remain shared with
media artwork, including images on child pages.

The generic surface draws a fit/fill frame only when read-only full binding,
policy readiness and asset ID match. It pulls the current component descriptor
for every draw, including animated frames. A second check after the image draw
rejects stale shadow work if a draw-task callback changes the page/source.
The configured label's one-pixel, half-opacity black shadow is a draw command
below the ordinary label rather than another widget. Detach clears the frame
before unbinding; the downloader drains queued draw work before pixel release.

Loading and unavailable cards reuse that surface as a full background. A
separate theme loading role preserves Light's secondary surface while fit
pixels use the sensor surface. The view preserves the loading/custom icon,
translated Configure/Loading/Unavailable/Too many captions, optional ready
name/icon overlays, friendly-name updates and raw-byte/NUL caption limits.
Unconfigured/unsupported cards consume no downloader. Every bind resets shared
surface styles and label geometry before another family uses the shell.

A press carries its original binding and HA connection generation. A valid
click emits an image-modal intent only when the source policy permits opening;
it does not create the modal inline. The actual modal adapter and its own
request lifetime are still required. Image `size` updates only mark policy
work; all URL, transfer and decode activity stays in maintenance outside paint.

`PageBasicActions::bound` is required for image and cover-art draw ownership.
It delegates to `renderer.bound`, which remains valid during bind/paint;
`valid` continues to guard input and navigation. Native tests explicitly check
that the read-only gate is reached while the input gate is closed. Other scene
adapters must supply both gates rather than routing drawing through `valid`.

Tests cover 576 prepared option combinations on each of nine profiles against
existing configuration helpers, including cached friendly-name reads. Native
LVGL tests use the actual renderer, graph, image policy and bounded lease pool
with transfer I/O/component pixels replaced. They cover placeholders, fit/fill,
overlays, Light colors, reconnect/old press rejection, image replacement inside
draw, child pages, late descriptors and 1,000 navigation cycles per profile
without C++ allocations, retained LVGL growth or registration growth. The draw
frame also matches LVGL image widgets in 27,648 pixel cases spanning fit/fill,
geometry, borders, rounding, tint, opacity and antialiasing, plus 1,000 retained
surface changes. Optional native CMake builds keep the pure image-policy test
outside the LVGL global support linkage.

Source/template SDK checks and host tests do not establish actual device font
roles, modal geometry, download/decode peaks or RAM savings. Before cutover,
qualify the S3 and a working twenty-slot P4 with matched heap/PSRAM, minimum
heap/largest block, navigation latency, reconnect/configuration replacement and
image workloads. Then compile the current factory matrix and check the other
profiles. P4-86 remains reserved for USB diagnosis.
