# Configuration-owned calendar state

`PageCalendarRuntime` preserves the current shared calendar behavior without a
registry of hidden widget references. All configured calendars share one ordered
date/time state, regardless of their HA date source. A valid report from any
configured source replaces the shared date; invalid/unavailable reports leave it
unchanged. Empty source names use `sensor.date`. Parsing reads at most 16 bytes
and retains the current ISO/DMY syntax and day/month range rules.

The existing local clock producer can update the date, time or both. Invalid
time clears time fields while preserving the last valid date; a valid time
also updates the date. Reconnect does not erase calendar history. Source reports
and clock events retain their original last-writer order, including on hidden
pages. Calendar labels, clocks and timezone views are separate remaining ports.

Prepare the owner before the shared HA graph and include it in the fixed
observer set. Preparation copies the active scalar state into a private staged
slot. Copied/live date reports update only their matching configuration. Clock
producer events update both slots independently, preserving a staged date when
time becomes invalid. Commit promotes the private generation, including zero;
failed/discarded preparation never publishes candidate state. Even a document
with no calendar cards retains the current shared date/time state, matching the
legacy global state across configuration rebuilds.

`PageCalendarEvents` bridges the current producer entry points after the legacy
grid is retired and its calendar reference registry is empty. Install it before
the first configuration prepare. It seeds current producer state, attaches the
optional date/time events and publishes active scalar state back to the existing
calendar global. That publication keeps `calendar_date_valid()` authoritative for
the time addon's fallback path. Staged HA reports never change the global date;
a redundant invalid-clock date event cannot erase a valid candidate HA date
before the first commit. Publication uses direct scalar assignment, without
redispatching an active HA report as a clock event into a candidate
configuration. Uninstall the bridge
before destroying the owner. Duplicate/unsafe installation is rejected.

The owner uses fixed active/staged scalar records and borrowed PageStore views;
it allocates no arena, source strings, widgets, callbacks, subscriptions or
timers. Only the graph owns HA registration/cache bytes. Counters report fixed
owner size, active/staged distinct source counts and preparation failures.
Source filtering scans configured navigable calendar cards on a report; frame
reads copy only the shared scalar state. Store backing memory remains alive
until matching retirement.

Generated tests extract current producer/parser functions and compare 100,000
mixed date/time/report sequences. Other checks cover default/duplicate/multiple
sources, invalid reports, publisher installation, the HA fallback-date path,
active/staged isolation and producer ordering, real private-to-zero publication,
reconnect, discarded candidates, empty documents, all nine profiles and 1,000
allocation-free source replacements. Existing clock/screensaver tests also pass.

Production remains on the existing renderer. Calendar/clock/timezone views,
remaining drivers, confirmations, scene integration, physical qualification and
the current factory matrix remain required. Production cutover must retire the
legacy calendar/timezone registries and subscription paths, including compiled
storage; merely keeping their reference count at zero is not the memory gate.
No device is flashed by this stage. Stored/backup formats and nesting depth are
unchanged; fixed host bytes do not establish physical heap/PSRAM savings.
