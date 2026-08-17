"use strict";

const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("card editor draft controller", () => {
  const { runCardEditorDraftControllerTests } = loadTypescriptTest("tests/web/card_editor_draft_controller.test.ts");
  runCardEditorDraftControllerTests();
});
