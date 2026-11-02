const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("native page transfers commit before display, retain rejected cuts and protect concurrent edits", async () => {
  await loadTypescriptTest("tests/web/native_page_transfers.test.ts").runNativePageTransferTests();
});
