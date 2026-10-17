# Card code review — 17 October 2026

## Summary

The card system has useful foundations: a shared product contract, browser/firmware configuration-parity checks, family-level firmware drivers, and targeted runtime tests. The main weaknesses are lifecycle ownership, failure handling, and differences between the configuration preview and live device behaviour.

This review identifies **nine actionable defects: three high priority (P1) and six medium priority (P2)**. Fix the local-sensor lifetime bug, unsafe Cover Stop fallback, and silent relay reassignment first. Five further recommendations address integration risks and maintainability. No production fixes are included in this report.

### Scope and confidence

- Reviewed baseline: `76a90c0a9cc51ebe870d075496b5f02190f64627`, fetched from `origin/main` on 17 October 2026.
- Inventory: **44 declared types and three runtime/legacy aliases**, cross-checked against [the runtime inventory][inventory] and [the generated coverage baseline][coverage]. Hidden variants and accepted old configurations are included.
- Followed the editor registration, relevant option/normalization helpers, shared save/parse boundary, firmware driver, and supporting state/action/lifecycle code for every type. Larger media, image, climate, and slider implementations received targeted review of the critical paths described below; this was not an exhaustive line-by-line verification of every helper.
- **Reproduced** means production functions were executed with host-side display/network substitutes. **Source-traced** means the relevant control flow was inspected. Neither means the behaviour has been confirmed on a physical panel.
- Existing tests passing does not establish that a card has no bugs. The coverage table records the result of this investigation, including where more device/integration testing is needed.

All firmware recommendations concern shared code and therefore potentially affect every display using the relevant card, including main-grid and subpage configurations unless stated otherwise.

## Required fixes

### F1 — P1: Remove local Sensor registrations when their widgets are retired

**Affected:** Sensor in local-device mode; old `local_sensor` configurations; main grid and subpages.

**Evidence:** [Registration and cleanup][fw-sensor], particularly `sensor_driver_cleanup` at line 113 and `sensor_driver_register_local_value` at line 121; [local update dispatch][fw-local] at lines 126–147.

Registration stores raw pointers to labels and their owner. It only removes an earlier registration when another local Sensor is registered with the same owner. Cleanup does nothing, and local updates do not check or release the stored pointers. There is no other registry-clear/unregister path in the repository. Removing the card, replacing it with another type, or deleting its subpage can therefore leave callbacks pointing at retired widgets.

**Validation:** Executed production registration, cleanup, and update functions. Cleanup left one registration and a later update wrote to a retired label. A separate AddressSanitizer run that deleted the label after cleanup reproduced **heap-use-after-free**. This demonstrates the lifetime defect with host substitutes; no device crash was induced.

**Recommended fix:** Unregister local consumers before visual reset and on owner deletion, independently of permanent ESPHome sensor callbacks. Keep the callbacks keyed by sensor, but let them update only currently registered consumers. Make unregistering idempotent.

**Effort:** Medium. Preserve saved card formats and source keys. Test removal, replacement with an HA Sensor/another card, subpage deletion, and repeated rebuilds while numeric and text updates continue. Acceptance: no stale writes, stable registry size, and sanitizer-clean execution.

### F2 — P1: Do not turn a failed Cover Stop into a toggle command

**Affected:** Cover command mode `stop`, Gate Stop, and other callers of the shared normal-cover Stop sender, including the cover modal.

**Evidence:** [Cover action sender][fw-actions], lines 458–500, especially the response callback at lines 486–490.

A failed `cover.stop_cover` response triggers `send_toggle_action(entity_id)` regardless of the error. Stop and toggle have different meanings. A toggle can start or reverse movement; the failure response does not establish that the cover is still moving or that toggle is a valid substitute. The callback also lacks a configuration-generation check.

**Validation:** Executed the production sender with a simulated failed response. The recorded sequence was `cover.stop_cover` followed by `homeassistant.toggle`.

**Recommended fix:** Remove the automatic toggle fallback. Surface/log a bounded failure and keep Stop semantically limited to stopping. If particular hardware needs toggle-to-stop compatibility, require an explicit, documented option and validate fresh motion state before using it. Prevent late responses from acting on retired configurations.

**Effort:** Small for removing the fallback; medium for any explicit compatibility mode. Existing saved cards can stay unchanged. Test success, unsupported Stop, permission failure, timeout, disconnection, and a response arriving after the card is edited. Acceptance: none of these failures sends an unsolicited movement command.

### F3 — P1: Do not silently change an existing Internal Relay card to the first relay

**Affected:** Internal Relay cards with an unavailable or unrecognised saved relay key, especially imported configurations or changed device profiles.

**Evidence:** [Internal Relay editor][e-internal], `ensureInternalRelaySelection` at lines 31–40, called during rendering at lines 42 and 102.

If the saved key is absent from the current relay catalogue, simply rendering the editor changes the draft to the first available relay. Saving an unrelated label/icon change can then target a different circuit. Defaulting a new card is reasonable; silently reassigning an existing one is not.

**Validation:** Loaded the production TypeScript registration and option service. Rendering a saved `relay_2` card against a catalogue containing only `relay_1` changed the draft entity to `relay_1` without a selection event.

**Recommended fix:** Apply first-relay defaults only when creating a new card. Retain unavailable saved keys in an explicit unavailable state and require the user to choose a replacement before saving changes that would activate that card.

**Effort:** Small. No schema migration is needed. Test a missing key, an initially empty catalogue, a later catalogue refresh, and an imported card from a device with different relay names. Acceptance: opening an existing editor never silently changes the controlled relay.

### F4 — P2: Only close the Climate modal belonging to the unavailable entity

**Affected:** `climate` and `climate_control` on layouts with multiple thermostat entities.

**Evidence:** [Climate state subscription][fw-climate], lines 2713–2723. Context-specific cleanup already uses an ownership check at line 2270.

Every climate state callback calls the global `climate_control_hide_modal()` when its own entity becomes unavailable. It does not check whether that context owns the open modal. An unavailable hallway thermostat can therefore dismiss controls for the working kitchen thermostat.

**Validation:** Used the repository's production-callback generator with two thermostat contexts. An unavailable update for the second entity invoked the global modal-close boundary while the other entity was being controlled. The host harness observes the close call; it does not render the full panel UI.

**Recommended fix:** Close only when `climate_control_modal_ui().active == ctx`, and handle that context's pending temperature work consistently.

**Effort:** Small. No configuration change. Test independent entities on both surfaces; the active thermostat must remain open when a different one fails, and close when its own entity fails.

### F5 — P2: Clear invalid local Sensor readings instead of retaining the last valid value

**Affected:** Numeric Sensor in local-device mode and `local_sensor` compatibility input.

**Evidence:** [Local numeric updates][fw-local], lines 126–138, and initial rendering in [the sensor driver][fw-sensor], lines 156–162.

`local_sensor_apply_value` returns immediately for NaN. After a valid reading, a sensor failure therefore leaves that old number on screen with no indication that it is stale. Infinity is not rejected either. HA numeric Sensor handling already checks for finite values and displays a placeholder.

**Validation:** A valid `23.5` followed by NaN continued to display `23.5` in the production update function.

**Recommended fix:** Treat all non-finite values as unavailable, update matching cards to `--`, and distinguish “registered but invalid value” from “no registered consumer.” Use the same policy during initial binding and later updates.

**Effort:** Small. No migration. Test valid → NaN → valid, infinity, startup without a reading, and multiple cards bound to the same local sensor.

### F6 — P2: Preserve custom robot icons after state updates

**Affected:** State-bound Vacuum and Lawn Mower modes.

**Evidence:** [Vacuum state rendering][fw-vacuum], lines 117–125; [mower state rendering][fw-mower], lines 101–109; custom-icon controls in [Vacuum][e-vacuum] and [Lawn Mower][e-lawn_mower].

Setup honours the configured icon, but the first state update always replaces it with a state-derived default. The contexts do not retain enough information to distinguish a selected custom icon from automatic icon behaviour. The editor preview consequently promises a choice the panel discards. Command-only modes without state binding do not all exhibit this problem.

**Validation:** Executed both production state renderers with a custom icon already displayed. A normal `docked` update replaced it in both cases.

**Recommended fix:** Store the configured-icon policy in each runtime context. Use state-derived icons only for automatic/default selection, preserving existing automatic behaviour and intentional custom choices. Also add the mower equivalent of Vacuum's live translated-text refresh, or explicitly document when language changes take effect; the shared cleaning driver's refresh currently handles Vacuum only.

**Effort:** Small to medium. Preserve saved values; do not require users to reselect icons. Test default/custom icons in each state-bound mode and on both surfaces, plus language changes on an already visible mower card.

### F7 — P2: Do not display an Option Select change when dispatch fails

**Affected:** Action card's select/input-select mode and legacy `option_select` cards.

**Evidence:** [Option Select action sender][fw-select], lines 212–220; [shared HA send result][fw-ha], lines 286–293.

The sender ignores the boolean result of `ha_send_entity_action` and immediately changes `current_option` and the displayed text. A disconnected transport or low-memory rejection can therefore show “Away” even though the panel never sent the command. Service rejection after a successful send is a further acknowledgement gap.

**Validation:** A host substitute returning `false` from the HA sender still changed the production context from `Home` to `Away`.

**Recommended fix:** Retain the last confirmed option when sending fails. Prefer a pending indicator after a successful dispatch, then confirm from HA state or a response; restore the prior value with a clear failure on timeout/rejection. Avoid leaving a false confirmed state.

**Effort:** Small for dispatch-failure handling; medium for acknowledged pending state. No saved-format change. Test failed dispatch, failed service, successful echo, delayed echo, and two cards controlling the same select entity.

### F8 — P2: Remove secrets from Webhook URL logs

**Affected:** Webhook on all generated device configurations.

**Evidence:** [The device YAML generator][generator], lines 501–514. The editor itself suggests a query-string API URL in [Webhook settings][e-webhook].

The generated sender logs the full URL on start failure, HTTP failure, and success. A webhook identifier, query-string token, API key, or URL credentials can therefore appear in ordinary device logs and subsequently in shared diagnostics. The higher-level action sender avoids logging the URL, but the generated sender adds it back.

**Validation:** Source-traced all three logging branches in the generator and confirmed corresponding generated device senders contain them. No live webhook or real credentials were used.

**Recommended fix:** Log a card/request identifier and status instead. If an address is useful, sanitise it with a conservative policy that strips user information, query strings, and potentially secret path components. Change the generator and regenerate device outputs together.

**Effort:** Small. No user migration. Test synthetic secret-bearing URLs through success and both failure paths and assert that no secret reaches captured logs.

### F9 — P2: Give unavailable status sensors a visibly unknown state

**Affected:** Door/Window, Presence, and the icon-only HA Sensor presentation.

**Evidence:** [Status-entity subscription][fw-status], lines 127–150; [icon Sensor subscription][fw-subscriptions], lines 226–244.

An unavailable/unknown state is converted to inactive, and the normal inactive icon is shown. Door/Window therefore looks closed and Presence looks clear when the input is unavailable. These paths do not add a separate unavailable indicator. This is a presentation defect rather than proof that an action is dispatched incorrectly.

**Validation:** Source-traced the explicit `!unavailable && ...` branch and its inactive-icon fallback. The normal inactive and unavailable states use the same visible representation.

**Recommended fix:** Keep unavailable distinct from inactive. Use a disabled/unknown treatment or an explicit placeholder indicator while preserving the configured icons and colours when valid data returns. Ensure informational cards remain informational during recovery.

**Effort:** Small to medium. This deliberately improves visible behaviour; explain it in release notes. Test open/detected → unavailable → closed/clear and the reverse, including missing startup data. Acceptance: unavailable data cannot be mistaken for a confirmed closed/clear state.

## Further recommendations

These are code-quality improvements or integration risks, separate from the nine required fixes.

### R1 — Medium: Sequence playlist source selection and playback

In [the playlist sender][fw-actions] at lines 775–823, `media_player.play_media` is dispatched immediately after `select_source`; the source response only logs the result. There is no dependency between successful source selection and playback. **The ordering risk is source-confirmed; an incorrect destination has not been reproduced against a real integration.**

For players that take time to change source, use a bounded operation: select the source, confirm the response/state appropriate to that integration, then play. Abort or give a clear error on failure. Capture configuration generation and cancel on teardown. Keep existing playlist settings and add delayed/failing-source integration tests before changing the flow. Effort: medium.

### R2 — Medium: Give Webhook a bounded request lifecycle and visible outcome

[The click path][fw-actions] at lines 255–270 calls the sender directly and discards its result. [The generated sender][generator] opens HTTP directly; [the shared configuration][core-infra] permits a 10-second timeout. **No slow-server timing experiment was run**, so the actual panel stall duration remains unmeasured.

Measure a delayed endpoint, then move blocking network work off the touch-event path using an ESPHome-compatible asynchronous/worker design. Schedule display updates back on the UI thread. Bound concurrency and queue size, prevent repeated taps from stacking requests, and show pending/success/failure without logging secrets. Simply delaying the same blocking call on the main loop would not solve a stall. Preserve HTTP methods, bodies, and headers. Effort: medium.

### R3 — Medium: Define one explicit availability policy for actionable card families

Availability handling differs substantially: Gate uses `apply_control_availability`, Fan guards actions through feature checks, Timer checks connection/state, while Garage, Lock command mode, normal Cover commands, and some robot actions have different guards. For example, [Vacuum dispatch][fw-vacuum] does not reject its stored unavailable state, while [mower dispatch][fw-mower] does; command-only robot modes can lack a state subscription entirely. [Toggle subscriptions][fw-subscriptions] accept a `disable_interaction` parameter that is not used.

Define a small shared policy for never-seen, valid, unavailable, disconnected, and pending states. Distinguish commands that need a known state from commands that can validly run without one, especially button entities and local controls. Adopt it one family at a time with behaviour tests; avoid disabling every card indiscriminately or changing saved configurations. Effort: medium.

### R4 — Medium: Strengthen card interfaces and test observable lifecycle behaviour

[CardDefinition][registry] and [ControlsFieldsFeature][fields] expose most callbacks, metadata, and results as `any`. Renderers often both normalise drafts and build controls. This allows type checking to pass while behaviour such as F3 remains unsafe. Existing [runtime guards][runtime-check] and [coverage output][coverage] are useful, but primarily checking code shape, registration, or data parity does not exercise late callbacks and deleted widgets.

Introduce typed settings helpers and preview results, and typed family-specific option/config views around the existing saved fields. Separate pure normalisation/defaulting from rendering. Keep the compact wire format and generated compatibility adapters stable. Start with Internal Relay and local Sensor as representative cases, then expand after parity comparisons pass. Add executable teardown, card-replacement, failure, and out-of-order-update tests for each family; run lifetime-sensitive C++ tests with AddressSanitizer. Effort: large, delivered in small family-level changes. A wholesale card-system rewrite is not justified by this audit.

### R5 — Low: Validate script fields before saving, and keep host checks portable

[Script field validation][fw-actions] at lines 53–105 accepts names starting with digits and inserts them directly as keyword identifiers into a Jinja expression. For example, the production functions accept `1mode` and generate `{{ dict(1mode=1mode) }}`. The editor stores the text without equivalent field-level validation. **Generation was reproduced; end-to-end Home Assistant/Jinja rejection was not tested.** Prefer structured variables or proper dictionary-key construction, validate allowed names/duplicate keys, and show errors for ignored/malformed lines. Preserve valid existing scripts. Effort: small to medium.

Several host test doubles also fail Apple Clang's `-Werror` checks because of unused constants. Mark unused test-only constants appropriately or use narrowly scoped compiler settings, keeping warnings strict for production code. This review ran the affected assertions with only that warning suppressed, as recorded below. Effort: small.

## Per-card coverage

Every declared type has its own row below, including variants sharing one implementation. “No additional defect” means none was established in the inspected paths; it does not mean the card has been certified bug-free. The final column is the recommended focused regression/device test, not a claim that it was performed.

| Type | Editor / firmware driver | Review result | Focused test still recommended |
|---|---|---|---|
| `timer` | [Editor][e-timer] / [driver][d-timer] | No additional defect established; owned timers/subscriptions and confirmation were inspected. | Start, resume, confirmed cancel, completion and delete while armed. |
| Default Switch (`""`) | [Editor][e-switch] / [driver][d-basic] | R3: inconsistent unavailable-state treatment; confirmation and button dispatch inspected. | State changes during confirmation; reconnect with active sensor overlay. |
| `action` | [Editor][e-action] / [driver][d-basic] | F7 in select mode; R5 in script fields; local dispatch and numeric actions inspected. | Select rejection, script fields, local actions and numeric-domain validation. |
| `vacuum` | [Editor][e-vacuum] / [driver][d-cleaning] | F6: custom icon overwritten in state-bound modes; R3 availability. | Status/start-stop/start-dock/pause, command-only modes, area ID, unavailable entity. |
| `lawn_mower` | [Editor][e-lawn_mower] / [driver][d-cleaning] | F6: custom icon overwritten; inspect translated-text refresh and R3. | Status/start/dock/pause, unavailable entity and live language change. |
| `alarm` | [Editor][e-alarm] / [driver][d-alarm] | No additional defect established in PIN, action-state, takeover and context cleanup paths. | Wrong PIN, arm delay, disarm, multi-alarm layout and rebuild while PIN dialog is open. |
| `alarm_action` | [Editor][e-alarm] / [driver][d-basic] | Hidden/legacy action variant reviewed; no additional confirmed defect. | Arm/disarm PIN defaults, inherited configuration and subpage deletion. |
| `calendar` | [Editor][e-calendar] / [driver][d-date] | No additional defect established; global date/time fallback and registration inspected. | Date/datetime, midnight, unsynchronised time, rebuild and 12-hour format. |
| `clock` | [Editor][e-clock] / [driver][d-date] | No additional defect established; local-time updates and large layouts inspected. | 12/24-hour display, loss of time sync and full-width alignment. |
| `climate` | [Editor][e-climate] / [driver][d-climate] | F4; retained compatibility type normalises to climate_control. | Two thermostat entities, one unavailable; round-trip old configuration. |
| `climate_control` | [Editor][e-climate] / [driver][d-climate] | F4; target/capability guards and owned cleanup inspected. | Dual/single targets, capability changes, unavailable updates and pending edits. |
| `cover` | [Editor][e-slider] / [driver][d-access] | F2: failed Stop can toggle; R3 availability. Modal, position, tilt and commands traced. | Stop failures, supported features, position/tilt limits and delayed responses. |
| `door_window` | [Editor][e-door_window] / [driver][d-status] | F9: unavailable is visually indistinguishable from closed. | Door/window subtype, custom icons, active colour and unknown → valid recovery. |
| `presence` | [Editor][e-presence] / [driver][d-status] | F9: unavailable is visually indistinguishable from clear. | Detected/clear/unknown/unavailable and restored active colour. |
| `fan_direction` | [Editor][e-fan] / [driver][d-numeric] | No additional defect established; feature/state guards inspected. | Forward/reverse, missing capability and state change with modal open. |
| `fan_oscillate` | [Editor][e-fan] / [driver][d-numeric] | No additional defect established; feature/state guards inspected. | True/false/missing oscillation and unsupported entities. |
| `fan_preset` | [Editor][e-fan] / [driver][d-numeric] | No additional defect established; option parsing and modal ownership inspected. | Quoted/mixed-case preset names, changing option list and unavailable entity. |
| `fan_speed` | [Editor][e-fan] / [driver][d-numeric] | R3: align slider availability/capability policy with all-controls view. | Percentage updates during touch, speed steps, off and unavailable. |
| `fan_control` | [Editor][e-fan] / [driver][d-fan] | No additional defect established; tabs, optional light, context cleanup inspected. | All supported tabs, optional light independent of fan and rebuild while open. |
| `fan_switch` | [Editor][e-fan] / [driver][d-basic] | No additional defect established; uses guarded fan actions and shared state. | On/off, alternative icon, unavailable state and subpage indicator. |
| `garage` | [Editor][e-garage] / [driver][d-access] | R3: state/control availability differs from Gate; confirmation dispatch inspected. | Toggle/open/close, both confirmations, moving/unavailable states. |
| `gate` | [Editor][e-gate] / [driver][d-access] | F2 through the shared Stop sender; availability handling is a useful reference for R3. | Open/close/stop failures, unknown state and parent indicator. |
| `internal` | [Editor][e-internal] / [driver][d-basic] | F3: rendering can silently reassign an unavailable saved relay. | Missing relay key, switch/pulse, catalogue changes and imported device profile. |
| `light_brightness` | [Editor][e-slider] / [driver][d-numeric] | R3 availability; percentage conversion, zero/off action and cleanup inspected. | Zero/off, 1/100%, feedback during drag and repeated card replacement. |
| `light_switch` | [Editor][e-switch] / [driver][d-basic] | R3 availability; shares switch confirmation and state-overlay paths. | On/off confirmations, icon pair and active text/numeric overlay. |
| `light_temperature` | [Editor][e-light_temperature] / [driver][d-numeric] | No additional defect established in inspected paths; R3/R4 apply. | Configured Kelvin endpoints, off/on recovery, dragging feedback and invalid values. |
| `light_control` | [Editor][e-light_temperature] / [driver][d-light] | No additional defect established in inspected modal/state/cleanup paths. | RGB-only, temperature-only and dimmable lights; capability and tab changes. |
| `lock` | [Editor][e-lock] / [driver][d-access] | R3 availability; verified toggle defaults to lock unless known locked. | Locked/unlocked/jammed/unknown plus explicit lock/unlock modes. |
| `media` | [Editor][e-media] / [driver][d-media] | R1: playlist source/play ordering risk; R4 lifecycle tests. No new concrete defect established in other inspected modes. | Controls/group/volume/progress/transport/now-playing/artwork/playlist, rerouting and delayed source changes. |
| `option_select` | [Editor][e-action] / [driver][d-numeric] | F7; accepted legacy input normalises to Action select mode. | select.* and input_select.*, unknown current option and failed send. |
| `push` | [Editor][e-push] / [driver][d-basic] | No additional defect established; main/subpage event payload and ownership inspected. | One event per press, label payload and offline/reconnect. |
| `screensaver` | [Editor][e-screensaver] / [driver][d-basic] | No additional defect established; local action callback and fixed presentation inspected. | Manual start from both surfaces, wake and interaction with screen lock. |
| `screen_lock` | [Editor][e-screen_lock] / [driver][d-basic] | No additional defect established; shared PIN state and registry cleanup inspected. | Saved PIN requirement, locked/unlocked labels, rebuild and screensaver-on-lock. |
| `webhook` | [Editor][e-webhook] / [driver][d-basic] | F8: secret-bearing URL logs; R2: request lifecycle and result handling. | Methods, body/headers, slow/failing endpoint, repeated taps and log redaction. |
| `sensor` | [Editor][e-sensor] / [driver][d-sensor] | F1/F5 in local numeric/text modes; F9 in HA icon mode. Other HA display paths inspected. | Numeric/text/icon/time, local ↔ HA switching, invalid values and owner deletion. |
| `local_sensor` | [Editor][e-sensor] / [driver][d-sensor] | F1/F5 through compatibility normalisation to Sensor local mode. | Legacy load/save and numeric/text callback teardown. |
| `slider` | [Editor][e-slider] / [driver][d-numeric] | No additional numeric-slider defect established; metadata validation and snapping tested. R3 for percentage entities. | number/input_number ranges, decimal steps, units and missing metadata. |
| `subpage` | [Editor][e-subpage] / [driver][d-navigation] | Child lifecycle exposes F1; parent state aggregation/navigation ownership inspected. | Repeated open/delete/recreate, child updates, presets and parent indicators. |
| `timezone` | [Editor][e-timezone] / [driver][d-date] | No additional defect established; hidden World Clock mode and timezone conversion inspected. | Different zones, daylight-saving boundary, local timezone change and lost time sync. |
| `weather` | [Editor][e-weather] / [driver][d-weather] | No additional defect established in current/today/tomorrow/daily paths; bounded queue and generation guards inspected. | Multiple forecast sources, missing forecast/permission, retries and midnight rollover. |
| `image` | [Editor][e-image] / [driver][d-image] | No additional defect established in targeted cache/refresh/reset paths at this baseline. Complex image pipeline needs hardware stress testing. | Still/GIF, periodic/activity refresh, source edit, slow download, modal fit and low memory. |
| `wifi_qr` | [Editor][e-wifi_qr] / [driver][d-wifi] | No additional defect established; credential codec, tab state and guest-switch lifecycle inspected. | Escaped/non-ASCII credentials, open/hidden networks, guest failure and reconnect. |
| `wifi_qr_card` | [Editor][e-wifi_qr] / [driver][d-wifi] | No additional defect established; shares credentials/modal lifecycle with tile QR sizing. | Small/tall/wide QR scan, rotation and guest-control confirmation/failure. |
| `weather_forecast` | [Editor][e-weather_forecast] / [driver][d-weather] | Legacy forecast type uses the shared Weather runtime; no additional alias defect established. | Legacy load/save, tomorrow selection, absent forecasts and modal opening. |

### Additional runtime/legacy aliases

| Runtime alias | Code reviewed | Outcome / next test |
|---|---|---|
| `local` | [Action editor][e-action], [basic driver][d-basic], [saved-config normalisation][normalizer] | Accepted older local-action form becomes Action/local. No additional alias defect established; verify offline local dispatch after import. |
| `text_sensor` | [Sensor editor][e-sensor], [sensor driver][d-sensor], [saved-config normalisation][normalizer] | Accepted old text-sensor form becomes Sensor/text. Verify old entity/state mappings and escaped text after import. |
| `media_cover_art` | [Media compatibility registration][e-media], [media driver][d-media] | Hidden registration normalises to Media/cover_art. Retain the adapter; test saved alias import and primary/secondary artwork routes. |

## Suggested delivery order and device testing

1. **Stability and command intent:** F1, F2, F3. Keep these as separate fixes/PRs with small reproductions and no saved-format changes. On the display used for validation, repeatedly replace/delete local cards while sensors publish; test Cover failure using a simulated entity before trying moving hardware; verify an imported unavailable relay remains unassigned until deliberately selected.
2. **Trustworthy live state:** F4, F5, F7, F9. Add negative-state and multi-entity tests. Validate reconnection, unavailable data, rejected actions, and recovery on both main grid and subpages.
3. **User choices and diagnostics:** F6 and F8, then R1/R2 after focused integration/timing tests. Compare preview and real icons; capture only synthetic diagnostic secrets.
4. **Incremental quality work:** R3–R5. Characterise current valid behaviour first, migrate one family at a time, retain aliases/config parity, and keep rollback points at each PR.

For runtime fixes, start on a P4 display and an S3 display with constrained memory, then cover the supported device matrix required by the changed code. Test small/wide/tall cards, repeated navigation, open-modal rebuilds, reconnects, and language/theme changes where relevant. Image/media work additionally needs memory-pressure and slow-image checks. This report itself changes no firmware and requires no flashing.

## Checks performed and limits

| Check | Result |
|---|---|
| Inventory/coverage reconciliation | 44 declared types + three runtime aliases accounted for below; current generated coverage baseline has 96 cases. |
| Browser unit suite | **178 passed, zero failed**. The `check_tasks.py` wrapper subsequently failed while writing its shared Git-directory cache due to filesystem permissions; the test runner itself completed successfully. |
| TypeScript `tsc --noEmit` | Passed. |
| Direct `check_saved_config_parity.js` | **281 browser/firmware inputs passed**. |
| Direct `check_firmware_card_runtime.py` | Passed. This is a structural guard, not a complete runtime test. |
| Direct `generate_card_runtime_coverage.js --check` | Passed. |
| Existing strict C++ host tests | 13 passed: grid foundations; climate target/state/subscription policy; media volume/power/metadata policy and slider lifecycle; numeric sliders; daily weather parsing; guest Wi-Fi state; Wi-Fi QR codec; camera-refresh policy. |
| Existing strict runtime script tests | Remote modal, guest Wi-Fi lifecycle, and media-tab runtime tests passed. |
| Apple Clang warning adaptation | Timer card, daily-weather UI, Cover confirmation, Timer subpage cleanup, camera-refresh runtime, and fan-label tests passed after suppressing **only** `-Wunused-const-variable`. Original strict invocations failed on unused constants in host test doubles. Repository sources were not modified. |
| Review-specific reproductions | Production functions demonstrated F1–F7; F1 also produced an AddressSanitizer heap-use-after-free. F8/F9 were source-traced. R5's accepted invalid-looking field/template generation was executed, but no HA/Jinja parser was available locally. |
| Dependency-aware generated checks | The wrapper invocations for runtime, coverage, and parity were blocked by their shared generated prerequisite trying to fetch pinned MDI CSS; DNS/network access failed. Their underlying direct checks above passed against committed generated files. This is not a full generated-output pass. |
| Full CMake firmware test suite | Not run; CMake was unavailable. Targeted tests were compiled/executed directly as listed above. |
| Full ESPHome device compilation, browser visual smoke tests, live HA integrations, physical displays | Not run. Host checks cannot establish panel appearance, timing, memory behaviour, or physical device safety. |

The direct checks and reproductions used the committed source at the pinned baseline. Test binaries, compiler wrappers, and temporary harnesses were kept outside the repository; the report is the only intended tracked change.

[core-infra]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/common/device/core_infra.yaml#L155
[coverage]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/compatibility/fixtures/card_runtime_coverage.md#L1
[d-access]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_access_cover_driver.h#L1
[d-alarm]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_alarm_driver.h#L1
[d-basic]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_basic_action_driver.h#L1
[d-cleaning]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_cleaning_driver.h#L1
[d-climate]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_climate_control_driver.h#L1
[d-date]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_date_time_driver.h#L1
[d-fan]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_fan_control_driver.h#L1
[d-image]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_image_driver.h#L1
[d-light]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_light_control_driver.h#L1
[d-media]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_media_driver.h#L1
[d-navigation]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_navigation_driver.h#L1
[d-numeric]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_numeric_selectable_driver.h#L1
[d-sensor]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_sensor_driver.h#L1
[d-status]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_status_entity_driver.h#L1
[d-timer]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_timer_driver.h#L1
[d-weather]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_weather_driver.h#L1
[d-wifi]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_wifi_qr_driver.h#L1
[e-action]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/action.ts#L1
[e-alarm]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/alarm.ts#L1
[e-calendar]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/calendar.ts#L1
[e-climate]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/climate.ts#L1
[e-clock]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/clock.ts#L1
[e-door_window]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/door_window.ts#L1
[e-fan]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/fan.ts#L1
[e-garage]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/garage.ts#L1
[e-gate]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/gate.ts#L1
[e-image]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/image.ts#L1
[e-internal]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/internal.ts#L1
[e-lawn_mower]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/lawn_mower.ts#L1
[e-light_temperature]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/light_temperature.ts#L1
[e-lock]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/lock.ts#L1
[e-media]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/media.ts#L1
[e-presence]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/presence.ts#L1
[e-push]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/push.ts#L1
[e-screen_lock]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/screen_lock.ts#L1
[e-screensaver]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/screensaver.ts#L1
[e-sensor]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/sensor.ts#L1
[e-slider]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/slider.ts#L1
[e-subpage]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/subpage.ts#L1
[e-switch]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/switch.ts#L1
[e-timer]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/timer.ts#L1
[e-timezone]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/timezone.ts#L1
[e-vacuum]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/vacuum.ts#L1
[e-weather]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/weather.ts#L1
[e-weather_forecast]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/weather_forecast.ts#L1
[e-webhook]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/webhook.ts#L1
[e-wifi_qr]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/cards/wifi_qr.ts#L1
[fields]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/application/controls_fields.ts#L12
[fw-actions]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_actions.h#L1
[fw-climate]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_climate.h#L2700
[fw-ha]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_ha.h#L286
[fw-local]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_local_controls.h#L96
[fw-mower]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_lawn_mower.h#L101
[fw-select]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_option_select.h#L212
[fw-sensor]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_sensor_driver.h#L113
[fw-status]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_status_entity_driver.h#L110
[fw-subscriptions]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_subscriptions.h#L1
[fw-vacuum]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/components/espcontrol/button_grid_vacuum.h#L117
[generator]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/scripts/generate_device_slots.py#L501
[inventory]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/common/config/card_runtime_inventory.json#L1
[normalizer]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/generated/saved_config_shadow.ts#L362
[registry]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/src/webserver/application/card_registry.ts#L5
[runtime-check]: https://github.com/jtenniswood/espcontrol/blob/76a90c0a9cc51ebe870d075496b5f02190f64627/scripts/check_firmware_card_runtime.py#L1
