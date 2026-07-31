"use strict";

const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

describe("preview logic", () => {
  const { runPreviewFeatureTests } = loadTypescriptTest("tests/web/preview_feature.test.ts");
  const { runPreviewGridTests } = loadTypescriptTest("tests/web/preview_grid.test.ts");

  test("selects cards and constrains preview interactions", () => {
    runPreviewFeatureTests();
  });

  test("places, duplicates, and moves card spans", () => {
    runPreviewGridTests();
  });
});
