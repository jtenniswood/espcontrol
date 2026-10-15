const { test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { continuousCornerPath } = loadTypescriptTest("src/webserver/application/preview_corners.ts");
function diagonal(path) {
  return [...path.matchAll(/L ([\d.]+) ([\d.]+)/g)].map(([, x, y]) => [Number(x), Number(y)])
    .filter(([x, y]) => x > 0 && y > 0 && x < 32 && y < 32)
    .sort((a, b) => Math.abs(a[0] - a[1]) - Math.abs(b[0] - b[1]))[0][0];
}
test("badge ends sit between the card superellipse and a completely round pill", () => {
  const card = continuousCornerPath(160, 64, 16);
  const badge = continuousCornerPath(160, 64, 16, true);
  assert.notEqual(badge, card);
  assert(diagonal(badge) > diagonal(card), "badge corners cut inward further for a fuller pill silhouette");
  assert(diagonal(badge) < 32 * (1 - Math.SQRT1_2), "ends retain a flatter tip than a circular pill");
  assert(badge.endsWith(" Z"));
});
