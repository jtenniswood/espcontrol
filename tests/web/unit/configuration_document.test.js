"use strict";

const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

describe("revisioned configuration document", () => {
  const { runConfigurationDocumentTests } = loadTypescriptTest("tests/web/configuration_document.test.ts");

  test("round-trips entity identity and rejects malformed boundaries", () => {
    runConfigurationDocumentTests();
  });
});
