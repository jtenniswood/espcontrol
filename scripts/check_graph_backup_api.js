#!/usr/bin/env node
"use strict";

// Roundtrip actual C++ graph-backup output through the current browser importer.
const assert = require("assert");
const fs = require("fs");
const os = require("os");
const path = require("path");
const vm = require("vm");
const {execFileSync} = require("child_process");
const {loadTypeScriptModule} = require("./load_typescript_module");
const {loadBuiltWebSource} = require("./web_source");
const ROOT = path.resolve(__dirname, "..");
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "espcontrol-graph-backup-"));
try {
  const binary = path.join(temporary, "graph-backup-test");
  execFileSync(process.env.CXX || "c++", ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
    "-I", path.join(ROOT, "components/espcontrol"), "-I", path.join(ROOT, "tests/firmware"),
    path.join(ROOT, "tests/firmware/panel_graph_backup_test.cpp"), "-o", binary]);
  execFileSync(binary);
  const backup = JSON.parse(execFileSync(binary, ["--fixture"], {encoding: "utf8"}));
  const original = execFileSync(binary, ["--native"]);
  const native = loadTypeScriptModule(path.join(ROOT, "src/webserver/model/native_config_backup.ts"));
  const cards = loadTypeScriptModule(path.join(ROOT, "src/webserver/model/index.ts"));
  const contract = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/card_contract.ts"));
  const sandbox = {
    __ESPCONTROL_DEVICE_PROFILE__: backup.device, __ESPCONTROL_TEST_HOOKS__: {},
    console: {log() {}, warn() {}, error() {}},
    location: {search: "?device=" + backup.device}, URLSearchParams, TextEncoder, TextDecoder,
    setTimeout, clearTimeout, requestAnimationFrame: fn => setTimeout(fn, 0),
    document: {readyState: "loading", activeElement: null, addEventListener() {}},
  };
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox);
  const feature = sandbox.__ESPCONTROL_TEST_HOOKS__.config;
  assert.equal(backup.version, 3);
  assert.equal(backup.native_config.document_version, 2);
  assert.equal(backup.buttons.length, 9);
  assert.equal(backup.buttons[1].type, "subpage");
  assert.equal(backup.buttons[1].label, "\n".repeat(500));
  assert.equal(backup.buttons[1].options.length, 3500);
  const bytes = native.decodeNativeConfigBackupPayload(backup.native_config);
  assert.deepStrictEqual(Buffer.from(bytes), original);
  const document = native.decodeNativeConfigDocument(bytes);
  assert.equal(document.deviceProfile, backup.device);
  const subpage = key => cards.parseRawSubpageConfig(document.subpages[key],
    code => contract.CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE[code] || code);
  assert.equal(subpage(2).buttons[0].type, "subpage");
  assert.equal(subpage(65).buttons[0].label, "Nested Leaf");
  assert.equal(subpage(32).buttons[0].type, "subpage");
  assert.equal(subpage(1025).buttons[0].options.length, 3500);
  assert.equal(subpage(1025).backLabel, "Back Up");
  assert.equal(subpage(1025).backColor, "112233");
  const plan = feature.planBackupImport(backup);
  assert.equal(plan.warnings.length, 0);
  assert.equal(plan.config.native_config.payload, backup.native_config.payload);
  assert.equal(plan.config.identity.name, "Kitchen");
  assert.equal(plan.settings.clock_bar, false);
  assert.equal(plan.screen.brightness_day, 42);
  assert.equal(plan.subpages["2"].buttons[0].type, "subpage");
  assert.equal(backup.subpage_objects["1025"].buttons[0].options.length, 3500);
  const restoredNative = native.decodeNativeConfigDocument(native.decodeNativeConfigBackupPayload(plan.config.native_config));
  assert.equal(cards.parseRawSubpageConfig(restoredNative.subpages[1025],
    code => contract.CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE[code] || code).buttons[0].options.length, 3500);
  assert.throws(() => feature.planBackupImport(backup, {device: "other-panel", slots: 12}),
    /Nested-page backups must be restored to the same device profile/);
  const missing = {...backup};
  delete missing.native_config;
  assert.throws(() => feature.planBackupImport(missing), /complete native version-two document/);
  assert.throws(() => feature.planBackupImport({...backup, version: 4}), /newer version/);
  // Flat native-two exports still need their portable first-level pages when
  // a user intentionally migrates to a different device profile.
  const flat = JSON.parse(execFileSync(binary, ["--flat-fixture"], {encoding: "utf8"}));
  assert.deepStrictEqual(Buffer.from(native.decodeNativeConfigBackupPayload(flat.native_config)),
    execFileSync(binary, ["--flat-native"]));
  assert.equal(flat.buttons[0].label, 'Kitchen; "table"\n°');
  const migrated = feature.planBackupImport(flat, {device: "other-panel", slots: 12});
  assert.equal(migrated.buttons[0].label, 'Kitchen; "table"\n°');
  assert.equal(migrated.subpages["1"].buttons[0].sensor, "sensor.room");
  assert.equal(migrated.subpages["1"].backLabel, "Return Home");
  assert.equal(migrated.subpages["1"].backColor, "334455");
  console.log("C++ graph backup preserves exact native bytes, active/inactive nested pages and browser restore metadata.");
} finally {
  fs.rmSync(temporary, {recursive: true, force: true});
}
