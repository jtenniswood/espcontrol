"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("reconnect controller", () => {
  const { runReconnectControllerTests } = loadTypescriptTest("tests/web/reconnect.test.ts");
  runReconnectControllerTests();
});
