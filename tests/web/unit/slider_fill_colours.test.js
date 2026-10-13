"use strict";

const assert = require("node:assert/strict");
const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { cardSliderFillColor, cardPreviewTextColor } = loadTypescriptTest("src/webserver/features/preview.ts");
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
  assert.equal(presets.length, 20);
  for (const base of [...presets, "000000", "6633B9", "00BCD4", "FFEC16", "BFEAFF"]) {
    const foreground = cardPreviewTextColor(base);
    const fill = cardSliderFillColor(base);
    assert.notEqual(fill, base, base + " has a visible fill");
    // At partial values text may cover either surface; the fill must not
    // introduce lower contrast than the base or fall below 4.5:1 unnecessarily.
    assert(contrast(foreground, fill) + 0.0001 >= Math.min(4.5, contrast(foreground, base)), base);
  }
});
