"use strict";
const assert = require("node:assert/strict");
const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { createCardRegistry } = loadTypescriptTest("src/webserver/application/card_registry.ts");
const { registerBatteryCardTypes } = loadTypescriptTest("src/webserver/cards/battery.ts");
const { cardTypePickerOptions } = loadTypescriptTest("src/webserver/features/preview.ts");
const { normalizeSavedConfigStatic } = loadTypescriptTest("src/webserver/generated/saved_config_static.ts");
const { cardContractSubpageTypeCode, cardContractSubpageTypeFromCode } = loadTypescriptTest("src/webserver/generated/card_contract.ts");

test("Battery remains available on both surfaces and information-only displays", () => {
  const registry = createCardRegistry();
  registerBatteryCardTypes(registry, { cardBadgeLabelHtml: (_, text) => text });
  const card = registry.definitions.battery;
  for (const isSub of [false, true]) {
    const choices = cardTypePickerOptions(registry.definitions, [], true, isSub, "battery");
    assert(choices.some(choice => choice.key === "battery" && !choice.disabled));
  }
  const button = { entity: "sensor.phone_battery", label: "Old", icon: "Fan", type: "sensor", sensor: "local", unit: "V", precision: "2", options: "large_numbers=1" };
  card.onSelect(button);
  assert.equal(button.entity, "sensor.phone_battery");
  assert.equal(button.label, "");
  assert.equal(button.type, "battery");
  assert.equal(button.options, "");
  let field;
  card.renderSettings({}, button, 1, { renderCardEntityField: (_, b, __, metadata) => { field = { b, metadata }; } });
  assert.equal(field.b, button);
  assert.deepEqual(field.metadata.entity.domains(), ["sensor"]);
  assert.equal(field.metadata.entity.bindName, "entity");
  assert.equal(card.renderPreview(button, {}).labelHtml, "80%");
  assert(card.renderPreview(button, {}).iconHtml.includes("mdi-battery-80"));
});

test("Battery normalization clears unrelated fields without reusing the fan compact code", () => {
  const config = {entity: "sensor.phone_battery", label: "Old", icon: "Fan", icon_on: "Fan", type: "battery", sensor: "old", unit: "V", precision: "2", options: "large_numbers=1"};
  assert.equal(normalizeSavedConfigStatic(config), true);
  assert.equal(config.entity, "sensor.phone_battery");
  for (const key of ["label", "sensor", "unit", "precision", "options"]) assert.equal(config[key], "");
  assert.equal(config.icon, "Auto");
  assert.equal(config.icon_on, "Auto");
  assert.equal(cardContractSubpageTypeCode("battery"), "BT");
  assert.equal(cardContractSubpageTypeFromCode("BT"), "battery");
  assert.equal(cardContractSubpageTypeFromCode("B"), "fan_switch");
});
