#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { loadBuiltWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const SOURCE = path.join(ROOT, "src", "webserver", "entry.ts");
const COMPAT_FIXTURES = path.join(ROOT, "compatibility", "fixtures", "product_compatibility.json");

function loadHooks(search = "") {
  const params = new URLSearchParams(search);
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    location: { search },
    URLSearchParams,
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    document: {
      readyState: "loading",
      activeElement: null,
      addEventListener() {},
    },
  };
  const device = params.get("device");
  if (device) sandbox.__ESPCONTROL_DEVICE_PROFILE__ = device;
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox, { filename: SOURCE });
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
}

function plain(value) {
  return JSON.parse(JSON.stringify(value));
}

function buttonShape(b) {
  return {
    entity: b.entity || "",
    label: b.label || "",
    icon: b.icon || "Auto",
    icon_on: b.icon_on || "Auto",
    sensor: b.sensor || "",
    unit: b.unit || "",
    type: b.type || "",
    precision: b.precision || "",
    options: b.options || "",
  };
}

function throwsBackupMessage(fn, expected) {
  assert.throws(fn, (err) => {
    assert.strictEqual(err.backupMessage, expected);
    return true;
  });
}

const hooks = loadHooks();
const s3Hooks = loadHooks("?device=guition-esp32-s3-4848s040");
const fixtures = JSON.parse(fs.readFileSync(COMPAT_FIXTURES, "utf8"));
const legacyV1 = fixtures["legacy-v1"];
assert(hooks, "web config helpers were not exported");

const v2 = hooks.createBackupConfig({
  device: "panel-a",
  slots: 3,
  exported_at: "2026-05-24T12:00:00.000Z",
  grid: [1, 2, 0],
  sizes: { 2: 2 },
  button_on_color: "AA0000",
  buttons: [
    { entity: "light.kitchen", label: "Kitchen", icon: "Auto", icon_on: "Lightbulb" },
    { entity: "weather.home", label: "Weather", type: "weather_forecast" },
    {},
  ],
  subpages: {
    1: {
      order: ["1", "B"],
      buttons: [{ entity: "scene.movie", label: "Movie", icon: "Flash", type: "action", sensor: "scene.turn_on" }],
    },
  },
  settings: {
    timezone: "Europe/London (GMT+0)",
    clock_bar: true,
    cover_art_hide_external_input: true,
    home_assistant_artwork_protocol: "https",
    home_assistant_artwork_port: 80,
    firmware_auto_update: false,
    firmware_update_frequency: "Weekly",
  },
  screen: { brightness_day: 80, schedule_mode: "clock" },
});

assert.strictEqual(v2.version, 2, "exports v2 backups");
assert.strictEqual(v2.format, "espcontrol.backup", "exports backup format marker");
assert.deepStrictEqual(plain(v2.source), { device: "panel-a", slots: 3 }, "exports source metadata");
assert.strictEqual(v2.button_order, "1,2d", "exports legacy-compatible button_order");
assert(Array.isArray(v2.buttons), "exports legacy-compatible buttons array");
assert.strictEqual(typeof v2.subpages["1"], "string", "exports legacy-compatible subpage strings");
assert.deepStrictEqual(plain(v2.subpage_objects["1"]), {
  order: ["1", "B"],
  back_label: "Back",
  buttons: [{
    entity: "scene.movie",
    label: "Movie",
    icon: "Flash",
    icon_on: "Auto",
    sensor: "scene.turn_on",
    unit: "",
    type: "action",
    precision: "",
    options: "",
  }],
}, "exports readable structured subpage objects");
assert.strictEqual(v2.buttons[1].type, "weather", "exports canonical card types");
assert.strictEqual(v2.buttons[1].precision, "tomorrow", "exports migrated card details");
assert.strictEqual(v2.settings.cover_art_hide_external_input, true, "exports cover art external-input setting");
assert.strictEqual(v2.settings.home_assistant_artwork_protocol, "https", "exports Home Assistant artwork protocol setting");
assert.strictEqual(v2.settings.home_assistant_artwork_port, 80, "exports Home Assistant artwork port setting");
assert.strictEqual(v2.settings.firmware_auto_update, false, "exports firmware auto-update setting");
assert.strictEqual(v2.settings.firmware_update_frequency, "Weekly", "exports firmware update frequency setting");

const playlistButton = {
  entity: "media_player.kitchen",
  label: "Morning Mix",
  icon: "Music",
  icon_on: "Auto",
  sensor: "playlist",
  unit: "",
  type: "media",
  precision: "",
  options: "",
};
hooks.setMediaPlaylistContentId(playlistButton, "media-source://music/morning,mix=50%");
hooks.setMediaPlaylistContentType(playlistButton, "music");
hooks.setMediaPlaylistPlayerSource(playlistButton, "Kitchen, Main=Zone 50%");
const playlistBackup = hooks.createBackupConfig({
  device: "panel-a",
  slots: 1,
  exported_at: "2026-05-24T12:00:00.000Z",
  grid: [1],
  buttons: [playlistButton],
});
assert.strictEqual(
  playlistBackup.buttons[0].options,
  "playlist_content_id=media-source%3A//music/morning%2Cmix=50%25,playlist_content_type=music,playlist_player_source=Kitchen%2C Main=Zone 50%25",
  "backup exports encoded media playlist option values"
);
const normalizedPlaylistBackup = hooks.normalizeBackupConfig(playlistBackup);
assert.strictEqual(
  hooks.mediaPlaylistContentId(normalizedPlaylistBackup.buttons[0]),
  "media-source://music/morning,mix=50%",
  "backup normalization keeps media playlist content ID punctuation"
);
assert.strictEqual(
  hooks.mediaPlaylistPlayerSource(normalizedPlaylistBackup.buttons[0]),
  "Kitchen, Main=Zone 50%",
  "backup normalization keeps media playlist player source punctuation"
);
const playlistImportPlan = hooks.planBackupImport(playlistBackup, { device: "panel-a", slots: 1 });
assert.deepStrictEqual(plain(playlistImportPlan.warnings), [], "playlist backup same-device import has no warnings");
assert.strictEqual(
  hooks.mediaPlaylistContentId(playlistImportPlan.buttons[0]),
  "media-source://music/morning,mix=50%",
  "backup import keeps media playlist content ID punctuation"
);
assert.strictEqual(
  hooks.mediaPlaylistPlayerSource(playlistImportPlan.buttons[0]),
  "Kitchen, Main=Zone 50%",
  "backup import keeps media playlist player source punctuation"
);

const normalizedV1 = hooks.normalizeBackupConfig({
  version: 1,
  device: "panel-a",
  button_order: "1,2",
  buttons: [
    { entity: "weather.home", label: "Weather", icon: "Auto", icon_on: "Auto", type: "weather_forecast" },
    { entity: "sensor.washer", label: "Washer", icon: "Washer", icon_on: "Auto", type: "text_sensor" },
  ],
  subpages: {
    1: "1,B|media_player.living:Living:Speaker:Auto:controls::media",
  },
});

assert.strictEqual(normalizedV1.version, 2, "v1 imports normalize to v2");
assert.strictEqual(normalizedV1.format, hooks.BACKUP_FORMAT, "v1 imports gain the v2 marker");
assert.deepStrictEqual(plain(normalizedV1.source), { device: "panel-a", slots: 2 }, "v1 imports preserve source metadata");
assert.deepStrictEqual(buttonShape(normalizedV1.buttons[0]), buttonShape({
  entity: "weather.home",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  type: "weather",
  precision: "tomorrow",
}), "legacy weather card migrates inside a backup");
assert.deepStrictEqual(buttonShape(normalizedV1.buttons[1]), buttonShape({
  entity: "",
  label: "",
  icon: "Washer",
  icon_on: "Auto",
  type: "sensor",
  precision: "text",
}), "legacy text sensor card migrates inside a backup");
assert(
  normalizedV1.subpages["1"].includes("play_pause") || normalizedV1.subpages["1"].includes("M,"),
  "legacy subpage config is normalized"
);
assert.strictEqual(
  normalizedV1.subpage_objects["1"].buttons[0].sensor,
  "play_pause",
  "legacy subpage imports also gain readable structured objects"
);

const structuredOverride = hooks.normalizeBackupConfig({
  version: 2,
  format: hooks.BACKUP_FORMAT,
  device: "panel-a",
  button_order: "1",
  buttons: [{ entity: "light.kitchen", label: "Kitchen" }],
  subpages: {
    1: "1,B|scene.old:Old:Flash::scene.turn_on::action",
  },
  subpage_objects: {
    1: {
      order: ["1", "B"],
      back_label: "Back",
      buttons: [{ entity: "scene.new", label: "New", icon: "Flash", sensor: "scene.turn_on", type: "action" }],
    },
  },
});
assert(structuredOverride.subpages["1"].includes("scene.new"), "structured subpage object overrides stale string subpage");
assert(!structuredOverride.subpages["1"].includes("scene.old"), "stale string subpage is ignored when structured object exists");

const sameDevicePlan = hooks.planBackupImport(v2, { device: "panel-a", slots: 3 });
assert.deepStrictEqual(plain(sameDevicePlan.warnings), [], "same-device import has no warnings");
assert.strictEqual(sameDevicePlan.button_order, "1,2d", "same-device import keeps order");
assert.deepStrictEqual(buttonShape(sameDevicePlan.buttons[1]), buttonShape(v2.buttons[1]), "same-device import keeps migrated button");
assert(sameDevicePlan.subpages["1"], "same-device import keeps subpages");

const crossDevicePlan = hooks.planBackupImport(legacyV1.backup, { device: "small-panel", slots: 2 });

assert.strictEqual(crossDevicePlan.importedCount, 4, "legacy-v1 backup cross-device import records source slot count");
assert(crossDevicePlan.warnings.some((msg) => msg.includes("different panel")), "legacy-v1 backup cross-device import warns on device mismatch");
assert(crossDevicePlan.warnings.some((msg) => msg.includes("4 slots")), "legacy-v1 backup cross-device import warns on slot adaptation");
assert.deepStrictEqual(buttonShape(crossDevicePlan.buttons[0]), buttonShape({
  entity: "weather.home",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  type: "weather",
  precision: "tomorrow",
}), "legacy-v1 backup cross-device import preserves first used old slot");
assert.deepStrictEqual(buttonShape(crossDevicePlan.buttons[1]), buttonShape({
  entity: "light.kitchen",
  label: "Kitchen",
  icon: "Auto",
  icon_on: "Lightbulb",
}), "legacy-v1 backup cross-device import fills remaining target slots in order");
assert(crossDevicePlan.subpages["1"], "legacy-v1 backup cross-device import remaps subpages to the new slot");

const structuredCrossDevicePlan = hooks.planBackupImport({
  version: 2,
  format: hooks.BACKUP_FORMAT,
  device: "panel-a",
  button_order: "2,1,3",
  buttons: [
    { entity: "light.kitchen", label: "Kitchen" },
    { entity: "light.living", label: "Living" },
    { entity: "light.office", label: "Office" },
  ],
  subpage_objects: {
    2: {
      order: ["1", "B"],
      back_label: "Back",
      buttons: [{ entity: "scene.relax", label: "Relax", icon: "Flash", sensor: "scene.turn_on", type: "action" }],
    },
  },
}, { device: "small-panel", slots: 2 });
assert(structuredCrossDevicePlan.subpages["1"], "structured subpage cross-device import remaps subpage to new slot");
assert.strictEqual(
  structuredCrossDevicePlan.subpages["1"].buttons[0].label,
  "Relax",
  "structured subpage cross-device import keeps readable object content"
);

const unsupportedImageBackup = {
  version: 2,
  format: hooks.BACKUP_FORMAT,
  device: "panel-a",
  button_order: "1",
  buttons: [{ type: "image", entity: "camera.front_door", label: "Front Door" }],
};
throwsBackupMessage(
  () => s3Hooks.planBackupImport(unsupportedImageBackup, {
    device: "guition-esp32-s3-4848s040",
    slots: 9,
  }),
  "This controller does not support the image card type in this backup."
);
throwsBackupMessage(
  () => s3Hooks.planBackupImport({
    version: 2,
    format: hooks.BACKUP_FORMAT,
    device: "panel-a",
    button_order: "1",
    buttons: [{ type: "subpage", label: "Cameras" }],
    subpage_objects: {
      1: {
        order: ["1", "B"],
        back_label: "Back",
        buttons: [{ type: "image", entity: "camera.front_door", label: "Front Door" }],
      },
    },
  }, {
    device: "guition-esp32-s3-4848s040",
    slots: 9,
  }),
  "This controller does not support the image card type in this backup."
);

throwsBackupMessage(
  () => hooks.normalizeBackupConfig({ version: 999, buttons: [] }),
  "Backup was created by a newer version of EspControl"
);
throwsBackupMessage(
  () => hooks.normalizeBackupConfig({ version: 2, format: "other", buttons: [] }),
  "Invalid config file - unsupported backup format"
);
throwsBackupMessage(
  () => hooks.normalizeBackupConfig({ version: 2, buttons: [] }),
  "Invalid config file - unsupported backup format"
);
throwsBackupMessage(
  () => hooks.normalizeBackupConfig({ version: 1 }),
  "Invalid config file - missing required fields"
);
assert.throws(() => JSON.parse("{"), SyntaxError, "malformed JSON still fails before contract validation");

console.log("Backup contract tests passed.");
