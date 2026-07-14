# Display Lifecycle Transition Contract

This document is the behaviour baseline for centralising display-mode decisions.
It describes the behaviour that the staged display-lifecycle work must preserve;
it does not introduce a new public API or change any Home Assistant entity.

The lifecycle controller will decide **what should be visible**. ESPHome scripts
remain responsible for the physical effects: fades, LVGL page and widget changes,
touch guards, image release, and backlight writes.

Cover-art visibility is controller-owned through the `MEDIA_PLAYBACK` request.
Artwork URLs, decoded-image/cache state, track metadata and progress remain in
the cover-art runtime. Delayed activation and every asynchronous artwork effect
carry the display generation that created them, so a dismissed or replaced
cover-art view cannot be reopened or modified by an older callback. The S3
display still releases its decoded image whenever the controller hides cover art.

## Scope and ownership

The current behaviour is spread across:

- `common/addon/backlight.yaml` for idle, presence, touch wake, manual sleep,
  dimmed and clock presentation, setup dimming, and takeover suspension;
- `common/addon/backlight_schedule.yaml` for boot guard, scheduled modes,
  temporary off-hours wake, and brightness selection;
- `common/device/screen_cover_art.yaml` for media eligibility, delayed cover-art
  activation, dismissal, downloads, and S3 image release;
- `components/espcontrol/button_grid_image.h` for interactive image takeovers;
- `components/espcontrol/button_grid_alarm.h` for alarm arming and triggered
  takeovers; and
- generated device slot wiring under `devices/*/device/sensors.yaml`.

Normal control modals are not display modes. Only image-style modals and alarm
arming or triggered overlays participate in lifecycle takeover rules.

## Controller vocabulary

The shared controller introduced by later work uses these values:

| Type | Values | Meaning |
|---|---|---|
| `DisplayMode` | `ACTIVE`, `SETUP_DIMMED`, `DIMMED`, `CLOCK`, `COVER_ART`, `DISPLAY_OFF` | The one presentation mode that should be visible. |
| `DisplayRequestSource` | `BOOT_GUARD`, `IDLE_TIMER`, `PRESENCE_SENSOR`, `SCREEN_SCHEDULE`, `MANUAL_SLEEP`, `MEDIA_PLAYBACK`, `SETUP_TIMEOUT`, `USER_WAKE` | The event or policy currently requesting a mode. |
| `DisplayTakeoverKind` | `INTERACTIVE`, `CRITICAL` | Image-style modal or alarm takeover. |
| `DisplayTransition` | previous mode, target mode, winning source, generation | The complete decision handed to the effect adapter. |

Brightness is deliberately not a mode. The winning request selects the existing
normal, scheduled, temporary-wake, dimmed, or clock brightness calculation.

## Current state flags

These flags are compatibility state during the staged migration. They are not a
model to reproduce inside the controller.

| Flag | Current meaning | Notes |
|---|---|---|
| `display_asleep` | The normal active UI is not fully awake. | Also becomes true for setup dimming, so it cannot identify a mode alone. |
| `screen_schedule_asleep` | The schedule owns the current clock or off state. | Compatibility output only; boot behaviour is recalculated from live schedule and time inputs. |
| `backlight_manual_off` | A touchscreen long-press requested display off. | Cleared by the next wake. |
| `manual_wake_ms` | Start time of a temporary off-hours wake. | Zero means no temporary wake. |
| `screensaver_display_off_active` | The display-off presentation is selected. | Physical PWM must also be off. |
| `screensaver_dimmed_active` | The dimmed presentation is selected. | Requires the dim touch guard. |
| `is_clock_showing` | The full-screen clock overlay is visible. | Schedule ownership is tracked separately. |
| `cover_art_screensaver_active` | The cover-art presentation is visible. | Download/cache state is separate from lifecycle state. |
| `display_takeover_suspended` | Automatic display effects are suspended for a modal. | Currently shared by interactive and critical takeovers. |
| `screensaver_sensor_sleep_pending` | Presence requested sleep while a takeover blocked it. | Used by the current takeover restoration path. |
| `screensaver_wake_touch_guard_active` | The first wake touch is being swallowed. | Supporting interaction state, not a display mode. |

## Flag-to-mode transition table

`0` means false or zero, `1` means true or non-zero, and `-` means the flag does
not distinguish that row. Rows describe stable states after an effect completes,
not brief values observed part-way through a fade.

| Visible state | `display_asleep` | schedule asleep | manual off | temp wake | off active | dim active | clock | cover art | takeover | Controller mode / source |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| Normal UI | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | `ACTIVE` / default |
| Temporary off-hours wake | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | `ACTIVE` / `USER_WAKE` |
| Scheduled Screen Dimmed period | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | `ACTIVE` / `SCREEN_SCHEDULE` |
| Setup page dimmed after timeout | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | `SETUP_DIMMED` / `SETUP_TIMEOUT` |
| Idle or presence dimmed | 1 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | `DIMMED` / `IDLE_TIMER` or `PRESENCE_SENSOR` |
| Idle or presence clock | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | `CLOCK` / `IDLE_TIMER` or `PRESENCE_SENSOR` |
| Scheduled clock | 1 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | `CLOCK` / `SCREEN_SCHEDULE` |
| Eligible media cover art | 1 | - | 0 | 0 | 0 | 0 | 0 | 1 | 0 | `COVER_ART` / `MEDIA_PLAYBACK` |
| Idle or presence display off | 1 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | `DISPLAY_OFF` / `IDLE_TIMER` or `PRESENCE_SENSOR` |
| Scheduled display off or boot guard | 1 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | `DISPLAY_OFF` / `SCREEN_SCHEDULE` or `BOOT_GUARD` |
| Manual display off | 1 | 0 | 1 | 0 | 1 | 0 | 0 | 0 | 0 | `DISPLAY_OFF` / `MANUAL_SLEEP` |
| Interactive image takeover | 0 | - | 0 | - | 0 | 0 | 0 | 0 | 1 | `ACTIVE` with `INTERACTIVE` takeover |
| Alarm arming or triggered takeover | 0 | - | - | - | 0 | 0 | 0 | 0 | 1 | `ACTIVE` with `CRITICAL` takeover |

The cover-art row permits a stale schedule marker in the current implementation,
but schedule blocking is evaluated before activation. The controller must resolve
the live schedule request and must not depend on that stale marker.

### Invalid or transitional combinations

Every other combination of the presentation flags is ambiguous or invalid. The
adapter must converge it to the winning mode rather than preserve it. In
particular:

| Combination | Required interpretation |
|---|---|
| More than one of off, dimmed, clock, or cover art is visible | Re-resolve requests and apply only the winner. Never expose two presentations. |
| `display_asleep == false` with any off, dimmed, clock, or cover-art flag | Treat the presentation flag as stale and re-apply the winning transition. |
| `display_asleep == true` with no presentation flag while not on setup | Do not infer a saved transient mode; resolve current inputs. |
| Off active while PWM or logical backlight remains on | Complete the off effect immediately; logical and physical backlight state must agree. |
| Dimmed inactive while the dim touch guard is visible | Hide the guard before accepting normal UI interaction. |
| Dimmed active while the dim touch guard is hidden | Restore the guard before the mode is considered complete. |
| Takeover active while an automatic delayed effect finishes | Reject the obsolete effect; the takeover remains visible. |
| Enabled time-based schedule is waiting for valid time at boot | Select `DISPLAY_OFF` from `BOOT_GUARD`, regardless of the restored schedule marker; do not restore clock or another transient mode. |
| Manual off and temporary wake both set | Manual sleep wins and clears the temporary wake. |

## Request priority

The controller resolves all current requests every time an input changes. The
first matching row wins.

| Priority | Condition | Winning result | Existing behaviour to preserve |
|---:|---|---|---|
| 1 | Critical alarm takeover | `ACTIVE`, alarm overlay foreground | Alarm arming delay or triggered state prevents ordinary display transitions. |
| 2 | Manual sleep | `DISPLAY_OFF` / `MANUAL_SLEEP` | Closes ordinary and interactive modals; next deliberate user wake clears it. |
| 3 | Temporary user wake | `ACTIVE` / `USER_WAKE` | Uses configured wake brightness until the existing 10–3600 second timeout expires. |
| 4 | Night schedule | Off, `CLOCK`, or `ACTIVE` / `SCREEN_SCHEDULE` | Off and clock close image modals; Screen Dimmed keeps the active UI at `schedule_dimmed_brightness`. An enabled time-based schedule fails dark until local time is valid, whether or not a saved sleep marker exists. |
| 5 | Interactive takeover | `ACTIVE`, image modal foreground | Automatic idle or presence sleep is deferred. Manual sleep and scheduled off or clock may close it. |
| 6 | Eligible media playback | `COVER_ART` / `MEDIA_PLAYBACK` | Eligibility still includes entity, attribute, external-input, voice, schedule, and alarm checks. |
| 7 | Idle timer or absence | Configured `DIMMED`, `CLOCK`, or `DISPLAY_OFF` | Presence detected defers sensor sleep. Media sleep prevention and alarm takeover defer idle sleep. |
| 8 | Setup timeout | `SETUP_DIMMED` / `SETUP_TIMEOUT` | Static setup page dims to 50% after 120 seconds unless schedule or boot guard requires off. |
| 9 | No request | `ACTIVE` / default | Normal UI at normal calculated brightness. |

When requests have equal priority, the most recently changed request may trigger
resolution, but the result is derived from current inputs rather than a saved
presentation. Clearing a request always runs the same resolver.

## Takeover and restoration rules

### Interactive takeover

Opening an image-style modal begins an `INTERACTIVE` takeover, cancels automatic
idle effects in flight, hides clock, dimmed, and cover-art presentations, removes
the dim touch guard, and presents active UI brightness. While it is open:

- idle and presence sleep requests remain recorded but are not applied;
- media may change eligibility without replacing the modal;
- manual sleep and scheduled off or clock can close the modal and take effect;
- ordinary modals remain outside the lifecycle controller.

Closing the modal clears the takeover and resolves all current requests. It does
not restore a snapshot. For example, absence during the modal can resolve to the
configured idle mode, media that started can resolve to cover art, and a schedule
change can resolve to off or clock.

### Critical takeover

An alarm arming-delay or triggered state begins a `CRITICAL` takeover. It keeps
the alarm overlay in the foreground and outranks manual sleep, schedule, media,
idle, and setup requests. Those requests may change while the alarm is visible
but cannot dismiss it. When the alarm leaves its takeover state, the controller
clears the takeover and resolves the current requests again.

### Touch wake

A deliberate user wake clears manual sleep and selects `ACTIVE`. During scheduled
off-hours it also starts the existing temporary-wake timer and uses temporary-wake
brightness. The first touch that wakes dimmed, clock, cover art, or display off is
consumed by the applicable touch guard and must not activate an underlying card.

## Transition invariants

These are true after every completed transition:

1. Exactly one of `ACTIVE`, `SETUP_DIMMED`, `DIMMED`, `CLOCK`, `COVER_ART`, or
   `DISPLAY_OFF` is the presentation mode.
2. Only that mode's LVGL page or overlay is visible; ordinary modals are not
   counted as presentation modes.
3. The dim touch guard is visible only for `DIMMED`; wake guards exist only for
   their bounded touch-release or timeout window.
4. `DISPLAY_OFF` means the logical backlight is off and physical PWM is off.
   Every non-off mode has logical mode, PWM, and selected brightness aligned.
5. The clock brightness source is scheduled-clock brightness for a schedule-owned
   clock and day/night clock brightness for an idle-owned clock.
6. Cover-art download and cache state may outlive `COVER_ART`, but it cannot make
   the presentation visible without a current winning media request.
7. The adapter is the only writer of compatibility presentation flags once a
   mode is migrated. Existing readers may remain during the staged migration.
8. Every accepted request, clear, or takeover change creates a newer transition
   generation. A delayed callback must match the current generation before it
   changes widgets, flags, brightness, downloads, progress, or presentation.
9. Completing an obsolete generation has no effect, including no flag cleanup
   that could damage the current mode.
10. Releasing a takeover or clearing a request resolves live inputs; it never
    restores a saved transient presentation.

## Expected event sequences

These sequences are the named baseline for later host tests and physical-device
tests. `gN` denotes a transition generation.

| Sequence | Events | Expected decisions and checks |
|---|---|---|
| Default boot | Boot, valid time, schedule normal | Resolve `ACTIVE`; normal UI and normal brightness; idle timer starts. |
| Fail-dark boot | Enabled time-based schedule; boot; time invalid; repeat with and without a saved schedule-asleep marker | `BOOT_GUARD` requests `DISPLAY_OFF` in both cases; PWM remains off. When time becomes valid, clear boot guard and resolve the live schedule. |
| Idle dim | `ACTIVE`; idle timeout; configured action Dim | `IDLE_TIMER` requests `DIMMED` at `g1`; normal UI remains beneath the dim guard; dim brightness applies. |
| Idle clock | `ACTIVE`; idle timeout; configured action Clock | `IDLE_TIMER` requests `CLOCK`; full-screen clock alone is visible at day/night clock brightness. |
| Idle off | `ACTIVE`; idle timeout; configured action Off | `IDLE_TIMER` requests `DISPLAY_OFF`; fade completes, off page is selected, logical backlight and PWM are off. |
| Presence wake/sleep | Sensor mode; absence; presence; absence | Absence requests configured idle mode; presence clears it and resolves `ACTIVE`; later absence requests it again. Schedule requests continue to outrank presence. |
| Wake from each inactive mode | Begin in `DIMMED`, `CLOCK`, `COVER_ART`, then `DISPLAY_OFF`; touch each once | Each touch produces `USER_WAKE` and `ACTIVE`; presentation and stale guard are removed; the wake touch does not trigger the underlying UI. Cover art may re-request after its existing pause/delay. |
| Manual sleep | Any non-critical mode; long press; touch | `MANUAL_SLEEP` wins `DISPLAY_OFF`, clears temporary wake and closes interactive modal. Touch clears manual sleep and resolves `ACTIVE` or an off-hours temporary wake. |
| Scheduled off and automatic wake | `ACTIVE`; enter off-hours Off; enter normal hours | `SCREEN_SCHEDULE` selects `DISPLAY_OFF`; later clearing that request selects `ACTIVE` and restarts idle handling. |
| Scheduled clock | `ACTIVE`; enter off-hours Clock; setting or time changes | Select `CLOCK` with scheduled clock brightness. Re-evaluate current schedule on every change; do not restore a previous transient mode. |
| Temporary off-hours wake | Scheduled off or clock; touch; timeout | `USER_WAKE` selects `ACTIVE` at temporary-wake brightness. On existing timeout, clear it and re-resolve to the current scheduled off or clock mode. |
| Scheduled Screen Dimmed | Enter off-hours Screen Dimmed | `SCREEN_SCHEDULE` selects `ACTIVE` at `schedule_dimmed_brightness` and suppresses idle sleep for that period. The internal `screen_schedule_always_on_mode` helper name does not change the user-facing mode or its brightness contract. |
| Eligible playback | Active or idle request; eligible playback starts; delay completes | `MEDIA_PLAYBACK` selects `COVER_ART` unless a higher request blocks it. Artwork state stays outside the controller. |
| Playback stops | `COVER_ART`; playback stops | Clear media request and resolve live schedule, presence, idle, or default state. Never assume the result is `ACTIVE`. |
| Cover-art dismissal race | Start delayed activation `g1`; user wake or replacement creates `g2`; `g1` delay/download/retry/progress callback fires | Every `g1` callback is rejected. It cannot reopen cover art, modify its widgets, change brightness, or release resources belonging to `g2`. |
| Interactive modal defers idle | `ACTIVE`; open image modal; idle timeout or absence; close modal | `INTERACTIVE` keeps `ACTIVE` while recording the automatic request. Closing resolves to the current configured idle mode. |
| Schedule changes during interactive modal | Open image modal; enter scheduled off or clock | Schedule outranks interactive takeover, closes it, and applies off or clock. Entering normal hours before modal close is resolved from live schedule state. |
| Media changes during interactive modal | Open image modal; eligible playback starts or stops; close modal | Modal remains visible. On close, current media eligibility decides whether `COVER_ART` wins. |
| Critical alarm takeover | Any mode; alarm enters arming delay or triggered; other requests change; alarm clears | `CRITICAL` selects active UI with alarm overlay. No lower request changes presentation. On release, resolve all current requests. |
| Rapid opposing requests | Idle requests off `g1`; touch requests active `g2`; schedule requests clock `g3`; old fade callback from `g1` fires | Final mode is `CLOCK` from `g3`; callbacks from `g1` and `g2` are rejected and cannot change widgets, flags, or PWM. |
| Setup timeout | Static setup page; 120 seconds; schedule inactive | `SETUP_TIMEOUT` selects `SETUP_DIMMED` at 50%. A schedule or boot-guard off request outranks it. |
| Home Assistant reconnect | Any stable mode; API disconnect/reconnect; entities and time refresh | Re-resolve updated schedule, presence, media, and alarm inputs without publishing new entities or losing saved settings. |

## Required test mapping for later stages

Host-side tests must cover every priority pair, every request-clear path, all
sequences above, and explicit stale-generation rejection. Behaviour-changing PRs
must also run focused parser, modal, Home Assistant binding, and generator checks;
the full fast repository check; and compilation for every supported display.

Physical testing is required on the 10-inch P4 and 4-inch S3 for every
behaviour-changing PR. It must exercise touch wake, rapid wake/sleep, clock,
dimmed guard, scheduled and manual off, cover-art start/stop, image-modal
interruption, alarm takeover, and Home Assistant reconnect. Compilation is not a
substitute for those device tests. Compare memory with the preceding `main`, pay
particular attention to unexplained S3 growth or boot instability, and confirm
saved schedule, brightness, and screensaver settings survive upgrade.
