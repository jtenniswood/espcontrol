"use strict";

const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("card editor validation controller", () => {
  const { runCardEditorValidationControllerTests } = loadTypescriptTest("tests/web/card_editor_validation_controller.test.ts");
  runCardEditorValidationControllerTests();
});
