# Interface example renders

These PNGs are rendered from the production web card components using firmware layout measurements and browser-specific numeric calibration, not AI-generated images or physical-device screenshots.

From the repository root, with `npm ci` and Playwright Chromium installed, run:

```bash
node scripts/render_docs_examples.js
```

The renderer builds a temporary bundle with the existing test hooks, supplies the fixtures in `scripts/fixtures/docs-interface-examples.json`, and saves these files here:

- `living-room-render.png` — 1024×600, JC1060P470
- `hallway-render.png` — 480×800, JC4880P443
- `bedside-render.png` — 720×720, P4-86

## Sources and limits

- Card markup and preview styles: `src/webserver/cards/` and `src/webserver/application/styles.ts`.
- Grid, padding, corner radius, gaps and font roles: `devices/manifest.json`, generated from `product/v2/device_catalog.json`.
- Pixel font sizes and page insets: each device’s `device/fonts.yaml` and `device/lvgl.yaml`.
- Roboto and Material Design Icons: the bundled assets under `common/assets/fonts/`, embedded by the normal web build.
- Flat active, control and information colours: `src/webserver/state/ui_tokens.ts` (matching the firmware theme).

The capture script removes editor borders, badges and hover decoration, hides subpage chevrons, and supplies sample readings and active colours. It does not change the production interface. Standard numeric values render at 80% of the firmware font size, with their containers inset by half the normal card padding at the top-left. This is a visual calibration requested during device comparison, not a firmware sizing rule. The bedside example disables Large Clock.

The script verifies rendered dimensions, grid columns, padding, radius, gaps, label size, numeric size and position, font loading and hidden chevrons before saving. No live device or Home Assistant connection is used.

Browser text metrics, wrapping and antialiasing can differ from LVGL; firmware width compensation is not simulated. These are source-based documentation illustrations, not pixel-perfect firmware captures or hardware validation. Exact device screenshots would require capturing the LVGL framebuffer or running the firmware UI in an LVGL simulator.
