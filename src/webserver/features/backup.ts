import type { CardConfig } from "../contracts/types";
import {
  BACKUP_CONFIG_VERSION,
  BACKUP_FORMAT,
  cloneCardConfig,
  createBackupEnvelope,
  emptyCardConfig,
  normalizeBackupEnvelope,
  planBackupButtonLayout,
  parseStructuredSubpageConfig,
  serializeGridOrder,
  structuredSubpageFromParsed,
  validateBackupEnvelope,
  type NormalizedBackupEnvelope,
  type ParsedSubpageConfig,
  type SlotSizeMap,
} from "../model";

export interface FeatureSubpage extends ParsedSubpageConfig {
  grid?: number[];
  sizes?: SlotSizeMap;
}

export interface BackupFeatureSnapshot {
  readonly device?: string;
  readonly slots?: unknown;
  readonly exported_at?: string;
  readonly button_order?: unknown;
  readonly button_on_color?: string;
  readonly buttons?: readonly Partial<CardConfig>[];
  readonly subpages?: Record<string, FeatureSubpage | null | undefined>;
  readonly grid?: readonly number[];
  readonly sizes?: SlotSizeMap;
  readonly settings?: Record<string, unknown>;
  readonly screen?: Record<string, unknown>;
}

export interface BackupTargetDevice {
  readonly device?: string;
  readonly slots?: unknown;
}

export interface BackupImportPlan {
  readonly config: NormalizedBackupEnvelope;
  readonly warnings: string[];
  readonly importedCount: number;
  readonly buttons: CardConfig[];
  readonly button_order: string;
  readonly importedSizes: SlotSizeMap;
  readonly subpages: Record<string, FeatureSubpage>;
  readonly settings: Record<string, unknown> | null;
  readonly screen: Record<string, unknown> | null;
}

export interface BackupFeatureDependencies {
  readonly deviceId: string;
  readonly gridCols: number;
  readonly numSlots: number;
  normalizeButtonConfig(button: CardConfig): CardConfig;
  parseSubpageConfig(value: string): FeatureSubpage;
  serializeSubpageConfig(subpage: FeatureSubpage): string;
  buildSubpageGrid(subpage: FeatureSubpage): number[];
}

export interface BackupFeature {
  readonly BACKUP_CONFIG_VERSION: number;
  readonly BACKUP_FORMAT: string;
  emptyButtonConfig(): CardConfig;
  normalizeButtonConfig(button?: Partial<CardConfig> | null): CardConfig;
  createBackupConfig(snapshot?: BackupFeatureSnapshot): NormalizedBackupEnvelope;
  normalizeBackupConfig(data: unknown): NormalizedBackupEnvelope;
  planBackupImport(data: unknown, targetDevice?: BackupTargetDevice): BackupImportPlan;
}

export function createBackupFeature(dependencies: BackupFeatureDependencies): BackupFeature {
  const normalizeButton = (button?: Partial<CardConfig> | null): CardConfig =>
    dependencies.normalizeButtonConfig(cloneCardConfig(button || {}));

  const serializeSubpages = (
    subpages: Record<string, FeatureSubpage | null | undefined> = {},
  ): Record<string, string> => {
    const output: Record<string, string> = {};
    for (const [key, subpage] of Object.entries(subpages)) {
      if (!subpage) continue;
      const hasButtons = !!subpage.buttons?.length;
      const hasOrder = !!subpage.order?.length || !!subpage.grid?.length;
      if (hasButtons || hasOrder) output[key] = dependencies.serializeSubpageConfig(subpage);
    }
    return output;
  };

  const serializeSubpageObjects = (
    subpages: Record<string, FeatureSubpage | null | undefined> = {},
  ): Record<string, ReturnType<typeof structuredSubpageFromParsed>> => {
    const output: Record<string, ReturnType<typeof structuredSubpageFromParsed>> = {};
    for (const [key, subpage] of Object.entries(subpages)) {
      if (!subpage) continue;
      const hasButtons = !!subpage.buttons?.length;
      const hasOrder = !!subpage.order?.length || !!subpage.grid?.length;
      if (!hasButtons && !hasOrder) continue;
      const parsed = dependencies.parseSubpageConfig(dependencies.serializeSubpageConfig(subpage));
      output[key] = structuredSubpageFromParsed(parsed);
    }
    return output;
  };

  const createBackupConfig = (snapshot: BackupFeatureSnapshot = {}): NormalizedBackupEnvelope => {
    const buttons = (snapshot.buttons || []).map(normalizeButton);
    return createBackupEnvelope(snapshot, {
      buttons,
      subpages: serializeSubpages(snapshot.subpages),
      subpage_objects: serializeSubpageObjects(snapshot.subpages),
      button_order: snapshot.button_order != null
        ? String(snapshot.button_order)
        : serializeGridOrder(snapshot.grid || [], snapshot.sizes || {}),
    });
  };

  const normalizeBackupConfig = (data: unknown): NormalizedBackupEnvelope => {
    const input = validateBackupEnvelope(data);
    const rawButtons = input.buttons as Partial<CardConfig>[];
    const buttons = rawButtons.map(normalizeButton);
    const subpages: Record<string, string> = {};
    const subpageObjects: Record<string, ReturnType<typeof structuredSubpageFromParsed>> = {};

    if (input.subpage_objects && typeof input.subpage_objects === "object") {
      for (const [key, value] of Object.entries(input.subpage_objects)) {
        const parsed = parseStructuredSubpageConfig(value);
        const serialized = dependencies.serializeSubpageConfig(parsed);
        subpages[key] = serialized;
        subpageObjects[key] = structuredSubpageFromParsed(dependencies.parseSubpageConfig(serialized));
      }
    }
    if (input.subpages && typeof input.subpages === "object") {
      for (const [key, value] of Object.entries(input.subpages)) {
        if (Object.prototype.hasOwnProperty.call(subpages, key)) continue;
        const parsed = dependencies.parseSubpageConfig(String(value || ""));
        const serialized = dependencies.serializeSubpageConfig(parsed);
        subpages[key] = serialized;
        subpageObjects[key] = structuredSubpageFromParsed(dependencies.parseSubpageConfig(serialized));
      }
    }

    return normalizeBackupEnvelope(input, {
      buttons,
      subpages,
      subpage_objects: subpageObjects,
    });
  };

  const planBackupImport = (
    data: unknown,
    targetDevice: BackupTargetDevice = {},
  ): BackupImportPlan => {
    const config = normalizeBackupConfig(data);
    const targetSlots = parseInt(String(targetDevice.slots), 10) || dependencies.numSlots;
    const targetDeviceId = targetDevice.device || dependencies.deviceId;
    const importedCount = config.buttons.length;
    const warnings: string[] = [];

    if (config.device && config.device !== targetDeviceId) {
      warnings.push(`Config was exported from a different panel (${config.device}) - layout may look different`);
    }
    if (importedCount !== targetSlots) {
      warnings.push(`Backup has ${importedCount} slots, current config has ${targetSlots} - adapting`);
    }

    const layoutPlan = planBackupButtonLayout(
      config.buttons,
      config.button_order,
      targetSlots,
      dependencies.gridCols,
    );
    const subpages: Record<string, FeatureSubpage> = {};
    for (const [sourceKey, value] of Object.entries(config.subpages)) {
      const mappedKey = layoutPlan.slotMap[sourceKey];
      if (!mappedKey) continue;
      const subpage = dependencies.parseSubpageConfig(value);
      subpage.sizes = {};
      subpage.grid = dependencies.buildSubpageGrid(subpage);
      subpages[String(mappedKey)] = subpage;
    }

    return {
      config,
      warnings,
      importedCount,
      buttons: layoutPlan.buttons.map(normalizeButton),
      button_order: layoutPlan.button_order,
      importedSizes: layoutPlan.importedSizes,
      subpages,
      settings: config.settings,
      screen: config.screen,
    };
  };

  return {
    BACKUP_CONFIG_VERSION,
    BACKUP_FORMAT,
    emptyButtonConfig: emptyCardConfig,
    normalizeButtonConfig: normalizeButton,
    createBackupConfig,
    normalizeBackupConfig,
    planBackupImport,
  };
}
