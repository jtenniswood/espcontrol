# Native document v2 codec

Implementation stage: matching structural readers and writers are available in
C++ and TypeScript. Firmware capabilities, HTTP handlers, the editor, backups and durable
storage still use v1. This does not enable nested pages or establish a supported
downgrade floor. Lossless v1 conversions and service/runtime integration must
pass before advertising v2.

## Wire layout

Integers are little-endian. The 16-byte `EPCF` envelope retains the v1 header
layout: version at byte 4, header size at 6, payload length at 8, record count
at 12 and zero reserved bits at 14. V2 uses version 2 and a uint16 record count.
Each record starts with a uint8 kind and uint16 body length. Records can refer
to later records; their order is not their identity.

IDs are uint16 identities scoped by record kind, never arena offsets or runtime
PageStore indices. Root page ID is 0. String/entity ID 0 means an empty or absent
value and has no dictionary record. Other identities are 1–65534; 65535 means
no parent/owner and is permitted only on the root page.

| Kind | Body |
|---|---|
| 1, device profile | Nonempty UTF-8 profile, at most 64 bytes; exactly one |
| 2, string | ID, nonempty UTF-8 bytes |
| 3, entity reference | ID, nonempty string ID |
| 4, page | ID, parent page ID, owner card ID, order string ID, uint8 flags |
| 5, card | ID, page ID, slot, field mask, uint8 flags, field references, optional raw-source string ID |
| 6, navigation | Owner card ID, child page ID |
| 7, panel setting | Key string ID, value string ID |

Card mask bits 0–8 name entity, label, icon, active icon, sensor, unit, type,
precision and options. Each set bit contributes one uint16 reference in that
order. Entity and sensor use entity-reference IDs; other fields use string IDs.
A present field must have a nonzero reference. Slots are nonzero uint16 values,
so inactive definitions are not clipped to the current visible grid.

Page flag 1 records compact legacy-import spelling on child pages; the root's
flags must be zero. Card flag 1 adds a
raw-source string reference for noncanonical imports. These are preservation
hints for the conversion layer, not alternate executable definitions. That
layer must verify a raw import agrees with its semantic fields before
accepting or projecting it. Other flag and mask bits are rejected.

Strings retain the deployed 4,095-byte value boundary. Including their two-byte
identity, the maximum v2 record body is 4,097 bytes. Setting keys retain the
63-byte boundary. Profile, total document, per-page/card admission and legacy
byte/chunk limits remain separate checks; these codec bounds do not increase
advertised device capacity.

## Validation and memory ownership

The caller owns the immutable document and an aligned temporary index of eight
bytes per record. `workspace_bytes` checks the envelope, record shapes and
UTF-8 before reporting that index size. `validate` checks the complete graph;
queries are unavailable after any failed validation. The caller must keep both
document and index alive and unchanged while querying the reader. Neither
reader nor writer allocates memory internally.

IDs and page positions must be unique. Every reference must resolve in its
correct namespace. Each child page has an owner on its parent page, and each
card owns at most one page. Only explicit navigation makes a retained page a
target, preserving inactive pages behind non-folder cards. Navigation requires
a folder card with the matching child/parent/owner relationship. Parent chains
must reach the root within two levels; cycles and deeper chains are rejected.
The wire reader understanding two levels does not change runtime admission or
editor capabilities.

Setting uniqueness uses decoded key text, including when different string IDs
contain the same key. Unknown record kinds, reserved IDs/bits, truncated or
excess records, broken references and invalid UTF-8 fail closed. A sorted index
keeps ID/position/owner checks bounded without a table sized for every possible
identity or repeated whole-document reference scans.

The writer borrows an output buffer and caller-provided index. It validates
record shapes during append and the complete graph during finish. A usable
`EPCF` header and successful size are published only after validation; failed
graph validation clears the marker and size. Canonical output can order records
by kind and identity without changing their references.

`compatibility/fixtures/panel_config_v2.json` contains an independently assembled
wire golden and its semantic records, including forward references, inactive
records, Unicode, embedded NUL and an opaque setting. Host checks compare the
actual decoded fields and writer bytes with that fixture, exercise malformed
graphs and buffer boundaries, and forbid allocations during validation/writing.
The browser reader runs the same independent hostile-wire corpus as the C++
reader. Browser decoding preserves leading Unicode BOMs as data, including in
the existing v1 format, and both browser writers reject unpaired surrogates
instead of silently replacing them during UTF-8 encoding.
