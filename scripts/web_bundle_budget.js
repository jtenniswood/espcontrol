"use strict";

const assert = require("node:assert/strict");

function checkBundleBudget(size, budget) {
  const failures = [];
  const lines = [];
  for (const format of ["minified", "gzip"]) {
    assert(Number.isSafeInteger(budget[format]) && budget[format] > 0,
      `${format} bundle budget must be a positive byte count`);
    assert(Number.isSafeInteger(size[format]) && size[format] >= 0,
      `${format} bundle size must be a non-negative byte count`);
    const remaining = budget[format] - size[format];
    const message = `${format}: ${size[format]} / ${budget[format]} bytes (${remaining} bytes remaining)`;
    lines.push(message);
    if (remaining < 0) failures.push(message);
  }
  assert(failures.length === 0,
    `Web bundle exceeds its size budget:\n${failures.join("\n")}\n` +
    "Reduce bundle growth or explicitly review compatibility/fixtures/web_migration_baseline.json budgets.");
  return lines.join("\n");
}

module.exports = { checkBundleBudget };
