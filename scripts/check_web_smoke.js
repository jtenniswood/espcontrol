#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { loadBundledWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const SOURCE = path.join(ROOT, "src", "webserver", "www.js");
const DEVICE_MANIFEST = path.join(ROOT, "devices", "manifest.json");
const WEB_OUTPUT_DIR = path.join(ROOT, "docs", "public", "webserver");
const ALL_ROTATIONS = ["0", "90", "180", "270"];
const LARGE_LANDSCAPE_ROTATION_DEVICES = new Set([
  "guition-esp32-p4-jc1060p470",
  "guition-esp32-p4-jc8012p4a1",
]);

function loadHooks() {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    document: {
      readyState: "loading",
      activeElement: null,
      addEventListener() {},
    },
  };
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBundledWebSource(), sandbox, { filename: SOURCE });
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
}

function plain(value) {
  return JSON.parse(JSON.stringify(value));
}

function assertGeneratedRotationOptions(slug, generated, key, options) {
  assert(
    generated.includes(`${key}:${JSON.stringify(options)}`),
    `${slug}: generated web UI must include ${key} ${JSON.stringify(options)}`
  );
}

const hooks = loadHooks();
assert(hooks, "web test hooks were not exported");

const manifest = JSON.parse(fs.readFileSync(DEVICE_MANIFEST, "utf8"));
for (const [slug, device] of Object.entries(manifest.devices || {})) {
  if (!device.rotation || !device.rotation.enabled) continue;
  const webOutput = path.join(WEB_OUTPUT_DIR, slug, "www.js");
  const generated = fs.readFileSync(webOutput, "utf8");
  const featureConfig = generated.match(/features:\{[^}]*\}/)?.[0] || "";
  assert(
    /features:\{[^}]*screenRotation:!0/.test(generated),
    `${slug}: generated web UI must expose screen rotation when rotation is enabled`
  );
  if (LARGE_LANDSCAPE_ROTATION_DEVICES.has(slug)) {
    assert.deepStrictEqual(device.rotation.options, ["0", "180"], `${slug}: normal rotation options`);
    assert.deepStrictEqual(device.rotation.experimentalOptions, ["90", "270"], `${slug}: dev-only rotation options`);
    assertGeneratedRotationOptions(slug, featureConfig, "screenRotationOptions", ["0", "180"]);
    assertGeneratedRotationOptions(slug, featureConfig, "screenRotationExperimentalOptions", ["90", "270"]);
  } else {
    assert.deepStrictEqual(device.rotation.options, ALL_ROTATIONS, `${slug}: normal rotation options`);
    assert.strictEqual(device.rotation.experimentalOptions, undefined, `${slug}: no dev-only rotation options`);
    assertGeneratedRotationOptions(slug, featureConfig, "screenRotationOptions", ALL_ROTATIONS);
    assert(
      !featureConfig.includes("screenRotationExperimentalOptions"),
      `${slug}: generated web UI must not hide rotation options behind the dev flag`
    );
  }
}

const button = {
  entity: "light.kitchen",
  label: "Kitchen",
  icon: "Auto",
  icon_on: "Lightbulb",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
  options: "",
};

const encoded = hooks.serializeButtonConfig(button);
assert.strictEqual(encoded, "light.kitchen;Kitchen;Auto;Lightbulb");
assert.deepStrictEqual(plain(hooks.parseButtonConfig(encoded)), button);

const confirmationButton = {
  entity: "switch.printer",
  label: "3D Printer",
  icon: "Printer 3D",
  icon_on: "Printer 3D",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
  options: "confirm_off,confirm_message=Stop the print?,confirm_yes=Power Down,confirm_no=Keep On",
};
const confirmationRoundTrip = hooks.parseButtonConfig(hooks.serializeButtonConfig(confirmationButton));
assert.deepStrictEqual(plain(confirmationRoundTrip), confirmationButton);
assert.strictEqual(hooks.switchConfirmationEnabled(confirmationRoundTrip), true);
assert.strictEqual(hooks.switchConfirmationMessage(confirmationRoundTrip), "Stop the print?");
assert.strictEqual(hooks.switchConfirmationYesText(confirmationRoundTrip), "Power Down");
assert.strictEqual(hooks.switchConfirmationNoText(confirmationRoundTrip), "Keep On");

assert.strictEqual(hooks.normalizeTemperatureUnit("fahrenheit"), "\u00b0F");
assert.strictEqual(hooks.normalizeTemperatureUnit("centigrade"), "\u00b0C");
assert.strictEqual(hooks.normalizeScreensaverAction("Screen Dimmed"), "dim");
assert.strictEqual(hooks.previewHtmlValue({ labelHtml: "" }, "labelHtml", "fallback"), "");
assert.strictEqual(hooks.previewHtmlValue({}, "labelHtml", "fallback"), "fallback");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 24), "wifi-strength-1");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 25), "wifi-strength-2");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 50), "wifi-strength-3");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 75), "wifi-strength-4");
assert.strictEqual(hooks.networkPreviewIconSlug("ethernet", 0), "ethernet");
assert.strictEqual(hooks.firmwareUpdateControlsVisibleFor("wifi", true), true);
assert.strictEqual(hooks.firmwareUpdateControlsVisibleFor("wifi", false), false);
assert.strictEqual(hooks.firmwareUpdateControlsVisibleFor("ethernet", true), false);

console.log("Web UI smoke tests passed.");
