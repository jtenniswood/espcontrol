# Folder indicator parity for ported cards

The reusable renderer's automatic folder indicator follows the existing
subpage driver hooks. Light All Controls and Cover All Controls children now
contribute their primary entity state, alongside the previously ported toggle,
relay, fan, numeric/percent/temperature and media variants. A cover's position
history does not replace its existing primary-state indicator predicate.

Timer children contribute only for valid `timer.*` entities and only while
active. Explicit folder entity/numeric/text/climate/mower modes keep their
existing predicates. Climate cards, option selects, sensors and action buttons
keep their current exclusions. Cover command variants do not contribute.
Cover-toggle children use the ordinary primary-state predicate (open/opening/closing),
independent of their inverted checked background (closed/closing). Unknown cover
modes retain the existing modal fallback. Access toggles and garage/gate commands
with status contribute; direct commands without status and lock commands keep
their existing exclusion. Vacuum children use the ordinary primary-state
predicate; mower children use their mowing/returning predicate independently
of the card background or icon.

The audit checks the original driver registration hooks rather than inferring
eligibility from a card's visual checked state. Folder summaries borrow the
existing shared HA state and retain only configuration-owned card identifiers.
No additional widgets, callbacks, timers or HA registrations are created.

Model eligibility tests cover all nine profiles, including modal controls,
fallback modes, invalid timer keys and intentional exclusions. Native tests
isolate light/cover state from fan state, so an unrelated active fan cannot hide
a missing contribution. They cover on/off/open/closed/unavailable changes and
1,000 navigation cycles with stable retained resources. Use
`ESPCONTROL_TEST_FAMILY=folder_indicators` for the focused suite.

Production scene selection, remaining drivers, physical qualification and the
factory build matrix remain unfinished. This stage flashes no device and
changes no stored/backup format or nesting depth. New drivers and future nested
folders must extend the eligibility/predicate tests before production cutover.
