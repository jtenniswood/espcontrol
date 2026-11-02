import { NativePanelConfigController } from "../../src/webserver/controllers/native_panel_config_controller";
import { createNativePageTransfersFeature } from "../../src/webserver/application/native_page_transfers";
import { initializeAppState, state } from "../../src/webserver/state/app_instance";
import { initializeDeviceConfig } from "../../src/webserver/device_config";
import { encodePanelPageDocument, decodePanelPageDocument } from "../../src/webserver/model/panel_config_conversion";
import type { PanelConfigDocument } from "../../src/webserver/model/panel_config";
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
  const feature = createNativePageTransfersFeature({ native,
    editor: () => ({ invalidate() {}, replace(document: PanelConfigDocument) { visible = document; } }) as NativePageEditorLoader,
    layout: { previewSlots: 9, gridCols: 3 } as ApplicationLayoutState,
    api: { postQueueIdle: async () => {}, postQueueHadError: () => false },
    grid: { flushMainGridSave() {}, ctx: () => ({ grid: [1, 0, 0, 0, 0, 0, 0, 0, 0] }) },
    shell: { isConfigLocked: () => state.configLocked || state.nativeConfigLoading, syncConfigLockUi() {}, showBanner: message => banners.push(message) },
    validate() {},
  });
  check(await feature.copy([1], true), "cut captures the whole saved tree");
  check(writes === 0 && clipboard()?.native?.cut, "cut cannot delete before its paste transaction");
  const before = JSON.stringify(persisted); rejected = true;
  check(!(await feature.paste(2)).ok, "rejected PUT fails the transfer");
  check(JSON.stringify(persisted) === before && JSON.stringify(visible) === before && published === 0,
    "rejection changes neither storage nor visible state and cannot announce a successful save");
  check(clipboard()?.native?.cut && !state.nativeConfigLoading, "rejection retains the cut clipboard and unlocks retry");
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
}
