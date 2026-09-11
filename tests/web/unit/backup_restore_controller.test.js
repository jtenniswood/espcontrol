"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("backup restore controller", async () => {
  const { runBackupRestoreControllerTests } = loadTypescriptTest("tests/web/backup_restore_controller.test.ts");
  await runBackupRestoreControllerTests();
});
