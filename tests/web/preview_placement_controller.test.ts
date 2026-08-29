import { createPreviewPlacementController } from "../../src/webserver/features/preview_placement_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

export function runPreviewPlacementControllerTests(): void {
  const controller = createPreviewPlacementController();
  const moved = controller.moveSingle({
    grid: [1, -1, 2, 3, 0, 0, 0, 0, 0, 0],
    sizes: { "1": 3 },
    maxSlots: 10,
    selected: [],
  }, 0, 3, 5);
  equal(moved.accepted, true, "single-card moves are accepted");
  deepEqual(moved.grid, [3, 0, 2, 1, -1, 0, 0, 0, 0, 0], "moving a wide card keeps its span at the new position");
  deepEqual(moved.sizes, { "1": 3 }, "moving a wide card keeps its requested size when it still fits");

  const downgraded = controller.moveSingle({
    grid: [1, -1, 2, 3, 4, 0, 0, 0, 0, 0],
    sizes: { "1": 3 },
    maxSlots: 10,
    selected: [],
  }, 0, 4, 5);
  equal(downgraded.accepted, true, "a move into the final column is accepted");
  equal(downgraded.sizes["1"], undefined, "a wide card becomes single when its new cell cannot fit its span");

  const rejected = controller.moveSingle({
    grid: [1, -1, 2, 0],
    sizes: { "1": 3 },
    maxSlots: 4,
    selected: [],
  }, 2, -1, 2);
  equal(rejected.accepted, false, "invalid drop targets leave the grid unchanged");
  deepEqual(rejected.grid, [1, -1, 2, 0], "rejected targets preserve spans");

  const selected = controller.moveSelected({
    grid: [1, 2, 3, 4, 0, 0],
    sizes: {},
    maxSlots: 6,
    selected: [1, 2],
  }, 0, 3, 3);
  equal(selected.accepted, true, "multi-card movement remains available through the placement controller");
  deepEqual(selected.grid, [3, 4, 1, 2, 0, 0], "multi-card movement preserves selection order");
}
