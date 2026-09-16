"use strict";

const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

describe("notification card contract", () => {
  const { runNotificationCardTests } = loadTypescriptTest("tests/web/notification_card.test.ts");

  test("matches the firmware level aliases and option normalization", () => {
    runNotificationCardTests();
  });
});
