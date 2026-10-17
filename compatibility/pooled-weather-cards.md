# Pooled weather cards

Weather migration is in progress for #1825. The current-condition presenter is
implemented; forecast sources, forecast drawing and the demand-created forecast
modal remain required before the weather family is complete or the S3 canary can
use its representative saved configuration. Production selection and nesting
remain unchanged.

Current conditions reuse the shared state graph and existing short-state cap.
Generated immutable icon/label/alias maps come from the legacy production helpers;
run scripts/generate_weather_condition_maps.py when those helpers change.
The bounded presenter preserves canonical translated captions, alias/raw caption
formatting and icon choices without allocating strings during updates. Glyphs
resolve once into a shared table inside the prepared external arena, rather than
once per card or on navigation. Configured current-weather labels/icons remain
ignored as in the existing driver. Empty entities show the existing Cloudy
placeholder. A configured entity whose shared cache is stale shows Unavailable,
consistent with invalidating other pooled state after reconnect.

Pure parity tests compare production helpers on all known conditions/aliases,
Unicode captions and 10,000 varied inputs, and assert no new-presenter C++
allocations. Shipped translations are checked against the fixed ordinary label
capacity (current maximum 52 UTF-8 bytes, capacity 64). Pure card tests run for
all profiles with forecast support enabled and disabled. Native LVGL fixtures
run 1,000 current-weather page cycles per profile with state changes, a real
rendered frame, fixed retained memory/channel/descriptor counts and no new glyph
resolution. Factory/complete suite qualification belongs to the completed branch.

The next part uses generation/connection/request tokens for bounded forecast
sources. Reusable drawing data can represent forecast columns without creating
per-card LVGL labels, while matching the existing layout/translation/font rules.
Response and draw-task ownership must remain safe across configuration retirement.
No device has been flashed. Preserve installed S3 speaker/reliability changes in
later integration and leave P4-86 hardware untouched.
