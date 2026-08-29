import { createCardEditorSaveController } from "../../src/webserver/features/card_editor_save_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runCardEditorSaveControllerTests(): void {
  const empty = () => ({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" });
  const controller = createCardEditorSaveController({ emptyCard: empty, copyCard: (target, source) => Object.assign(target, source) });
  const grid = [0, 0]; const buttons = [empty()];
  let result = controller.apply({ key: "main:new:1:2", slot: 2, homeSlot: null, isSub: false, isNew: true, pos: 1, dirty: true, button: { ...empty(), label: "Kitchen" } }, { slot: 2, maxSlots: 2, isSubpage: false, grid, buttons });
  equal(result.accepted, true, "a new draft can claim a free grid cell");
  equal(grid[1], 2, "a new draft reserves its selected cell");
  equal(buttons[1]!.label, "Kitchen", "a new draft is copied into its allocated slot");
  equal(result.saveGrid, true, "new main cards save the grid order");

  result = controller.apply({ key: "main:new:1:2", slot: 2, homeSlot: null, isSub: false, isNew: true, pos: 1, dirty: true, button: empty() }, { slot: 2, maxSlots: 2, isSubpage: false, grid, buttons });
  equal(result.accepted, false, "a stale new draft cannot overwrite an occupied cell");

  result = controller.apply({ key: "sub:4:1", slot: 1, homeSlot: 4, isSub: true, dirty: true, button: { ...empty(), label: "Updated" } }, { slot: 1, maxSlots: 2, isSubpage: true, grid: [1, 0], buttons: [empty()] });
  equal(result.saveSubpage, true, "subpage edits use their subpage persistence route");
  equal(result.saveButton, false, "subpage edits do not post the main button entity");
}
