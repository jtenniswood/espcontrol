import { createBackupExportController } from "../../src/webserver/features/backup_export_controller";
import { decodePanelConfig, decodePanelConfigBackupPayload, encodePanelConfig } from "../../src/webserver/model";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runBackupExportControllerTests(): void {
  const controller = createBackupExportController({
    serializeButtonConfig: (button) => button.entity || "",
    serializeSubpageConfig: (subpage) => String(subpage || ""),
  });
  equal(controller.screenSizeSlug("10.1 inches"), "10.1-inch", "screen size keeps decimal and normalizes units");
  equal(controller.screenSizeSlug(""), "screen", "blank screen sizes keep the portable fallback");
  equal(
    controller.fileName("7 in", new Date(2026, 6, 13)),
    "espcontrol-7-inch-2026-07-13.json",
    "backup file names remain stable",
  );

  const backup = controller.addNativeConfig({
    button_order: "1,2d",
    button_on_color: "0073FF",
  }, {
    deviceProfile: "panel-a",
    buttons: [
      { entity: "light.kitchen", label: "Kitchen", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" },
      { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" },
    ],
    subpages: { "2": "subpage-config" },
  });
  const nativeConfig = decodePanelConfig(decodePanelConfigBackupPayload(backup.native_config));
  equal(nativeConfig.deviceProfile, "panel-a", "native backup records the panel profile");
  equal(nativeConfig.buttons[1], "light.kitchen", "native backup retains populated buttons");
  equal(nativeConfig.buttons[2], undefined, "native backup omits blank buttons");
  equal(nativeConfig.subpages[2], "subpage-config", "native backup retains subpages");
  equal(nativeConfig.settings.button_order, "1,2d", "native backup retains button order");
  equal(nativeConfig.settings.button_on_color, "0073FF", "native backup retains active colour");

  const wifiCard = "wifi_qr,,Connect,Wifi,Auto,,,,,ssid64=R3Vlc3QgV2lmaQ,pass64=dHJ1c3RlZC1uZXR3b3Jr";
  const wifiBackup = createBackupExportController({
    serializeButtonConfig: () => wifiCard,
    serializeSubpageConfig: (subpage) => String(subpage || ""),
  }).addNativeConfig({}, {
    deviceProfile: "panel-a",
    buttons: [
      { entity: "", label: "Connect", icon: "Wifi", icon_on: "Auto", sensor: "", unit: "", type: "wifi_qr", precision: "", options: "ssid64=R3Vlc3QgV2lmaQ,pass64=dHJ1c3RlZC1uZXR3b3Jr" },
    ],
    subpages: {},
  });
  const wifiNativeConfig = decodePanelConfig(decodePanelConfigBackupPayload(wifiBackup.native_config));
  equal(wifiNativeConfig.buttons[1], wifiCard,
    "native backup intentionally retains Wifi network names and passwords");

  const savedBytes = encodePanelConfig({
    deviceProfile: "panel-a",
    buttons: { 1: "saved-card", 9: "inactive-card" },
    subpages: { 9: "inactive-subpage" },
    settings: { button_order: "1", future_setting: "温度🌡️".repeat(100) },
  });
  const completeBackup = controller.addNativeConfig({}, {
    deviceProfile: "panel-a", buttons: [], subpages: {}, nativeDocumentBytes: savedBytes,
  });
  equal(JSON.stringify(Array.from(decodePanelConfigBackupPayload(completeBackup.native_config))),
    JSON.stringify(Array.from(savedBytes)),
    "the existing backup payload format preserves the complete native document byte for byte");
  for (const invalidBytes of [new Uint8Array([1, 2, 3]), encodePanelConfig({
    deviceProfile: "different-panel", buttons: {}, subpages: {}, settings: {},
  })]) {
    let rejected = false;
    try {
      controller.addNativeConfig({}, {
        deviceProfile: "panel-a", buttons: [], subpages: {}, nativeDocumentBytes: invalidBytes,
      });
    } catch { rejected = true; }
    equal(rejected, true, "invalid or mismatched native snapshots cannot be exported");
  }
}
