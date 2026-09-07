"use strict";
const { test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");
const { createCompanionCatalogue } = loadTypeScriptModule("src/webserver/api/companion_catalogue.ts");

test("catalogue caches are isolated and failed refresh preserves offline identities", async () => {
  let calls = 0, fail = false;
  const first = createCompanionCatalogue(async (path, options) => {
    assert.equal(path, "/companion/actions");
    assert.equal(options.cache, "no-store");
    ++calls;
    if (fail) throw new Error("offline");
    return { ok: true, json: async () => [{ id: "first", label: "First" }, null, { id: 3 }] };
  });
  const second = createCompanionCatalogue(async () => ({ ok: true, json: async () => [{ id: "second", label: "Second" }] }));
  const pending = first.load();
  assert.equal(first.load(), pending);
  assert.deepEqual(await pending, [{ id: "first", label: "First" }]);
  assert.deepEqual(await second.load(), [{ id: "second", label: "Second" }]);
  fail = true;
  await assert.rejects(first.load(true), /offline/);
  assert.deepEqual(await first.load(), [{ id: "first", label: "First" }]);
  assert.equal(calls, 2);
});
