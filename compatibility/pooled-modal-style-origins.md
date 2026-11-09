# Panel style source identity

All demanded card-panel style resolvers receive the opening's full
`PageModalToken`: configuration generation, card ID, opening and family. This
extends the older climate, shared control/select/media, alarm, cached weather
and expanded-image interfaces to match the QR and confirmation interfaces.

The application scene can resolve the source page and its font/palette rules
from that identity, even when the card is hidden or the visible grid changes.
It must not infer the source from the currently displayed page or a matching
entity/label. Expanded-image refreshes pass the retained opening token again.
No resolver gains a borrowed grid widget or an extra per-card record.

Native tests compare resolver identity with actual opening identity for climate,
shared light/cover, alarm and cached-weather panels. The expanded image test
selects a distinct hidden-page font by card identity and checks the loading
caption after creation and refresh. Existing repeated-opening, replacement,
retirement and allocation checks remain in place.

Production scene wiring, compiled legacy retirement, current factory firmware
builds and physical qualification remain unfinished. After integration, test
Guition 4848S040 S3 and a working 20-slot JC8012P4A1 P4 with different home and
subpage fonts, hidden/remote card openings, theme changes and configuration
replacement. Keep P4-86 reserved for USB diagnosis. This stage changes no saved
document format and establishes no physical memory saving.
