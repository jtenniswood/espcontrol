# Failure Cookbook

Use this when something is broken and the first cause is not obvious. Each entry
starts with the fastest place to look and the narrowest useful check.

## Setting Saves, Then Disappears After Reload

First files:

- `src/webserver/application/config_codec.ts`
- `components/espcontrol/button_grid_config.h`
- `product/v2/card_contract.json`

Likely cause: the editor writes an option, but web normalization or firmware
parsing strips it as unknown.

Fastest checks:

```bash
npm run check:web-smoke
npm run check:firmware-parser
```

Stop if the web codec and firmware parser do not preserve the same fields.

## Web Preview Works, Device Does Not

First files:

- `src/webserver/cards/<type>.ts`
- `components/espcontrol/button_grid_<type>.h`
- `components/espcontrol/button_grid_grid.h`

Likely cause: the setup page preview was updated without matching firmware
rendering, runtime wiring, or Home Assistant subscription handling.

Fastest checks:

```bash
npm run check:firmware-card-runtime
npm run check:firmware-ha-bindings
```

Stop if the saved config shown in the browser is not the same shape firmware
expects.

## Generated Files Changed Unexpectedly

First files:

- `dev-docs/source-of-truth.md`
- The authored source named by the generated file's row
- The generator listed in that same row

Likely cause: stale generated output, a generator behavior change, or manual
editing of generated files.

Fastest checks:

```bash
npm run check:generated
python3 scripts/generate_device_slots.py --check
npm run check:dev-docs
```

Stop if there is no authored source change that explains the generated diff.

## Backup Import Fails

First files:

- `product/v2/product_compatibility.json`
- `src/webserver/application/backup_contract.ts`
- `src/webserver/application/config_codec.ts`

Likely cause: the saved shape changed without a fixture or import fallback.

Fastest checks:

```bash
npm run check:backup-contract
npm run check:model-contract
```

Stop if an older backup cannot be normalized into the current model.

## P4 Works, S3 Fails

First files:

- `product/v2/device_catalog.json`
- `devices/manifest.json`
- `devices/guition-esp32-s3-4848s040/`
- `components/espcontrol/button_grid_grid.h`
- image, camera, media, and font-related headers for the feature

Likely cause: memory, image downloader count, font role, screen size, or feature
availability differs between ESP32-P4 and ESP32-S3 devices.

Fastest checks:

```bash
npm run check:device-profiles
npm run check:device-matrix
```

Before publishing, compile the affected S3 firmware.

## Card Exists in Editor, But Not on Device

First files:

- `product/v2/card_contract.json`
- `src/webserver/cards/<type>.ts`
- `components/espcontrol/button_grid.h`
- `components/espcontrol/button_grid_grid.h`

Likely cause: the card was registered in the web setup page but not included or
wired in firmware setup/runtime.

Fastest checks:

```bash
npm run check:card-contract-outputs
npm run check:firmware-card-runtime
```

Stop if a card can be saved by the web UI but has no firmware rendering path.

## Home Assistant Action Does Nothing

First files:

- `components/espcontrol/button_grid_actions.h`
- `components/espcontrol/button_grid_ha.h`
- `components/espcontrol/button_grid_contract_generated.h`
- The card-specific firmware header

Likely cause: service mapping, target entity, action permission, or runtime
context is missing.

Fastest checks:

```bash
npm run check:firmware-ha-bindings
npm run check:firmware-card-runtime
```

Also confirm the user enabled Home Assistant actions for the ESPHome device.

## Public Docs Build Fails

First files:

- The Markdown page named in the VitePress error
- `docs/.vitepress/config.mts`
- `dev-docs/card-type-map.md` when card docs moved

Likely cause: missing page, broken local link, invalid frontmatter, or sidebar
entry pointing at a removed file.

Fastest checks:

```bash
npm run docs:build
npm run check:dev-docs
```

## Camera Image Is Slow or Fails Intermittently

Use the controllable local endpoint to compare P4 image changes or reproduce
slow and broken responses:

```bash
python3 scripts/camera_test_endpoint.py --image path/to/camera-snapshot.jpg
```

The endpoint is available at `http://<computer-ip>:8765/camera.jpg`. Query
parameters can delay headers or chunks, change chunk counts, return HTTP errors,
or send malformed image data. For example,
`?header_delay_ms=500&chunk_delay_ms=100&chunks=8` simulates a slow camera;
`?status=503` and `?malformed=1` exercise failure handling.

Verify the harness first:

```bash
python3 scripts/camera_test_endpoint.py --self-test
```

When measuring on a panel, compare the same image, panel, network transport, and
Home Assistant path. Capture response, first-byte, transfer, decode, and total
timing log entries before and after the change.
