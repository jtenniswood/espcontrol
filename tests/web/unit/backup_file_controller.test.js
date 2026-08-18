"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("backup file controller", () => {
  const { runBackupFileControllerTests } = loadTypescriptTest("tests/web/backup_file_controller.test.ts");
  runBackupFileControllerTests();
});
