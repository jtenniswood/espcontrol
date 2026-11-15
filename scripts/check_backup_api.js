#!/usr/bin/env node
"use strict";

// Pass a file emitted by the actual firmware serializer through the existing
// browser importer, including native bytes and cross-device layout migration.
const assert = require("assert");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { execFileSync } = require("child_process");
const { loadTypeScriptModule } = require("./load_typescript_module");
const { loadBuiltWebSource } = require("./web_source");
const vm = require("vm");
const ROOT = path.resolve(__dirname, "..");
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "espcontrol-backup-api-"));
try {
  const binary = path.join(temporary, "backup-test");
  execFileSync(process.env.CXX || "c++", ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
    path.join(ROOT, "tests/firmware/panel_backup_test.cpp"), "-o", binary]);
  execFileSync(binary);
  const backup = JSON.parse(execFileSync(binary, ["--fixture"], { encoding: "utf8" }));
  const model = loadTypeScriptModule(path.join(ROOT, "src/webserver/model/index.ts"));
  const sandbox = {
    __ESPCONTROL_DEVICE_PROFILE__: backup.device,
    __ESPCONTROL_TEST_HOOKS__: {}, console: {log() {}, warn() {}, error() {}},
    location: {search: "?device=" + backup.device}, URLSearchParams, TextEncoder, TextDecoder,
    setTimeout, clearTimeout, requestAnimationFrame: fn => setTimeout(fn, 0),
    document: {readyState: "loading", activeElement: null, addEventListener() {}},
  };
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox);
  const feature = sandbox.__ESPCONTROL_TEST_HOOKS__.config;
  assert.equal(backup.buttons[2].options.length, 3500);
  const plan = feature.planBackupImport(backup);
  assert.equal(plan.warnings.length, 0);
  assert.equal(plan.buttons[0].entity, "light.kitchen");
  assert.equal(plan.buttons[0].label, 'Kitchen; "table"\n°');
  assert.equal(plan.buttons[1].unit, "°C");
  assert.equal(plan.buttons[8].entity, "");
  assert.equal(plan.subpages["1"].buttons[0].entity, "scene.movie");
  assert.equal(plan.subpages["2"].buttons[0].label, "Movie: night");
  assert.equal(plan.subpages["2"].backLabel, "Back; home");
  assert.equal(plan.settings.clock_bar, false);
  assert.equal(plan.settings.clock_bar_temperature_precision, 2);
  assert.equal(plan.settings.language, "de");
  assert.equal(plan.screen.brightness_day, 42);
  assert.equal(plan.screen.schedule_sensor_entity, "binary_sensor.room");
  assert.equal(plan.config.identity.name, "Kitchen");
  const native = model.decodePanelConfig(model.decodePanelConfigBackupPayload(backup.native_config));
  assert.equal(native.deviceProfile, backup.device);
  assert.equal(model.parseRawButtonConfig(native.buttons[1]).label, plan.buttons[0].label);
  assert.equal(native.subpages[1], backup.subpages["1"]);
  assert.equal(native.settings.button_order, backup.button_order);
  assert.equal(native.settings.button_on_color, backup.button_on_color);
  // Verify the redundant readable fields also restore without native support.
  delete backup.native_config;
  const portable = feature.planBackupImport(backup, {device: "other-panel", slots: 12});
  assert.equal(portable.buttons.length, 12);
  assert.equal(portable.buttons[0].label, plan.buttons[0].label);
  assert.equal(portable.subpages["1"].buttons[0].entity, "scene.movie");
  console.log("Firmware API backup imports and migrates successfully in the browser model.");
} finally {
  fs.rmSync(temporary, {recursive: true, force: true});
}
