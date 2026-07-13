# Web Configurator

The web configurator is the browser setup page loaded from a device's web
server. It is written as plain JavaScript modules and bundled into a single
`www.js` file per supported device.

## Source Layout

| Path | Purpose |
|---|---|
| `src/webserver/entry.js` | Bundle entry point. |
| `src/webserver/modules/` | Shared state, rendering, API, backup, settings, preview, and codec logic. |
| `src/webserver/types/` | Card-specific settings panels and previews. |
| `src/webserver/model/*.ts` | Typed model sources. |
| `src/webserver/state/*.ts` | Typed device configuration, application state factory, event aliases, and event parsing. |
| `src/webserver/api/*.ts` | Injectable HTTP transport, ordered POST queue, typed request results, and failure classification. |
| `src/webserver/generated/card_contract.ts` | Typed card metadata generated from the shared contract. |
| `scripts/web_modules.json` | Explicit order for shared modules. |
| `docs/public/webserver/<slug>/www.js` | Generated per-device bundles used for bundled firmware and hosted compatibility. |

Files in `src/webserver/types/` are discovered by the build. Shared files in
`src/webserver/modules/` must be listed in `scripts/web_modules.json`.
The TypeScript model and generated card contract are imported directly by the
bundle build, with no separate JavaScript generation step.
The application exposes one mutable state instance created by
`createInitialState(deviceConfig)`; tests create isolated instances from the
same factory.
Controllers keep responsibility for banners, reconnect scheduling, and UI
locking; the typed device API owns transport, fallback attempts, throttling,
keepalive requests, and JSON decoding.

## Build

```bash
python3 scripts/build.py www
```

That command writes `docs/public/webserver/<slug>/www.js` for each supported
device. Commit those generated bundles when web behavior changes.

The configurator page itself is served by the device. New build entry points in
`builds/*.yaml` bundle the matching JavaScript with `web_server.js_include`, so
a flashed branch uses that branch's setup UI. The generated files are still
published for older firmware that loads the hosted GitHub Pages copy:

```text
https://jtenniswood.github.io/espcontrol/webserver/<slug>/www.js
```

The fallback hosted bundle URL is set as `js_url` in
`common/device/core_infra.yaml`. Keep that path stable for older installed
firmware and imported configs.

## Device API Shape

The setup page reads and writes ESPHome web server entities exposed by the
device. Button configuration is saved in text entities such as:

```text
Button 1 Config
Button 2 Config
...
```

The setup page serializes card settings to a compact string. Firmware parses the
same string on-device. Keep `src/webserver/modules/config_codec.js` and
`components/espcontrol/button_grid_config.h` in sync.

To inspect what the device actually stored, read the matching ESPHome web server
entity:

```bash
curl -s "http://<device-ip>/text/Button%201%20Config?detail=all"
curl -s "http://<device-ip>/text/Button%20On%20Color?detail=all"
```

The setup page writes to these same text/select/number/switch entities, so the
REST response shows the exact compact string firmware will parse.

## Adding a Card Settings UI

For a card type named `example`, create or update:

```text
src/webserver/types/example.js
```

The usual registration shape is:

```js
registerButtonType("example", {
  label: function () { return cardContractCardLabel("example"); },
  defaultConfig: function () { return cardContractDefaultConfig("example"); },
  renderPreview: function (b, helpers) { /* return preview pieces */ },
  renderSettings: function (panel, b, helpers) { /* add form fields */ },
  onSelect: function (b) { /* initialize fields */ },
});
```

Prefer contract helpers for labels, defaults, picker behavior, and visibility so
the setup page stays aligned with firmware metadata.

## Preview and Persistence Rules

- Update the draft object first.
- Use the existing helper save functions where available.
- Schedule a preview refresh after changing fields that affect the tile.
- Keep option-backed fields in the `options` string, not as new top-level fields,
  unless the saved config format intentionally changes.
- Confirm reload behavior. If the setting saves but vanishes after reload, check
  option preservation in `config_codec.js`.

## Local Testing

Run the lightweight web checks:

```bash
npm run check:web-smoke
npm run check:web-browser-smoke
```

For full product-facing changes:

```bash
npm run check:product
```

To test on a physical display, rebuild `www.js`, serve it locally, and override
the device `web_server.js_url` in a local `dev.yaml`.

Typical physical-device loop:

1. Rebuild the bundle:

```bash
python3 scripts/build.py www
```

2. Serve the generated device bundle from the development machine:

```bash
python3 -m http.server 8080 --directory docs/public/webserver/<slug>
```

3. Override `js_url` in the device's local `dev.yaml`:

```yaml
web_server:
  js_url: "http://<your-computer-ip>:8080/www.js"
```

4. Open `http://<device-ip>/` in a browser and hard reload after each rebuild.

Browsers cache `www.js` aggressively. Use Cmd/Ctrl+Shift+R after rebuilding or
the page may keep running the previous bundle.
