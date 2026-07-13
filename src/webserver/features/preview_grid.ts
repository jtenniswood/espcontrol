import {
  clearSpans,
  coveredCells,
  markSpannedCells,
  sizeFitsAt,
  type SlotSizeMap,
} from "../model/grid";

export interface DuplicatePlacement {
  readonly pos: number;
  readonly size: number;
}

export interface SelectedGridMove {
  readonly accepted: boolean;
  readonly grid: number[];
}

export function resolveSpanPosition(
  grid: readonly number[],
  sizes: SlotSizeMap,
  pos: number,
  maxSlots: number,
  gridCols: number,
): number {
  if (grid[pos] !== -1) return pos;
  for (let anchor = 0; anchor < maxSlots; anchor += 1) {
    const slot = grid[anchor] ?? 0;
    if (!(slot > 0 || slot === -2)) continue;
    const cells = coveredCells(anchor, sizes[String(slot)] || 1, maxSlots, gridCols, false);
    if (cells.indexOf(pos) !== -1) return anchor;
  }
  return pos;
}

export function canPlaceSlotAt(
  grid: readonly number[],
  pos: number,
  size: number,
  maxSlots: number,
  gridCols: number,
): boolean {
  if (pos < 0 || pos >= maxSlots || grid[pos] !== 0) return false;
  if (!sizeFitsAt(pos, size, maxSlots, gridCols)) return false;
  const cells = coveredCells(pos, size, maxSlots, gridCols, false);
  return cells.every((cell) => grid[cell] === 0);
}

export function findPlacementCell(
  grid: readonly number[],
  start: number,
  size: number,
  maxSlots: number,
  gridCols: number,
): number {
  for (let offset = 0; offset < maxSlots; offset += 1) {
    const candidate = (start + offset) % maxSlots;
    if (canPlaceSlotAt(grid, candidate, size, maxSlots, gridCols)) return candidate;
  }
  return -1;
}

export function findDuplicatePlacement(
  grid: readonly number[],
  start: number,
  size: number,
  maxSlots: number,
  gridCols: number,
): DuplicatePlacement {
  const targetSize = size || 1;
  let pos = findPlacementCell(grid, start, targetSize, maxSlots, gridCols);
  if (pos >= 0) return { pos, size: targetSize };
  if (targetSize !== 1) {
    pos = findPlacementCell(grid, start, 1, maxSlots, gridCols);
    if (pos >= 0) return { pos, size: 1 };
  }
  return { pos: -1, size: targetSize };
}

export function placeSlotAt(grid: number[], slot: number, pos: number, size: number, gridCols: number): void {
  grid[pos] = slot;
  markSpannedCells(grid, pos, size, grid.length, gridCols);
}

export function placeOrderedGridEntries(
  entries: readonly number[],
  sizes: SlotSizeMap,
  maxSlots: number,
  gridCols: number,
): number[] {
  const grid = Array<number>(maxSlots).fill(0);
  for (let index = 0; index < entries.length && index < maxSlots; index += 1) {
    const slot = entries[index] ?? 0;
    if (!(slot > 0 || slot === -2)) continue;

    let targetSize = sizes[String(slot)] || 1;
    let place = index;
    if (!canPlaceSlotAt(grid, place, targetSize, maxSlots, gridCols)) {
      place = findPlacementCell(grid, place, targetSize, maxSlots, gridCols);
    }
    if (place < 0 && targetSize !== 1) {
      targetSize = 1;
      place = canPlaceSlotAt(grid, index, targetSize, maxSlots, gridCols)
        ? index
        : findPlacementCell(grid, index, targetSize, maxSlots, gridCols);
    }
    if (place < 0) continue;

    if (targetSize === 1) delete sizes[String(slot)];
    else sizes[String(slot)] = targetSize;
    placeSlotAt(grid, slot, place, targetSize, gridCols);
  }
  return grid;
}

export function moveSelectedGridEntries(
  sourceGrid: readonly number[],
  sizes: SlotSizeMap,
  selected: readonly number[],
  fromPos: number,
  toPos: number,
  maxSlots: number,
  gridCols: number,
): SelectedGridMove {
  const entriesAtPositions = sourceGrid.slice(0, maxSlots);
  clearSpans(entriesAtPositions, maxSlots);
  const resolvedTarget = resolveSpanPosition(sourceGrid, sizes, toPos, maxSlots, gridCols);
  if (resolvedTarget < 0 || resolvedTarget >= maxSlots) return { accepted: false, grid: sourceGrid.slice() };

  const movingSlot = entriesAtPositions[fromPos] ?? 0;
  if (movingSlot === -2 || selected.indexOf(-2) !== -1) return { accepted: false, grid: sourceGrid.slice() };
  if (selected.length <= 1 || selected.indexOf(movingSlot) === -1) {
    return { accepted: false, grid: sourceGrid.slice() };
  }

  const targetSlot = entriesAtPositions[resolvedTarget] ?? 0;
  if (targetSlot > 0 && selected.indexOf(targetSlot) !== -1) {
    return { accepted: true, grid: sourceGrid.slice() };
  }

  const entries = entriesAtPositions.filter((entry) => !(entry > 0 && selected.indexOf(entry) !== -1));
  while (entries.length < maxSlots) entries.push(0);

  let insertPos: number;
  if (targetSlot > 0 || targetSlot === -2) {
    const targetIndex = entries.indexOf(targetSlot);
    insertPos = targetIndex < 0 ? resolvedTarget : targetIndex + 1;
  } else {
    insertPos = resolvedTarget;
    for (let index = 0; index < resolvedTarget; index += 1) {
      const entry = entriesAtPositions[index] ?? 0;
      if (entry > 0 && selected.indexOf(entry) !== -1) insertPos -= 1;
    }
  }
  insertPos = Math.max(0, Math.min(insertPos, entries.length));
  entries.splice(insertPos, 0, ...selected);

  return {
    accepted: true,
    grid: placeOrderedGridEntries(entries.slice(0, maxSlots), sizes, maxSlots, gridCols),
  };
}
