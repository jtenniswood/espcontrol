"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("backup export controller", () => {
  const { runBackupExportControllerTests } = loadTypescriptTest("tests/web/backup_export_controller.test.ts");
  runBackupExportControllerTests();
});
