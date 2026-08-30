import { NativePanelConfigController } from "../controllers/native_panel_config_controller";
import type { NativePanelConfigFetch, NativePanelConfigRequest, NativePanelConfigResponse } from "../features/native_panel_config";
import type { NativePanelConfigControllerDependencies } from "../controllers/native_panel_config_controller";

export type NativePanelConfigMigrationDependencies = Omit<NativePanelConfigControllerDependencies, "fetch">;

/** Creates the typed configuration persistence controller for the browser app. */
export function createNativePanelConfigMigrationController(
  dependencies: NativePanelConfigMigrationDependencies,
): NativePanelConfigController {
  const fetchNative: NativePanelConfigFetch | null = typeof fetch === "function"
    ? (path: string, request?: NativePanelConfigRequest) =>
      fetch(path, request as RequestInit) as unknown as Promise<NativePanelConfigResponse>
    : null;
  const controller = new NativePanelConfigController({
    fetch: fetchNative,
    ...dependencies,
  });

  if (fetchNative) void controller.begin();
  return controller;
}
