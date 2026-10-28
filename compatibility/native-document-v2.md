# Native document v2 codec

Implementation stage: matching structural readers and writers are available in
C++ and TypeScript. A staged TypeScript bridge preserves v1 saved values through
a v2 round trip. Allocation-free firmware conversion components are available
in both directions. Profile/legacy admission and service/runtime integration
remain unfinished. Firmware capabilities, HTTP
handlers, the editor, backups and durable storage still use v1. This does not
enable nested pages or establish a supported downgrade floor.

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
flags must be zero. Card flag 1 adds a raw-source string reference, preserving
exact imported spelling, extra fields and explicitly empty values. Page/card
flag 2 marks an absent v1 record: an implicit empty folder target or a synthesized
empty root owner for retained inactive subpage data. Flags 1 and 2 cannot be
combined. Absent owner cards have no fields; absent pages have no order and are
children of the root. Conversion additionally verifies ownership, navigation
and empty contents before omitting these synthesized records from v1.

These are preservation hints, not alternate executable definitions. Conversion
must verify raw import fields agree with the referenced saved fields before
projection. Saved fields precede existing card-family normalization; they must
not discard unknown options or silently migrate legacy types during conversion.
The child parser's empty-icon defaults are `Auto`. Entity/sensor references
retain existing saved tokens, including auxiliary modes; runtime dependency
construction still interprets those tokens using the card contract. Other flag
and mask bits are rejected.

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

## Staged compatibility conversion

`panel_config_conversion.ts` imports v1 into explicit graph records and projects
v2 back through the deployed v1 encoder and its byte/count limits. The bridge
retains order/spans/Back labels and colours as exact order text, all panel
settings, inactive definitions, missing versus empty records, malformed escape
spellings and unknown extra imported fields. It uses the generated compact type
tables with own-key lookups, so prototype-like unknown names remain data.

Projection rejects stale raw hints, inconsistent root-order/settings, sparse
child positions, invalid absence provenance and nested pages. New fields use a
representable plain or compact spelling; an imported raw child cannot change
its page's spelling if that would lose its original source. Edited cards must
clear their old raw hint. The bridge rejects an unrepresentable result rather
than truncate it. Profile card/channel limits and preference byte/chunk limits
remain separate admission work before this bridge can be used by live saves.

`panel_config_projection.h` implements the firmware v2-to-v1 direction with
caller-owned, disjoint input, output and aligned scratch. Scratch contains two
4,095-byte text windows, fixed root/page metadata, the reader's eight-byte record
index and an eight-byte-per-record upper bound for grouping cards. It constructs
no complete decoded model and allocates no heap memory. Generated borrowed type
lookups use the same compact-type contract without temporary owned strings.
The output marker and size are usable only after successful conversion; argument
alias failures leave input/output buffers untouched. Profile/channel admission
and preference byte/chunk limits remain separate gates.

Canonical export trims empty trailing fields and implicit child `Auto` icons.
It falls back to a lossless plain spelling when compact encoding would exceed
the v1 byte limit or reinterpret a separator-like unknown type. Imported raw
values are still preserved exactly. `native-document-projection` runs actual
C++/browser comparisons after their host/unit checks, and CMake enables address
and undefined-behavior sanitizers for all three native conversion host executables when the
compiler supports them. This remains host qualification, not a device result.

`panel_config_import.h` implements firmware v1-to-v2 import. It borrows source
views, decodes one record at a time, and interns text directly into the private
output buffer. A caller-owned table stores byte hashes, output offsets and
string/entity identities. Full-byte comparisons distinguish hash collisions,
including embedded NUL and Unicode; equal values share dictionary records.
After emission, the same table memory becomes the writer's validation index.
It retains no complete decoded model and performs no internal heap allocation.

`panel_config_v1_to_v2_bounds` reports conservative workspace/output bounds.
Callers may use a smaller fixed scratch budget; exhausted dictionary/index space
returns a failure with no usable output instead of allocating more memory or
clipping records. These bounds describe the wire conversion, not device capacity
or an admission decision. Profile and legacy byte/chunk checks are still needed
before exposing either direction through the service. Settings may receive
different dictionary IDs across implementations; their resolved graph and exact
saved values must agree. The registered cross-language task checks both
directions, fixed budgets, distinct-field fixtures and opaque identity ordering.
