"use strict";
const { test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { createStateLoaderFeature } = loadTypescriptTest("src/webserver/application/state_loader_api.ts");
const { createApplicationApiFeature } = loadTypescriptTest("src/webserver/application/api.ts");
const { createDeviceApi } = loadTypescriptTest("src/webserver/api/device_api.ts");
const tick = () => new Promise(resolve => setImmediate(resolve));
function deferred() { let resolve, reject; const promise = new Promise((r, j) => { resolve = r; reject = j; }); return { promise, resolve, reject }; }
function fixture(t, read) {
  const runtime = {};
  t.after(() => clearTimeout(runtime.migrationTimer));
  const entityState = {
    entityStateItems: keys => [["text", keys.includes("button_order") ? "card" : "setting"]],
    entityStateItemsForSlots: keys => keys.includes("subpage") ? [["text", "subpage"]] : [],
  };
  return createStateLoaderFeature(runtime, { config: { features: {} } }, {}, {}, {}, {}, entityState,
    { setConfigLocked() {}, showBanner() {} }, {
      getJsonQuietly: read,
      entityDetailPath: (domain, name) => `/${domain}/${name}`,
      entityInitialDetail: () => "state",
    }, { schedule() {} }, { subpageEntityKeys: () => ["subpage"], connectEvents() {} });
}
test("backup waits for both delayed settings and subpages after the layout is available", async t => {
  const settings = deferred(), subpage = deferred();
  const loader = fixture(t, path => path.endsWith("setting") ? settings.promise : path.endsWith("subpage") ? subpage.promise : Promise.resolve({ state: "card" }));
  let layoutReady = false, backupReady = false;
  loader.loadInitialState(() => {}, () => { layoutReady = true; });
  const ready = loader.waitForInitialState().then(() => { backupReady = true; });
  await tick(); assert.equal(layoutReady, true); assert.equal(backupReady, false);
  settings.resolve({ state: "saved setting" }); await tick(); assert.equal(backupReady, false);
  subpage.resolve({ state: "saved subpage" }); await ready; assert.equal(backupReady, true);
});
test("failed initial settings block backup instead of accepting defaults", async t => {
  const loader = fixture(t, async path => { if (path.endsWith("setting")) throw new Error("offline"); return { state: "card" }; });
  loader.loadInitialState(() => {});
  await assert.rejects(loader.waitForInitialState(), /Could not read/);
});
test("backup readiness follows a reconnect that starts while the first read is pending", async t => {
  const first = deferred(), second = deferred(); let settingRead = 0;
  const loader = fixture(t, path => path.endsWith("setting") ? (++settingRead === 1 ? first.promise : second.promise) : Promise.resolve({ state: "saved" }));
  loader.loadInitialState(() => {}); await tick(); let ready = false;
  const completion = loader.waitForInitialState().then(() => { ready = true; });
  loader.loadInitialState(() => {}); await tick(); first.resolve({ state: "old" }); await tick(); assert.equal(ready, false);
  second.resolve({ state: "new" }); await completion; assert.equal(ready, true);
});
test("strict backup reads accept absent older-firmware fields but reject busy, disconnected and malformed replies", async () => {
  let mode = 404;
  const device = createDeviceApi(async () => {
    if (mode === "network") throw new Error("offline");
    return { ok: mode === 200 || mode === "bad-json", status: mode === "bad-json" ? 200 : mode,
      async json() { if (mode === "bad-json") throw new Error("malformed"); return { state: "saved" }; } };
  });
  const api = createApplicationApiFeature(null, device, {}, {}, {});
  assert.equal(await api.getJsonQuietly("/optional", undefined, { strict: true }), null);
  for (mode of [503, 401, "network", "bad-json"]) await assert.rejects(api.getJsonQuietly("/setting", undefined, { strict: true }), /Could not read/);
  mode = 200; assert.deepEqual(await api.getJsonQuietly("/setting", undefined, { strict: true }), { state: "saved" });
});

test("application export waits for readiness before reading or downloading a backup", async () => {
  const { createAppBackupFeature } = loadTypescriptTest("src/webserver/application/app_backup.ts");
  const gate = deferred(); let reads = 0, downloads = 0;
  const controllers = Object.fromEntries([
    "settingsHelpers", "preview", "buttonSettings", "configPersistence", "backupContract", "entityState",
    "shell", "requestApi", "statusPreview", "grid", "artworkPostApi", "schedulePostApi", "clockBarPostApi",
    "core", "codec", "screenScheduleState", "screensaverTimeout", "firmwareUpdate", "clockBar",
  ].map(name => [name, {}]));
  controllers.runtime = { els: {} };
  controllers.waitForInitialState = () => gate.promise;
  controllers.nativePanelConfig = { async readForBackup() { reads++; throw new Error("Stop after readiness"); } };
  controllers.backupFile = { download() { downloads++; } };
  const application = createAppBackupFeature(controllers);
  const exporting = application.exportConfig(); await tick();
  assert.equal(reads, 0); assert.equal(downloads, 0);
  gate.resolve(); await exporting; assert.equal(reads, 1); assert.equal(downloads, 0);
});

test("an obsolete failed read does not cancel backup readiness after reconnect", async t => {
  const first = deferred(), second = deferred(); let settingRead = 0;
  const loader = fixture(t, path => path.endsWith("setting") ? (++settingRead === 1 ? first.promise : second.promise) : Promise.resolve({ state: "saved" }));
  loader.loadInitialState(() => {}); await tick(); let ready = false;
  const completion = loader.waitForInitialState().then(() => { ready = true; });
  loader.loadInitialState(() => {}); await tick(); first.reject(new Error("old connection failed")); await tick();
  assert.equal(ready, false); second.resolve({ state: "new" }); await completion; assert.equal(ready, true);
});
