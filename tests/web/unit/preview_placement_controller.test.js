"use strict";

const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("preview placement controller", () => {
  const { runPreviewPlacementControllerTests } = loadTypescriptTest("tests/web/preview_placement_controller.test.ts");
  runPreviewPlacementControllerTests();
});
