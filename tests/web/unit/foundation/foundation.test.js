const assert = require("node:assert/strict");
const test = require("node:test");

test("the Node test runner discovers nested repository unit tests", () => {
  assert.deepEqual(["firmware", "web"].sort(), ["web", "firmware"].sort());
});
