import { createAppBackupFeature } from "../../src/webserver/application/app_backup";
import { initializeAppState } from "../../src/webserver/state/app_instance";
import { initializeDeviceConfig } from "../../src/webserver/device_config";

export async function runAppBackupExportMessageTest(outcome: "success" | "cancel" | "download-error") {
  const globals = globalThis as any;
  const keys = ["__ESPCONTROL_DEFAULT_DEVICE_ID__", "__ESPCONTROL_DEVICE_PROFILES__", "__ESPCONTROL_TIMEZONE_OPTIONS__"];
  const previousGlobals = keys.map(key => globals[key]);
  globals.__ESPCONTROL_DEFAULT_DEVICE_ID__ = "test";
  globals.__ESPCONTROL_DEVICE_PROFILES__ = { test: { slots: 9 } };
  globals.__ESPCONTROL_TIMEZONE_OPTIONS__ = [];
  initializeDeviceConfig();
  initializeAppState();
  const banners: Array<{ message: string; kind: string }> = [];
  let downloads = 0;
  const noop = () => undefined;
  const emptyFeature = new Proxy({}, { get: () => noop });
  const controllers: any = new Proxy({
    layout: { config: { screenSize: "4inch" }, deviceId: "test", numSlots: 9 },
    runtime: { els: {} },
    identity: { load: async () => { throw new Error("Identity unavailable"); } },
    backupContract: { createBackupConfig: (value: unknown) => value },
    backupExport: { addNativeConfig: (value: unknown) => value, fileName: () => "backup.json" },
    shell: { showBanner: (message: string, kind: string) => banners.push({ message, kind }) },
    backupFile: { downloadArchive: () => {
      if (outcome === "download-error") throw new Error("Download failed");
      downloads++;
    } },
    cardImages: { backupAssetProvider: { createArchiveEntries: async () => {
      if (outcome === "cancel") throw new Error("Could not read an image");
      return [{ name: "images.json", bytes: new Uint8Array() }];
    } } },
  }, { get: (target: any, key) => target[key] ?? emptyFeature });
  const previousWindow = globalThis.window;
  globalThis.window = { confirm: () => false } as unknown as Window & typeof globalThis;
  try { await createAppBackupFeature(controllers).exportConfig(); }
  finally {
    globalThis.window = previousWindow;
    keys.forEach((key, index) => {
      if (previousGlobals[index] === undefined) delete globals[key];
      else globals[key] = previousGlobals[index];
    });
  }
  return { banners, downloads };
}
