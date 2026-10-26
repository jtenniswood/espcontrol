# Numeric font and unit placement parity

The reusable renderer now uses the existing drivers' exact large-number rules.
HA numeric and duration sensors use the large font only on exact two-by-two cards.
Climate numbers also honor an explicitly enabled large-number option on other
shapes. Numeric action and folder captions retain their ordinary font, matching
the current drivers. Calendar and Clock/World Clock retain their separately tested
size rules; other card families do not gain large numbers from a generic span test.

The existing duration configuration normalizer discards large-number options,
including explicit off. Duration sensors therefore still enlarge automatically
on exact two-by-two shapes. This stage preserves that current behavior rather than
changing stored option semantics. Sensor and climate off options retain their
current effect.

Climate's explicit flag occupies existing definition padding; the host definition
remains 264 bytes. `PageBasicTheme::large_sensor_unit_offset_percent` supplies the
existing display profile's unit adjustment. The port applies font line height times
that percentage while large text is active, and resets the unit translation to zero
for ordinary text, theme changes and rebinds. Scene integration must populate the
field from the selected display profile. The style property is reserved at widget
initialization; no per-shell state or navigation allocation is added.

Generated comparisons extract the actual sensor and climate layout functions and
check 1,800 type/precision/shape/option/font/unit-offset combinations. Model checks
exercise actual preparation, including numeric actions/folders and duration option
normalization. Native checks cover all nine profiles, single/wide/two-by-two/extra-
large shapes, live positive/negative unit offsets and large-font theme removal and
restoration. Existing clock layout/presentation and full renderer regressions remain
part of verification.

Production still selects the legacy renderer. These are parity corrections before
scene cutover and physical qualification; no device is flashed and host results do
not establish physical memory savings. Stored/backup formats and nesting depth are
unchanged. Remaining runtime families, confirmations, registry retirement and the
current factory build matrix remain required before enabling nested pages.
