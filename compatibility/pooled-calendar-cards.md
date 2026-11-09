# Reusable calendar cards

The unselected renderer supports Calendar and Calendar Date/Time cards through
`PageCalendarRuntime`. Pass the owner to the grid port and prepare it before the
HA graph. Missing owners reject preparation. The port owns no calendar widget
registry, callback, subscription or timer. Production remains on the existing
renderer; Clock and World Clock are separate remaining ports.

Calendar cards remain read-only, without an icon, unit, checked background or
push feedback. Their configured label/icon fields retain the existing ignored
behavior. Date cards display the valid shared day and translated month; Date/Time
cards use the central clock's valid time and a day/month label. HA date reports
update the date only. Invalid values show the same Date/`--` fallback as before.
Twelve-hour formatting has no AM/PM suffix, matching the current card.

Formatting uses bounded local buffers and firmware-lifetime translated month
labels. Date/time label truncation preserves the existing 32-byte limit. Frames
allocate nothing and retain no per-card state. Owner revisions redraw only the
visible bank when shared date/time changes; hidden pages read the current state
when bound. Calendar children keep their existing exclusion from automatic
folder indicators.

The existing calendar font rule is preserved: the large sensor font applies
only to exact two-by-two cards when enabled. Single, wide and extra-large spans
keep the normal font, including with an explicit large-number option. Labels
remain visible. Unit text is always empty. The port adds one fixed owner pointer
and revision; its date/time flag fits the existing prepared-kind union, without
extra shell fields or per-card pointers.

Generated text tests compare 180,000 date/time states against the current
calendar renderer and clock formatter across nine languages. Model tests cover
all nine profiles, ignored configured labels/icons, fallback/12-hour text,
allocation-free reads, invalid handles and retained history across reconnect.
Native checks cover real private-to-zero publication, duplicate/hidden date
sources, read-only input, immediate updates and 1,000 navigation/time cycles
with stable retained resources. Size tests cover single, wide, two-by-two,
extra-large and disabled-large variants. Use
`ESPCONTROL_TEST_FAMILY=calendar` for the focused native suite.

Remaining drivers, confirmations, scene/producer integration, compiled legacy
registry retirement, physical S3/working 20-slot P4 qualification and the current
factory matrix remain required. No device is flashed by this stage. Stored and
backup formats and nesting depth are unchanged. Host counters and SDK syntax
checks do not establish physical layout or heap/PSRAM savings. P4-86 remains
reserved for USB diagnosis.
