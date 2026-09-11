"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("card editor save controller", () => {
  const { runCardEditorSaveControllerTests } = loadTypescriptTest("tests/web/card_editor_save_controller.test.ts");
  runCardEditorSaveControllerTests();
});
