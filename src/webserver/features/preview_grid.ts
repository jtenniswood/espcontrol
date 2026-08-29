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

export interface GridSlotResize {
  readonly accepted: boolean;
  readonly grid: number[];
  readonly sizes: SlotSizeMap;
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

export function resizeGridSlot(
  sourceGrid: readonly number[],
  sourceSizes: SlotSizeMap,
  slot: number,
  pos: number,
  targetSize: number,
  maxSlots: number,
  gridCols: number,
  allowCardDisplacement: boolean,
): GridSlotResize {
  const grid = sourceGrid.slice(0, maxSlots);
  while (grid.length < maxSlots) grid.push(0);
  const sizes = { ...sourceSizes };
  const currentSize = sizes[String(slot)] || 1;

  if (!sizeFitsAt(pos, targetSize, maxSlots, gridCols)) {
    return { accepted: false, grid: sourceGrid.slice(), sizes: { ...sourceSizes } };
  }

  const targetCells = coveredCells(pos, targetSize, maxSlots, gridCols, true);
  const targetCellSet = new Set(targetCells);
  const displaced: Array<{ slot: number; pos: number; size: number; cells: number[] }> = [];
  for (let anchor = 0; anchor < maxSlots; anchor += 1) {
    const displacedSlot = sourceGrid[anchor] ?? 0;
    if (!(displacedSlot > 0 || displacedSlot === -2) || displacedSlot === slot) continue;
    const displacedSize = sizes[String(displacedSlot)] || 1;
    const displacedCells = coveredCells(anchor, displacedSize, maxSlots, gridCols, true);
    if (!displacedCells.some((cell) => targetCellSet.has(cell))) continue;
    if (displacedSlot > 0 && !allowCardDisplacement) {
      return { accepted: false, grid: sourceGrid.slice(), sizes: { ...sourceSizes } };
    }
    displaced.push({ slot: displacedSlot, pos: anchor, size: displacedSize, cells: displacedCells });
  }

  for (const cell of coveredCells(pos, currentSize, maxSlots, gridCols, true)) grid[cell] = 0;
  for (const item of displaced) {
    for (const cell of item.cells) grid[cell] = 0;
  }

  placeSlotAt(grid, slot, pos, targetSize, gridCols);
  const orderedDisplaced = displaced.slice().sort((a, b) => {
    const aArea = coveredCells(0, a.size, maxSlots, gridCols, true).length;
    const bArea = coveredCells(0, b.size, maxSlots, gridCols, true).length;
    return bArea - aArea || a.pos - b.pos;
  });
  const placeDisplaced = (index: number, plannedGrid: number[]): number[] | null => {
    if (index >= orderedDisplaced.length) return plannedGrid;
    const item = orderedDisplaced[index]!;
    for (let offset = 1; offset <= maxSlots; offset += 1) {
      const candidate = (item.pos + offset) % maxSlots;
      if (!canPlaceSlotAt(plannedGrid, candidate, item.size, maxSlots, gridCols)) continue;
      const nextGrid = plannedGrid.slice();
      placeSlotAt(nextGrid, item.slot, candidate, item.size, gridCols);
      const placed = placeDisplaced(index + 1, nextGrid);
      if (placed) return placed;
    }
    return null;
  };
  const plannedGrid = placeDisplaced(0, grid);
  if (!plannedGrid) return { accepted: false, grid: sourceGrid.slice(), sizes: { ...sourceSizes } };

  if (targetSize === 1) delete sizes[String(slot)];
  else sizes[String(slot)] = targetSize;
  return { accepted: true, grid: plannedGrid, sizes };
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
