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
      backupImport: { plan: () => ({ backupPlan: { screen: {} } }) },
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

for (const result of ['saved', 'mirror-failed', 'failed', 'discovery-failed']) {
  test(`image durability boundary follows the actual layout write: ${result}`, async () => {
    const events = [];
    const noop = () => undefined;
    const emptyFeature = new Proxy({}, { get: () => noop });
    const document = { deviceProfile: 'test-panel', buttons: {}, subpages: {}, settings: { button_order: '', button_on_color: '' } };
    const plan = { importedSettings: {}, importedGridCols: 3, backupPlan: {
      screen: { device: 'test-panel' }, buttons: [], subpages: {}, config: { native_config: document },
    } };
    let complete;
    const completion = new Promise(resolve => { complete = resolve; });
    const controllers = new Proxy({
      layout: { deviceId: 'test-panel', numSlots: 0 },
      runtime: { els: {} },
      requestApi: { postQueue: Promise.resolve(), postQueueError: false },
      identity: { chooseRestoreName: async () => undefined },
      backupImport: { plan: () => plan },
      backupFile: { import: receive => receive({}, null) },
      nativePanelConfig: {
        client: {}, waitForDiscovery: async () => result === 'discovery-failed' ? 'failed' : 'native',
        writeDocument: async () => { events.push('layout-write'); return result; },
      },
      backupRestore: { restore: async (_data, _target, apply) => {
        try {
          await apply(plan, {
            layoutWriteStarted: () => events.push('write-started'),
            layoutPersisted: async () => { events.push('assets-commit'); throw new Error('stop at durability boundary'); },
          });
        } catch (error) { events.push(error.message); }
        complete();
        return false;
      } },
    }, { get: (target, key) => target[key] ?? emptyFeature });
    createAppBackupFeature(controllers).importConfig();
    await completion;
    if (result === 'saved' || result === 'mirror-failed') {
      assert.deepEqual(events, ['write-started', 'layout-write', 'assets-commit', 'stop at durability boundary']);
    } else {
      assert(!events.includes('assets-commit'), 'unconfirmed layout cannot commit staged assets');
      assert.equal(events.includes('write-started'), result !== 'discovery-failed');
    }
  });
}
