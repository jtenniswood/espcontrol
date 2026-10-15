const assert = require("node:assert/strict");
const { test } = require("node:test");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module.js");
const model = loadTypeScriptModule("src/webserver/model/clock_bar.ts");
const { placeClockBarItems } = loadTypeScriptModule("src/webserver/model/clock_bar_layout.ts");
function item(type = "temperature", group = "left", existing = []) { return model.createClockBarElement(type, group, existing); }

test("clock bar round trips duplicate sensor types and preserves empty layouts", () => {
  const first = item(); first.entity = "sensor.room_temperature";
  const second = item("temperature", "right", [first]); second.source = "local"; second.entity = "room_temp";
  const value = { version: 1, items: [first, second] };
  assert.deepEqual(model.parseClockBarLayout(JSON.stringify(value)), value);
  assert.deepEqual(model.parseClockBarLayout('{"version":1,"items":[]}'), { version: 1, items: [] });
  assert.equal(model.parseClockBarLayout(""), null);
});
test("clock bar rejects unsupported versions, duplicate IDs, unknown types and invalid formatting", () => {
  const first = item();
  for (const value of ["{", { version: 2, items: [] }, { version: 1, items: [first, first] },
    ...[{ type: "toString" }, { group: "bottom" }, { source: "other" }, { precision: 9 }, { label: "x".repeat(33) }].map(change => ({ version: 1, items: [{ ...first, ...change }] }))]) {
    assert.throws(() => model.parseClockBarLayout(value));
  }
});
test("moving elements inserts within and between groups without losing duplicate types", () => {
  const a = item(), b = item("humidity", "left", [a]), c = item("network", "right", [a, b]);
  let layout = model.moveClockBarElement({ version: 1, items: [a, b, c] }, b.id, "right", c.id);
  assert.deepEqual(layout.items.filter(i => i.group === "right").map(i => i.id), [b.id, c.id]);
  layout = model.moveClockBarElement(layout, c.id, "right", b.id);
  assert.deepEqual(layout.items.filter(i => i.group === "right").map(i => i.id), [c.id, b.id]);
  assert.equal(layout.items.length, 3);
});
test("middle is centred in the remaining gap between unequal outer groups", () => {
  const result = placeClockBarItems([{ id: "a", group: "left", width: 100 }, { id: "b", group: "middle", width: 60 }, { id: "c", group: "right", width: 40 }], 400, 8);
  assert.equal(result.items.find(i => i.id === "b").x, 200);
});
test("overflow drops middle then inner left and right elements; title displaces left", () => {
  const input = [{ id: "a", group: "left", width: 70 }, { id: "b", group: "middle", width: 60 }, { id: "c", group: "right", width: 40 }];
  assert.deepEqual(placeClockBarItems(input, 120, 8).items.map(i => i.id), ["a", "c"]);
  const titled = placeClockBarItems(input, 120, 8, 110);
  assert.deepEqual(titled.items.map(i => i.id), ["c"]);
  assert.equal(titled.titleWidth, 72);
  assert.equal(placeClockBarItems(input, 20, 8).items.length, 0);
});
test("layout never overlaps or crosses the bar across device sizes and title lengths", () => {
  for (const width of [0, 20, 80, 240, 468, 788, 1012, 1188]) {
    for (const title of [0, 40, 300, 1500]) {
      const input = Array.from({ length: 12 }, (_, i) => ({ id: String(i), group: ["left", "middle", "right"][i % 3], width: 24 + i * 9 }));
      const result = placeClockBarItems(input, width, 6, title);
      const ordered = result.items.toSorted((a, b) => a.x - b.x);
      let end = result.titleWidth;
      for (const item of ordered) { assert(item.x >= end, JSON.stringify({ width, title, result })); assert(item.x + item.width <= width); end = item.x + item.width; }
    }
  }
});

test("backups retain editable and deliberately empty clock bars and reject malformed layouts", () => {
  const { normalizeBackupPanelSettings } = loadTypeScriptModule("src/webserver/model/settings.ts");
  const layout = { version: 1, items: [item("humidity")] };
  const current = { clockFormatOptions: ["12h", "24h"], clockFormat: "24h", updateFrequencyOptions: ["Daily"], updateFrequency: "Daily", screenRotationOptions: ["0"], ntpDefaults: ["pool.ntp.org", "time.nist.gov", "time.google.com"] };
  assert.deepEqual(normalizeBackupPanelSettings({ clock_bar_layout: layout }, current).clockBarLayout, layout);
  assert.deepEqual(normalizeBackupPanelSettings({ clock_bar_layout: { version: 1, items: [] } }, current).clockBarLayout, { version: 1, items: [] });
  assert.equal(normalizeBackupPanelSettings({}, current).clockBarLayout, null);
  assert.throws(() => normalizeBackupPanelSettings({ clock_bar_layout: { version: 2, items: [] } }, current));
  const many = [];
  for (let i = 0; i < 13; i++) many.push(item("temperature", "left", many));
  assert.throws(() => model.parseClockBarLayout({ version: 1, items: many }));
});

test("clock bar discovery only requests the endpoint when firmware advertises it", async () => {
  const { loadEditableClockBarLayout } = loadTypeScriptModule("src/webserver/application/clock_bar_post_api.ts");
  const urls = [];
  const result = await loadEditableClockBarLayout(async url => { urls.push(url); return new Response('{"api":{"version":1}}'); });
  assert.equal(result, undefined);
  assert.deepEqual(urls, ["/api/v1/capabilities"]);
});
test("clock bar discovery retries startup responses and reads the saved layout", async () => {
  const { loadEditableClockBarLayout } = loadTypeScriptModule("src/webserver/application/clock_bar_post_api.ts");
  let calls = 0; const delays = [];
  const responses = [new Response("Starting", { status: 503 }), new Response('{"clock_bar":{"version":1}}'), new Response("Starting", { status: 503 }), new Response('{"clock_bar":{"version":1}}'), new Response('{"version":1,"items":[]}')];
  const result = await loadEditableClockBarLayout(async () => responses[calls++], async ms => { delays.push(ms); });
  assert.deepEqual(result, { version: 1, items: [] });
  assert.deepEqual(delays, [2000, 2000]);
});
test("clock bar discovery stops after bounded startup retries and does not retry auth failures", async () => {
  const { loadEditableClockBarLayout } = loadTypeScriptModule("src/webserver/application/clock_bar_post_api.ts");
  let calls = 0;
  assert.equal(await loadEditableClockBarLayout(async () => { calls++; return new Response("Starting", { status: 503 }); }, async () => {}), undefined);
  assert.equal(calls, 4);
  calls = 0;
  assert.equal(await loadEditableClockBarLayout(async () => { calls++; return new Response("Authenticate", { status: 401 }); }), undefined);
  assert.equal(calls, 1);
});
