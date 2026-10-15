const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("configuration capacity follows firmware capabilities", async () => {
  const { runConfigCapacityTests } = loadTypescriptTest("tests/web/config_capacity.test.ts");
  await runConfigCapacityTests();
});
