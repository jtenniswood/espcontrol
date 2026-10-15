import { createConfigImageOptionsFeature } from "../../src/webserver/application/config_image_options";
import { createBadgeRowFeature } from "../../src/webserver/application/badge_row";
import { initializeDeviceConfig } from "../../src/webserver/device_config";
import { initializeAppState, state } from "../../src/webserver/state/app_instance";

export async function runBadgeLoadTests() {
  Object.assign(globalThis, { __ESPCONTROL_DEFAULT_DEVICE_ID__: "test", __ESPCONTROL_DEVICE_PROFILES__: { test: { slots: 1 } }, __ESPCONTROL_TIMEZONE_OPTIONS__: [] });
  await initializeDeviceConfig();
  initializeAppState();
  let resolveRead: (value: any) => void = () => {};
  let fail = false;
  const native = {
    waitForDiscovery: async () => true,
    client: { badgeRowsSupported: () => true, read: () => fail ? Promise.reject(new Error("offline")) : new Promise(resolve => { resolveRead = resolve; }) },
  };
  const feature = createBadgeRowFeature({ native, runtime: { els: {} }, shell: { showBanner() {} }, renderPreview() {} } as any);
  state.badgeRowsReady = true;
  const pending = feature.load();
  if (state.badgeRowsReady) throw new Error("A fresh read must invalidate cached readiness immediately");
  await Promise.resolve();
  resolveRead({ settings: {} });
  await pending;
  if (!state.badgeRowsReady) throw new Error("A successful read must mark rows ready");
  const imageOptions = createConfigImageOptionsFeature({ layout: { config: { imageSlotCapacity: 2 } }, mediaOptions: { mediaEditorMode: (mode: string) => mode }, showBanner() {} } as any);
  const image = { entity: "camera.kitchen", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "badge", precision: "", options: "badge_action=card,badge_card_type=image" };
  const count = imageOptions.imageCardCountWithCandidate({ badgePage: 0, badgeIndex: 0, button: { ...image, type: "image" }, badgeRow: [image, image, image] });
  if (count !== 3) throw new Error("A new row must count all image-backed badges against the shared capacity");
  fail = true;
  await feature.load();
  if (state.badgeRowsReady) throw new Error("A failed refresh must not expose cached rows as current");
}
