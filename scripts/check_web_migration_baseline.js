#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const zlib = require("zlib");
const { freshWebOutputDir, loadBuiltWebSource } = require("./web_source");
const { loadTypeScriptModule } = require("./load_typescript_module");

const ROOT = path.resolve(__dirname, "..");
const FIXTURE_PATH = path.join(ROOT, "compatibility", "fixtures", "web_migration_baseline.json");
const MANIFEST_PATH = path.join(ROOT, "devices", "manifest.json");
const MODEL_PATH = path.join(ROOT, "src", "webserver", "model", "index.ts");
const WEB_OUTPUT_DIR = path.join(ROOT, "docs", "public", "webserver");

function plain(value) {
  return JSON.parse(JSON.stringify(value));
}

function loadRuntime() {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    document: { readyState: "loading", activeElement: null, addEventListener() {} },
  };
  sandbox.window = sandbox;
  sandbox.globalThis = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox, { filename: "web-migration-baseline.js" });
  return sandbox;
}

function loadModel() {
  return loadTypeScriptModule(MODEL_PATH);
}

const fixture = JSON.parse(fs.readFileSync(FIXTURE_PATH, "utf8"));
const manifest = JSON.parse(fs.readFileSync(MANIFEST_PATH, "utf8"));
const runtime = loadRuntime();
const hooks = runtime.__ESPCONTROL_TEST_HOOKS__.config;
const model = loadModel();

assert.deepStrictEqual(
  Object.keys(manifest.devices || {}),
  fixture.deviceProfiles,
  "browser and VM coverage must retain the six characterized device profiles in stable order"
);

const button = hooks.parseButtonConfig(fixture.button.input);
assert.deepStrictEqual(plain(button), fixture.button.normalized, "button normalization changed");
assert.strictEqual(hooks.serializeButtonConfig(button), fixture.button.serialized, "button serialization changed");

const subpage = hooks.parseSubpageConfig(fixture.subpage.input);
assert.strictEqual(hooks.serializeSubpageConfig(subpage), fixture.subpage.serialized, "subpage serialization changed");

const currentPanel = {
  timezone: "UTC (GMT+0)", temperatureUnit: "Auto", language: "en",
  clockFormat: "12h", clockFormatOptions: ["12h", "24h"],
  ntpDefaults: ["pool.ntp.org", "time.nist.gov", "time.google.com"],
  ntpServer1: "pool.ntp.org", ntpServer2: "time.nist.gov", ntpServer3: "time.google.com",
  screensaverMode: "off", screensaverAction: "main", coverArtHomeAssistantProtocol: "http",
  coverArtHomeAssistantPort: 80, updateFrequency: "Daily", screenRotation: "0",
  autoUpdate: true, updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
  screenRotationOptions: ["0", "90", "180", "270"],
};
const panel = plain(model.normalizeBackupPanelSettings(fixture.settings.panelInput, currentPanel));
assert.deepStrictEqual(panel, fixture.settings.panelNormalized, "panel settings normalization changed");

const screen = plain(model.normalizeBackupScreenSettings(fixture.settings.screenInput, {}));
assert.deepStrictEqual(screen, fixture.settings.screenNormalized, "screen settings normalization changed");

const backup = plain(hooks.createBackupConfig({
  device: "panel-a", slots: 2, exported_at: "2026-07-12T12:00:00.000Z",
  grid: [1, 2], sizes: { 2: 2 },
  buttons: [{ entity: "light.kitchen", label: "Kitchen", type: "switch" }, {}],
  subpages: {}, settings: { timezone: "Europe/London (GMT+0)" },
  screen: { brightness_day: 80 },
}));
assert.deepStrictEqual(backup, fixture.backup, "backup export structure changed");

assert.deepStrictEqual(Array.from(hooks.voiceServicesPostUrls(true)), fixture.postUrls.voiceServicesOn,
  "voice-services fallback request ordering changed");
assert.deepStrictEqual(Array.from(hooks.coverArtDelayPostUrls(30)), fixture.postUrls.coverArtDelay30,
  "cover-art fallback request ordering changed");

const freshOutput = freshWebOutputDir({ testHooks: false });
for (const slug of fixture.deviceProfiles) {
  const bytes = fs.readFileSync(path.join(freshOutput, slug, "www.js"));
  const source = bytes.toString("utf8");
  assert(/^\s*(?:["']use strict["'];)?\(\(\)=>\{/.test(source),
    `${slug}: bundle must remain a normal browser IIFE`);
  assert(!/\b(?:import|export)\s/.test(source), `${slug}: bundle must not require module script loading`);
  assert.deepStrictEqual({ minified: bytes.length, gzip: zlib.gzipSync(bytes, { level: 9, mtime: 0 }).length },
    fixture.bundleSizes[slug], `${slug}: minified or compressed migration baseline changed`);
}

console.log("Web migration characterization baseline checks passed.");
