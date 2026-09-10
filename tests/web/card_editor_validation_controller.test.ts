import { createCardEditorValidationController } from "../../src/webserver/features/card_editor_validation_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runCardEditorValidationControllerTests(): void {
  const controller = createCardEditorValidationController();
  let result = controller.validateSave({
    fields: [{ value: "" }, { value: "ignored", active: false }],
    isSubpage: false, serializedConfigLength: 1, imageCardCount: 0, imageCardCapacity: 1,
  });
  equal(result.reason, "required-field", "an active empty card field blocks saving");
  equal(result.firstInvalidIndex, 0, "the first missing field is identified for focus");

  result = controller.validateSave({
    fields: [{ value: "        ", present: true }],
    isSubpage: false, serializedConfigLength: 1, imageCardCount: 0, imageCardCapacity: 1,
  });
  equal(result.valid, true, "credential-specific presence rules can preserve all-space values");

  result = controller.validateSave({
    fields: [{ value: "non-empty", present: false }],
    isSubpage: false, serializedConfigLength: 1, imageCardCount: 0, imageCardCapacity: 1,
  });
  equal(result.reason, "required-field", "credential-specific presence rules remain authoritative");

  result = controller.validateSave({
    fields: [{ value: "light.kitchen" }], isSubpage: false, serializedConfigLength: 256,
    imageCardCount: 0, imageCardCapacity: 1,
  });
  equal(result.reason, "config-size", "main-panel card configurations retain their firmware size limit");

  result = controller.validateSave({
    fields: [{ value: "image.example" }], isSubpage: true, serializedConfigLength: 256,
    imageCardCount: 2, imageCardCapacity: 1,
  });
  equal(result.reason, "image-limit", "image capacity is enforced for subpages as well");

  result = controller.validateSave({
    fields: [{ value: "image.example" }], isSubpage: true, serializedConfigLength: 256,
    imageCardCount: 1, imageCardCapacity: 1,
  });
  equal(result.valid, true, "subpage cards can save beyond the main-panel configuration limit");
}
