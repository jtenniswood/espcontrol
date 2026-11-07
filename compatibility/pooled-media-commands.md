# Deferred media grid actions

Media Play/Pause, Previous, Next, Playlist and inline Position/Now Playing seek
intents now share the eight-entry `PageCardCommands` FIFO. Previous/Next still
admit once on press and suppress the later click. Other buttons retain their
pressed binding/connection through click; seeks capture both on pointer press.
Native sends run in maintenance outside LVGL/native/coordinator dispatch.

`PageMediaCommandPolicy` uses allocation-free configuration views and the shared
media/graph owners. It rejects modal-only modes, invalid seek values, changed
playback revisions/durations, unavailable or exhausted data, stale/covered input,
reconnect and replaced configuration. Transport buttons keep the existing
ability to act without playback metadata when HA is connected. Overflow rejects
the latest intent without replacing an earlier accepted action.

The shared media source gets a monotonic seek revision. Content-ID changes,
changed duration/availability and reconnect invalidate an old gesture or queued
seek, including A→B→A changes with identical duration. Ordinary position/state
updates and unchanged content IDs leave it valid. Revision exhaustion disables
seek until the owner is recreated. Position dependencies now also include
`media_content_id`, deduplicated with other cards for that player. Budget this
extra graph channel/registration for a standalone Position card.

Release ends the drag and admits the seek without installing a sent-action
preview. After helper acceptance, the scene's native accepted-control hook calls
`PageBasicLvglPort::preview_seek` with the captured proof/percent/duration. That
method rechecks the full binding, connection, playback revision and current
availability, and rejects an older result during a newer drag. A new track,
reconnect, takeover, navigation or configuration replacement retires the hint;
otherwise the existing three-second/report acknowledgement rules apply. The
shell reuses its mutually exclusive artwork storage for the seek revision.

The existing production media helpers now return acceptance instead of dropping
the result; legacy callers can continue ignoring it. Playlist dispatch retains
its decoded content ID/type, optional source selection and owned response
callbacks. Source selection and playback remain independent sends; the return
value describes the play request. `DISPATCHED` is helper acceptance, not HA
acknowledgement. Dispatch strings/callbacks can allocate in maintenance.

Configured-card records, shell storage and scalar FIFO payload size do not grow. Host
measurements keep the FIFO at 920 bytes and a one-source/one-card media arena at
146 bytes; the media owner's fixed size changes from 240 to 248 bytes. The
source revision fits existing host padding; target layouts and physical heap/
PSRAM costs still need device qualification. The native adapter adds one fixed
optional scene notification pointer.

Normal host tests run 1,000 five-action cycles on all nine profiles with stable
arena/registrations, allocation-free admission, reconnect/overflow/invalid data,
track/duration changes and reentrant same-generation replacement after poisoning
the old model arena. Tests extract the actual production send helpers and verify
services, rounded seek seconds, 1,000 decoded playlist payload/response cycles,
default content type and failed-send callback cleanup. Native adapter spies
check routing and accepted-notification plumbing.

The real LVGL `media_commands` fixture runs 1,000 root/child media cycles per
profile, including actual pointer release, press/click suppression, delayed
preview, rejected/failed sends, track changes during touch/queue/send, a newer
drag, takeover and exhausted graph data. It checks stable retained LVGL/arena
memory and registrations. The `media_parity` fixture retains the existing media
action, Position geometry and Now Playing layout/navigation checks.

Production scene selection, remaining access/cover/cleaning/fan dispatch, full
scene composition, current factory builds and physical S3/working 20-slot P4
qualification remain pending. This stage does not flash devices or enable
nested pages, change saved documents/backups, or prove device memory savings.
P4-86 remains reserved for its separate USB diagnosis.
