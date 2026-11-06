"use strict";

const { describe, test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const { customIconCodepoint, normalizeCustomIcon, iconSlug } =
  loadTypescriptTest("src/webserver/application/ui_primitives.ts");

describe("custom MDI icons", () => {
  test("accepts the same saved values as find_icon() in icons.h", () => {
    assert.equal(customIconCodepoint("mdi:F0335"), 0xF0335);
    assert.equal(customIconCodepoint("mdi:f0335"), 0xF0335);
    assert.equal(customIconCodepoint("mdi:F0F92"), 0xF0F92);
    assert.equal(customIconCodepoint("mdi:F0000"), 0xF0000);
    assert.equal(customIconCodepoint("mdi:10FFFD"), 0x10FFFD);
  });

  test("rejects the same malformed values as find_icon() in icons.h", () => {
    for (const value of [
      "mdi:", "mdi:XYZ1", "mdi:F0F92:home-flood", "mdi:0041", "mdi:1F03350",
      "mdi:E000", "mdi:F8FF", "mdi:F0EF", "mdi:DFFF", "mdi:F900", "mdi:EFFFF", "mdi:10FFFE", "MDI:F0335",
      "F0335", "Not An Icon", "", null, undefined,
    ]) {
      assert.equal(customIconCodepoint(value), null, String(value));
    }
  });

  test("normalizes what people type in the picker", () => {
    for (const input of [
      "F0F92", "f0f92", "mdi:F0F92", "MDI:f0f92", "\\U000F0F92", "\\uF0F92",
      "U+F0F92", "u+f0f92", "0xF0F92", "0XF0F92", "  F0F92  ", "mdi:0xF0F92",
    ]) {
      assert.equal(normalizeCustomIcon(input), "mdi:F0F92", input);
    }
    assert.equal(normalizeCustomIcon("\\U0010FFFD"), "mdi:10FFFD");
  });

  test("rejects typed values that are not icon codepoints", () => {
    for (const input of [
      "", "   ", "F0F", "F0EF", "e000", "U+F8FF", "F0F92X", "1F03350", "0041", "U+0041", "Lightbulb",
      "mdi:", "\\U", "0x", null, undefined,
    ]) {
      assert.equal(normalizeCustomIcon(input), null, String(input));
    }
  });

  test("normalized values round-trip through the saved-value check", () => {
    const saved = normalizeCustomIcon("u+f0335");
    assert.equal(saved, "mdi:F0335");
    assert.equal(customIconCodepoint(saved), 0xF0335);
  });

  test("gives custom icons their own preview class", () => {
    assert.equal(iconSlug("mdi:F0F92"), "custom-f0f92");
    assert.equal(iconSlug("mdi:f0f92"), "custom-f0f92");
    assert.equal(iconSlug("mdi:10FFFD"), "custom-10fffd");
  });

  test("shows the cog for malformed custom values, like the firmware", () => {
    assert.equal(iconSlug("mdi:"), "cog");
    assert.equal(iconSlug("mdi:XYZ1"), "cog");
    assert.equal(iconSlug("mdi:0041"), "cog");
    assert.equal(iconSlug("mdi:E000"), "cog");
    assert.equal(iconSlug("MDI:F0335"), "cog");
  });

  test("leaves built-in icon names unchanged", () => {
    assert.equal(iconSlug("Lightbulb"), "lightbulb");
    assert.equal(iconSlug("Power Plug"), "power-plug");
    assert.equal(iconSlug(""), "cog");
  });
});
