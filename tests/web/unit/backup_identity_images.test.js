"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { createAppBackupFeature } = loadTypescriptTest("src/webserver/application/app_backup.ts");

const tick = async () => { for (let i = 0; i < 12; i++) await Promise.resolve(); };

for (const succeeds of [true, false]) {
  test(`panel rename waits for image restore ${succeeds ? "success" : "failure"}`, async () => {
    const events = [];
    let finishRestore;
    const completion = new Promise((resolve) => { finishRestore = resolve; });
    const noop = () => undefined;
    const emptyFeature = new Proxy({}, { get: () => noop });
    const controllers = new Proxy({
      layout: { deviceId: "test-panel", numSlots: 0 },
      runtime: { els: {} },
      identity: {
        chooseRestoreName: async () => "Kitchen",
        saveAndRestart: async (name) => events.push(`restart:${name}`),
      },
      backupImport: { plan: () => ({}) },
      backupFile: { import: (receive) => receive({}, []) },
      cardImages: { backupAssetProvider: { createRestore: () => ({
        stage: async () => events.push("stage"),
        commit: async () => { events.push("commit"); await completion; },
        rollback: noop,
        remapImportedReferences: noop,
      }) } },
      backupRestore: { restore: async (_data, _target, _apply, assets) => {
        await assets.stage();
        await assets.commit();
        events.push("finished");
        return succeeds;
      } },
    }, { get: (target, key) => target[key] ?? emptyFeature });

    createAppBackupFeature(controllers).importConfig();
    await tick();
    assert.deepEqual(events, ["stage", "commit"], "restart must not interrupt a pending image commit");
    finishRestore();
    await tick();
    assert.deepEqual(events, succeeds
      ? ["stage", "commit", "finished", "restart:Kitchen"]
      : ["stage", "commit", "finished"], "only a completed successful restore may restart the panel");
  });
}
