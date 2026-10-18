"use strict";

const assert = require("node:assert/strict");
const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

function element(value = "") {
  return {
    value, disabled: false, listeners: {},
    classList: { add() {}, toggle() {} },
    appendChild() {}, querySelector() { return null; },
    addEventListener(name, callback) { this.listeners[name] = callback; },
  };
}

function editor(relays) {
  const profile = { features: { internalRelays: relays } };
  const options = loadTypescriptTest("src/webserver/application/config_internal_relay_options.ts")
    .createConfigInternalRelayOptionsFeature(profile);
  let definition, selected, choices, validate;
  const writes = [];
  const helpers = {
    idPrefix: "test-", saveField(name, value) { writes.push([name, value]); },
    disclosureSection() { return { panel: element(), section: element() }; },
    selectField(_label, _id, entries, value) { choices = entries; selected = element(value); return { field: element(), select: selected }; },
    requireField(_input, _message, _active, hasValue) { validate = hasValue; },
    renderCardSegmentControl() { return { buttons: { switch: element(), push: element() } }; },
    renderCardTextField() {}, renderCardIconPicker() { return element(); },
  };
  loadTypescriptTest("src/webserver/cards/internal.ts").registerInternalCardTypes(
    { register(_key, value) { definition = value; } }, options,
    { createElement: () => element() }, { condField: () => element(), cardBadgeLabelHtml() {} },
  );
  return {
    profile, definition, writes,
    render(button) {
      definition.renderSettingsBeforeLabel(element(), button, 1, helpers);
      definition.renderSettings(element(), button, 1, helpers);
    },
    get selected() { return selected; }, get choices() { return choices; },
    valid() { return validate(selected.value); },
  };
}

test("opening an unavailable saved relay never retargets its circuit", () => {
  const e = editor([{ key: "relay_1", label: "Heating" }]);
  const button = { type: "internal", entity: "relay_2", sensor: "", icon: "Auto", icon_on: "Auto", label: "Lights" };
  e.render(button);
  assert.equal(button.entity, "relay_2");
  assert.equal(e.selected.value, "relay_2");
  assert.match(e.choices[0].label, /Unavailable relay.*relay_2/);
  assert.equal(e.valid(), false);
  button.label = "New label";
  e.render(button);
  assert.equal(button.entity, "relay_2");
  assert.equal(e.writes.some(([name]) => name === "entity"), false);
  e.selected.value = "relay_1";
  e.selected.listeners.change.call(e.selected);
  assert.equal(button.entity, "relay_1");
  assert.equal(e.valid(), true);
  assert.deepEqual(e.writes.at(-1), ["entity", "relay_1"]);
});

test("new cards default once; empty or changing catalogues preserve existing keys", () => {
  const e = editor([{ key: "relay_1", label: "Relay 1" }]);
  const fresh = {};
  e.definition.onSelect(fresh);
  assert.equal(fresh.entity, "relay_1");
  e.profile.features.internalRelays = [];
  e.render(fresh);
  assert.equal(fresh.entity, "relay_1");
  assert.equal(e.selected.disabled, true);
  assert.equal(e.valid(), false);
  e.profile.features.internalRelays = [{ key: "relay_1", label: "Returned relay" }];
  e.render(fresh);
  assert.equal(fresh.entity, "relay_1");
  assert.equal(e.valid(), true);
  const blank = { type: "internal", entity: "" };
  e.render(blank);
  assert.equal(blank.entity, "");
  assert.equal(e.valid(), false);
});
