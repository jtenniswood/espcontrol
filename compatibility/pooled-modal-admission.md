# Panel request classification

Panel requests borrow configuration text while classifying media controls,
volume, action-based option selection, alarm PIN and cached forecast panels.
They do not allocate temporary strings. Panel metadata and widgets remain
demanded by the maintenance loop after the request is accepted.

Media modes, select actions and forecast precision names come from the generated
card contract. Invalid media modes keep the existing `play_pause` default.
Option tokens use exact comma-delimited matching. Alarm PIN flags preserve the
saved-option decoder's first-value semantics: `0` and `%30` disable a PIN;
whitespace, malformed escapes and extra decoded bytes do not. There is no
configuration migration or format change.

The host parity test compares borrowed classification with the existing saved
card helpers, including long values, duplicate flags and every two-byte percent
escape spelling. Native LVGL tests assert that panel requests allocate no C++
or LVGL memory, including accepted and rejected requests, repeated openings,
replacement and takeover paths. These are host checks, not physical memory
measurements.

Production scene wiring, compiled legacy retirement, current factory builds and
physical qualification remain. After integration, test the Guition 4848S040 S3
and a working 20-slot JC8012P4A1 P4 with repeated panel openings, reconnects and
configuration replacements. Keep P4-86 reserved for USB diagnosis. Record matched
heap/PSRAM, minimum free memory, largest blocks and opening peaks before claiming
a device memory saving.
