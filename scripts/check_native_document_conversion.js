"use strict";
// Cross-language projection contract, including independent v1 source values.
const assert = require("node:assert/strict");
const fs = require("node:fs"), path = require("node:path"), { spawnSync } = require("node:child_process");
const { loadTypeScriptModule } = require("./load_typescript_module");
const ROOT = path.resolve(__dirname, ".."), cli = process.argv[2] || path.join(ROOT, "build/tests/firmware/panel_config_projection_cli");
const importCli = process.argv[3] || path.join(ROOT, "build/tests/firmware/panel_config_import_cli");
const load = (name) => loadTypeScriptModule(path.join(ROOT, "src/webserver/model", name));
const v1 = load("panel_config.ts"), v2 = load("panel_config_v2.ts"), bridge = load("panel_config_conversion.ts");
const codes = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/card_contract.ts")).CARD_CONTRACT_SUBPAGE_TYPE_CODES;
const doc = (overrides = {}) => ({ deviceProfile: "panel-a", buttons: {}, subpages: {}, settings: {}, ...overrides });
const folder = ";Folder;;;;;subpage";
const cases = [];
for (const raw of ["", "~", "|", "~|", "B=Home%7CUp:ABCDEF,1w|sensor.one:Room:::sensor.other:°C:sensor:1:extra:future", "~B,1|HC,climate.room,%EF%BB%BFRoom,,,sensor.other,°C,1,opaque=%FF%2C|"]) {
  cases.push(doc({ buttons: { 1: folder, 2: "", 3: "switch.fan;Fan", 4: folder }, subpages: { 1: raw, 3: raw, 32: raw }, settings: { button_order: "4,3d,,1,2", future: "\ufeffA\0😀" } }));
}
for (const [type, code] of [...Object.entries(codes), ["__proto__", "__proto__"], ["constructor", "constructor"], ["future_family", "future_family"]]) {
  cases.push(doc({ buttons: { 1: folder }, subpages: { 1: "~B,1|" + code + ",sensor.one,Label%3B%3A%7C" }, settings: { type } }));
}
const profiles = JSON.parse(fs.readFileSync(path.join(ROOT, "devices/manifest.json"), "utf8")).devices;
const budgetCases = [];
for (const [deviceProfile, limits] of Object.entries(profiles)) {
  const buttons = {}, subpages = {};
  for (let slot = 1; slot <= limits.slots; ++slot) {
    buttons[slot] = folder;
    subpages[slot] = "~B=Return:ABCDEF," + Array.from({ length: limits.slots - 1 }, (_, i) => i + 1).join(",") + "|" +
      Array.from({ length: limits.slots - 1 }, (_, i) => "S,sensor." + slot + "_" + i + ",%EF%BB%BFLabel,,,sensor.shared,°C,1,unknown=x").join("|");
  }
  const source = { deviceProfile, buttons, subpages, settings: { button_order: Object.keys(buttons).join(","), future: "kept" } };
  cases.push(source); budgetCases.push(source);
}
const distinct = doc({ deviceProfile: "guition-esp32-p4-jc8012p4a1-v3" });
for (let slot = 1; slot <= 20; ++slot) {
  distinct.buttons[slot] = folder;
  distinct.subpages[slot] = "~B," + Array.from({ length: 19 }, (_, i) => i + 1).join(",") + "|" +
    Array.from({ length: 19 }, (_, child) => ["S", ...Array.from({ length: 8 }, (_, field) => (`f${field}_s${slot}_c${child}`).padEnd(18, "x"))].join(",")).join("|");
}
cases.push(distinct);
cases.push(doc({ buttons: { 1: ":".repeat(4095) }, subpages: { 32: "B|" + "x".repeat(4093) }, settings: { x: "y".repeat(4094) } }));
cases.push(doc({ buttons: { 1: folder }, subpages: { 1: "B|" + "x".repeat(4093) } }));
cases.push(doc({ settings: JSON.parse('{"__proto__":"kept","constructor":"value","\\ufeffname":"\\ufeffvalue"}') }));
cases.push(doc({ settings: { "\uE000": "private", "😀": "astral", "\ufeffname": "\ufeffvalue" } }));
cases.push(doc({ buttons: { 1: "^Ur8>KPrf[;One", 2: "BJNQ*lxYGn;Two", 3: "sensor.a\0sensor.b;NUL" } }));
function fnv(value) { let h = 2166136261; for (const byte of Buffer.from(value)) h = Math.imul(h ^ byte, 16777619) >>> 0; return h; }
assert.equal(fnv("^Ur8>KPrf["), 145478724); assert.equal(fnv("BJNQ*lxYGn"), 145478724);
let seed = 1825;
const next = () => (seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0);
const alphabet = ["a", ";", ":", ",", "|", "%", "~", "\0", "é", "😀", "\ufeff", " "];
const random = () => Array.from({ length: next() % 60 }, () => alphabet[next() % alphabet.length]).join("");
for (let i = 0; i < 512; ++i) cases.push(doc({ buttons: { 1: i % 2 ? folder : random(), 2: random(), 32: random() }, subpages: { 1: random(), 2: random(), 31: random() }, settings: { button_order: random(), future: random() } }));
const fixture = JSON.parse(fs.readFileSync(path.join(ROOT, "compatibility/fixtures/panel_config_v1.json"), "utf8")); cases.push(fixture.document);
const migrations = JSON.parse(fs.readFileSync(path.join(ROOT, "compatibility/fixtures/panel_config_migration_v1.json"), "utf8"));
for (const scenario of Object.values(migrations.scenarios)) for (const key of ["native_document", "document", "expected_document"]) if (scenario[key]) cases.push(scenario[key]);
function cpp(input, mode) {
  return spawnSync(cli, mode ? [mode] : [], { input, cwd: ROOT, maxBuffer: 2 * 1024 * 1024 });
}
let successes = 0, failures = 0;
function compare(input, expected) {
  let browser;
  try { browser = bridge.convertNativeV2ToV1(input); } catch (error) {
    if (expected) throw error;
    assert.equal(error.name, "PanelConfigError");
    const result = cpp(input); assert.equal(result.status, 2, result.stderr.toString()); ++failures; return;
  }
  const result = cpp(input); assert.equal(result.status, 0, result.stderr.toString());
  const actual = v1.decodePanelConfig(result.stdout);
  assert.deepEqual(actual, v1.decodePanelConfig(browser));
  if (expected) assert.deepEqual(actual, v1.decodePanelConfig(v1.encodePanelConfig(expected)));
  ++successes;
}
function semanticGraph(input) {
  const rows = v2.decodeNativeV2(input), index = new Map(rows.map((r) => [r.kind + ":" + r.id, r]));
  const get = (kind, id) => index.get(kind + ":" + id), text = (id) => id ? get(2, id).text : "";
  const strings = rows.filter((r) => r.kind === 2).map((r) => r.text);
  const entities = rows.filter((r) => r.kind === 3).map((r) => text(r.first));
  assert.equal(strings.length, new Set(strings).size, "import must intern equal string values");
  assert.equal(entities.length, new Set(entities).size, "import must intern equal entity tokens");
  return {
    profile: get(1, 0).text,
    settings: Object.fromEntries(rows.filter((r) => r.kind === 7).map((r) => [text(r.id), text(r.first)])),
    pages: rows.filter((r) => r.kind === 4).map((page) => ({
      ownerSlot: page.id ? get(5, page.second).second : 0, order: text(page.third), flags: page.flags,
      navigable: page.id === 0 || !!get(6, page.second),
      cards: rows.filter((r) => r.kind === 5 && r.first === page.id).sort((a, b) => a.second - b.second).map((card) => ({
        slot: card.second, flags: card.flags, raw: text(card.rawSource),
        fields: card.fields.map((id, i) => text((i === 0 || i === 4) && id ? get(3, id).first : id)),
      })),
    })).sort((a, b) => a.ownerSlot - b.ownerSlot),
  };
}
let imports = 0;
for (const source of cases) {
  const original = v1.encodePanelConfig(source), browser = bridge.convertNativeV1ToV2(original);
  const result = spawnSync(importCli, [], { input: original, cwd: ROOT, maxBuffer: 16 * 1024 * 1024 });
  assert.equal(result.status, 0, result.stderr.toString());
  assert.deepEqual(semanticGraph(result.stdout), semanticGraph(browser));
  compare(browser, source); compare(result.stdout, source); ++imports;
}
const basic = bridge.convertNativeV1ToV2(v1.encodePanelConfig(doc({ buttons: { 1: folder }, subpages: { 1: "~B,1|S,sensor.one,One" }, settings: { button_order: "1" } })));
for (const value of ["future,kind", "G", "future|kind", "future:kind", "__proto__", "constructor", "", "sensor"]) {
  const rows = v2.decodeNativeV2(basic), child = rows.find((r) => r.kind === 5 && r.first !== 0);
  child.flags = 0; child.rawSource = 0;
  rows.find((r) => r.kind === 2 && r.id === child.fields[6]).text = value || "sensor";
  if (!value) { child.fields[6] = 0; child.fieldMask &= ~(1 << 6); }
  compare(v2.encodeNativeV2(rows));
}
for (const source of cases.slice(0, 6).concat(cases.filter((d) => Object.values(d.buttons).some((v) => v.length > 4000)))) {
  const rows = v2.decodeNativeV2(bridge.convertNativeV1ToV2(v1.encodePanelConfig(source)));
  for (const row of rows) if (row.kind === 5 && row.flags === 1) { row.flags = 0; row.rawSource = 0; }
  compare(v2.encodeNativeV2(rows));
}
for (const edit of [
  (rows) => { rows.find((r) => r.kind === 5 && r.first !== 0).rawSource = 0; },
  (rows) => { rows.find((r) => r.kind === 5 && r.first === 0).rawSource = 0; },
  (rows) => { rows.find((r) => r.kind === 4 && r.id === 0).third = 0; },
  (rows) => { rows.find((r) => r.kind === 5 && r.first !== 0).second = 2; },
  (rows) => { rows.find((r) => r.kind === 5 && r.first === 0).second = 33; },
  (rows) => { const p = rows.find((r) => r.kind === 4 && r.id); p.flags = 2; p.third = 0; },
]) { const rows = v2.decodeNativeV2(basic); edit(rows); compare(v2.encodeNativeV2(rows)); }
const deep = v2.decodeNativeV2(bridge.convertNativeV1ToV2(v1.encodePanelConfig(doc({ buttons: { 1: folder }, subpages: { 1: "B,1|::::::subpage" } }))));
const child = deep.find((r) => r.kind === 5 && r.first !== 0);
deep.push({ kind: 4, id: 999, first: child.first, second: child.id }, { kind: 6, id: child.id, first: 999 }); compare(v2.encodeNativeV2(deep));
const orphan = v2.decodeNativeV2(basic); orphan.push({ kind: 5, id: 65000, first: 0, second: 2, flags: 2 }); compare(v2.encodeNativeV2(orphan));
for (const mode of ["short", "unaligned", "small-output", "input-output-alias", "input-workspace-alias", "output-workspace-alias", "result-input-alias", "result-output-alias", "result-workspace-alias", "null-result"]) {
  const result = cpp(basic, mode); assert.equal(result.status, 2, mode + ": " + result.stderr.toString()); ++failures;
}
assert.ok(successes >= cases.length && failures >= 18);
const basicV1 = v1.encodePanelConfig(doc({ buttons: { 1: folder }, subpages: { 1: "~B,1|S,sensor.one,One" }, settings: { button_order: "1" } }));
let importFailures = 0;
for (const mode of ["short", "table-full", "unaligned", "small-output", "input-output-alias", "input-workspace-alias", "output-workspace-alias", "result-input-alias", "result-output-alias", "result-workspace-alias", "null-result", "null-bounds", "bounds-input-alias"]) {
  const result = spawnSync(importCli, [mode], { input: basicV1, cwd: ROOT });
  assert.equal(result.status, 2, mode + ": " + result.stderr.toString()); ++importFailures;
}
const manyEmpty = v1.encodePanelConfig(doc({ buttons: { 1: folder }, subpages: { 1: "B" + "|".repeat(100) } }));
const shortFinish = spawnSync(importCli, ["finish-short"], { input: manyEmpty, cwd: ROOT });
assert.equal(shortFinish.status, 2, shortFinish.stderr.toString()); ++importFailures;
for (const source of budgetCases) {
  const result = spawnSync(importCli, ["budget"], { input: v1.encodePanelConfig(source), cwd: ROOT, maxBuffer: 16 * 1024 * 1024 });
  assert.equal(result.status, 0, result.stderr.toString());
  const browser = bridge.convertNativeV1ToV2(v1.encodePanelConfig(source));
  assert.deepEqual(semanticGraph(result.stdout), semanticGraph(browser));
}
const constrained = spawnSync(importCli, ["budget"], { input: v1.encodePanelConfig(distinct), cwd: ROOT });
assert.equal(constrained.status, 2, constrained.stderr.toString());
assert.equal(constrained.stderr.toString().trim(), "4", "a small fixed budget must fail rather than allocate");
importFailures++;
console.log(`Native conversion parity: ${imports} C++/browser imports, ${successes} successful projections, ${failures} projection rejections and ${importFailures} import bounds/alias failures. C++ allocation guards enabled; live integration remains disabled.`);
