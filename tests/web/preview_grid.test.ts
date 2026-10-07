import {
  findDuplicatePlacement,
  moveSelectedGridEntries,
  placeOrderedGridEntries,
  resizeGridSlot,
  resolveSpanPosition,
} from "../../src/webserver/features/preview_grid";
import { applySpans, parseGridOrder, serializeGridOrder } from "../../src/webserver/model/grid";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

export function runPreviewGridTests(): void {
  const portraitOrder = [19, 20, ...Array.from({ length: 16 }, (_, index) => index + 1), 17, 18];
  const portraitSizes: Record<string, number> = { "19": 3 };
  applySpans(portraitOrder, portraitSizes, 18, 3);
  deepEqual(
    portraitOrder,
    [...Array.from({ length: 18 }, (_, index) => index + 1), 19, 20],
    "portrait span normalization keeps all active slots visible and moves hidden slots to the landscape tail",
  );
  equal(portraitSizes["19"], 3, "portrait span normalization preserves hidden slot sizing");
  equal(
    serializeGridOrder(portraitOrder, portraitSizes),
    "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19w,20",
    "portrait order serialization retains hidden slots for landscape restoration",
  );

  const sparsePortraitOrder = [19, ...Array.from({ length: 15 }, (_, index) => index + 1), 0, 0, 16, 17];
  const sparsePortraitSizes: Record<string, number> = {};
  applySpans(sparsePortraitOrder, sparsePortraitSizes, 18, 3);
  deepEqual(
    sparsePortraitOrder,
    [...Array.from({ length: 17 }, (_, index) => index + 1), 0, 19, 0],
    "portrait normalization uses empty prefix cells for visible cards from the landscape tail",
  );

  const deferredPortraitOrder = parseGridOrder(
    "19w,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,,,16,17",
    20,
    3,
    {},
    18,
  );
  deepEqual(
    deferredPortraitOrder.grid,
    [...Array.from({ length: 17 }, (_, index) => index + 1), 0, 19, 0],
    "deferred startup parsing normalizes against the active portrait capacity",
  );
  equal(deferredPortraitOrder.sizes["19"], 3, "deferred portrait parsing preserves hidden card size");

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

  const crowded = [1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
  const rejectedResize = resizeGridSlot(crowded, {}, 1, 0, 11, 15, 5, true);
  equal(rejectedResize.accepted, false, "landscape expansion is rejected when displaced cards cannot all fit");
  deepEqual(rejectedResize.grid, crowded, "rejected landscape expansion leaves every card in place");
  deepEqual(rejectedResize.sizes, {}, "rejected landscape expansion leaves card sizes unchanged");

  const gridWithWideCard = [1, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
  const resizedAroundWideCard = resizeGridSlot(gridWithWideCard, { "2": 3 }, 1, 0, 11, 20, 5, true);
  equal(resizedAroundWideCard.accepted, true, "landscape expansion relocates a displaced wide card");
  deepEqual(
    resizedAroundWideCard.grid,
    [1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 2, -1, 0, 0, 0],
    "a relocated wide card keeps its complete span",
  );
  deepEqual(resizedAroundWideCard.sizes, { "1": 11, "2": 3 }, "a relocated wide card keeps its size");

  const noRoomForWideCard = gridWithWideCard.slice(0, 15);
  const rejectedWideResize = resizeGridSlot(noRoomForWideCard, { "2": 3 }, 1, 0, 11, 15, 5, true);
  equal(rejectedWideResize.accepted, false, "landscape expansion is rejected when a wide card cannot fit");
  deepEqual(rejectedWideResize.grid, noRoomForWideCard, "rejected expansion preserves the wide card span");
  deepEqual(rejectedWideResize.sizes, { "2": 3 }, "rejected expansion preserves the wide card size");

  const constrainedGrid = [0, 1, 0, 0, 2, 0, 0, 3, 0, 0, 0, 0, -1, 0, 0];
  const constrainedResize = resizeGridSlot(constrainedGrid, { "3": 2 }, 1, 1, 11, 15, 5, true);
  equal(constrainedResize.accepted, true, "landscape expansion plans constrained cards before singles");
  equal(constrainedResize.grid[0], 3, "the displaced tall card uses the only two-cell destination");
  equal(constrainedResize.grid[5], -1, "the relocated tall card keeps its full span");
  equal(constrainedResize.grid[10], 2, "the flexible single card moves after the tall card");

  const moved = moveSelectedGridEntries([1, 2, 3, 4, 0, 0], {}, [1, 2], 0, 3, 6, 3);
  equal(moved.accepted, true, "multi-selection move is accepted");
  deepEqual(moved.grid, [3, 4, 1, 2, 0, 0], "multi-selection keeps selection order after the target");

  const clockMove = moveSelectedGridEntries([-2, 1, 2, 0], {}, [-2, 1], 0, 2, 4, 2);
  equal(clockMove.accepted, false, "clock bar cannot be moved with selected cards");
}
