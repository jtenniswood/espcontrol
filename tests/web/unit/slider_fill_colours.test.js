"use strict";

const assert = require("node:assert/strict");
const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { cardSliderFillColor, cardPreviewTextColor, cardPatternHighlightColor } = loadTypescriptTest("src/webserver/features/preview.ts");
const { activeCardColor } = loadTypescriptTest("src/webserver/model/config_primitives.ts");

function luminance(hex) {
  const rgb = parseInt(hex.replace(/^#/, ""), 16);
  const linear = channel => {
    const value = channel / 255;
    return value <= 0.04045 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4;
  };
  return linear(rgb >> 16) * 0.2126 + linear((rgb >> 8) & 255) * 0.7152 + linear(rgb & 255) * 0.0722;
}

function contrast(a, b) {
  const first = luminance(a), second = luminance(b);
  return (Math.max(first, second) + 0.05) / (Math.min(first, second) + 0.05);
}

test("slider fills retain readable foregrounds without changing toggle on colours", () => {
  assert.equal(activeCardColor("6633B9"), "A385D5");
  assert(contrast("FFFFFF", activeCardColor("6633B9")) < 4.5);
  assert.equal(cardSliderFillColor("6633B9"), "826AAA");
  assert(contrast("FFFFFF", cardSliderFillColor("6633B9")) > 4.5);
  assert.equal(cardSliderFillColor("#ffffff"), "BFBFBF");
  assert.equal(cardSliderFillColor("invalid"), "");
});

test("custom fills maintain contrast at partial and full values across the preset palette", () => {
  const fs = require("node:fs");
  const path = require("node:path");
  const swatches = fs.readFileSync(path.resolve(__dirname, "../../../src/webserver/components/color_swatches.ts"), "utf8");
  const presets = Array.from(swatches.matchAll(/\["([A-F0-9]{6})", "[^"]+"\]/g), match => match[1]);
  assert.equal(presets.length, 19);
  for (const base of [...presets, "FFFFFF", "000000", "6633B9", "00BCD4", "FFEC16", "BFEAFF"]) {
    const foreground = cardPreviewTextColor(base);
    const fill = cardSliderFillColor(base);
    assert.notEqual(fill, base, base + " has a visible fill");
    // At partial values text may cover either surface; the fill must not
    // introduce lower contrast than the base or fall below 4.5:1 unnecessarily.
    assert(contrast(foreground, fill) + 0.0001 >= Math.min(4.5, contrast(foreground, base)), base);
  }
});

test("grey slider fills stay visible, including colours near the former rounding collision", () => {
  for (let value = 0; value <= 255; value++) {
    const base = value.toString(16).padStart(2, "0").repeat(3).toUpperCase();
    assert.notEqual(cardSliderFillColor(base), base, base);
  }
  for (const base of ["6D6D6D", "6D6D6E", "6D6E6D", "6E6D6D", "6E6E6D", "6D6E6E", "6E6D6E", "6E6E6E"]) {
    assert.notEqual(cardSliderFillColor(base), base, base);
    assert(contrast(cardPreviewTextColor(base), cardSliderFillColor(base)) >= 4.5, base);
  }
});

test("stripe tones preserve the label contrast of the active background", () => {
  assert.equal(activeCardColor("A0A0A0"), "787878");
  assert.equal(cardPatternHighlightColor("787878"), "A7A7A7");
  assert.equal(cardPatternHighlightColor("#bfbfbf"), "D5D5D5");
  assert.equal(cardPatternHighlightColor("invalid"), "");
  for (const base of ["A0A0A0", "6D6D6D", "6633B9", "FF0000", "FF8C00", "00BCD4", "FFEC16", "FFFFFF", "000000"]) {
    const active = activeCardColor(base);
    const foreground = cardPreviewTextColor(active);
    assert(contrast(foreground, cardPatternHighlightColor(active)) >= contrast(foreground, active), base);
  }
});

test("striped custom cards render the readable stripe tones on both pages and themes", () => {
  const path = require("node:path");
  const vm = require("node:vm");
  const esbuild = require("esbuild");
  const root = path.resolve(__dirname, "../../..");
  const source = esbuild.buildSync({
    stdin: { contents: `
      export { createPreviewRenderFeature } from "./src/webserver/application/preview_render";
      export { initializeAppState, state } from "./src/webserver/state/app_instance";
      export { initializeDeviceConfig } from "./src/webserver/device_config";
    `, resolveDir: root, loader: "ts" },
    bundle: true, format: "cjs", platform: "node", write: false, logLevel: "silent",
  }).outputFiles[0].text;
  const module = { exports: {} };
  vm.runInNewContext(source, {
    module, exports: module.exports,
    __ESPCONTROL_DEFAULT_DEVICE_ID__: "test",
    __ESPCONTROL_DEVICE_PROFILES__: { test: { slots: 1, cols: 1, rows: 1, features: {}, timezoneOptions: [] } },
    __ESPCONTROL_TIMEZONE_OPTIONS__: [],
  });
  const ui = module.exports;
  ui.initializeDeviceConfig();
  ui.initializeAppState();
  function element() {
    const node = { children: [], className: "", style: { setProperty(name, value) { this[name] = value; } }, setAttribute() {}, removeAttribute() {}, appendChild(child) { this.children.push(child); } };
    node.classList = { contains(name) { return node.className.split(" ").includes(name); } };
    return node;
  }
  for (const theme of ["Dark", "Light"]) for (const subpage of [false, true]) {
    ui.state.themeMode = theme;
    const main = element();
    const button = { type: "", entity: "switch.test", label: "Grey", icon_on: "Lightbulb", options: "on_pattern=stripes,card_off_color=A0A0A0" };
    ui.createPreviewRenderFeature({
      document: { createElement: element }, updateClockBarItemUi() {},
      layout: { config: { disabledCardTypes: [], infoOnly: false } }, cards: { definitions: {} },
      confirmationOptions: { cardOnPattern() { return "stripes"; } }, codec: { getSubpage() { return {}; } },
      runtime: { els: { previewMain: main } }, screenRotation: { gridPreviewBlocked() { return false; } },
      shell: { isConfigLocked() { return false; } },
      grid: { ctx() { return { grid: [1], buttons: [button], maxSlots: 1, sizes: {}, selected: [], isSub: subpage }; }, resolveIcon() { return "lightbulb"; }, sizeClass() { return ""; } },
      selection: { renderSelectionBar() {}, updatePreviewHint() {} },
    }).render();
    const card = main.children[0];
    assert.equal(card.style["--card-text-color"], "#000000", `${theme}/${subpage}`);
    assert(card.style.backgroundImage.includes("#787878"));
    assert(card.style.backgroundImage.includes("#A7A7A7"));
    assert(!card.style.backgroundImage.includes("rgba(255,255,255"));
  }
});

test("active presets choose a readable foreground for solid and striped cards", () => {
  assert.equal(activeCardColor("AAC93F"), "80972F");
  assert.equal(activeCardColor("EED146"), "B39D35");
  assert.equal(cardPreviewTextColor("80972F"), "#212121");
  assert.equal(cardPreviewTextColor("B39D35"), "#212121");
  const fs = require("node:fs");
  const path = require("node:path");
  const swatches = fs.readFileSync(path.resolve(__dirname, "../../../src/webserver/components/color_swatches.ts"), "utf8");
  const presets = Array.from(swatches.matchAll(/\["([A-F0-9]{6})", "[^"]+"\]/g), match => match[1]);
  for (const base of [...presets, "FFFFFF", "A0A0A0"]) {
    const active = activeCardColor(base);
    const foreground = cardPreviewTextColor(active);
    assert(contrast(foreground, active) >= 4.5, base);
    assert(contrast(foreground, cardPatternHighlightColor(active)) >= 4.5, base + " stripes");
  }
});

test("foreground selection meets contrast across a sample of the full RGB space", () => {
  for (let red = 0; red <= 255; red += 17) for (let green = 0; green <= 255; green += 17) for (let blue = 0; blue <= 255; blue += 17) {
    const color = [red, green, blue].map(channel => channel.toString(16).padStart(2, "0")).join("");
    assert(contrast(cardPreviewTextColor(color), color) >= 4.5 - 0.00005, color);
  }
});
