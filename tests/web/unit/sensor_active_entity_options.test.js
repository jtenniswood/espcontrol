const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("sensor active entity options", () => {
  const { runSensorActiveEntityOptionsTests } = loadTypescriptTest("tests/web/sensor_active_entity_options.test.ts");
  runSensorActiveEntityOptionsTests();
});
