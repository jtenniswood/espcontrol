import { createBackupExportController } from "../../src/webserver/features/backup_export_controller";
import { decodePanelConfig, decodePanelConfigBackupPayload } from "../../src/webserver/model";

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
}
