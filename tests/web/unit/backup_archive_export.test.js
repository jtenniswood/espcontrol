"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("backup archive export", async () => {
  const { runBackupArchiveExportTests } = loadTypescriptTest("tests/web/backup_archive_export.test.ts");
  await runBackupArchiveExportTests();
});
