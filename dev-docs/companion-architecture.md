# Companion architecture

Companion is a local connector and must remain independent of Home Assistant's
API, cards, and runtime. Its product contract is authored in
`product/v2/companion_capabilities.json`; `python3 scripts/build.py companion`
generates the matching C++, TypeScript, Swift, and output manifest.

## Ownership boundaries

- `components/companion/` owns TLS, pairing, authentication, protocol parsing,
  catalogue transfer, and artwork transport.
- `components/espcontrol/companion_*` owns the device-facing Companion state and
  card integration. It must not add behaviour to Home Assistant card drivers.
- `src/webserver/cards/companion.ts` and the Companion settings modules own the
  browser experience. Saved cards must remain editable while the Mac is offline.
- `macos/Companion/` owns macOS permissions, approved resources, providers, and
  action execution. Folder paths and arbitrary commands never cross the protocol.

Protocol v3 uses typed JSON control messages on `/companion/v3`; only bounded
artwork chunks are binary. New messages and card modes must be added to the
product contract before they are implemented. Pairing is activated on the
physical touchscreen and the browser status endpoint never returns its code.

## Compatibility and testing

Saved panel configuration compatibility remains separate from transport
compatibility. Changes should run generated-output checks, TypeScript checks,
web smoke tests, firmware parser tests, a 4848S040 compile, and a Swift build.
Physical pairing, Accessibility actions, reconnects, and artwork still require
testing with matching firmware and Companion builds.
