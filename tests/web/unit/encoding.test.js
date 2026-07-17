"use strict";

const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

describe("saved-configuration encoding", () => {
  const { runEncodingTests } = loadTypescriptTest("tests/web/encoding.test.ts");

  test("round-trips fields, options, cards, and subpages", () => {
    runEncodingTests();
  });
});
