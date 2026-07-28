# Firmware

Most EspControl firmware behavior is implemented as ESPHome components and
header-only C++ under `components/espcontrol/`.

## Important Files

| Path | Purpose |
|---|---|
| `components/espcontrol/button_grid.h` | Umbrella include for grid/card code. |
| `components/espcontrol/button_grid_grid.h` | Main grid creation, card setup, runtime wiring, and subpage wiring. |
| `components/espcontrol/button_grid_config.h` | Compact saved config parser and normalized `ParsedCfg`. |
| `components/espcontrol/button_grid_<type>.h` | Card-specific rendering and runtime behavior. |
| `components/espcontrol/button_grid_modal.h` | Shared modal registry, lifecycle, LVGL shell, and layout adapters. |
| `components/espcontrol/button_grid_modal_layout.h` | Pure device-aware frame, tab, and content layout recipes. |
| `components/espcontrol/button_grid_subpages.h` | Subpage support. |
| `components/espcontrol/icons.h` | Icon lookup. |
| `components/espcontrol/i18n_generated.h` | Generated translation strings. |
| `components/espcontrol/config_api.h` | HTTP config API (`/api/config`), see below. |
| `components/espcontrol/entity_backup_map_generated.h` | Generated envelope field -> entity map. |

## Runtime Model

1. ESPHome YAML creates LVGL objects and exposes text/select/number/switch
   entities.
2. The grid code reads saved button config from text entities.
3. `parse_cfg` normalizes the saved compact string into `ParsedCfg`.
4. Visual setup creates the card face.
5. Runtime wiring subscribes to Home Assistant state where needed and attaches
   tap/hold handlers.
6. Some cards open a shared full-screen modal.

Visual setup and runtime wiring are separate. A new card often needs both.

## Adding Firmware Support for a Card

1. Create `components/espcontrol/button_grid_<type>.h`.
2. Include it in `components/espcontrol/button_grid.h`.
3. Add visual setup in `components/espcontrol/button_grid_grid.h`.
4. Add runtime/subscription behavior in `button_grid_grid.h` if the card reacts
   to Home Assistant state or user taps.
5. Update `components/espcontrol/button_grid_config.h` if the saved config or
   options need parser support.
6. Add modal enum/context behavior when the card opens a modal.

Use an existing card with similar behavior as the template:

- Static display card: sensor-like or time-like cards.
- Toggle/action card: switch/action cards.
- Rich modal card: media, climate, or light cards.
- Image loading card: camera or media cover-art behavior.

## Modal Pattern

Cards that open a full-screen detail view use the shared modal system. See
[Modal Layout System](modal-layout-system.md) for the full ownership model.

1. Add a value to `ControlModalKind` in `button_grid_modal.h`.
2. Add its presentation, chrome, and dismissal policy to
   `control_modal_definition(...)`.
3. Store the card's runtime state in a small context struct.
4. Save the context on the button with `lv_obj_set_user_data`.
5. Attach a click handler in the runtime pass.
6. Open the modal with `control_modal_open_shell(...)`.
7. Use the shared tab row and content layout recipes instead of repeating modal
   frame geometry inside the card header.

For a simple static card, the context usually needs the button pointer, display
font pointers, width compensation, and any text or state the modal should render.
For cards that subscribe to Home Assistant state, keep the context updated from
the subscription callback and guard async work as described in the LVGL gotchas
below.

Modal layout changes must preserve the geometry fixtures for every display
profile or deliberately update the fixtures and generated visual reference.
Run:

```bash
npm run check:firmware-modal-layouts
npm run check:firmware-modals
```

## Fonts and Glyphs

Firmware fonts only contain the glyphs declared in YAML. Missing glyphs render
as boxes.

- Device font definitions: `devices/<slug>/device/fonts.yaml`
- Shared glyph sets: `common/assets/*glyphs.yaml`
- Icon registry: `common/assets/icons.json`
- Icon lookup in firmware: `components/espcontrol/icons.h`

Use font role substitutions from device profiles instead of hardcoding one
device's physical font id in card logic.

## Home Assistant Bindings

Cards that reflect Home Assistant state must subscribe to the entity or
attribute they need. Keep subscriptions narrow because display memory and update
work are limited.

Useful checks:

```bash
npm run check:firmware-ha-bindings
npm run check:firmware-card-runtime
```

## Config Parser Rules

`button_grid_config.h` should accept existing saved values after an upgrade. Be
careful with:

- renamed card types
- renamed option keys
- new required fields
- default values that change behavior
- clearing unknown options

When parser behavior changes, update compatibility fixtures and run:

```bash
npm run check:firmware-parser
npm run check:backup-contract
npm run check:product
```

## HTTP Config API

`config_api.h` serves the `espcontrol.backup` v2 envelope over HTTP for reads and
writes. User-facing documentation is `docs/reference/config-api.md`. Four things
about it are load-bearing:

**Entity writes must run on the main loop.** A template entity's `set_action:` is
arbitrary YAML, and in this repo that YAML drives LVGL and the scheduler - the
`Screen: Clock Bar` switch runs a script with a `delay:` in it, which is an async
state machine only the main loop pumps. Writing an entity from the httpd task
hard-locks the panel: pingable, no HTTP, no serial, until a power cycle. Use
`run_on_main_loop()`, batch a whole request into one handoff, and `App.feed_wdt()`
every ~16 writes. Reads are safe from the httpd task. ESPHome's own `web_server`
follows the same rule (`web_server.cpp` defers switch actions).

**Reading a JSON body depends on fork behaviour.** POST is registered on a
wildcard URI, and for a content type it does not understand
`web_server_idf.cpp`'s `request_post_handler` logs a warning and falls through to
the GET path *before* its size check and its only `httpd_req_recv()` - so the body
is still in the socket when our handler runs. `check_firmware_ha_bindings.py`
guards that ordering, because a fork sync that reorders it would not fail to
compile, it would silently make import see an empty body.

**Never materialise a response.** Responses stream through `JsonStream` in ~1 KB
chunks. This is not tidiness: by the time an import's validation pass finishes,
internal RAM is fragmented down to a ~10 KB largest free block while ~28 KB is
still free. A `std::string` that grows past ~5 KB asks for one contiguous piece
bigger than that, `malloc` returns null, and because `operator new` cannot throw in
this build the panel **panics and reboots** instead of failing the request. A
rejected import did exactly that. `check_firmware_ha_bindings.py` guards it.

**Register through `global_web_server_base->add_handler()`**, never
`global_async_web_server()->addHandler()` or `add_handler_without_auth()`. Only
the first wraps the handler in `AuthMiddlewareHandler`, so only the first inherits
`web_server: auth:` when a user enables it. Same guard script enforces this for
every handler in the component, and that registration happens in
`core_infra.yaml`'s `on_boot` at priority 600 - not from an `api:` trigger, or the
API would not exist until Home Assistant connected.

**The envelope map is generated and gated in both directions.** Add a setting and
`build.py entities` fails until it either declares `backup: {section, field}` in
`entity_names.json` or is listed in `backupUnmapped` with a reason; add a field to
`app_backup.ts`'s export literal and it fails until the firmware can write it.
Fields that are not a plain 1:1 entity read/write are marked `derived: true` and
handled by a hand-written rule in `config_api.h`.

The component is header-only and everything lands in one generated `main.cpp`, so
the S3 needs `-mtext-section-literals`, which `__init__.py` already adds for that
variant. It must stay gated to Xtensa - it is a hard error on RISC-V, which is every
other device in the fleet. If a link fails with `l32r: literal target out of range`,
that is what it means: `main.cpp` has outgrown the 256 KB window `l32r` can reach.

## ESPHome Entry Points

Each device has:

- `devices/<slug>/esphome.yaml` - production entry, pulls remote packages.
- `devices/<slug>/dev.yaml` - local development entry, points components at the
  working tree.
- `devices/<slug>/packages.yaml` - package and substitution manifest.

For local firmware work, build from `dev.yaml`.

## Logs and Debugging

Stream logs over the ESPHome native API:

```bash
cd devices/<slug>
esphome logs dev.yaml --device <device-ip>
```

Boot-time logs print once at startup. Connecting after boot does not replay them,
so connect before rebooting or trigger the relevant behavior again.

Use ESPHome logging macros in firmware headers:

```cpp
ESP_LOGI("mytag", "value=%s", v.c_str());
ESP_LOGD("mytag", "debug value=%s", v.c_str());
```

Remove or downgrade noisy logs before finalizing a change.

## LVGL Gotchas

- A container does not lay out children unless a layout is set, such as
  `lv_obj_set_layout(..., LV_LAYOUT_FLEX)` plus a flex flow.
- Labels that should clamp need a fixed width or height and
  `lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT)`.
- Grid widgets such as `button_N` are persistent. Reconfiguring a card rebuilds
  its context and points `user_data` at the new context. If a card creates a
  timer or async callback, check that the button still points at the same context
  before writing to shared labels.
