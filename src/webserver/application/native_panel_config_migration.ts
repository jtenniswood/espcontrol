import { NativePanelConfigController, type NativePanelConfigUpdate } from "../controllers/native_panel_config_controller";
import type { NativePanelConfigFetch, NativePanelConfigRequest, NativePanelConfigResponse } from "../features/native_panel_config";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";

/**
 * Compatibility bridge for editor modules that still call native configuration
 * globals. Native persistence itself is owned by the typed controller.
 */
export function installNativePanelConfigMigrationModule(): GlobalDescriptors {
  const fetchNative: NativePanelConfigFetch | null = typeof fetch === "function"
    ? (path: string, request?: NativePanelConfigRequest) =>
      fetch(path, request as RequestInit) as unknown as Promise<NativePanelConfigResponse>
    : null;
  const controller = new NativePanelConfigController({
    fetch: fetchNative,
    deviceProfile: () => DEVICE_ID,
    slotCount: () => NUM_SLOTS,
    entityName: (name) => entityName(name),
    entityNameForSlot: (name, slot) => entityNameForSlot(name, slot),
    normalizeHexColor: (value, fallback) => normalizeHexColor(value, fallback),
    showBanner: (message, level) => showBanner(message, level),
    delay: (callback, milliseconds) => setTimeout(callback, milliseconds),
  });

  if (fetchNative) void controller.begin();
  return {
    "_nativePanelConfigClient": liveGlobal(() => controller.client, (value) => { controller.client = value as typeof controller.client; }),
    "_nativePanelConfigSaveQueue": liveGlobal(() => controller.saveQueue, (value) => { controller.saveQueue = value as typeof controller.saveQueue; }),
    "NATIVE_PANEL_CONFIG_RETRY_DELAY_MS": liveGlobal(() => controller.retryDelayMs, (value) => { controller.retryDelayMs = value as number; }),
    "beginNativePanelConfigMigration": staticGlobal(() => controller.begin()),
    "nativePanelConfigMigrationSupported": staticGlobal(() => controller.supported()),
    "nativePanelConfigSubpageWrite": staticGlobal((slot?: unknown, value?: unknown) =>
      controller.writeSubpage(Number.parseInt(String(slot), 10), String(value || ""))),
    "nativePanelConfigTextWrite": staticGlobal((name?: unknown, value?: unknown) =>
      controller.writeText(String(name || ""), String(value || ""))),
    "scheduleNativePanelConfigSave": staticGlobal((update: NativePanelConfigUpdate) => controller.schedule(update)),
    "waitForNativePanelConfigDiscovery": staticGlobal(() => controller.waitForDiscovery()),
  };
}
