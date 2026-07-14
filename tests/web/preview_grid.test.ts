import {
  findDuplicatePlacement,
  moveSelectedGridEntries,
  placeOrderedGridEntries,
  resolveSpanPosition,
} from "../../src/webserver/features/preview_grid";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

export function runPreviewGridTests(): void {
  const duplicateGrid = Array.from({ length: 20 }, (_, index) => index + 1);
  duplicateGrid[1] = 0;
  duplicateGrid[2] = 0;
  deepEqual(
    findDuplicatePlacement(duplicateGrid, 19, 3, 20, 5),
    { pos: 1, size: 3 },
    "duplicate placement wraps and preserves a wide card",
  );
  duplicateGrid[2] = 3;
  deepEqual(
    findDuplicatePlacement(duplicateGrid, 19, 3, 20, 5),
    { pos: 1, size: 1 },
    "duplicate placement falls back to a single card",
  );

  const sizes: Record<string, number> = { "1": 3 };
  const placed = placeOrderedGridEntries([1, 2, 3], sizes, 10, 5);
  deepEqual(placed, [1, -1, 2, 3, 0, 0, 0, 0, 0, 0], "ordered placement reserves wide spans");
  equal(resolveSpanPosition(placed, sizes, 1, 10, 5), 0, "spanned cells resolve to their anchor");

  const moved = moveSelectedGridEntries([1, 2, 3, 4, 0, 0], {}, [1, 2], 0, 3, 6, 3);
  equal(moved.accepted, true, "multi-selection move is accepted");
  deepEqual(moved.grid, [3, 4, 1, 2, 0, 0], "multi-selection keeps selection order after the target");

  const clockMove = moveSelectedGridEntries([-2, 1, 2, 0], {}, [-2, 1], 0, 2, 4, 2);
  equal(clockMove.accepted, false, "clock bar cannot be moved with selected cards");
}
