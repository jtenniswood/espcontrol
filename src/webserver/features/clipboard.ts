import type { CardConfig } from "../contracts/types";
import { cloneCardConfig } from "../model/card";
import type { SlotSizeMap } from "../model/grid";
import { findDuplicatePlacement, placeSlotAt } from "./preview_grid";

export interface ClipboardEntry extends CardConfig {
  readonly subpageConfig: string | null;
  readonly size: number;
}

export interface ClipboardPlacement {
  readonly slot: number;
  readonly pos: number;
  readonly size: number;
  readonly button: CardConfig;
  readonly subpageConfig: string | null;
}

export interface ClipboardPastePlan {
  readonly grid: number[];
  readonly sizes: SlotSizeMap;
  readonly placements: ClipboardPlacement[];
}

export function createClipboardEntry(
  source: Partial<CardConfig>,
  size: number,
  subpageConfig: string | null = null,
): ClipboardEntry {
  return {
    ...cloneCardConfig(source),
    subpageConfig,
    size: size || 1,
  };
}

export function planClipboardPaste(
  entries: readonly ClipboardEntry[],
  sourceGrid: readonly number[],
  sourceSizes: SlotSizeMap,
  start: number,
  availableSlots: readonly number[],
  maxSlots: number,
  gridCols: number,
): ClipboardPastePlan {
  const grid = sourceGrid.slice(0, maxSlots);
  while (grid.length < maxSlots) grid.push(0);
  const sizes = { ...sourceSizes };
  const placements: ClipboardPlacement[] = [];
  for (let index = 0; index < entries.length && index < availableSlots.length; index += 1) {
    const entry = entries[index];
    const slot = availableSlots[index];
    if (!entry || !slot) continue;
    const placement = findDuplicatePlacement(grid, start, entry.size || 1, maxSlots, gridCols);
    if (placement.pos < 0) break;
    if (placement.size === 1) delete sizes[String(slot)];
    else sizes[String(slot)] = placement.size;
    placeSlotAt(grid, slot, placement.pos, placement.size, gridCols);
    placements.push({
      slot,
      pos: placement.pos,
      size: placement.size,
      button: cloneCardConfig(entry),
      subpageConfig: entry.subpageConfig,
    });
  }
  return { grid, sizes, placements };
}
