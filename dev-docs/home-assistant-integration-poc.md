# Home Assistant integration proof of concept

This is a local prototype and is intentionally not prepared as a Home
Assistant Core pull request. Its purpose is to prove discovery, pairing,
entity cataloguing, and configurator pieces before we choose a production
native API transport.

## Current protocol

Each experimental firmware build publishes an additional `_espcontrol._tcp`
mDNS service. The existing `_esphomelib._tcp` service remains available so the
firmware can be rolled back without losing normal ESPHome tooling.

The additional service advertises:

| TXT key | Meaning |
| --- | --- |
| `protocol` | Integer protocol version, currently `1` |
| `device_id` | Stable Wi-Fi MAC address |
| `model` | EspControl device profile |
| `web_port` | Existing device web server port |

The custom integration uses the MAC address as its config-entry unique ID, so a
DHCP address change does not create a second device. This follows Home
Assistant's discovery and unique-ID rules.

Pairing is deliberately two-stage:

1. An authenticated Home Assistant session calls `POST /api/espcontrol/{id}/pair`.
2. Home Assistant returns a random, ten-minute grant scoped to that device.
3. The grant is stored only as a SHA-256 digest in the integration process.
4. The device-hosted configurator sends the grant in
   `Authorization: Bearer <grant>` when requesting catalogue pages.

The grant is not a Home Assistant long-lived token. It cannot call arbitrary
Home Assistant APIs, is rejected for another device, and is revoked when it
expires or when the integration is restarted. The mDNS-discovered device host
is the only allowed browser origin for catalogue CORS requests.

## Catalogue contract

`GET /api/espcontrol/{id}/entities` accepts:

| Query key | Purpose |
| --- | --- |
| `q` | Friendly name, entity ID, device, or area search |
| `field` | Card field rule such as `light`, `media_player`, or `entity` |
| `area` | Exact area filter |
| `limit` / `cursor` | Bounded pagination |
| `include_hidden` / `include_disabled` | Explicit advanced results |

The response contains entity ID, domain, friendly name, device and area names,
device class, icon, unit, availability, registry flags, and a normalized
capability list. It intentionally excludes raw state values and arbitrary
attributes, which prevents camera URLs or other private attributes from being
copied to the display.

The field rules cover the existing card contract: lights, switches, fans,
climate, covers, locks, alarms, media players, vacuums, lawn mowers, weather,
cameras/images, selectors, numbers, sensors, binary sensors, text sensors,
actions, and secondary entity references. An unknown field uses the generic
all-domain rule so a future Home Assistant domain remains selectable.

The web configurator keeps the current remembered-entity suggestions as a
fallback. When a pairing grant exists in the URL fragment, it queries the
catalogue as the user types. Manual entity IDs remain valid for offline editing
and migration.

## Home Assistant quality checks to run in a HA checkout

Install the repository through HACS as a custom **Integration** repository, or
copy `custom_components/espcontrol` into a test Home Assistant configuration
for runtime testing. For Core-style validation, place it under
`homeassistant/components/espcontrol` in a disposable checkout and run:

```bash
python3 -m script.hassfest
pre-commit run --all-files
pytest -q tests/components/espcontrol
```

The integration follows the relevant documented rules: manifest and config
flow, discovery with a stable identifier, a device registry entry, translated
config/service strings, setup-time service registration, bounded HTTP data,
and tests for pairing and field matching. Core acceptance still requires an
independent review of the native device communication library and the full
Bronze quality-scale test suite.

## Known POC boundary

The current runtime probes the existing device HTTP identity endpoint and owns
the catalogue API, but it does not yet replace ESPHome's native API connection
for state subscriptions and actions. That transport must be implemented and
tested before this can be considered a production integration. Keeping that
boundary explicit prevents the catalogue experiment from silently creating a
second action path or duplicating ESPHome state delivery.
