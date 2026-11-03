const test = require("node:test");
const assert = require("node:assert/strict");
const path = require("node:path");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");
const load = name => loadTypeScriptModule(path.resolve(__dirname, "../../../src/webserver", name));
const editor = load("features/page_editor.ts"), model = load("model/index.ts"), keys = load("model/page_keys.ts");
const child = keys.childPageKey(2, 1);
const source = () => ({ deviceProfile: "panel-a", buttons: { 2: ";Root;;;;;subpage", 32: ";Retained;;;;;subpage" },
  subpages: { 2: "~B,1|subpage,,Child", [child]: "~B=Parent%20Page:ABCDEF,1|,light.leaf,Leaf",
    32: "~B,1|subpage,,Hidden", [keys.childPageKey(32, 1)]: "~B,1|,light.hidden,Hidden" },
  settings: { button_order: "2", future: "opaque\0🌡️" } });
const codec = {
  parseButtonConfig: model.parseRawButtonConfig,
  parseSubpageConfig: value => model.parseRawSubpageConfig(value, code => code),
  buildSubpageGrid: page => Object.assign(page, model.buildSubpageGrid(page, 9, 3)),
};
test("editor snapshot preserves both levels, inactive data and immediate owner titles", () => {
  const input = source(), before = JSON.stringify(input);
  const document = editor.preparePageEditorDocument(input, 9, codec);
  assert.equal(document.buttons.length, 9);
  assert.equal(document.nested, true);
  assert.equal(editor.editorPageOwner(2, document.buttons, document.subpages).label, "Root");
  assert.equal(editor.editorPageOwner(child, document.buttons, document.subpages).label, "Child");
  assert.equal(document.subpages[child].backLabel, "Parent Page");
  assert.equal(document.subpages[keys.childPageKey(32, 1)].buttons[0].entity, "light.hidden");
  assert.equal(editor.restoredEditorPage(child, document), child);
  document.buttons[1].type = "";
  assert.equal(editor.restoredEditorPage(child, document), null);
  document.source.settings.future = "changed copy";
  assert.equal(JSON.stringify(input), before);
});
test("a rejected snapshot has no partially applied document", () => {
  const input = source(), before = JSON.stringify(input);
  let parsed = 0, published;
  assert.throws(() => { published = editor.preparePageEditorDocument(input, 9, {
    ...codec, parseSubpageConfig: value => { if (++parsed === 2) throw new Error("bad page"); return codec.parseSubpageConfig(value); },
  }); });
  assert.equal(published, undefined);
  assert.equal(JSON.stringify(input), before);
});
const deferred = () => { let resolve, reject; const promise = new Promise((yes, no) => { resolve = yes; reject = no; }); return { promise, resolve, reject }; };
function loader(overrides = {}) {
  const applied = [], failures = [], reads = [];
  let revision = 0, editable = true;
  const instance = new editor.NativePageEditorLoader({
    discover: async () => true, readDepth: () => 2, revision: () => revision,
    read: signal => { const item = {...deferred(),signal}; reads.push(item); return item.promise; },
    canApply: () => editable, apply: document => applied.push(document), failed: error => failures.push(error), ...overrides,
  });
  return { instance, applied, failures, reads, edit() { ++revision; }, dirty() { editable = false; } };
}
const settle = async () => { await Promise.resolve(); await Promise.resolve(); };
const snapshot = document => ({ document, etag: '"v2-2"', version: 2 });
test("legacy connections keep their entity-based loading", async () => {
  for (const overrides of [{ discover: async () => "legacy-fallback" }, { readDepth: () => 1 }]) {
    const fixture = loader(overrides);
    assert.equal(await fixture.instance.refresh(), false);
    assert.equal(fixture.reads.length, 0);
  }
});
test("coalesced graph loading applies one complete snapshot", async () => {
  const fixture = loader(), first = fixture.instance.refresh();
  assert.equal(fixture.instance.refresh(), first);
  await settle(); fixture.reads[0].resolve(snapshot(source()));
  assert.equal(await first, true);
  assert.equal(fixture.applied.length, 1);
});
test("configuration events during a pending read coalesce one fresh follow-up", async () => {
  const fixture = loader(), first = fixture.instance.refresh(); await settle();
  fixture.instance.refreshAfterChange(); fixture.instance.refreshAfterChange();
  fixture.reads[0].resolve(snapshot(source()));
  assert.equal(await first, true); await settle();
  assert.equal(fixture.reads.length, 2);
  const latest = {...source(), settings: {marker: "latest"}};
  fixture.reads[1].resolve(snapshot(latest)); await settle();
  assert.equal(fixture.applied.at(-1).settings.marker, "latest");
  assert.equal(fixture.reads.length, 2);
});
test("disconnect cancels a queued configuration refresh", async () => {
  const fixture = loader(), first = fixture.instance.refresh(); await settle();
  fixture.instance.refreshAfterChange(); fixture.instance.invalidate();
  fixture.reads[0].resolve(snapshot(source()));
  assert.equal(await first, false); await settle();
  assert.equal(fixture.reads.length, 1);
});
test("edits and dirty drafts prevent a late snapshot from changing the editor", async () => {
  for (const action of ["edit", "dirty"]) {
    const fixture = loader(), pending = fixture.instance.refresh();
    await settle(); fixture[action](); fixture.reads[0].resolve(snapshot(source()));
    assert.equal(await pending, false);
    assert.equal(fixture.applied.length, 0);
  }
});
test("disconnect and reconnect discard superseded graph responses", async () => {
  const fixture = loader(), old = fixture.instance.refresh(); await settle();
  fixture.instance.invalidate();
  assert.equal(fixture.reads[0].signal.aborted, true, "disconnect cancels the obsolete HTTP read");
  const current = fixture.instance.refresh(); await settle();
  fixture.reads[0].resolve(snapshot({ ...source(), settings: { marker: "old" } }));
  assert.equal(await old, false);
  assert.equal(fixture.instance.refresh(), current);
  fixture.reads[1].resolve(snapshot(source()));
  assert.equal(await current, true);
  assert.equal(fixture.applied.length, 1);
});
test("a superseded load failure cannot replace the current connection banner", async () => {
  const fixture = loader(), old = fixture.instance.refresh(); await settle();
  fixture.instance.invalidate(); fixture.reads[0].reject(new Error("old connection"));
  assert.equal(await old, false);
  assert.equal(fixture.failures.length, 0);
});
