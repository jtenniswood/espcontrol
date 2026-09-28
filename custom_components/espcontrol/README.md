# EspControl Home Assistant integration proof of concept

This directory is a local custom integration prototype. It is not a Home
Assistant Core contribution.

The integration discovers an EspControl display using the `_espcontrol._tcp`
Zeroconf service, creates a config entry keyed by the device's stable ID, and
offers a short-lived pairing grant. The grant is deliberately scoped to one
device and is never a Home Assistant long-lived access token.

After pairing, the entity endpoint returns a bounded, searchable catalogue. It
combines live state with entity, device, and area registry metadata and applies
the field rules in `const.py`. Unknown fields fall back to the all-domain rule,
so a new Home Assistant domain remains selectable while the configurator is
updated.

The HTTP contract is:

```text
POST /api/espcontrol/{device_id}/pair       (Home Assistant-authenticated)
GET  /api/espcontrol/{device_id}/entities  (Authorization: Bearer <grant>)
```

Pairing grants expire after ten minutes and are held in memory in this POC; a
Home Assistant restart revokes them. Before a production implementation, the
native API transport must deliver the grant to the display over an authenticated
physical pairing flow. The browser client in `src/webserver/application/entity_catalog.ts`
consumes the catalogue and keeps a local fallback to the existing remembered
entity suggestions.

The intended catalogue response contains only selection metadata. Raw entity
attributes, camera URLs, and access tokens are never forwarded.
