const test = require("node:test");
const assert = require("node:assert/strict");
const path = require("node:path");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");
const load = name => loadTypeScriptModule(path.resolve(__dirname, "../../../src/webserver/model", name));
const transfer = load("page_card_transfer.ts"), graph = load("panel_config_conversion.ts"), keys = load("page_keys.ts");
const model = load("index.ts");
const contract = loadTypeScriptModule(path.resolve(__dirname, "../../../src/webserver/generated/card_contract.ts"));
const codes = load("card_transfer.ts");
const layout = { slots: 9, columns: 3 };
const source = () => ({ deviceProfile: "panel-a", buttons: { 1: ";Root;;;;;subpage", 2: ";Destination;;;;;subpage", 32: "unknown.future;Hidden" },
  subpages: { 1: "~B=Home:ABCDEF,1,2|subpage,,Child|,light.other,Other",
    [keys.childPageKey(1, 1)]: "~B=Parent%20Page:123456,1|,light.leaf,Leaf,,,,,opaque=keep",
    2: "B=Home%20Page:FACE00,1|switch.destination:Destination", 32: "B,1|unknown.hidden:Inactive" },
  settings: { button_order: "1,2", future: "opaque\0🌡️" } });
const child = (root, slot) => keys.childPageKey(root, slot);
const parsed = raw => model.parseRawSubpageConfig(raw, contract.cardContractSubpageTypeFromCode);
const comparable = value => JSON.parse(JSON.stringify(value));
function valid(document) { assert.deepEqual(comparable(graph.decodePanelPageDocument(graph.encodePanelPageDocument(document))), comparable(document)); }

for (const profile of [{ slots: 6, columns: 2 }, layout, { slots: 15, columns: 3 }, { slots: 20, columns: 4 },
  { slots: 15, columns: 3, retainedSlot: 32 }]) {
  const slot = profile.retainedSlot ?? profile.slots + 1;
  test(`visible retained card ${slot} on ${profile.slots} cells can copy, duplicate, move and delete`, () => {
    const document = source();
    const tokens = Array(slot).fill("");
    tokens[0] = ",light.hidden,Inactive";
    tokens[slot - 1] = "subpage,,Retained";
    document.subpages[1] = [`~B=Return:ABCDEF,${slot}`, ...tokens].join("|");
    const leaf = "~B=Parent:123456,1|,light.retained_leaf,Leaf";
    document.subpages[child(1, slot)] = leaf;
    const before = JSON.stringify(document);
    const clipboard = transfer.capturePageCards(document, 1, [slot], profile);
    assert.equal(clipboard.entries[0].button.label, "Retained");
    const copied = transfer.planPageCardPaste(document, 2, 2, clipboard.entries, profile);
    assert.equal(copied.document.subpages[child(2, copied.slots[0])], leaf);
    const duplicated = transfer.planPageCardPaste(document, 1, 2, clipboard.entries, profile);
    assert.equal(duplicated.document.subpages[child(1, duplicated.slots[0])], leaf);
    assert.equal(duplicated.document.subpages[child(1, slot)], leaf);
    const moved = transfer.planPageCardPaste(document, 2, 2, clipboard.entries, profile, clipboard.origin);
    assert.equal(moved.document.subpages[child(1, slot)], undefined);
    assert.equal(moved.document.subpages[child(2, moved.slots[0])], leaf);
    const deleted = transfer.planPageCardDelete(document, clipboard.origin, profile);
    assert.equal(deleted.subpages[child(1, slot)], undefined);
    assert.equal(parsed(deleted.subpages[1]).buttons[slot - 1].label, "");
    for (const result of [copied.document, duplicated.document, moved.document, deleted]) {
      assert.equal(result.subpages[1].split("|")[1], tokens[0], "inactive saved sibling is untouched");
      assert.equal(parsed(result.subpages[1]).backLabel, "Return");
      assert.equal(parsed(result.subpages[1]).backColor, "ABCDEF");
      valid(result);
    }
    assert.equal(JSON.stringify(document), before);
    assert.throws(() => transfer.capturePageCards(document, 1, [1], profile), /no longer on this page/);
    for (const invalid of [0, 1.5, 33])
      assert.throws(() => transfer.capturePageCards(document, 1, [invalid], profile), /Invalid card slot/);
  });
}

for (const profile of [{ slots: 6, columns: 2 }, layout, { slots: 15, columns: 3 }, { slots: 20, columns: 4 }]) {
  test(`folder copy preserves descendants and inactive data on ${profile.slots} slots`, () => {
    const document = source(), before = JSON.stringify(document);
    const clipboard = transfer.capturePageCards(document, 0, [1], profile);
    const result = transfer.planPageCardPaste(document, 0, 2, clipboard.entries, profile);
    assert.deepEqual(result.slots, [3]);
    assert.equal(result.document.subpages[3], document.subpages[1]);
    assert.equal(result.document.subpages[child(3, 1)], document.subpages[child(1, 1)]);
    assert.equal(result.document.buttons[32], document.buttons[32]);
    assert.equal(result.document.subpages[32], document.subpages[32]);
    assert.deepEqual(result.document.settings, { ...document.settings, button_order: "1,2,3" });
    assert.equal(JSON.stringify(document), before);
    valid(result.document);
  });
}
test("nested folder copy to a different parent remaps its saved page", () => {
  const document = source(), clipboard = transfer.capturePageCards(document, 1, [1], layout);
  const result = transfer.planPageCardPaste(document, 2, 2, clipboard.entries, layout);
  assert.deepEqual(result.slots, [2]);
  assert.equal(result.document.subpages[child(2, 2)], document.subpages[child(1, 1)]);
  assert.equal(result.document.subpages[child(1, 1)], document.subpages[child(1, 1)]);
  assert.equal(parsed(result.document.subpages[2]).buttons[1].label, "Child");
  assert.equal(parsed(result.document.subpages[2]).backLabel, "Home Page");
  assert.equal(parsed(result.document.subpages[2]).backColor, "FACE00");
  valid(result.document);
});
test("cut moves both the owner and whole tree in one immutable candidate", () => {
  const document = source(), before = JSON.stringify(document);
  const clipboard = transfer.capturePageCards(document, 1, [1], layout);
  const result = transfer.planPageCardPaste(document, 2, 2, clipboard.entries, layout, clipboard.origin);
  assert.equal(result.document.subpages[child(1, 1)], undefined);
  assert.equal(result.document.subpages[child(2, 2)], document.subpages[child(1, 1)]);
  assert.equal(parsed(result.document.subpages[1]).buttons[0].type, "");
  assert.equal(result.document.subpages[1].split("|")[2], document.subpages[1].split("|")[2]);
  assert.equal(JSON.stringify(document), before);
  valid(result.document);
});
test("moving multiple cards on the same page can reuse their freed slots", () => {
  const document = source(), clipboard = transfer.capturePageCards(document, 1, [1, 2], layout);
  const result = transfer.planPageCardPaste(document, 1, 5, clipboard.entries, layout, clipboard.origin);
  assert.deepEqual(result.slots, [1, 2]);
  assert.equal(result.document.subpages[child(1, 1)], document.subpages[child(1, 1)]);
  assert.deepEqual(parsed(result.document.subpages[1]).order, ["B", "", "", "", "", "1", "2"]);
  valid(result.document);
});
test("multi-delete removes complete trees and leaves every unrelated saved record intact", () => {
  const document = source(), clipboard = transfer.capturePageCards(document, 0, [1, 2], layout);
  const result = transfer.planPageCardDelete(document, clipboard.origin, layout);
  assert.deepEqual(Object.keys(result.subpages), ["32"]);
  assert.equal(result.subpages[32], document.subpages[32]);
  assert.equal(result.settings.future, document.settings.future);
  assert.equal(result.buttons[32], document.buttons[32]);
  assert.equal(result.settings.button_order, "");
  valid(result);
});
test("saved data below a former folder travels and deletes with its owner", () => {
  const document = source();
  document.buttons[1] = "light.now;Former folder";
  const clipboard = transfer.capturePageCards(document, 0, [1], layout);
  const copied = transfer.planPageCardPaste(document, 0, 2, clipboard.entries, layout).document;
  assert.equal(copied.subpages[3], document.subpages[1]);
  assert.equal(copied.subpages[child(3, 1)], document.subpages[child(1, 1)]);
  const deleted = transfer.planPageCardDelete(document, clipboard.origin, layout);
  assert.equal(deleted.subpages[1], undefined);
  assert.equal(deleted.subpages[child(1, 1)], undefined);
  valid(copied); valid(deleted);
});
test("copying or moving a full two-level tree into a folder rejects without changing the source", () => {
  const document = source(), before = JSON.stringify(document);
  const clipboard = transfer.capturePageCards(document, 0, [1], layout);
  for (const origin of [undefined, clipboard.origin])
    assert.throws(() => transfer.planPageCardPaste(document, 2, 2, clipboard.entries, layout, origin), /two levels/);
  assert.equal(JSON.stringify(document), before);
});
test("empty folder targets still cannot create a third level", () => {
  const document = source(); document.subpages[child(1, 1)] = "B";
  document.subpages[2] = "~B,1|subpage,,Empty";
  const clipboard = transfer.capturePageCards(document, 2, [1], layout);
  assert.throws(() => transfer.planPageCardPaste(document, child(1, 1), 1, clipboard.entries, layout), /two levels/);
});
test("a move cannot target the source folder or one of its descendants", () => {
  const document = source(), clipboard = transfer.capturePageCards(document, 0, [1], layout);
  for (const target of [1, child(1, 1)])
    assert.throws(() => transfer.planPageCardPaste(document, target, 2, clipboard.entries, layout, clipboard.origin), /inside itself/);
});
test("changed source cards, layout or descendants reject a cut instead of deleting new data", () => {
  const original = source(), clipboard = transfer.capturePageCards(original, 0, [1], layout);
  for (const change of [document => { document.buttons[1] = ";Changed;;;;;subpage"; },
    document => { document.settings.button_order = "2,1"; },
    document => { document.subpages[child(1, 1)] += "|,light.new,New"; }]) {
    const current = source(); change(current); const before = JSON.stringify(current);
    assert.throws(() => transfer.planPageCardPaste(current, 0, 2, clipboard.entries, layout, clipboard.origin), transfer.PageTransferConflict);
    assert.throws(() => transfer.planPageCardDelete(current, clipboard.origin, layout), transfer.PageTransferConflict);
    assert.equal(JSON.stringify(current), before);
  }
});
test("unrelated concurrent settings are retained by a valid move", () => {
  const document = source(), clipboard = transfer.capturePageCards(document, 0, [1], layout);
  const current = { ...document, settings: { ...document.settings, new_setting: "concurrent" } };
  const result = transfer.planPageCardPaste(current, 0, 3, clipboard.entries, layout, clipboard.origin);
  assert.equal(result.document.settings.new_setting, "concurrent");
  valid(result.document);
});
test("a capacity failure cannot paste only part of a batch or overwrite invisible saved cards", () => {
  const document = source(), clipboard = transfer.capturePageCards(document, 1, [1, 2], layout);
  for (let slot = 3; slot <= 32; ++slot) document.buttons[slot] = `light.hidden_${slot};Hidden`;
  const before = JSON.stringify(document);
  assert.throws(() => transfer.planPageCardPaste(document, 0, 2, clipboard.entries, layout), /without replacing saved cards/);
  assert.equal(JSON.stringify(document), before);
});
test("edited page preserves untouched raw sibling tokens and inactive order slots", () => {
  const document = source(); document.subpages[2] = "~B=Home%20Page:FACE00,1,,,,,,,,20|,light.destination,Destination";
  const clipboard = transfer.capturePageCards(document, 1, [2], layout);
  const result = transfer.planPageCardPaste(document, 2, 2, clipboard.entries, layout);
  assert.equal(result.document.subpages[2].split("|")[1], document.subpages[2].split("|")[1]);
  assert.equal(result.document.subpages[2].split("|")[0].split(",")[9], "20");
  assert.equal(parsed(result.document.subpages[2]).backColor, "FACE00");
  valid(result.document);
});
for (const profile of [{ slots: 6, columns: 2 }, layout, { slots: 15, columns: 3 }, { slots: 20, columns: 4 }]) {
  for (const explicitBack of [false, true]) {
    test(`delete preserves hidden order after ${explicitBack ? "explicit" : "implicit"} Back on ${profile.slots} cells`, () => {
      const document = source();
      const records = Array.from({ length: profile.slots + 2 }, (_, i) => `,light.kept_${i + 1},Record ${i + 1}`);
      const order = records.map((_, i) => String(i + 1));
      if (explicitBack) order.unshift("B=Return:ABCDEF");
      const boundary = profile.slots - (explicitBack ? 1 : 0);
      records[boundary - 1] = "subpage,,Hidden folder";
      const hiddenTree = child(1, boundary);
      document.subpages[hiddenTree] = "~B,1|,light.hidden_leaf,Leaf";
      document.subpages[1] = ["~" + order.join(","), ...records].join("|");
      const before = JSON.stringify(document);
      const captured = transfer.capturePageCards(document, 1, [1], profile);
      const deleted = transfer.planPageCardDelete(document, captured.origin, profile);
      const actual = deleted.subpages[1].split("|");
      const hidden = order.slice(profile.slots - (explicitBack ? 0 : 1));
      assert.deepEqual(actual[0].split(",").slice(profile.slots), hidden);
      assert.deepEqual(actual.slice(2), records.slice(1), "unselected saved records stay byte-identical");
      assert.equal(deleted.subpages[hiddenTree], document.subpages[hiddenTree]);
      if (explicitBack) {
        assert.equal(parsed(deleted.subpages[1]).backLabel, "Return");
        assert.equal(parsed(deleted.subpages[1]).backColor, "ABCDEF");
      }
      valid(deleted);
      assert.equal(JSON.stringify(document), before);
    });
  }
  test(`Home refuses copy and cut when its ${profile.slots} renderable identities are occupied`, () => {
    const document = source();
    for (let slot = 3; slot <= profile.slots; ++slot) document.buttons[slot] = `light.saved_${slot};Saved ${slot}`;
    document.settings.button_order = "1";
    const clipboard = transfer.capturePageCards(document, 1, [2], profile);
    const before = JSON.stringify(document);
    for (const move of [undefined, clipboard.origin]) {
      assert.throws(() => transfer.planPageCardPaste(document, 0, 2, clipboard.entries, profile, move), /without replacing saved cards/);
      assert.equal(JSON.stringify(document), before, "rejected Home paste keeps the complete source and dormant data");
    }
  });
  test(`paste allocates a saved identity above ${profile.slots} visible cells without overwriting dormant data`, () => {
    const document = source();
    const records = Array.from({ length: profile.slots }, (_, i) => `,light.retained_${i + 1},Retained ${i + 1}`);
    document.subpages[2] = ["~B=Return:ABCDEF,1", ...records].join("|");
    // The next identity is empty but still owns saved descendant data.
    const dormant = child(2, profile.slots + 1);
    document.subpages[dormant] = "~B,1|,light.dormant,Keep";
    const before = JSON.stringify(document);
    const clipboard = transfer.capturePageCards(document, 1, [2], profile);
    const pasted = transfer.planPageCardPaste(document, 2, 2, clipboard.entries, profile);
    assert.deepEqual(pasted.slots, [profile.slots + 2]);
    assert.deepEqual(pasted.document.subpages[2].split("|").slice(1, profile.slots + 1), records);
    assert.equal(pasted.document.subpages[dormant], document.subpages[dormant]);
    assert.equal(parsed(pasted.document.subpages[2]).buttons[profile.slots + 1].label, "Other");
    assert.equal(parsed(pasted.document.subpages[2]).backLabel, "Return");
    valid(pasted.document);
    const duplicate = transfer.capturePageCards(pasted.document, 2, pasted.slots, profile);
    const duplicated = transfer.planPageCardPaste(pasted.document, 2, 3, duplicate.entries, profile);
    assert.deepEqual(duplicated.slots, [profile.slots + 3]);
    valid(duplicated.document);
    assert.equal(JSON.stringify(document), before);
  });
}

test("flat card codes stay version one while nested trees require version two", () => {
  const document = source(), captured = transfer.capturePageCards(document, 0, [1], layout).entries[0];
  const flat = { ...model.emptyCardConfig(), entity: "light.flat", label: "Flat", size: 1 };
  assert.equal(JSON.parse(codes.createCardTransferCode({ device: "panel-a", firmware: "test" }, [flat])).version, 1);
  const entry = { ...captured.button, size: captured.size, page_tree: captured.subtree };
  const code = codes.createCardTransferCode({ device: "panel-a", firmware: "test" }, [entry]);
  const envelope = codes.parseCardTransferCode(code);
  assert.equal(envelope.version, 2);
  assert.deepEqual(comparable(envelope.cards[0].page_tree), comparable(captured.subtree));
  assert.throws(() => codes.normalizeCardTransferEnvelope({ ...envelope, version: 1 }), /nested-page format/);
  assert.throws(() => codes.normalizeCardTransferEnvelope({ ...envelope, version: 3 }), /newer version/);
});
test("nested card codes reject orphaned, overdeep and oversized page trees", () => {
  const document = source(), captured = transfer.capturePageCards(document, 0, [1], layout).entries[0];
  const envelope = { format: "espcontrol.cards", version: 2, source: { device: "panel-a", firmware: "test" },
    cards: [{ ...captured.button, size: 1, page_tree: captured.subtree }] };
  for (const tree of [
    { configuration: null, children: { 1: "B" } },
    { configuration: "B", children: { "01": "B" } },
    { configuration: "B", children: { 33: "B" } },
    { configuration: "~B,1|G,,Child", children: { 1: "~B,1|G,,Grandchild" } },
    { configuration: "B".repeat(4096), children: {} },
  ]) assert.throws(() => codes.normalizeCardTransferEnvelope({ ...envelope, cards: [{ ...envelope.cards[0], page_tree: tree }] }), /Invalid card code/);
  const mixed = { ...envelope.cards[0], subpage: { order: [], back_label: "Back", buttons: [] } };
  assert.throws(() => codes.normalizeCardTransferEnvelope({ ...envelope, cards: [mixed] }), /nested-page format/);
});
