"use strict";

const assert = require("node:assert/strict");
const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

function element(value = "") {
  return {
    value, checked: false, style: {}, children: [], listeners: {},
    classList: { add() {}, remove() {} },
    appendChild(child) { this.children.push(child); },
    querySelector() { return null; },
    addEventListener(name, callback) { this.listeners[name] = callback; },
    setCustomValidity() {},
  };
}

function editor() {
  const document = { createElement: () => element() };
  const registry = { definitions: {}, register(type, definition) { this.definitions[type] = definition; } };
  const ui = { renderButtonSettings() {} };
  const fields = { cardBadgeLabelHtml() {}, cardBadgePreview() {} };
  const robot = loadTypescriptTest("src/webserver/application/config_robot_card_options.ts")
    .createConfigRobotCardOptionsFeature();
  const tabs = loadTypescriptTest("src/webserver/application/config_modal_tab_options.ts")
    .createConfigModalTabOptionsFeature({ document, ...ui });
  // Layout is simulated; real option normalizers and card renderers run below.
  tabs.renderModalTabSettings = () => {};
  loadTypescriptTest("src/webserver/cards/vacuum.ts").registerVacuumCardTypes(registry, robot, fields, ui);
  loadTypescriptTest("src/webserver/cards/lawn_mower.ts").registerLawnMowerCardTypes(registry, robot, fields, ui);
  loadTypescriptTest("src/webserver/cards/fan.ts").registerFanCardTypes(registry, tabs, fields, ui);
  loadTypescriptTest("src/webserver/cards/slider.ts").registerSliderCardTypes(
    registry, tabs, { renderControlTypeField() {} }, fields, { inlineDisclosure: () => element() });
  loadTypescriptTest("src/webserver/cards/wifi_qr.ts").registerWifiQrCardTypes(
    registry, tabs, fields, ui, { supported: () => true });
  const saved = {}, inputs = {}, modes = [];
  function field(value = "") { return { field: element(), input: element(value), select: element(value) }; }
  const helpers = {
    idPrefix: "test-",
    saveField(name, value) { saved[name] = value; },
    renderCardModeSelector(_panel, _button, _helpers, metadata) { modes.push(metadata.mode); return field(); },
    renderCardEntityField: () => field(), renderCardTextField: () => field(),
    renderCardNumberField: () => field("50"), renderCardIconPicker: () => element(),
    renderBasicCardFields() {}, requireField() {}, requireEntityDomain() {},
    disclosureSection: () => ({ panel: element(), section: element() }),
    textField(_label, id, value) { const result = field(value); inputs[id] = result.input; return result; },
    selectField(_label, id, _options, value) { const result = field(value); inputs[id] = result.select; return result; },
    toggleRow(_label, id, checked) { const result = { row: element(), input: element() }; result.input.checked = checked; inputs[id] = result.input; return result; },
  };
  return { registry, document, helpers, saved, inputs, modes };
}

test("reopening robot, fan and cover editors keeps the chosen colour", () => {
  const e = editor();
  const previousDocument = global.document;
  global.document = e.document;
  try {
    for (const [type, sensor] of [
      ["vacuum", "start_stop"], ["lawn_mower", "start_mowing"],
      ...["fan_switch", "fan_speed", "fan_oscillate", "fan_direction", "fan_preset"].map(type => [type, ""]),
      ...["", "tilt", "toggle", "open", "close", "stop", "set_position"].map(mode => ["cover", mode]),
    ]) {
      const button = { type, sensor, entity: "test.entity", icon: "Auto", icon_on: "Auto", options: "old_mode,card_off_color=00BCD4" };
      for (let reopen = 0; reopen < 2; reopen++) {
        e.registry.definitions[type].renderSettings(element(), button, 1, e.helpers);
        assert.equal(button.options, "card_off_color=00BCD4", `${type}/${sensor}`);
      }
    }
  } finally { global.document = previousDocument; }
});

test("robot and fan mode changes retain the colour in saved options", () => {
  for (const [type, from, to] of [["vacuum", "start_stop", "dock"], ["lawn_mower", "start_mowing", "dock"], ["fan_speed", "", "fan_switch"]]) {
    const e = editor();
    const button = { type, sensor: from, options: "card_off_color=6633B9", icon: "Auto", icon_on: "Auto" };
    e.registry.definitions[type].renderSettings(element(), button, 1, e.helpers);
    e.modes[0].onChange.call({ value: to });
    assert.equal(button.options, "card_off_color=6633B9", type);
    assert.equal(e.saved.options, button.options, type);
  }
});

test("Connect-card normalization and credential edits retain colour and Wi-Fi tabs", () => {
  const e = editor();
  const button = { type: "wifi_qr", options: "ssid64=VGVzdA,pass64=cGFzc3dvcmQ,hidden,wifi_tabs=qr,card_off_color=00BCD4" };
  const definition = e.registry.definitions.wifi_qr;
  definition.normalizeConfig(button);
  assert(button.options.includes("card_off_color=00BCD4"));
  definition.renderSettings(element(), button, 1, e.helpers);
  const beforeTabs = button.options.split(",").filter(part => part.startsWith("wifi_tabs="));
  assert.deepEqual(beforeTabs, ["wifi_tabs=qr"]);
  e.inputs["test-wifi-ssid"].value = "Changed network";
  e.inputs["test-wifi-password"].value = "newpassword";
  e.inputs["test-wifi-ssid"].listeners.change();
  assert(button.options.includes("card_off_color=00BCD4"));
  assert(button.options.includes("ssid64=Q2hhbmdlZCBuZXR3b3Jr"));
  assert(button.options.includes("pass64=bmV3cGFzc3dvcmQ"));
  assert.deepEqual(button.options.split(",").filter(part => part.startsWith("wifi_tabs=")), beforeTabs);
  assert.equal(e.saved.options, button.options);
  definition.normalizeConfig(button);
  assert.equal(button.options, e.saved.options);
});
