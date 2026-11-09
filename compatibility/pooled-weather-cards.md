# Pooled weather cards

This stage of #1825 supplies current-condition and forecast presenters, an owned
forecast request/cache runtime, and a demand-created detail panel. These adapters
remain unselected in production. It does not enable nested pages, change backup
formats or capacities, or establish device memory savings.

## Current conditions

Current conditions reuse the shared entity-state graph. Generated immutable
icon/label/alias maps come from the existing production helpers; run
`scripts/generate_weather_condition_maps.py` when those helpers change.
The bounded presenter preserves translated captions, alias/raw caption formatting
and icon choices without allocating strings during updates. Glyphs resolve once
into a shared table in each prepared external arena. Configured labels/icons
remain ignored as in the existing driver. Empty entities show Cloudy; stale
configured state shows Unavailable after reconnect.

## Forecast ownership and drawing

The runtime owns one snapshot per distinct entity and one pending request across
all pages. Existing limits remain eight sources, sixteen accepted forecast cards,
eight days per snapshot and 2,048 response bytes. Invalid or over-limit entries
keep their saved configuration and show Forecast unavailable, matching the
existing registration limits. Sources can share one request/cache entry.

Request tokens include configuration, connection and request generations. Retiring
a configuration invalidates pending responses immediately; transport cancellation
and protected arena release happen on safe loop maintenance. Entity names belong
to the source arena rather than the document being replaced. Previously successful
snapshots survive compatible replacements and remain available offline with the
Last forecast caption. Ready delay, timeout, retry and hourly refresh follow the
existing schedule. Activation neither allocates nor starts a network operation.

Each grid slot has one plain forecast drawing object. Prepared external storage
holds only enough drawing frames for the greatest number of forecasts visible on
one page. Layout and date selection share helpers with the existing widget
renderer. Drawing commands own bounded text, and queued LVGL tasks receive text
copies. Scaled drawing reserves clipped strips instead of full-card ARGB buffers.
Navigation creates no forecast labels, source registrations or requests. Drawing
scratch allocations are separate from retained grid memory.

The detail panel owns its source snapshot, fonts, configuration and gesture guards
for one opening. It preallocates six day panels, reuses them as forecasts change,
and releases the panel on close. It remains usable offline. Configuration
replacement invalidates input immediately and defers deletion until callbacks
finish. Legacy card/source registries must be retired before installing the new
panel port. The legacy renderer retains its existing dynamic panel behaviour.

## Integration and validation

Production integration must select one renderer and forecast scheduler, activate
all prepared owners together, and call forecast maintenance before presenter and
modal updates. Fonts and glyphs must outlive the selected runtime. Read-only
weather panels remain available offline even when action panels require a live
Home Assistant connection.

Host checks compare current-condition output with the production helpers,
including known aliases, Unicode and 10,000 varied inputs. Forecast drawing is
compared pixel-for-pixel with the existing LVGL widget renderer across all device
profiles, including scaled and large-icon layouts. Native fixtures exercise
1,000 navigation cycles per profile and 1,000 detail-panel open/close cycles,
reconnects, replacement during callbacks, response rejection and allocation
failures. They track retained allocations separately from rendering scratch.
These are automated checks, not physical display validation.

The S3 remains the first hardware canary after its remaining card families and
production integration are ready. Preserve unrelated installed changes, take a
fresh source/configuration backup, and measure matched before/after memory before
selecting this runtime for other displays.
