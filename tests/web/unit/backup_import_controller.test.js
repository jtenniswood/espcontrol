"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("backup import controller", () => {
  const { runBackupImportControllerTests } = loadTypescriptTest("tests/web/backup_import_controller.test.ts");
  runBackupImportControllerTests();
});
