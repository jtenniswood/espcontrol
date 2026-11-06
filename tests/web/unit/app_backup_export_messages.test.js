"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { runAppBackupExportMessageTest } = loadTypescriptTest("tests/web/app_backup_export_messages.test.ts");

for (const outcome of ["success", "cancel", "download-error"]) {
  test(`identity omission is reported only after a downloaded backup: ${outcome}`, async () => {
    const result = await runAppBackupExportMessageTest(outcome);
    assert.equal(result.downloads, outcome === "success" ? 1 : 0);
    assert.equal(result.banners.some(item => item.message.includes("Backup exported without the panel name")), outcome === "success");
    if (outcome === "download-error") assert.deepEqual(result.banners.at(-1), { message: "Download failed", kind: "error" });
  });
}
