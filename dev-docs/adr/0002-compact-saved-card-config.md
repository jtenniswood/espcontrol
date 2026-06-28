# ADR 0002: Compact Saved Card Config

## Status

Accepted.

## Context

Cards are saved in ESPHome text entities such as `Button N Config`. The same
saved string must be readable by the browser setup page, firmware parser, backup
export, and backup import.

## Decision

Keep the compact saved card format and treat it as durable user data.

## Why

- ESPHome text entities are simple and visible for debugging.
- Compact strings fit the constrained device environment.
- The same format supports web editing, firmware rendering, and backup files.

## Consequences

- Web and firmware parsers must change together.
- Old values need aliases, defaults, or fallback parsing.
- New settings should normally go in `options`, not new top-level fields.
- Compatibility fixtures are required when the saved shape changes.
