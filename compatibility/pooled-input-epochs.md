# Press-captured HA input epochs

This #1825 prerequisite extends the
[prepared confirmation input guards](pooled-confirmation-cards.md) to the other
HA-backed card inputs. Production still selects the legacy renderer; scene
wiring, compiled retirement, factory builds and physical qualification remain.
No firmware was flashed or physical heap/PSRAM saving claimed.

Prepared cards explicitly identify whether their input depends on HA. Generic
toggles/actions, fan/media/modal controls and existing HA driver families capture
the connection on press and reject a later click after reconnect, even when
fresh reports arrive before release. The flag fits existing definition padding:
264 bytes on the host and 156 bytes on both actual S3 and P4 compilers remain
unchanged. Local actions, webhooks, screensaver actions, folder navigation and
static QR/cached forecast panels retain their independence from HA connection changes.

The button press marker is separate from slider/media-preview state. Accepting
a volume/group preview on a fresh connection cannot make an older button press
current. This marker reuses the storage reserved for timer confirmation revision
in non-timer card modes, so it adds no per-shell storage. Timers keep their existing
revision and connection handling; shared slider/preview markers remain unchanged.
Bindings still guard configuration, page, shell and view generation. Scene
callbacks must only queue work; command owners validate again before sending.

Native tests use the actual renderer, prepared port and entity graph, covering
HA and local inputs, fresh reports before stale clicks, root/child controls and
1,000 reconnect/navigation cycles per profile with stable retained resources.
Media tests additionally replace the preview's connection after an old press.
Metadata models distinguish HA controls from local/action/QR/navigation variants.
These checks are host tests, not physical device/font/network qualification.

After scene integration, qualify Guition 4848S040 S3 and a working 20-slot
JC8012P4A1 P4: press across HA reconnect, receive fresh state before release,
update a media preview while held, and verify only a new press acts. Check local
controls, QR, webhooks, Back/folders and existing timer confirmation. Compare
matched heap/PSRAM, minimum/largest block, latency and transient peaks. Keep
P4-86 reserved for USB diagnosis. Backup formats and navigation depth are
unchanged by this stage.
