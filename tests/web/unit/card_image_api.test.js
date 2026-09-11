"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { createCardImageApi } = loadTypescriptTest("src/webserver/api/card_image_api.ts");
const { ResetSession } = loadTypescriptTest("src/webserver/api/reset_session.ts");

test("image mutations retain the editor epoch and stop after reset", async () => {
  let epoch = 4;
  const writes = [];
  const session = new ResetSession(async (url, init) => {
    if (url === "/api/v1/reset") return new Response(JSON.stringify({ modes: ["factory"], epoch, pending: false }));
    writes.push([url, init.method, init.headers.get("X-EspControl-Epoch"), init.body]);
    return new Response("{}", { status: epoch === 4 ? 200 : 409 });
  });
  const api = createCardImageApi((url, init) => session.fetch(url, init), id => `/card-images/${id}.jpg`);
  await api.upload(new Uint8Array([1]), "restore-session");
  await api.rename("image", "Kitchen & dining");
  await api.beginRestore();
  await api.commitRestore("restore-session");
  await api.rollbackRestore("restore-session");
  epoch = 5;
  await api.delete("image");
  await assert.rejects(api.upload(new Uint8Array([1])), /reload/);
  assert.equal(writes.length, 6);
  assert(writes.every(write => write[2] === "4"));
  assert.equal(writes[0][0], "/api/card-images?restore=restore-session");
  assert.equal(writes[1][3], "name=Kitchen%20%26%20dining");
  assert.equal(writes[5][1], "DELETE");
});
