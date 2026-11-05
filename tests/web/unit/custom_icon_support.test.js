"use strict";

const { describe, test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const { createCustomIconSupport } = loadTypescriptTest("src/webserver/application/custom_icon_support.ts");

function respond(status, body) {
  return { ok: status >= 200 && status < 300, status, json: async () => body };
}

function support(responses) {
  const requests = [];
  const queue = [...responses];
  const request = async (url, init) => {
    requests.push({ url, init });
    const next = queue.shift();
    if (next instanceof Error) throw next;
    return next;
  };
  const created = createCustomIconSupport(request, (callback) => callback());
  return { created, requests };
}

describe("custom icon support", () => {
  test("is off until the firmware answers", () => {
    const { created } = support([respond(200, { icons: { custom: true } })]);
    assert.equal(created.available(), false);
  });

  test("turns on only when the firmware reports custom icons", async () => {
    const on = support([respond(200, { icons: { custom: true } })]);
    assert.equal(await on.created.ready, true);
    assert.equal(on.created.available(), true);
    assert.equal(on.requests[0].url, "/api/v1/capabilities");
    assert.equal(on.requests[0].init.credentials, "include");

    const off = support([respond(200, { icons: { custom: false } })]);
    assert.equal(await off.created.ready, false);
    assert.equal(off.created.available(), false);
  });

  test("stays off for older firmware without the icons field", async () => {
    const { created } = support([respond(200, { configuration: { read: true } })]);
    assert.equal(await created.ready, false);
    assert.equal(created.available(), false);
  });

  test("stays off when the panel cannot be reached", async () => {
    const failed = support([new Error("offline")]);
    assert.equal(await failed.created.ready, false);
    const missing = support([respond(404, null)]);
    assert.equal(await missing.created.ready, false);
    assert.equal(missing.requests.length, 1);
  });

  test("retries while the firmware is still starting", async () => {
    const { created, requests } = support([
      respond(503, null), respond(503, null), respond(200, { icons: { custom: true } }),
    ]);
    assert.equal(await created.ready, true);
    assert.equal(requests.length, 3);
  });

  test("gives up after a few attempts", async () => {
    const { created, requests } = support([
      respond(503, null), respond(503, null), respond(503, null), respond(503, null),
    ]);
    assert.equal(await created.ready, false);
    assert.equal(requests.length, 4);
  });
});
