const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("badge refresh readiness follows completed reads and failures", async () => {
  await loadTypescriptTest("tests/web/badge_row_load.test.ts").runBadgeLoadTests();
});
