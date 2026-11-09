# Shared clock and timezone state

`PageTimezoneRuntime` keeps one bounded central clock input and borrowed active
and prepared document views. Every clock and world-clock card reads this owner;
there are no per-card widgets, strings, HA registrations, timers or state records.
Prepare/activate/discard follows the shared runtime contract, including promotion
of a private generation to generation zero. Retiring a configuration preserves the
central time input for the next configuration. Failed or discarded preparation
does not change the active document.

Clock cards follow the active selector. World clocks use their configured zone;
the existing parser still supplies UTC for an empty world-clock setting and clears
clock overrides. The owner preserves the complete configured zone field, including
unknown city names. The selector producer has a separate 127-byte input budget,
well above every declared selector option. Oversized producer updates are rejected
without truncation or changing the last accepted time; rejection counts are exposed.
This budget does not limit configured world-clock entity fields.

The firmware resolver borrows from its input or the existing ROM table. It resolves
Auto using the existing global-timezone matching order and UTC fallback. City views
preserve underscores for the view formatter to replace, and retain unknown zone
names while using UTC offsets, matching the old renderer. Invalid clock input
retains the selected city and twelve-hour preference but returns invalid time.
Readers must consume a snapshot before the next producer update or configuration
replacement, and must never retain its borrowed city view.

The POSIX parser and transition arithmetic moved from `sun_calc.h` into the shared
allocation-free `timezone_offset.h`; the existing string-based path delegates its
offset calculation to that helper. The view resolver uses the same ROM lookup,
Casablanca UTC windows and northern/southern DST rules without constructing strings
or changing process/global TZ state. Each producer event advances the revision,
including identical Auto inputs that can reflect a changed HA timezone.

`PageTimezoneEvents` bridges the unchanged central producer after the legacy grid
is retired and its timezone reference count is zero. Install before the first
configuration prepare; it seeds current clock input and refuses duplicate or unsafe
installation. Uninstall before destroying the owner. Production cutover must also
remove compiled legacy timezone/calendar registries and legacy global string state;
an empty reference count alone does not establish RAM savings.

Generated tests use the actual timezone table, option resolution and legacy clock
presentation, plus an independent host-libc POSIX reference around DST transition
minutes and monthly samples from 2024 through 2050. They cover Auto/fallback,
fractional offsets, Casablanca boundaries, invalid input, full configured city names,
all nine profiles, 1,000 allocation-free replacements per profile, failed/discarded
preparation, real private-to-zero publication and producer bridge lifecycle.

Production still selects the legacy renderer. Clock/timezone views, remaining
families, confirmations, scene integration, physical qualification and the current
factory build matrix remain required. No device is flashed by this stage. Stored
and backup formats and nesting depth are unchanged; fixed host memory counters do
not establish physical heap/PSRAM savings.
