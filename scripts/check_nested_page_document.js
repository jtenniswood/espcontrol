"use strict";
const assert = require("node:assert/strict");
const path = require("node:path");
const { loadTypeScriptModule } = require("./load_typescript_module");
const load = (name) => loadTypeScriptModule(path.join(__dirname, "../src/webserver/model", name));
const v1 = load("panel_config.ts"), v2 = load("panel_config_v2.ts");
const graph = load("panel_config_conversion.ts"), keys = load("page_keys.ts");
const tree = load("page_subtree.ts");
const K = v2.NativeV2Kind;
const document = () => ({ deviceProfile: "panel-a", buttons: {}, subpages: {}, settings: {} });
let cases = 0;
const paths = new Set();
for (let root = 1; root <= 32; ++root) {
  assert.equal(keys.childPageKey(0, root), root);
  assert.equal(keys.parentPageKey(root), 0);
  assert.equal(keys.pageOwnerSlot(root), root);
  for (let slot = 1; slot <= 32; ++slot) {
    const key = keys.childPageKey(root, slot);
    assert.equal(keys.pageDepth(key), 2);
    assert.equal(keys.parentPageKey(key), root);
    assert.equal(keys.pageOwnerSlot(key), slot);
    assert(!paths.has(key)); paths.add(key);
    assert.throws(() => keys.childPageKey(key, 1), /two levels/);
    ++cases;
  }
}
for (const bad of [-1, 1.5, NaN, Infinity, 1057]) assert.throws(() => keys.pageDepth(bad));
for (const bad of [0, -1, 33, 1.5, NaN]) assert.throws(() => keys.childPageKey(0, bad));

const flat = document();
flat.buttons = { 2: ";Rooms;;;;;subpage", 32: "switch.hidden;Hidden" };
flat.subpages = { 2: "~B=Home:123456,1|,light.one,Leaf", 31: "B,1|switch.retained:Hidden" };
flat.settings = { button_order: "2,32", future: "kept\u0000😀" };
const flatBytes = v1.encodePanelConfig(flat);
assert.deepEqual(graph.encodePanelPageDocument(flat), graph.convertNativeV1ToV2(flatBytes));
assert.deepEqual(v1.encodePanelConfig(graph.decodePanelPageDocument(graph.encodePanelPageDocument(flat))), flatBytes);
++cases;

const nested = document(), child = keys.childPageKey(2, 1);
nested.buttons = { 2: ";Rooms;;;;;subpage", 32: "switch.hidden;Hidden" };
nested.subpages = {
  2: "~B=Home:123456,1|subpage,,Child",
  [child]: "~B=Parent%20Page:ABCDEF,1|,light.nested,Nested%20Leaf,,,,,future=value",
};
nested.settings = { button_order: "2,32", future: "kept\u0000😀" };
const snapshot = JSON.stringify(nested), bytes = graph.encodePanelPageDocument(nested);
const decoded = graph.decodePanelPageDocument(bytes);
assert.deepEqual(JSON.parse(JSON.stringify(decoded)), nested);
assert.deepEqual(graph.encodePanelPageDocument(decoded), bytes);
assert.equal(JSON.stringify(nested), snapshot);
assert.throws(() => v1.encodePanelConfig(decoded), /slot/);
assert.throws(() => graph.convertNativeV2ToV1(bytes), /nested pages/);
const rows = v2.decodeNativeV2(bytes), pages = rows.filter((r) => r.kind === K.Page);
assert.equal(pages.length, 3);
assert.equal(pages[2].first, pages[1].id);
const owner = rows.find((r) => r.kind === K.Card && r.id === pages[2].second);
assert.equal(owner.first, pages[1].id);
assert.equal(rows.find((r) => r.kind === K.Navigation && r.id === owner.id).first, pages[2].id);
++cases;

// Missing saved page data still creates an explicit, empty folder target.
const empty = { ...nested, subpages: { 2: nested.subpages[2] } };
const emptyRows = v2.decodeNativeV2(graph.encodePanelPageDocument(empty));
assert.equal(emptyRows.filter((r) => r.kind === K.Page).length, 3);
assert.equal(graph.decodePanelPageDocument(graph.encodePanelPageDocument(empty)).subpages[child], "");
++cases;

// Inactive root records retain the complete subtree without flattening.
const retained = { ...nested, buttons: { 32: nested.buttons[2] }, subpages: {
  32: nested.subpages[2], [keys.childPageKey(32, 1)]: nested.subpages[child],
} };
assert.deepEqual(JSON.parse(JSON.stringify(graph.decodePanelPageDocument(graph.encodePanelPageDocument(retained)))), retained);
++cases;

for (const subpages of [
  { ...nested.subpages, [child]: "~B,1|subpage,,Too%20Deep" },
  { ...nested.subpages, [keys.childPageKey(3, 1)]: "~B" },
  { ...nested.subpages, 1057: "~B" },
  { ...nested.subpages, "01": "~B" },
  { ...nested.subpages, ...JSON.parse('{"__proto__":"~B"}') },
]) {
  assert.throws(() => graph.encodePanelPageDocument({ ...nested, subpages }));
  assert.equal(JSON.stringify(nested), snapshot);
  ++cases;
}
const payload = tree.capturePanelPageSubtree(nested, 2);
assert.equal(payload.configuration, nested.subpages[2]);
assert.deepEqual({ ...payload.children }, { 1: nested.subpages[child] });
const destination = { ...nested, buttons: { ...nested.buttons, 3: nested.buttons[2] } };
const copied = tree.replacePanelPageSubtree(destination, 3, payload);
assert.equal(copied.subpages[3], nested.subpages[2]);
assert.equal(copied.subpages[keys.childPageKey(3, 1)], nested.subpages[child]);
assert.equal(copied.subpages[child], nested.subpages[child]);
assert.equal(JSON.stringify(nested), snapshot);
assert.deepEqual(JSON.parse(JSON.stringify(graph.decodePanelPageDocument(graph.encodePanelPageDocument(copied)))), copied);
++cases;
const movedOwner = { ...destination, buttons: { ...destination.buttons, 2: "" } };
const moved = tree.replacePanelPageSubtree(movedOwner, 3, payload, 2);
assert(!Object.hasOwn(moved.subpages, 2) && !Object.hasOwn(moved.subpages, child));
assert.equal(moved.subpages[keys.childPageKey(3, 1)], nested.subpages[child]);
++cases;
const leaf = tree.capturePanelPageSubtree(nested, child);
const leafMoveOwner = { ...destination, subpages: { ...destination.subpages, 2: "~B,1|,light.replacement,Replacement" } };
const leafMoved = tree.replacePanelPageSubtree(leafMoveOwner, 3, leaf, child);
assert(!Object.hasOwn(leafMoved.subpages, child));
assert.equal(leafMoved.subpages[3], nested.subpages[child]);
++cases;
for (const operation of [
  () => tree.replacePanelPageSubtree(destination, keys.childPageKey(3, 1), payload),
  () => tree.replacePanelPageSubtree(destination, child, payload, 2),
  () => tree.replacePanelPageSubtree(destination, child, { configuration: "~B,1|subpage,,Third", children: {} }),
  () => tree.replacePanelPageSubtree(destination, 3, { configuration: null, children: { 1: "~B" } }),
  () => tree.replacePanelPageSubtree(destination, 0, payload),
]) {
  const before = JSON.stringify(destination);
  assert.throws(operation);
  assert.equal(JSON.stringify(destination), before);
  ++cases;
}
console.log(`PASS nested page paths, editable graph round trips, inactive data, explicit targets, depth/legacy/orphan guards: ${cases} cases`);
