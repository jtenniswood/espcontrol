# Deferred timer actions

Timer start/cancel intents share the existing eight-entry `PageCardCommands`
FIFO with basic and slider actions. `PageTimerCommandPolicy` borrows the timer
history and entity graph. It adds no fields to configured cards, no per-card
callbacks/timers and no subscriptions or queue payload fields.

The prepared grid captures the pressed HA connection, current timer state
revision and whether the required second tap was confirmed before repainting.
It only admits an intent. The command owner checks the complete binding,
configuration activation, connection, service, graph capacity and current timer
phase again before sending the explicit `timer.start` or `timer.cancel` action.
A state change away from active invalidates an earlier confirmed cancellation,
even if the timer becomes active again before the next frame.

Only an accepted native cancellation updates that card's cancel history. Queue
admission, overflow, failure and cancellation leave the natural-finish grace
unchanged. An older failed send cannot erase an already accepted cancel. The
post-send acknowledgement checks the activation, connection and source state
revision so reentrant replacement/reconnect/state changes cannot mark the new
timer cycle cancelled. Helper acceptance is not an HA acknowledgement.

The normal host fixture runs 1,000 start/confirmed-cancel cycles on all nine
profiles, with allocation-free admission and stable arena/graph registrations.
It also covers unconfirmed/unsupported/wrong-page input, busy dispatch, overflow,
failed sends, changed phases, unavailable service, offline/covered/stale input,
reconnect and same-generation replacement during native dispatch after the old
arena is poisoned. Native helper spies verify both explicit services and failed
send propagation.

The real LVGL timer fixture (`ESPCONTROL_TEST_FAMILY=timers`) now routes actual
grid callbacks through this owner and defers sends until after the event. Its
existing countdown, two-tap expiry, unseen state cycle, stale press, failed-send,
reentrant navigation and 1,000 confirmation/navigation checks retain stable
arena allocations, registrations, LVGL memory and timer counts. C++ allocation
is checked in admission; owned string copies may allocate during maintenance.

Production selection, final scene composition, remaining command families and
physical S3/working 20-slot P4 qualification are still pending. No device is
flashed by this stage; P4-86 remains reserved for USB diagnosis. Saved documents,
backups and navigation depth are unchanged. No physical memory saving is claimed.
