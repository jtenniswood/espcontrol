"use strict";

const { test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const { createEntityCatalogClient } = loadTypescriptTest("src/webserver/application/entity_catalog.ts");

function response(status, body) {
  return {
    status,
    ok: status >= 200 && status < 300,
    json: async () => body,
  };
}

test("entity catalog accepts pending responses and follows pagination", async () => {
  const requests = [];
  const pages = [
    response(500, { status: "pending", request_id: 41 }),
    response(200, {
      protocol_version: 1,
      entities: [{ entity_id: "light.kitchen", name: "Kitchen Lights" }],
      next_cursor: 1,
    }),
    response(200, { status: "pending", request_id: 42 }),
    response(200, {
      protocol_version: 1,
      entities: [{ entity_id: "light.office", name: "Office Lights" }],
      next_cursor: null,
    }),
  ];
  const client = createEntityCatalogClient(undefined, async (url) => {
    requests.push(String(url));
    return pages.shift();
  });

  const records = await client.search("light", ["light"]);

  assert.deepEqual(records.map((record) => record.entity_id), ["light.kitchen", "light.office"]);
  assert.match(requests[0], /field=light/);
  assert.match(requests[0], /limit=25/);
  assert.match(requests[2], /cursor=1/);
});

test("entity catalog sends area, device and visibility filters", async () => {
  const requests = [];
  const client = createEntityCatalogClient(undefined, async (url) => {
    requests.push(String(url));
    if (requests.length % 2 === 1) return response(200, { status: "pending", request_id: 7 });
    return response(200, { protocol_version: 1, entities: [], next_cursor: null });
  });

  await client.search("", ["sensor", "binary_sensor", "text_sensor"], {
    area: "Kitchen",
    deviceId: "device-1",
    includeHidden: true,
    includeDisabled: true,
  });

  assert.match(requests[0], /field=sensor/);
  assert.match(requests[0], /area=Kitchen/);
  assert.match(requests[0], /device_id=device-1/);
  assert.match(requests[0], /include_hidden=1/);
  assert.match(requests[0], /include_disabled=1/);
});

test("entity catalog exposes a meaningful transport error", async () => {
  const client = createEntityCatalogClient(undefined, async () => response(503, {
    error: "Home Assistant is not ready",
  }));

  await assert.rejects(
    client.search("light", ["light"]),
    /Home Assistant is not ready/,
  );
});
