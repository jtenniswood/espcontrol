import type { CardConfig } from "../contracts/types";
import {
  createPanelConfigBackupPayload,
  encodePanelConfig,
  type PanelConfigBackupPayload,
} from "../model";

export interface BackupNativeConfigSource {
  readonly deviceProfile: string;
  readonly buttons: readonly CardConfig[];
  readonly subpages: Readonly<Record<string, unknown>>;
  readonly buttonOrder?: string;
  readonly buttonOnColor?: string;
}

export interface BackupExportControllerOptions {
  serializeButtonConfig(button: CardConfig): string;
  serializeSubpageConfig(subpage: unknown): string;
}

export interface BackupWithNativeConfig {
  readonly button_order?: string;
  readonly button_on_color?: string;
  native_config?: PanelConfigBackupPayload;
}

export interface BackupExportController {
  screenSizeSlug(value?: unknown): string;
  fileDate(value: Date): string;
  fileName(screenSize?: unknown, value?: Date): string;
  addNativeConfig<Backup extends BackupWithNativeConfig>(
    backup: Backup,
    source: BackupNativeConfigSource,
  ): Backup & { native_config: PanelConfigBackupPayload };
}

/** Owns the stable, portable pieces of the browser backup export journey. */
export function createBackupExportController(
  options: BackupExportControllerOptions,
): BackupExportController {
  const screenSizeSlug = (value?: unknown): string => {
    let normalized = String(value || "").trim().toLowerCase();
    if (!normalized) return "screen";
    normalized = normalized.replace(/\binches\b/g, "inch").replace(/\bin\b/g, "inch");
    normalized = normalized.replace(/[^a-z0-9.]+/g, "-").replace(/^-+|-+$/g, "");
    return normalized || "screen";
  };

  const fileDate = (value: Date): string =>
    `${value.getFullYear()}-${String(value.getMonth() + 1).padStart(2, "0")}-${String(value.getDate()).padStart(2, "0")}`;

  const fileName = (screenSize?: unknown, value: Date = new Date()): string =>
    `espcontrol-${screenSizeSlug(screenSize)}-${fileDate(value)}.json`;

  const addNativeConfig = <Backup extends BackupWithNativeConfig>(
    backup: Backup,
    source: BackupNativeConfigSource,
  ): Backup & { native_config: PanelConfigBackupPayload } => {
    const buttons: Record<number, string> = {};
    const subpages: Record<number, string> = {};
    for (let index = 0; index < source.buttons.length; index += 1) {
      const button = source.buttons[index];
      if (!button) continue;
      const buttonConfig = options.serializeButtonConfig(button);
      if (buttonConfig) buttons[index + 1] = buttonConfig;
    }
    for (const [slot, subpage] of Object.entries(source.subpages)) {
      const subpageConfig = options.serializeSubpageConfig(subpage);
      if (subpageConfig) subpages[Number(slot)] = subpageConfig;
    }
    backup.native_config = createPanelConfigBackupPayload(encodePanelConfig({
      deviceProfile: source.deviceProfile,
      buttons,
      subpages,
      settings: {
        button_order: source.buttonOrder || backup.button_order || "",
        button_on_color: source.buttonOnColor || backup.button_on_color || "",
      },
    }));
    return backup as Backup & { native_config: PanelConfigBackupPayload };
  };

  return { screenSizeSlug, fileDate, fileName, addNativeConfig };
}
