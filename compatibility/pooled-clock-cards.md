# Reusable Clock and World Clock cards

The basic widget bank now renders Clock and World Clock cards through the shared
`PageTimezoneRuntime`. Cards are readonly numeric views with hidden icons, empty
units and the existing sensor background. They add no hidden widgets, callbacks,
HA registrations or timers. The owner is mandatory during preparation; its central
producer revision dirties visible cards after time, format or timezone updates.

Clock labels stay empty. World Clock labels use the existing configured/city/fallback
policy. The model borrows the complete city suffix, and the static label buffer
maps underscores to spaces while copying valid display text. Preparation reserves
capacity from configured fields, the bounded active selector and the translated
World Clock fallback. City labels are not limited by the model's short formatted
text buffer. Repainting an unchanged city does not rewrite its static label pointer.

Large clock behavior matches the current driver: local Clock uses the large font
for exact two-by-two, standard wide and wider-than-two-column layouts, or when
explicitly enabled on another shape. Disabling large numbers or omitting the large
font restores the normal font. World Clock only uses the large font on exact
two-by-two cards, even when explicitly enabled. Wide local clocks align left;
extra-wide clocks use the center option when selected. Those large wide layouts
hide the bottom label. Rebinding restores the ordinary label, alignment and font
for the next card; live font-theme revisions also restore/update the layout.

Clock options occupy the existing per-definition kind union. The port adds one
shared owner pointer and revision, without adding per-shell clock state. Clock and
world-clock children do not contribute to automatic folder indicators. Cache
reconnect does not erase the independent central clock input.

Generated tests compare the current legacy driver across 600 shape/option/font
cases and compare text/city formatting against the actual legacy formatter across
705,834 timestamp samples. Model checks cover readonly defaults, complete long city
fields, no allocation on reads and all nine profiles. Native checks cover real
private-to-zero configuration publication, hidden-page updates, twelve-hour changes,
invalid time, city underscore conversion, mixed sensor/clock reuse, theme font
changes and 1,000 navigation/time cycles with stable retained resources.

Production still selects the legacy renderer. Remaining card/modal families,
confirmations, scene integration, removal of compiled legacy registries, physical
qualification and the current factory build matrix remain prerequisites to cutover.
No device is flashed by this stage; host resource counts and SDK source checks do
not establish physical RAM savings. Stored/backup formats and nesting depth remain
unchanged pending the rest of the runtime and compatibility programme.
