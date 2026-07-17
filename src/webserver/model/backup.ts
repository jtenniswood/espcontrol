import type { CardConfig } from "../contracts/types";
import { cloneCardConfig, emptyCardConfig } from "./card";
import {
  markSpannedCells,
  serializeGridOrder,
  sizeFitsAt,
  sizeFromToken,
  type SlotSizeMap,
} from "./grid";
import type { StructuredSubpageConfig } from "./subpage";

export const BACKUP_CONFIG_VERSION = 2;
export const BACKUP_FORMAT = "espcontrol.backup";

export interface BackupSource {
  device: string;
  slots: number;
}

export interface BackupEnvelopeOutputs {
  buttons: CardConfig[];
  subpages: Record<string, string>;
  subpage_objects?: Record<string, StructuredSubpageConfig>;
  button_order?: string;
}

export interface NormalizedBackupEnvelope {
  version: number;
  format: string;
  device: string;
  source: BackupSource;
  exported_at: string;
  button_order: string;
  button_on_color: string;
  buttons: CardConfig[];
  subpages: Record<string, string>;
  subpage_objects: Record<string, StructuredSubpageConfig>;
  settings: Record<string, unknown> | null;
  screen: Record<string, unknown> | null;
}

export interface BackupSnapshotEnvelope {
  device?: string;
  slots?: unknown;
  exported_at?: string;
  button_order?: unknown;
  button_on_color?: string;
  settings?: Record<string, unknown>;
  screen?: Record<string, unknown>;
}

export interface BackupUsedSlot {
  oldSlot: number;
  size: number;
}

export interface BackupOrderSlots {
  usedSlots: BackupUsedSlot[];
  seen: Record<string, boolean>;
}

export interface BackupButtonLayoutPlan {
  importedCount: number;
  buttons: CardConfig[];
  button_order: string;
  importedSizes: SlotSizeMap;
  slotMap: Record<string, number>;
}

function backupConfigError(message: string): Error & { backupMessage: string } {
  const err = new Error(message) as Error & { backupMessage: string };
  err.backupMessage = message;
  return err;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return !!value && typeof value === "object" && !Array.isArray(value);
}

export function validateBackupEnvelope(data: unknown): Record<string, unknown> {
  if (!isRecord(data)) {
    throw backupConfigError("Invalid config file - backup must be a JSON object");
  }

  const version = parseInt(String(data.version), 10);
  if (!version || version < 1) {
    throw backupConfigError("Invalid config file - missing required fields");
  }
  if (version > BACKUP_CONFIG_VERSION) {
    throw backupConfigError("Backup was created by a newer version of EspControl");
  }
  if (version >= 2 && data.format !== BACKUP_FORMAT) {
    throw backupConfigError("Invalid config file - unsupported backup format");
  }
  if (!Array.isArray(data.buttons)) {
    throw backupConfigError("Invalid config file - missing required fields");
  }

  return data;
}

export function backupSource(data: Record<string, unknown>, slots: number): BackupSource {
  const source = isRecord(data.source) ? data.source : {};
  return {
    device: String(source.device || data.device || ""),
    slots: parseInt(String(source.slots), 10) || slots || 0,
  };
}

export function createBackupEnvelope(
  snapshot: BackupSnapshotEnvelope,
  outputs: BackupEnvelopeOutputs,
): NormalizedBackupEnvelope {
  const slots = parseInt(String(snapshot.slots), 10) || outputs.buttons.length;
  const device = snapshot.device || "";
  const settings = { ...(snapshot.settings || {}) };
  delete settings.screensaver_pin;
  return {
    version: BACKUP_CONFIG_VERSION,
    format: BACKUP_FORMAT,
    device,
    source: {
      device,
      slots,
    },
    exported_at: snapshot.exported_at || new Date().toISOString(),
    button_order: outputs.button_order != null ? String(outputs.button_order) : "",
    button_on_color: snapshot.button_on_color || "0073FF",
    buttons: outputs.buttons,
    subpages: outputs.subpages,
    subpage_objects: outputs.subpage_objects || {},
    settings,
    screen: snapshot.screen || {},
  };
}

export function normalizeBackupEnvelope(
  data: Record<string, unknown>,
  outputs: BackupEnvelopeOutputs,
): NormalizedBackupEnvelope {
  return {
    version: BACKUP_CONFIG_VERSION,
    format: BACKUP_FORMAT,
    device: String(data.device || ""),
    source: backupSource(data, outputs.buttons.length),
    exported_at: String(data.exported_at || ""),
    button_order: String(data.button_order || ""),
    button_on_color: String(data.button_on_color || "0073FF"),
    buttons: outputs.buttons,
    subpages: outputs.subpages,
    subpage_objects: outputs.subpage_objects || {},
    settings: isRecord(data.settings) ? data.settings : null,
    screen: isRecord(data.screen)
      ? data.screen
      : (isRecord(data.settings) && isRecord(data.settings.screen) ? data.settings.screen : null),
  };
}

export function backupOrderUsedSlots(
  order: string | null | undefined,
  importedCount: number,
): BackupOrderSlots {
  const parts = String(order || "").split(",");
  const usedSlots: BackupUsedSlot[] = [];
  const seen: Record<string, boolean> = {};
  for (const part of parts) {
    const token = part.trim();
    if (!token) continue;
    const lastCh = token.charAt(token.length - 1);
    const parsedSize = sizeFromToken(lastCh);
    const num = parseInt(token, 10);
    if (Number.isNaN(num) || num < 1 || num > importedCount || seen[String(num)]) continue;
    seen[String(num)] = true;
    usedSlots.push({ oldSlot: num, size: parsedSize });
  }
  return { usedSlots, seen };
}

export function backupPlaceSlotAt(
  grid: number[],
  slot: number,
  pos: number,
  size: number,
  maxSlots: number,
  gridCols: number,
): void {
  grid[pos] = slot;
  if (size > 1) {
    markSpannedCells(grid, pos, size, maxSlots, gridCols);
  }
}

export function planBackupButtonLayout(
  sourceButtons: readonly Partial<CardConfig>[],
  buttonOrder: string | null | undefined,
  targetSlots: number,
  gridCols: number,
): BackupButtonLayoutPlan {
  const importedCount = sourceButtons.length;
  const buttons: CardConfig[] = [];
  const importedSizes: SlotSizeMap = {};
  const slotMap: Record<string, number> = {};
  let orderStr = "";

  if (importedCount !== targetSlots) {
    const orderInfo = backupOrderUsedSlots(buttonOrder, importedCount);
    const usedSlots = orderInfo.usedSlots;
    const seen = orderInfo.seen;
    for (let j = 0; j < importedCount; j += 1) {
      const slotNum = j + 1;
      if (seen[String(slotNum)]) continue;
      const button = sourceButtons[j] || emptyCardConfig();
      if (button.entity || button.label || button.type) {
        usedSlots.push({ oldSlot: slotNum, size: 1 });
      }
    }

    const limit = Math.min(usedSlots.length, targetSlots);
    for (let u = 0; u < limit; u += 1) {
      const newSlot = u + 1;
      const used = usedSlots[u];
      if (!used) continue;
      slotMap[String(used.oldSlot)] = newSlot;
      buttons.push(cloneCardConfig(sourceButtons[used.oldSlot - 1] || emptyCardConfig()));
      if (used.size > 1) importedSizes[String(newSlot)] = used.size;
    }
    for (let fill = limit; fill < targetSlots; fill += 1) {
      buttons.push(emptyCardConfig());
    }

    const newGrid = Array<number>(targetSlots).fill(0);
    let pos = 0;
    for (let p = 0; p < limit && pos < targetSlots; p += 1) {
      const newSlot = p + 1;
      let targetSize = importedSizes[String(newSlot)] || 1;
      if (!sizeFitsAt(pos, targetSize, targetSlots, gridCols)) {
        targetSize = 1;
        delete importedSizes[String(newSlot)];
      }
      backupPlaceSlotAt(newGrid, newSlot, pos, targetSize, targetSlots, gridCols);
      pos += 1;
      while (pos < targetSlots && newGrid[pos] === -1) pos += 1;
    }
    orderStr = serializeGridOrder(newGrid, importedSizes);
  } else {
    for (let i = 0; i < targetSlots; i += 1) {
      buttons.push(cloneCardConfig(i < importedCount ? sourceButtons[i] : emptyCardConfig()));
      slotMap[String(i + 1)] = i + 1;
    }
    orderStr = String(buttonOrder || "");
  }

  return {
    importedCount,
    buttons,
    button_order: orderStr,
    importedSizes,
    slotMap,
  };
}
