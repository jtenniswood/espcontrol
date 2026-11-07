import { migrateSavedConfigSensorLegacy } from "../../src/webserver/generated/saved_config_sensor";
import { parseRawButtonConfig, parseRawSubpageConfig } from "../../src/webserver/model";
import { NativePanelConfigController } from "../../src/webserver/controllers/native_panel_config_controller";
import { createNativePageTransfersFeature, validatePageTransferCards } from "../../src/webserver/application/native_page_transfers";
import { initializeAppState, state } from "../../src/webserver/state/app_instance";
import { initializeDeviceConfig } from "../../src/webserver/device_config";
import { encodePanelPageDocument, decodePanelPageDocument } from "../../src/webserver/model/panel_config_conversion";
import type { PanelConfigDocument } from "../../src/webserver/model/panel_config";
import { capturePageCards, planPageCardPaste, planPageCardDelete } from "../../src/webserver/model/page_card_transfer";
import type { ApplicationLayoutState } from "../../src/webserver/application/application_context";
import type { NativePageEditorLoader } from "../../src/webserver/features/page_editor";

function check(value: unknown, message: string): void { if (!value) throw new Error(message); }
const snapshot = (): PanelConfigDocument => ({ deviceProfile: "test", buttons: { 1: ";Root;;;;;subpage" },
  subpages: { 1: "~B,1|G,,Child", 33: "~B=Root:ABCDEF,1|,light.leaf,Leaf" }, settings: { button_order: "1", opaque: "🌡️" } });

export async function runNativePageTransferTests(): Promise<void> {
  const globals = globalThis as any;
  globals.__ESPCONTROL_DEFAULT_DEVICE_ID__ = "test";
  globals.__ESPCONTROL_DEVICE_PROFILES__ = { test: { slots: 9 } };
  globals.__ESPCONTROL_TIMEZONE_OPTIONS__ = [];
  initializeDeviceConfig();
  initializeAppState();
  let persisted = snapshot(), visible = persisted, writes = 0, published = 0, rejected = false, conflict = false, lostAck = false;
  const banners: string[] = [];
  const native = new NativePanelConfigController({
    deviceProfile: () => "test", slotCount: () => 9, entityName: name => name,
    entityNameForSlot: (name, slot) => `${name}_${slot}`, normalizeHexColor: value => value,
    showBanner: message => banners.push(message), delay: callback => callback(),
    saved: () => { ++published; },
    fetch: async (path, request) => {
      let status = 200;
      if (request?.method === "PUT") {
        ++writes;
        if (rejected) { rejected = false; status = 422; }
        else if (conflict) {
          conflict = false; status = 409;
          persisted = { ...persisted, buttons: { ...persisted.buttons, 1: ";External edit;;;;;subpage" } };
        } else {
          persisted = decodePanelPageDocument(request.body!); status = 204;
          if (lostAck) { lostAck = false; throw new Error("Connection lost after commit"); }
        }
      }
      return { ok: status < 300, status, headers: { get: name => name === "ETag" ? '"v2-7"' : name === "X-Panel-Config-Version" ? "2" : null },
        json: async () => ({ configuration: { read: true, write: true, document_versions: [1, 2], max_record_body_bytes: 4096,
          max_navigation_depth: 2, writable_navigation_depth: 2, durable_document_version: 2 } }),
        arrayBuffer: async () => new Uint8Array(encodePanelPageDocument(persisted)).buffer as ArrayBuffer };
    },
  });
  await native.begin(); native.client!.acceptSnapshot(persisted);
  state.configLocked = false; state.nativeConfigLoading = false; state.settingsDraft = null; state.editingSubpage = null; state.clipboard = null;
  const clipboard = () => state.clipboard;
  const layout = { previewSlots: 9, gridCols: 5 } as ApplicationLayoutState;
  const feature = createNativePageTransfersFeature({ native,
    editor: () => ({ invalidate(preserve?: boolean) { check(preserve === true, "transfers retain interrupted external refreshes"); }, replace(document: PanelConfigDocument) { visible = document; } }) as NativePageEditorLoader,
    layout,
    api: { postQueueIdle: async () => { check(state.nativeTransferActive && !state.nativeConfigLoading, "transfers own an independent lock"); }, postQueueHadError: () => false },
    grid: { flushMainGridSave() {}, ctx: () => ({ grid: [1, 0, 0, 0, 0, 0, 0, 0, 0] }) },
    shell: { isConfigLocked: () => state.configLocked || state.nativeConfigLoading || state.nativeTransferActive, syncConfigLockUi() {}, showBanner: message => banners.push(message) },
    validate() {},
  });
  layout.gridCols = 3; // Orientation changes after application composition.
  check(await feature.copy([1], true), "cut captures the whole saved tree");
  check(writes === 0 && clipboard()?.native?.cut, "cut cannot delete before its paste transaction");
  const before = JSON.stringify(persisted); rejected = true;
  check(!(await feature.paste(2)).ok, "rejected PUT fails the transfer");
  check(JSON.stringify(persisted) === before && JSON.stringify(visible) === before && published === 0,
    "rejection changes neither storage nor visible state and cannot announce a successful save");
  check(clipboard()?.native?.cut && !state.nativeConfigLoading && !state.nativeTransferActive, "rejection retains the cut clipboard and unlocks retry");
  check(!!await feature.capture([1]), "copy can read a source after an explicit rejection");
  await native.readSnapshot();
  await native.readForBackup().catch(() => null);
  check(await native.saveQueue === "nested-pages-unavailable", "read-only operations preserve the rejected outcome");
  check((await feature.paste(2)).ok, "retry succeeds without silently resetting the previous failure outcome");
  check(writes === 2 && published === 1 && !state.clipboard, "retry has exactly one successful whole-document transaction");
  check(JSON.stringify(visible) === JSON.stringify(persisted), "editor applies the complete committed document only after success");
  persisted = snapshot(); visible = persisted; native.client!.acceptSnapshot(persisted);
  conflict = true;
  check(!await feature.delete([1]), "concurrent source change rejects deletion");
  check(writes === 3 && persisted.buttons[1]!.includes("External edit") && persisted.subpages[33]!.includes("light.leaf"),
    "stale retry cannot erase a newly changed folder or descendants");
  check(JSON.stringify(visible) === before, "conflict cannot publish the discarded candidate");
  check(banners.some(message => message.includes("changed")), "conflict is explained");
  const cached = native.client!.lastDocument();
  await native.readSnapshot();
  check(native.client!.lastDocument() === cached, "an unapplied background read cannot replace the editor's accepted source");
  persisted = snapshot(); visible = persisted; native.client!.acceptSnapshot(persisted);
  native.saveQueue = Promise.resolve("saved"); lostAck = true;
  check(!await feature.duplicate(1), "lost acknowledgement is an uncertain save");
  check(writes === 4 && persisted.subpages[65] && JSON.stringify(visible) === before,
    "server may have published even though the editor could not confirm the transfer");
  await native.readSnapshot();
  await native.readForBackup().catch(() => null);
  check(!await feature.duplicate(1) && writes === 4, "uncertain publication cannot be retried into an accidental duplicate");
  check(banners.some(message => message.includes("Could not confirm")), "uncertain publication requires a saved-state reload");
  persisted = snapshot();
  for (let slot = 2; slot <= 8; ++slot) persisted.buttons[slot] = `light.existing_${slot};Existing`;
  persisted.settings.button_order = "1,2,3,4,5,6,7,8";
  visible = persisted; native.client!.acceptSnapshot(persisted); native.saveQueue = Promise.resolve("saved");
  const copied = await feature.capture([1]);
  check(!!copied, "resize fixture captures its source");
  const pasted = await feature.paste(8, 0, [{ ...copied!.entries[0]!, size: 4 }]);
  check(pasted.ok && pasted.resized === 1 && writes === 5, "paste reports the successful size adjustment so its caller can explain it");
  // A wide unrelated card at a portrait row boundary exposes stale columns:
  // startup geometry would retain a two-cell span across the wrong row.
  persisted = snapshot(); persisted.buttons[2] = "light.wide;Wide";
  persisted.settings.button_order = "1,,,2w";
  visible = persisted; native.client!.acceptSnapshot(persisted); native.saveQueue = Promise.resolve("saved");
  layout.gridCols = 2;
  const geometry = { slots: 9, columns: 2 };
  const expectedCapture = capturePageCards(persisted, 0, [2], geometry);
  const rotatedCapture = await feature.capture([2]);
  check(JSON.stringify(rotatedCapture) === JSON.stringify(expectedCapture), "capture reads the rotated geometry");
  const expectedPaste = planPageCardPaste(persisted, 0, 7, expectedCapture.entries, geometry).document;
  check((await feature.paste(7, 0, expectedCapture.entries)).ok, "paste after rotation succeeds");
  check(JSON.stringify(persisted) === JSON.stringify(expectedPaste), "paste preserves unrelated cards using current columns");
  const expectedDelete = planPageCardDelete(persisted, capturePageCards(persisted, 0, [2], geometry).origin, geometry);
  check(await feature.delete([2]), "delete after rotation succeeds");
  check(JSON.stringify(persisted) === JSON.stringify(expectedDelete), "delete uses the current layout");

  layout.gridCols = 3;
  persisted = snapshot();
  for (let slot = 2; slot <= 9; ++slot) persisted.buttons[slot] = `light.existing_${slot};Existing`;
  persisted.settings.button_order = "1,2,3,4,5,6,7,8,9";
  visible = persisted; native.client!.acceptSnapshot(persisted); native.saveQueue = Promise.resolve("saved");
  const fullGrid = JSON.stringify(persisted), beforeLocalFailure = writes;
  check(!await feature.duplicate(1), "full-grid duplication is rejected locally");
  check(writes === beforeLocalFailure && JSON.stringify(persisted) === fullGrid, "local planning failure sends no PUT");
  check(banners.at(-1)?.includes("room"), "local planning failure retains its useful explanation");
  await native.readSnapshot();
  check(await feature.delete([2]), "a locally rejected duplicate must allow deleting a card to make room");
  check(await feature.duplicate(1), "duplicate succeeds after making room");
  check(writes === beforeLocalFailure + 2, "only the two valid edits are written");

  persisted = snapshot(); native.client!.acceptSnapshot(persisted); native.saveQueue = Promise.resolve("saved");
  rejected = true;
  check(!await feature.duplicate(1), "transfer retry fixture is explicitly rejected");
  rejected = true;
  await native.schedule(current => current);
  const writesBeforeBlockedRetry = writes;
  check(!await feature.duplicate(1) && writes === writesBeforeBlockedRetry,
    "a later unrelated failed save cannot inherit the transfer's retry permission");
}

export function runNativeTransferValidationTests(): void {
  const codec = {
    normalizeButtonConfig(button: any) { migrateSavedConfigSensorLegacy(button); return button; },
    parseSubpageConfig(raw: string) { return parseRawSubpageConfig(raw, value => value); },
  } as any;
  for (const alias of ["text_sensor", "local_sensor"]) {
    const button = { ...parseRawButtonConfig(""), type: alias, label: "Saved label", options: "future=value" };
    const entries = [{ button, size: 1, subtree: null }];
    const before = JSON.stringify(entries), seen: any[] = [];
    validatePageTransferCards(entries, 1, codec, (normalized, subpage) => {
      check(normalized.type === "sensor" && subpage, "legacy sensor is checked using its supported type"); seen.push(normalized);
    });
    check(seen.length === 1 && JSON.stringify(entries) === before && button.type === alias,
      "validation cannot rewrite the original card or saved values");
  }
  const folder = [{ button: { ...parseRawButtonConfig(""), type: "subpage" }, size: 1,
    subtree: { configuration: "~B,1|text_sensor,,Legacy", children: { 1: "~B,1|local_sensor,,Local" } } }];
  const before = JSON.stringify(folder), types: string[] = [];
  validatePageTransferCards(folder, 0, codec, (button, subpage) => {
    check(button.type === (subpage ? "sensor" : "subpage"), "descendant aliases use normalized validation too"); types.push(button.type);
  });
  check(types.join() === "subpage,sensor,sensor" && JSON.stringify(folder) === before,
    "folder validation preserves both levels of original serialized data");
}
