import { NativePanelConfigController } from "../../src/webserver/controllers/native_panel_config_controller";
import { createConfigPersistenceFeature } from "../../src/webserver/application/config_post_api";
import { createNativePanelConfigClient, type NativePanelConfigResponse } from "../../src/webserver/features/native_panel_config";
import { decodePanelConfig, encodePanelConfig, parseLegacySubpageConfig, serializeLegacySubpageConfig } from "../../src/webserver/model";
import { initializeDeviceConfig } from "../../src/webserver/device_config";
import { initializeAppState, state } from "../../src/webserver/state/app_instance";

function equal(actual: unknown, expected: unknown, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${expected}, received ${actual}`);
}

export async function runConfigCapacityTests(): Promise<void> {
  const globals = globalThis as any;
  globals.__ESPCONTROL_DEFAULT_DEVICE_ID__ = "test";
  globals.__ESPCONTROL_DEVICE_PROFILES__ = { test: { slots: 12 } };
  globals.__ESPCONTROL_TIMEZONE_OPTIONS__ = [];
  initializeDeviceConfig();
  initializeAppState();
  const document = { deviceProfile: "panel-a", buttons: {}, subpages: {}, settings: {} };
  const largeSubpage = serializeLegacySubpageConfig(
    ["B", ...Array.from({ length: 12 }, (_, index) => String(index + 1))],
    Array.from({ length: 12 }, (_, index) => [
      "light.kitchen_" + "zone_".repeat(30) + index,
      "Kitchen " + "area ".repeat(8), "Auto", "Auto", "", "", "", "", "card_color=AA55FF",
    ]),
  );
  equal(parseLegacySubpageConfig(largeSubpage).buttons.length, 12, "the capacity fixture is a full coloured subpage");
  const banners: string[] = [];
  let postedValue = "";
  let legacyPosts = 0;
  let legacy = false;
  let putCount = 0;
  const response = (status: number): NativePanelConfigResponse => ({
    ok: status >= 200 && status < 300, status,
    headers: { get: () => '"1"' },
    json: async () => ({ configuration: { read: true, write: true, document_versions: [1],
      ...(legacy ? {} : { max_record_body_bytes: 8192 }) } }),
    arrayBuffer: async () => encodePanelConfig(document).buffer as ArrayBuffer,
  });
  const controller = new NativePanelConfigController({
    fetch: async (_path, request) => {
      if (request?.method === "PUT") {
        putCount += 1;
        postedValue = decodePanelConfig(request.body!).subpages[1] || "";
        return response(204);
      }
      return response(200);
    },
    deviceProfile: () => "panel-a", slotCount: () => 12,
    entityName: (name) => name, entityNameForSlot: (name, slot) => name + slot,
    normalizeHexColor: (value) => value,
    showBanner: (message) => banners.push(message), delay: (callback) => callback(),
  });
  const api = {
    postQueue: Promise.resolve(), postQueueError: false,
    postText: () => { legacyPosts += 1; return Promise.resolve(); },
    postTextLegacy: () => { legacyPosts += 1; return Promise.resolve(); },
  };
  const persistence = createConfigPersistenceFeature(controller, {} as any,
    { config: { features: { subpageConfigChunks: 4 } } } as any,
    { entityName: (name) => name, entityNameForSlot: (name, slot) => name + slot,
      hasRememberedPostPath: () => false },
    { showBanner: (message) => banners.push(message) });
  persistence.connectRequestApi(api as any);
  persistence.connectCodec({ serializeButtonConfig: () => "light.kitchen",
    serializeSubpageConfig: () => largeSubpage });
  equal(persistence.buttonConfigByteLimit(), 255, "undiscovered firmware keeps the safe card limit");
  equal(persistence.subpageConfigFits(largeSubpage), false, "undiscovered firmware keeps the configured chunk limit");
  await controller.begin();
  equal(persistence.buttonConfigByteLimit(), 8191, "current firmware advertises larger card capacity");
  equal(persistence.subpageConfigFits(largeSubpage), true, "coloured subpages can exceed the old chunks");
  equal(persistence.subpageConfigFits("é".repeat(4096)), false, "native capacity counts UTF-8 bytes");
  state.subpages[1] = {} as any;
  equal(await persistence.saveSubpageEntity(1), "saved", "large subpages use native storage");
  equal(postedValue, largeSubpage, "native saves retain every byte");
  equal(legacyPosts, 0, "native overflow never spills into legacy text calls");
  equal(banners.length, 0, "supported native saves produce no errors");

  legacy = true;
  controller.client = createNativePanelConfigClient(async () => response(200));
  await controller.begin();
  equal(persistence.buttonConfigByteLimit(), 255, "older native firmware retains the old main card limit");
  equal(persistence.subpageConfigFits("x".repeat(1021)), false, "older four-chunk panels retain their actual capacity");
  equal(await persistence.saveSubpageEntity(1), "failed", "oversized legacy saves report failure to the editor");
  equal(api.postQueueError, true, "oversized saves cannot be reported as successful");
  equal(await controller.writeSubpage(1, largeSubpage), "too-large", "backup and direct writes respect older native limits");
  equal(putCount, 1, "unsupported large documents never reach the older firmware");
}
