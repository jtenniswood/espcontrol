"use strict";

const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

describe("generated card contracts", () => {
  const { runCardContractTests } = loadTypescriptTest("tests/web/card_contracts.test.ts");

  test("keeps defaults, runtime drivers, aliases, and compact codes aligned", () => {
    runCardContractTests();
  });
});
