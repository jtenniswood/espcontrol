"use strict";
const { describe, test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { convertNativeV1ToV2: up, convertNativeV2ToV1: down } = loadTypescriptTest("src/webserver/model/panel_config_conversion.ts");
const v1 = loadTypescriptTest("src/webserver/model/panel_config.ts");
const v2 = loadTypescriptTest("src/webserver/model/panel_config_v2.ts");
const contract = loadTypescriptTest("src/webserver/generated/card_contract.ts");
const ROOT = path.resolve(__dirname, "../../..");
const manifest = JSON.parse(fs.readFileSync(path.join(ROOT, "devices/manifest.json"), "utf8"));
const error = (fn) => assert.throws(fn, (e) => e.name === "PanelConfigError");
const compact = (fields) => "~" + fields.map((s) => s.replace(/[%,;|:]/g, (ch) => "%" + ch.charCodeAt(0).toString(16).toUpperCase())).join(",");
const root = (type = "subpage") => ["", "Folder", "", "", "", "", type].join(";");
function doc(overrides = {}) { return { deviceProfile: "panel-a", buttons: {}, subpages: {}, settings: {}, ...overrides }; }
function roundtrip(source) {
  const bytes = up(v1.encodePanelConfig(source));
  const decoded = v1.decodePanelConfig(down(bytes));
  assert.deepEqual({ ...decoded, settings: { ...decoded.settings } }, { ...source, settings: { ...source.settings } });
  return v2.decodeNativeV2(bytes);
}
function view(rows) {
  const find = (kind, id) => rows.find((r) => r.kind === kind && r.id === id);
  const text = (id) => id ? find(2, id).text : "";
  return { text, fields: (card) => card.fields.map((id, i) => text((i === 0 || i === 4) && id ? find(3, id).first : id)) };
}
function mutate(input, change) { const rows = v2.decodeNativeV2(input); change(rows); return v2.encodeNativeV2(rows); }

describe("Staged native v1/v2 conversion", () => {
  test("preserves missing and empty records, inactive pages and implicit folder targets", () => {
    for (const value of ["", "~", "|", "~|", "1, B=Home%3Aroom:ABCDEF", "~1, B=Home%3Aroom:ABCDEF|", "   "]) {
      const source = doc({ buttons: { 1: root(), 2: "", 3: "switch.fan;Fan", 4: root() }, subpages: { 1: value, 3: value, 32: value }, settings: { button_order: "4,3d,,1,2" } });
      const rows = roundtrip(source), get = view(rows);
      const missingOwner = rows.find((r) => r.kind === 5 && r.second === 32 && r.first === 0);
      assert.equal(missingOwner.flags, 2);
      assert.equal(missingOwner.fieldMask, 0);
      const missingPage = rows.find((r) => r.kind === 4 && r.second === 4);
      assert.equal(missingPage.flags, 2);
      assert.equal(rows.filter((r) => r.kind === 6).length, 2);
      assert.equal(get.text(rows.find((r) => r.kind === 4 && r.id === 0).third), "4,3d,,1,2");
      const inactive = rows.find((r) => r.kind === 4 && r.second === 3);
      assert.ok(inactive);
      assert.ok(!rows.some((r) => r.kind === 6 && r.id === 3));
    }
    roundtrip(doc());
    roundtrip(doc({ buttons: { 1: "" }, subpages: { 1: "" }, settings: { button_order: "" } }));
    roundtrip(doc({ buttons: { 1: root() } }));
  });

  test("retains exact imported spellings and resolved fields without family normalization", () => {
    const source = doc({
      buttons: { 1: compact(["light.kitchen", "Kitchen; West", "", "", "sensor.room", "°C", "subpage", "2", "future=%G1"]), 2: "raw;label;;;;;future_family;;opaque;unknown-extra" },
      subpages: { 1: "~B=Home%7CUp:ABCDEF,1w|HC,climate.room,Room,,,sensor.temp,°C,1,unknown=%FF%2C|__proto__,entity.future,%EF%BB%BFLabel,,,mode,unit,0,opaque|", 2: "B,1d|sensor.room:Room:::sensor.other:%:sensor:1:opaque:unknown-extra" },
      settings: Object.assign(Object.create(null), { button_order: "2d,1", future: "\ufeffA\0😀", __proto__: "" }),
    });
    Object.defineProperty(source.settings, "__proto__", { value: "preserved", enumerable: true });
    const rows = roundtrip(source), get = view(rows);
    const children = rows.filter((r) => r.kind === 5 && r.first !== 0);
    assert.deepEqual(get.fields(children[0]), ["climate.room", "Room", "Auto", "Auto", "sensor.temp", "°C", "climate_control", "1", "unknown=%FF%2C"]);
    assert.equal(get.fields(children[1])[6], "__proto__");
    assert.equal(get.fields(children[1])[1], "\ufeffLabel");
    assert.equal(get.text(children[1].rawSource).split(",")[0], "__proto__");
    assert.ok(rows.filter((r) => r.kind === 5).every((r) => r.flags === 1));
  });

  test("round-trips every card code and unknown import across all declared profiles", () => {
    const types = Object.entries(contract.CARD_CONTRACT_SUBPAGE_TYPE_CODES);
    types.push(["unknown_future", "unknown_future"], ["__proto__", "__proto__"], ["constructor", "constructor"]);
    for (const [profile, definition] of Object.entries(manifest.devices)) {
      const buttons = {}, subpages = {};
      for (let slot = 1; slot <= definition.slots; slot++) {
        buttons[slot] = root();
        const tokens = [];
        for (let child = 1; child < definition.slots; child++) {
          const [type, code] = types[(slot + child) % types.length];
          tokens.push([code, `sensor.${slot}_${child}`, "Label%3A%7C%F0%9F%98%80", "", "", "sensor.shared", "%", "text", "future=x"].join(","));
        }
        subpages[slot] = "~B=Return:ABCDEF," + Array.from({ length: definition.slots - 1 }, (_, i) => i + 1).join(",") + "|" + tokens.join("|");
      }
      const source = doc({ deviceProfile: profile, buttons, subpages, settings: { button_order: Object.keys(buttons).join(","), future: "kept" } });
      const rows = roundtrip(source);
      assert.equal(rows.filter((r) => r.kind === 4).length, definition.slots + 1);
    }
    // Every code is exercised independently even on smaller profile matrices.
    for (const [type, code] of types) {
      const rows = roundtrip(doc({ buttons: { 1: root() }, subpages: { 1: "~B,1|" + code + ",sensor.one" } }));
      assert.equal(view(rows).fields(rows.find((r) => r.kind === 5 && r.first !== 0))[6], type);
    }
  });

  test("keeps byte boundaries and opaque imported fields without clipping", () => {
    roundtrip(doc({ buttons: { 32: "x".repeat(4095) }, subpages: { 32: "B|" + "x".repeat(4093) }, settings: { x: "y".repeat(4094) } }));
    roundtrip(doc({ buttons: { 1: "\ufeff" + "é".repeat(2046) }, settings: { "\ufeffname": "\ufeffvalue\0😀" } }));
    const fixture = JSON.parse(fs.readFileSync(path.join(ROOT, "compatibility/fixtures/panel_config_v1.json"), "utf8"));
    roundtrip(fixture.document);
    const migration = JSON.parse(fs.readFileSync(path.join(ROOT, "compatibility/fixtures/panel_config_migration_v1.json"), "utf8"));
    for (const scenario of Object.values(migration.scenarios)) {
      if (scenario.native_document) roundtrip(scenario.native_document);
      if (scenario.document) roundtrip(scenario.document);
      if (scenario.expected_document) roundtrip(scenario.expected_document);
    }
  });

  test("preserves 512 deterministic irregular legacy documents without normalizing their text", () => {
    let state = 1825;
    const next = () => (state = (Math.imul(state, 1664525) + 1013904223) >>> 0);
    const alphabet = ["a", ";", ":", ",", "|", "%", "~", "\0", "é", "😀", "\ufeff", " "];
    const random = () => Array.from({ length: next() % 60 }, () => alphabet[next() % alphabet.length]).join("");
    for (let i = 0; i < 512; ++i) {
      const buttons = { 1: i % 2 ? root() : random(), 2: random(), 32: random() };
      const subpages = { 1: random(), 2: random(), 31: random() };
      roundtrip(doc({ buttons, subpages, settings: { button_order: random(), future: random() } }));
    }
  });

  test("fails closed on stale raw sources and provenance before returning a projection", () => {
    const input = up(v1.encodePanelConfig(doc({ buttons: { 1: root() }, subpages: { 1: "B,1|sensor.one:One" }, settings: { button_order: "1" } })));
    for (const edit of [
      (rows) => { rows.find((r) => r.kind === 5 && r.first !== 0).rawSource = 0; },
      (rows) => { rows.find((r) => r.kind === 5 && r.first === 0).rawSource = 0; },
      (rows) => { rows.find((r) => r.kind === 4 && r.id === 0).third = 0; },
      (rows) => { rows.find((r) => r.kind === 5 && r.first !== 0).second = 2; },
      (rows) => { rows.find((r) => r.kind === 5 && r.first === 0).second = 33; },
      (rows) => { const p = rows.find((r) => r.kind === 4 && r.id !== 0); p.flags = 2; p.third = 0; },
    ]) error(() => down(mutate(input, edit)));
    const missing = up(v1.encodePanelConfig(doc({ buttons: { 1: root() } })));
    error(() => down(mutate(missing, (rows) => { rows.splice(rows.findIndex((r) => r.kind === 4 && r.id), 1); rows.splice(rows.findIndex((r) => r.kind === 6), 1); })));
  });

  test("projects new saved fields with safe spelling and rejects depth-two data", () => {
    const input = up(v1.encodePanelConfig(doc({ buttons: { 1: root() }, subpages: { 1: "B,1|sensor.one:One" } })));
    const rows = v2.decodeNativeV2(input), child = rows.find((r) => r.kind === 5 && r.first !== 0);
    child.flags = 0; child.rawSource = 0;
    const label = rows.find((r) => r.kind === 2 && r.text === "One"); label.text = "Unicode 😀; : | , %";
    const projected = v1.decodePanelConfig(down(v2.encodeNativeV2(rows)));
    assert.ok(projected.subpages[1].startsWith("~"));
    const imported = v2.decodeNativeV2(up(v1.encodePanelConfig(projected)));
    assert.equal(view(imported).fields(imported.find((r) => r.kind === 5 && r.first !== 0))[1], label.text);
    const golden = JSON.parse(fs.readFileSync(path.join(ROOT, "compatibility/fixtures/panel_config_v2.json"), "utf8"));
    // Structural validity cannot authorize a stale raw hint.
    error(() => down(mutate(Buffer.from(golden.wire_hex, "hex"), (rows) => { rows.find((r) => r.kind === 5 && r.flags === 1).rawSource = 0; })));
    const deep = v2.decodeNativeV2(up(v1.encodePanelConfig(doc({ buttons: { 1: root() }, subpages: { 1: "B,1|::::::subpage" } }))));
    const nestedOwner = deep.find((r) => r.kind === 5 && r.first !== 0);
    deep.push({ kind: 4, id: 999, first: nestedOwner.first, second: nestedOwner.id });
    deep.push({ kind: 6, id: nestedOwner.id, first: 999 });
    error(() => down(v2.encodeNativeV2(deep)));
  });

  test("chooses a lossless spelling for separator-like new type names and long root values", () => {
    const input = up(v1.encodePanelConfig(doc({ buttons: { 1: root() }, subpages: { 1: "~B,1|S,sensor.one" } })));
    const rows = v2.decodeNativeV2(input), child = rows.find((r) => r.kind === 5 && r.first !== 0);
    child.flags = 0; child.rawSource = 0;
    rows.find((r) => r.kind === 2 && r.id === child.fields[6]).text = "future,kind";
    const reimported = v2.decodeNativeV2(up(down(v2.encodeNativeV2(rows))));
    assert.equal(view(reimported).fields(reimported.find((r) => r.kind === 5 && r.first !== 0))[6], "future,kind");
    const long = v2.decodeNativeV2(up(v1.encodePanelConfig(doc({ buttons: { 1: ":".repeat(4095) } }))));
    const card = long.find((r) => r.kind === 5); card.flags = 0; card.rawSource = 0;
    const projected = v1.decodePanelConfig(down(v2.encodeNativeV2(long)));
    assert.equal(projected.buttons[1].split(";")[0], ":".repeat(4095));
    const childLong = v2.decodeNativeV2(up(v1.encodePanelConfig(doc({ buttons: { 1: root() }, subpages: { 1: "B|" + "x".repeat(4093) } }))));
    const childCard = childLong.find((r) => r.kind === 5 && r.first !== 0); childCard.flags = 0; childCard.rawSource = 0;
    assert.equal(v1.decodePanelConfig(down(v2.encodeNativeV2(childLong))).subpages[1], "B|" + "x".repeat(4093));
  });
});
