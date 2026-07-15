export type SlotSizeMap = Record<string, number>;

export const CARD_SIZE_SINGLE = 1;
export const CARD_SIZE_TALL = 2;
export const CARD_SIZE_WIDE = 3;
export const CARD_SIZE_LARGE = 4;
export const CARD_SIZE_EXTRA_TALL = 5;
export const CARD_SIZE_EXTRA_WIDE = 6;
export const CARD_SIZE_EXTRA_LARGE = 7;
export const CARD_SIZE_MAX_WIDE = 8;
export const CARD_SIZE_MAX_TALL = 9;

export interface CardSizeDefinition {
  size: number;
  token: string;
  rowSpan: number;
  colSpan: number;
  className: string;
}

const CARD_SIZE_SINGLE_DEFINITION: CardSizeDefinition = {
  size: CARD_SIZE_SINGLE,
  token: "",
  rowSpan: 1,
  colSpan: 1,
  className: "",
};

export const CARD_SIZE_DEFINITIONS: readonly CardSizeDefinition[] = [
  CARD_SIZE_SINGLE_DEFINITION,
  { size: CARD_SIZE_TALL, token: "d", rowSpan: 2, colSpan: 1, className: "sp-btn-double" },
  { size: CARD_SIZE_WIDE, token: "w", rowSpan: 1, colSpan: 2, className: "sp-btn-wide" },
  { size: CARD_SIZE_LARGE, token: "b", rowSpan: 2, colSpan: 2, className: "sp-btn-big" },
  { size: CARD_SIZE_EXTRA_TALL, token: "t", rowSpan: 3, colSpan: 1, className: "sp-btn-extra-tall" },
  { size: CARD_SIZE_EXTRA_WIDE, token: "x", rowSpan: 1, colSpan: 3, className: "sp-btn-extra-wide" },
  { size: CARD_SIZE_EXTRA_LARGE, token: "q", rowSpan: 3, colSpan: 3, className: "sp-btn-extra-large" },
  { size: CARD_SIZE_MAX_WIDE, token: "h", rowSpan: 2, colSpan: 3, className: "sp-btn-max-wide" },
  { size: CARD_SIZE_MAX_TALL, token: "v", rowSpan: 3, colSpan: 2, className: "sp-btn-max-tall" },
];

export interface ParsedGridOrder {
  grid: number[];
  sizes: SlotSizeMap;
}

function copySizes(sizes: SlotSizeMap | undefined): SlotSizeMap {
  return { ...(sizes || {}) };
}

export function cardSizeDefinition(size: number | null | undefined): CardSizeDefinition {
  const normalized = size || CARD_SIZE_SINGLE;
  for (const definition of CARD_SIZE_DEFINITIONS) {
    if (definition.size === normalized) return definition;
  }
  return CARD_SIZE_SINGLE_DEFINITION;
}

export function sizeFromToken(token: string | null | undefined): number {
  const normalized = token || "";
  for (const definition of CARD_SIZE_DEFINITIONS) {
    if (definition.token === normalized) return definition.size;
  }
  return CARD_SIZE_SINGLE;
}

export function sizeToken(size: number | null | undefined): string {
  return cardSizeDefinition(size).token;
}

export function sizeRowSpan(size: number | null | undefined): number {
  return cardSizeDefinition(size).rowSpan;
}

export function sizeColSpan(size: number | null | undefined): number {
  return cardSizeDefinition(size).colSpan;
}

export function cardSizeClass(size: number | null | undefined): string {
  return cardSizeDefinition(size).className;
}

export function coveredCells(
  pos: number,
  size: number | null | undefined,
  _maxSlots: number,
  gridCols: number,
  includeOrigin: boolean,
): number[] {
  const cells: number[] = [];
  const rowSpan = sizeRowSpan(size);
  const colSpan = sizeColSpan(size);
  for (let r = 0; r < rowSpan; r += 1) {
    for (let c = 0; c < colSpan; c += 1) {
      if (!includeOrigin && r === 0 && c === 0) continue;
      cells.push(pos + r * gridCols + c);
    }
  }
  return cells;
}

export function sizeFitsAt(
  pos: number,
  size: number | null | undefined,
  maxSlots: number,
  gridCols: number,
): boolean {
  if (pos < 0 || pos >= maxSlots || gridCols <= 0) return false;
  const col = pos % gridCols;
  const row = Math.floor(pos / gridCols);
  const rows = Math.ceil(maxSlots / gridCols);
  return col + sizeColSpan(size) <= gridCols &&
    row + sizeRowSpan(size) <= rows &&
    pos + (sizeRowSpan(size) - 1) * gridCols + sizeColSpan(size) - 1 < maxSlots;
}

export function markSpannedCells(
  grid: number[],
  pos: number,
  size: number | null | undefined,
  maxSlots: number,
  gridCols: number,
): void {
  const cells = coveredCells(pos, size, maxSlots, gridCols, false);
  for (const cell of cells) {
    if (cell >= 0 && cell < maxSlots) grid[cell] = -1;
  }
}

export function applySpans(
  grid: number[],
  sizes: SlotSizeMap,
  maxSlots: number,
  gridCols: number,
): void {
  for (let i = 0; i < maxSlots; i += 1) {
    const slot = grid[i] ?? 0;
    if (!(slot > 0 || slot === -2)) continue;
    const slotKey = String(slot);
    const size = sizes[slotKey] || 1;
    if (size <= 1) continue;
    if (!sizeFitsAt(i, size, maxSlots, gridCols)) {
      delete sizes[slotKey];
      continue;
    }
    const toReserve = coveredCells(i, size, maxSlots, gridCols, false);
    let ok = true;
    for (const cell of toReserve) {
      const displaced = grid[cell] ?? 0;
      if (displaced > 0 || displaced === -2) {
        let placed = false;
        for (let j = 0; j < maxSlots; j += 1) {
          if ((grid[j] ?? 0) === 0 && toReserve.indexOf(j) === -1) {
            grid[j] = displaced;
            placed = true;
            break;
          }
        }
        if (!placed) {
          ok = false;
          break;
        }
        grid[cell] = 0;
      }
    }
    if (!ok) {
      delete sizes[slotKey];
      continue;
    }
    for (const cell of toReserve) grid[cell] = -1;
  }
}

export function parseGridOrder(
  order: string | null | undefined,
  maxSlots: number,
  gridCols: number,
  initialSizes?: SlotSizeMap,
): ParsedGridOrder {
  const grid = Array<number>(maxSlots).fill(0);
  const sizes = copySizes(initialSizes);
  if (!order || !order.trim()) return { grid, sizes };
  const parts = order.split(",");
  for (let i = 0; i < parts.length && i < maxSlots; i += 1) {
    const value = (parts[i] || "").trim();
    if (!value) continue;
    const last = value.charAt(value.length - 1);
    const parsedSize = sizeFromToken(last);
    const slot = parseInt(value, 10);
    if (slot >= 1 && slot <= maxSlots && !Number.isNaN(slot)) {
      grid[i] = slot;
      if (parsedSize > 1) sizes[String(slot)] = parsedSize;
    }
  }
  applySpans(grid, sizes, maxSlots, gridCols);
  return { grid, sizes };
}

export function serializeGridOrder(grid: readonly number[], sizes: SlotSizeMap): string {
  let last = -1;
  for (let i = grid.length - 1; i >= 0; i -= 1) {
    if ((grid[i] ?? 0) > 0) {
      last = i;
      break;
    }
  }
  if (last < 0) return "";
  return grid.slice(0, last + 1).map((slot) => {
    if (slot <= 0) return "";
    return String(slot) + sizeToken(sizes[String(slot)]);
  }).join(",");
}

export function clearSpans(grid: number[], maxSlots: number): void {
  for (let i = 0; i < maxSlots; i += 1) {
    if (grid[i] === -1) grid[i] = 0;
  }
}
