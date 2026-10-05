"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");
const { checkBundleBudget } = require("../../../scripts/web_bundle_budget");

const budget = { minified: 2750000, gzip: 1150000 };

test("routine growth, shrinkage and the exact budget are allowed", () => {
  for (const size of [
    { minified: 2625897, gzip: 1094027 },
    { minified: 2623977, gzip: 1090000 },
    budget,
  ]) {
    assert.match(checkBundleBudget(size, budget), /bytes remaining/);
  }
});

test("each budget is enforced independently with actionable measurements", () => {
  for (const format of ["minified", "gzip"]) {
    assert.throws(() => checkBundleBudget({ ...budget, [format]: budget[format] + 1 }, budget),
      new RegExp(`${format}: ${budget[format] + 1} / ${budget[format]} bytes`));
  }
});

test("missing or invalid budgets cannot silently disable the size check", () => {
  for (const value of [undefined, 0, -1, NaN, Infinity, "2750000"]) {
    assert.throws(() => checkBundleBudget(budget, { ...budget, minified: value }),
      /budget must be a positive byte count/);
  }
});
