# Developing EspControl

This guide is for contributors working on the firmware or the web configurator —
running things locally, flashing devices, reading logs, and adding or fixing a
widget (card type). For end-user install/usage, see `README.md` and the
[documentation site](https://jtenniswood.github.io/espcontrol/).

---

## 1. Repository layout

| Path | What lives here |
|---|---|
| `common/` | Shared ESPHome YAML: theme, screens, addons, and `config/` (the card contract, entity names) and `assets/` (icons, glyph sets). |
| `common/config/card_contract.json` | **Source of truth** for card types: labels, allowed entity domains, options, defaults. Generates JS and C++ headers (see §3). |
| `components/espcontrol/*.h` | Header-only C++ for the on-device UI: LVGL grid, card faces, modals, HA bindings. `button_grid.h` is the umbrella include. |
| `src/webserver/` | The web configurator UI (plain ES5-style JS, bundled with esbuild). `types/<card>.js` = per-card settings panel; `modules/` = shared logic (`config_codec.js`, `api.js`, `button_settings.js`, …). |
| `devices/<slug>/` | Per-device entry points and config (see §5). |
| `docs/public/webserver/<slug>/www.js` | **Built** configurator bundles, one per device, served to the device at runtime. |
| `scripts/` | Build + verification scripts (`build.py` and the `check_*` validators). |

The web UI and the firmware share card metadata through generated files, so the
two stay in sync — you describe a card once in the contract and both sides pick
it up.

---

## 2. Prerequisites

- **Node.js** (for the web bundle + JS checks). Install deps with `npm ci`
  (esbuild is a dependency and is required before any `build.py www` run).
- **Python 3** (runs `scripts/build.py` and the Python validators).
- **ESPHome** (`esphome` CLI) for compiling/flashing firmware.

```bash
npm ci
```

---

## 3. Build system

Everything generated is produced by one script:

```bash
python3 scripts/build.py            # run all generators (icons, model, devices, www)
python3 scripts/build.py www        # build only the docs/public/webserver/*/www.js bundles
python3 scripts/build.py model      # rebuild the generated web model
python3 scripts/build.py icons      # sync icon assets
python3 scripts/build.py --check    # exit non-zero if any generated output is stale
```

Key idea: **`common/config/card_contract.json` is the source of truth.** Building
regenerates, among others:

- `src/webserver/modules/card_contract_generated.js`
- `components/espcontrol/button_grid_contract_generated.h`

So card label/domain/option/default metadata is written once and consumed by both
the JS UI and the C++ firmware. After editing the contract, run `build.py` (or at
least `model` + `www`) and commit the regenerated files. `build.py --check` is the
gate that fails CI when generated files are out of date.

---

## 4. Local web configurator development

The configurator page is served **by the device**, but the `www.js` bundle it
loads is fetched at runtime from GitHub Pages:

```
https://jtenniswood.github.io/espcontrol/webserver/<slug>/www.js
```

(The default URL is set as `js_url` in `common/device/core_infra.yaml`.) To test
local JS changes, point the device at a bundle you serve yourself.

**1. Build the bundle** after any change under `src/webserver/`:

```bash
python3 scripts/build.py www
```

**2. Serve it locally** from the device's bundle directory:

```bash
python3 -m http.server 8080 --directory docs/public/webserver/<slug>
# bundle is now at http://<your-computer-ip>:8080/www.js
```

**3. Override `js_url`** so the device loads your local bundle. Add to the
device's `dev.yaml` (see §5):

```yaml
web_server:
  js_url: "http://<your-computer-ip>:8080/www.js"
```

**4. Reload the configurator** in your browser at `http://<device-ip>/`. The
browser caches `www.js` aggressively — do a **hard reload** (Cmd/Ctrl+Shift+R)
after each rebuild, or you will keep running the old bundle.

> Tip: run `npm run check:web-smoke` (or `check:web-browser-smoke`) to exercise
> the bundle's logic without a device.

---

## 5. Firmware: build & flash

Each device folder has two entry points and a packages manifest:

| File | Purpose |
|---|---|
| `esphome.yaml` | **Production** entry. Pulls the whole config as a remote package from GitHub. This is what end users flash. |
| `dev.yaml` | **Local development** entry. Builds against your working tree. |
| `packages.yaml` | The include manifest + per-device substitutions (fonts, sizes, slug). |
| `device/` | Device-specific YAML (display driver, fonts, pins). |
| `secrets.yaml` | WiFi credentials. **Gitignored — never commit.** |

`dev.yaml` builds against your **local component source** instead of the GitHub
git source, via a local `external_components` override:

```yaml
external_components:
  - source:
      type: local
      path: ../../components
    components: [espcontrol, web_server_idf]
    refresh: 1s
```

Compile and upload:

```bash
cd devices/<slug>
esphome run dev.yaml
```

**Choosing the upload target.** If both a USB serial port and an
over-the-air (OTA) target are available, ESPHome prompts you to pick one. In a
non-interactive context (scripts, background runs) that prompt aborts the upload,
so always pass `--device` explicitly:

```bash
esphome run dev.yaml --device 192.168.x.x         # OTA to a network device
esphome run dev.yaml --device /dev/cu.usbserial-… # USB serial
esphome run dev.yaml --device <ip> --no-logs      # skip the post-upload log stream
```

OTA only works once the device is already running EspControl firmware and is on
the network. First flash is over USB.

---

## 6. Logs & on-device debugging

**Stream logs over the network** (ESPHome native API):

```bash
esphome logs dev.yaml --device <device-ip>
```

- Boot-time logs print **once** at startup. Connecting *after* boot does not
  replay them — connect at boot (or trigger the relevant code path again) to see
  setup-time output.
- Add logging in the C++ headers with the ESPHome macros, e.g.
  `ESP_LOGI("mytag", "value=%s", v.c_str());` (`ESP_LOGD` for debug-level). Remove
  or downgrade noisy logs before finalizing.

**Inspect what the device actually stored** via the web_server REST API — useful
for confirming a config round-tripped correctly:

```bash
# Per-button config strings are stored in text entities "Button N Config"
curl -s "http://<device-ip>/text/Button%201%20Config?detail=all"
# Any text/select/number/switch entity works the same way
curl -s "http://<device-ip>/text/Button%20On%20Color?detail=all"
```

The configurator writes button config by POSTing the serialized string to these
same text entities, so reading them back shows exactly what will be parsed on the
device.

---

## 7. Adding or fixing a widget (card type)

A card type spans the contract, the web UI, and the firmware. Touch these in
order:

1. **`common/config/card_contract.json`** — register the type: `label`, allowed
   `domains`, any `options` (name/kind/values/default), and the `default` config.
   Then run `python3 scripts/build.py` to regenerate the shared JS/C++ contract
   files and commit them.

2. **`src/webserver/types/<card>.js`** — `registerButtonType("<card>", {...})`:
   - `renderSettings(panel, b, slot, helpers)` — the settings form.
   - `renderPreview(b, helpers)` — the tile preview in the configurator.
   - `onSelect(b)` — defaults applied when the type is chosen.
   Files in `types/` are auto-discovered (the build reads the directory), so
   creating the file is enough — then `build.py www`. (Files in `modules/` are
   ordered explicitly via `scripts/web_modules.json`, not auto-discovered.)

3. **`src/webserver/modules/config_codec.js`** — if the card stores **options**,
   add the option get/set/normalize helpers, and **add the type to the
   option-preservation exclusions** (see the gotcha in §10 — this is the most
   common reason a new card's settings silently fail to save).

4. **`components/espcontrol/button_grid_<card>.h`** — the firmware: card-face
   setup + render, optional modal, and HA state/attribute subscriptions. Keep it
   header-only and `inline`.

5. **`components/espcontrol/button_grid.h`** — `#include` your new header. Mind
   include order if it depends on helpers defined in other card headers.

6. **`components/espcontrol/button_grid_grid.h`** — wire the type into both passes:
   the **visual setup** pass (`if (p.type == "<card>") setup_…`) and the
   **runtime/subscription** pass (create the card context, subscribe, attach the
   tap handler). There are usually two call sites: the main grid and subpages.

7. **`components/espcontrol/button_grid_config.h`** — `parse_cfg` ALSO normalizes
   options and has the same wipe-unknown-options clause; if your card uses
   options, exclude the type there too (again, see §10).

8. **Modal?** If the card opens a full-screen modal, add a `ControlModalKind`
   enum value and use the shared `control_modal_open_shell(...)` helper.

Then rebuild the web bundle (`build.py www`), flash `dev.yaml`, and verify.

---

## 8. Worked example: a "Hello World" widget

A complete minimal card type called `hello` with **one setting** — a `name` that
replaces "world", so the tile shows **"Hello &lt;name&gt;"** — plus a settings
page, the live tile, and an optional tap-to-open modal. Use it as a template.

### 8.1 Register the card in the contract

`common/config/card_contract.json` — add to `cards`:

```json
"hello": {
  "label": "Hello",
  "allowInSubpage": true,
  "domains": [],
  "options": [
    { "name": "name", "label": "Name", "kind": "text", "defaultValue": "" }
  ],
  "default": {
    "entity": "", "label": "", "icon": "Auto", "icon_on": "Auto",
    "sensor": "", "unit": "", "type": "hello", "precision": "", "options": ""
  }
}
```

Then regenerate the shared JS/C++ contract files and commit them:

```bash
python3 scripts/build.py
```

### 8.2 Option helpers + persistence exclusions

`src/webserver/modules/config_codec.js` — add get/set helpers for the `name`
option:

```js
function helloName(b) {
  return configOptionValue(b && b.options, "name");
}

function setHelloName(b, name) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options || "", "name", String(name || "").trim());
  return b.options;
}
```

**Critical:** add `"hello"` to the two "keep options for this type" exclusions in
the same file, or the setting will silently fail to save (see §10):

```js
// in normalizeButtonConfig(...)  — the catch-all that clears unknown options:
} else if (b && b.type !== "action" && /* …other types… */ && b.type !== "hello" && !cardLargeNumbersSupported(b)) {
  b.options = "";
}

// in buttonConfigFields(...)     — the serialization catch-all:
} else if (type !== "action" && /* …other types… */ && type !== "hello" && !cardLargeNumbersSupported({ type: type, precision: precision })) {
  options = "";
}
```

### 8.3 The settings page + preview

`src/webserver/types/hello.js` (files in `types/` are auto-discovered):

```js
var HELLO_CARD_METADATA = {
  nameField: {
    label: "Name",
    idSuffix: "name",
    placeholder: "world",
    bindName: null,                                   // bound to options, not a top-level field
    value: function (b) { return helloName(b); },
  },
  preview: { badge: "hand-wave" },                    // an MDI icon name for the type badge
};

registerButtonType("hello", {
  label: function () { return cardContractCardLabel("hello"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("hello"); },
  pickerKey: function () { return cardContractPickerKey("hello"); },
  hidden: function () { return cardContractHidden("hello"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("hello"); },
  cardMetadata: HELLO_CARD_METADATA,

  renderPreview: function (b, helpers) {
    var greeting = "Hello " + (helloName(b) || "world");
    return {
      labelHtml: cardBadgeLabelHtml(helpers, greeting, HELLO_CARD_METADATA.preview.badge),
    };
  },

  onSelect: function (b) {
    b.entity = ""; b.label = ""; b.sensor = ""; b.unit = "";
    b.precision = ""; b.icon = "Auto"; b.icon_on = "Auto"; b.options = "";
  },

  renderSettings: function (panel, b, helpers) {
    var field = helpers.renderCardTextField(panel, b, helpers, HELLO_CARD_METADATA.nameField);
    field.input.maxLength = 32;
    function save() {
      setHelloName(b, field.input.value);
      helpers.saveField("options", b.options);   // marks the draft dirty + persists
      scheduleRender();                           // refresh the live preview
    }
    field.input.addEventListener("input", save);
    field.input.addEventListener("change", save);
    field.input.addEventListener("blur", save);
  },
});
```

Rebuild the bundle and hard-reload the configurator (§4): `python3 scripts/build.py www`.

### 8.4 The live tile (firmware)

`components/espcontrol/button_grid_hello.h`:

```cpp
#pragma once
#include <string>
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "button_grid_config.h"   // ParsedCfg, BtnSlot, cfg_option_value
#include "button_grid_modal.h"    // ControlModalShell, control_modal_open_shell (for §8.5)

// Build the greeting from the "name" option (defaults to "world").
inline std::string hello_greeting(const ParsedCfg &p) {
  std::string name = cfg_option_value(p.options, "name");
  if (name.empty()) name = "world";
  return "Hello " + name;
}

// Card face: hide the icon/sensor slots and show the greeting in the text label.
inline void setup_hello_card(BtnSlot &s, const ParsedCfg &p) {
  if (s.icon_lbl) lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.sensor_container) lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (s.text_lbl) lv_label_set_text(s.text_lbl, hello_greeting(p).c_str());
}
```

Include it from `components/espcontrol/button_grid.h` (order matters only if it
depends on helpers from other card headers):

```cpp
#include "button_grid_hello.h"
```

Wire the **visual-setup** pass in `components/espcontrol/button_grid_grid.h`
(inside `setup_card_visual`, alongside the other `if (p.type == ...)` blocks):

```cpp
if (p.type == "hello") {
  setup_hello_card(s, p);
  return;
}
```

That's a complete, working tile. Because the greeting comes from static config
(no HA entity), nothing else is needed. **For live data**, subscribe to a Home
Assistant entity in the runtime pass and re-render on state changes — model it on
an existing data-driven card (e.g. the sensor or media headers).

### 8.5 An optional modal (tap to open)

Add a modal kind to the `ControlModalKind` enum (in `button_grid_modal.h`):

```cpp
enum class ControlModalKind { /* …existing… */ HELLO };
```

Add a context + open function in `button_grid_hello.h`:

```cpp
struct HelloCardCtx {
  std::string greeting;
  lv_obj_t *btn = nullptr;
  const lv_font_t *title_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
};

inline void hello_modal_hide() { /* nothing extra to clean up */ }

inline void hello_open_modal(HelloCardCtx *ctx) {
  if (!ctx) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::HELLO, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141" /* chevron-left back icon */, false, hello_modal_hide);
  lv_obj_t *lbl = lv_label_create(shell.panel);
  lv_label_set_text(lbl, ctx->greeting.c_str());
  if (ctx->title_font) lv_obj_set_style_text_font(lbl, ctx->title_font, LV_PART_MAIN);
  lv_obj_center(lbl);
}

inline HelloCardCtx *create_hello_card_context(
    BtnSlot &s, const ParsedCfg &p,
    const lv_font_t *title_font, const lv_font_t *icon_font, int width_compensation_percent) {
  HelloCardCtx *ctx = new HelloCardCtx();
  ctx->greeting = hello_greeting(p);
  ctx->btn = s.btn;
  ctx->title_font = title_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  lv_obj_set_user_data(s.btn, ctx);
  return ctx;
}
```

Wire the **runtime** pass in `button_grid_grid.h` (the loop that attaches
behavior, separate from the visual pass — there are usually two call sites: main
grid and subpages):

```cpp
if (p.type == "hello") {
  HelloCardCtx *ctx = create_hello_card_context(
    s, p,
    display_media_title_font_or(display, lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN)),
    display_icon_font(display),
    display_main_width_percent(display));
  lv_obj_add_event_cb(s.btn, [](lv_event_t *e) {
    HelloCardCtx *ctx = static_cast<HelloCardCtx *>(lv_event_get_user_data(e));
    if (ctx) hello_open_modal(ctx);
  }, LV_EVENT_CLICKED, ctx);
  continue;
}
```

### 8.6 Build, flash, verify

```bash
python3 scripts/build.py            # regenerate contract + bundles
cd devices/<slug> && esphome run dev.yaml --device <ip>
```

Then in the configurator: add a **Hello** card, type a name, Save, and confirm the
tile reads "Hello &lt;name&gt;" and the setting survives a reload (§10). Read it
back over REST to be sure: `curl "http://<ip>/text/Button%20N%20Config?detail=all"`.

---

## 9. Fonts, glyphs, and images

The on-device UI is built **entirely from fonts and icon glyphs** — there are no
raster images in the firmware UI by design (memory on these panels is tight).

**Fonts** live in `devices/<slug>/device/fonts.yaml`, one entry per style id
(e.g. body text, title, sensor numbers, icons). A few things to know:

- **Weight** is chosen via the Google-Fonts suffix on the `file:` — `gfonts://Roboto`
  (regular), `gfonts://Roboto@Light`, `@Thin`, `@Medium`, `@Bold`. There is no
  runtime "bold" toggle in LVGL; a heavier weight means a different font id.
- **Size** is per-id and device-specific. Reference fonts by their **substitution
  alias** (set in `packages.yaml`, e.g. `label_font`, `sensor_value_font`,
  `icon_font`) rather than hardcoding a physical id, so each device keeps its own
  sizing behind the same name.
- A font only contains the **glyphs you list**. `font_number_*` fonts include only
  digits and a few symbols (`0-9 . - ° / % :`), so writing letters into them renders
  missing-glyph boxes. Text fonts include the set in `common/assets/text_glyphs.yaml`.
  If you need a character that isn't currently included, add it to the relevant
  glyph set (`common/assets/*_glyphs.yaml`) so it's compiled into the font.

**`devices/<slug>/device/sensors.yaml` is GENERATED — do not hand-edit it.** It is
produced by `scripts/generate_device_slots.py` (with `scripts/device_profiles.py`)
from the per-device `firmware.fonts` role map in `devices/manifest.json`. Manual
edits there will be reported as "stale" by `python3 scripts/generate_device_slots.py
--check` (part of `check:product`). To expose a **new font role** to a card's C++
`cfg`, wire it through all three layers, then regenerate:

1. Add the role → physical font id mapping under each device's `firmware.fonts` in
   `devices/manifest.json` (e.g. `"tiny": "font_text_tiny"`), and add the matching
   `font:` entry to each `devices/<slug>/device/fonts.yaml`.
2. In `scripts/device_profiles.py`, read it into the slot dict
   (`"tiny_font": fonts.get("tiny")`).
3. In `scripts/generate_device_slots.py`, emit the assignment
   (`cfg.tiny_font = id(font_text_tiny)->get_lv_font();`).
4. Run `python3 scripts/generate_device_slots.py` to rewrite every `sensors.yaml`,
   and add the field to the C++ `GridConfig`/`DisplayProfile` so the card can read it.


**Icons** are MDI (Material Design Icons) glyphs from the MDI webfont, built into
the icon fonts via `common/assets/icon_glyphs.yaml`. In firmware, resolve an icon
name to its glyph string with `find_icon("Some Name")` (see
`components/espcontrol/icons.h`), or embed a codepoint directly as a UTF-8 escape
(e.g. `"\U000F0141"` is chevron-left). Only codepoints present in the compiled
glyph set will render — an absent one shows as a rectangle. To use a new icon,
add it to the icon set and run `python3 scripts/build.py icons`. In the web UI,
icons are referenced by MDI name (e.g. the `badge` in a card's preview metadata)
and rendered with the MDI webfont/CSS.

**Images:** the project convention is to use icon fonts, not bitmaps, so you won't
find an `image:` component in the device YAML. If you genuinely need a raster
image you'd add an ESPHome `image:` (or `online_image:`) entry and render it via
an LVGL image widget — but prefer an icon glyph first; it's smaller, themeable,
and scales with the font system.

---

## 10. Other gotchas worth knowing

**Option persistence has THREE wipe points.** For robustness, several places
clear the `options` field for card types they don't recognize. A card that stores
options must be excluded from **all** of them, or its settings won't persist:

- `src/webserver/modules/config_codec.js` → `normalizeButtonConfig` (in-editor
  normalization) **and** `buttonConfigFields` (serialization to the device).
- `components/espcontrol/button_grid_config.h` → `parse_cfg` (parsing on the
  device).

If options save in the editor but revert after reload, or apply but show defaults
on the device, you've missed one of these exclusions.

**Fonts/glyphs are limited to compiled glyphs** (icon labels show garbage for
ASCII; number fonts show boxes for letters) — see §9 for the full rundown.

**LVGL layout basics.** A container won't lay its children out unless you set a
layout (`lv_obj_set_layout(..., LV_LAYOUT_FLEX)` + a flex flow) — otherwise
children stack at (0,0). To clamp + ellipsize a label, give it a fixed
width/height and `lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT)`.

**The card context outlives a reconfigure.** Grid widgets (`button_N`) are
persistent; reconfiguring rebuilds card contexts and re-points `user_data`. If you
create an `lv_timer` or async callback bound to a context, guard it against the
button having been re-pointed to a different context before writing to shared
labels.

---

## 11. Pre-commit checks

Run before committing — these validate the contract, generated outputs, web
bundle, and firmware parsers:

```bash
npm run check:fast        # full fast suite (schema, generated, config, web smoke, firmware parsers, …)
npm run check:product     # product-focused gate (contract + generated + web smoke + runtime)
python3 scripts/build.py --check   # fail if any generated file is uncommitted/stale
```

If you changed generated inputs (the card contract, icons, entity names), make
sure you committed the regenerated outputs — `--check` is what catches a missed
rebuild.
