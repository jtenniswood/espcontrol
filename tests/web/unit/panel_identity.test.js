"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("panel naming and compatibility", async () => {
  await loadTypescriptTest("tests/web/panel_identity.test.ts").runPanelIdentityTests();
});
