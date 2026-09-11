const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("legacy layout restore", async () => {
  const { runLegacyLayoutRestoreTests } = loadTypescriptTest("tests/web/legacy_layout_restore.test.ts");
  await runLegacyLayoutRestoreTests();
});
