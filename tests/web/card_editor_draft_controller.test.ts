import { createCardEditorDraftController } from "../../src/webserver/features/card_editor_draft_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runCardEditorDraftControllerTests(): void {
  const controller = createCardEditorDraftController({
    cloneCard: (button) => ({ ...button }),
    emptyCard: () => ({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" }),
  });
  const main = { slot: 2, homeSlot: null, isSub: false };
  equal(controller.keyFor(main), "main:2", "main card drafts use a stable key");
  const created = controller.newDraft({ ...main, pos: 4 });
  equal(created.key, "main:new:4:2", "new drafts include their original cell");
  equal(controller.matchesNewDraft(created, main), true, "new draft matches its active editor location");
  const existing = controller.ensureExistingDraft(null, main, { ...created.button, label: "Kitchen" });
  equal(existing.isNew, undefined, "existing cards do not become new drafts");
  equal(existing.button.label, "Kitchen", "existing cards are cloned into their draft");
  controller.markDirty(existing, existing.key);
  equal(existing.dirty, true, "editing marks only the active draft as dirty");
  controller.markDirty(existing, "main:3");
  equal(existing.dirty, true, "a different draft key does not replace the active draft");
}
