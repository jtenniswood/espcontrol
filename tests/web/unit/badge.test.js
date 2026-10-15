"use strict";
const { test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { BADGE_SUBTYPES, badgeAction, badgeSubtype, setBadgeSubtype, normalizeBadgeConfig } = loadTypescriptTest("src/webserver/model/badge.ts");
const { configOptionValue } = loadTypescriptTest("src/webserver/model/config_primitives.ts");
const { serializeBadgeRow, parseBadgeRow, badgeRowSettings, badgeRowsFromSettings } = loadTypescriptTest("src/webserver/model/badge_row.ts");
const { encodePanelConfig, decodePanelConfig } = loadTypescriptTest("src/webserver/model/panel_config.ts");
const { NativePanelConfigController } = loadTypescriptTest("src/webserver/controllers/native_panel_config_controller.ts");

function badge(options) {
  return { entity: "climate.kitchen", sensor: "sensor.kitchen", label: "Kitchen", icon: "Home", icon_on: "Auto", type: "badge", precision: "value", unit: "°C", options };
}

test("old subpage badge data remains intact for future compatibility", () => {
  const button = badge("badge_action=subpage,badge_page=6,card_off_color=112233");
  normalizeBadgeConfig(button);
  assert.equal(button.sensor, "sensor.kitchen");
  assert.equal(button.entity, "climate.kitchen");
  button.label = "Renamed room";
  normalizeBadgeConfig(button);
  assert.equal(configOptionValue(button.options, "badge_page"), "6");
  assert.equal(configOptionValue(button.options, "card_off_color"), "112233");
});

test("badge subtypes select the correct behaviour without a Label type", () => {
  assert.deepEqual(BADGE_SUBTYPES.map(type => type.key), ["sensor", "switch", "button", "slider", "modal"]);
  for (const type of BADGE_SUBTYPES) {
    const button = badge("");
    setBadgeSubtype(button, type.key);
    assert.equal(badgeSubtype(button), type.key);
    const restored = parseBadgeRow(serializeBadgeRow([button]))[0];
    assert.equal(badgeSubtype(restored), type.key);
    assert.equal(restored.sensor, type.key === "slider" ? "" : "sensor.kitchen");
  }
  const legacy = badge("badge_action=toggle");
  assert.equal(badgeSubtype(legacy), "switch");
  const staticLegacy = badge(""); staticLegacy.precision = "label";
  normalizeBadgeConfig(staticLegacy);
  assert.equal(staticLegacy.precision, "label", "Existing labels remain readable until edited");
  const momentary = badge("active_color");
  setBadgeSubtype(momentary, "button");
  assert(!momentary.options.includes("active_color"), "Momentary buttons do not retain switch colouring");
});

test("inactive subpage destinations remain stored and unknown actions fall back safely", () => {
  const highestPage = badge("badge_action=subpage,badge_page=32");
  normalizeBadgeConfig(highestPage);
  assert.equal(configOptionValue(highestPage.options, "badge_page"), "32");
  for (const page of ["0", "33", "-1", "6oops", "6.1"]) {
    const button = badge("badge_action=subpage,badge_page=" + page);
    normalizeBadgeConfig(button);
    assert.equal(configOptionValue(button.options, "badge_page"), page);
  }
  const button = badge("badge_action=erase,badge_page=6,unknown=anything");
  normalizeBadgeConfig(button);
  assert.equal(badgeAction(button), "display");
  assert.equal(button.options, "");
});

test("changing away from navigation removes the old destination while preserving value formatting", () => {
  const button = badge("badge_action=modal,badge_page=6,badge_attribute=current_temperature,badge_decimals=2,active_color");
  normalizeBadgeConfig(button);
  assert.equal(configOptionValue(button.options, "badge_page"), "");
  assert.equal(configOptionValue(button.options, "badge_attribute"), "current_temperature");
  assert.equal(configOptionValue(button.options, "badge_decimals"), "2");
  assert.equal(badgeAction(button), "modal");
});

test("existing slider badges preserve label/value choices and discard value formatting overrides", () => {
  const button = badge("badge_action=slider,badge_page=6,badge_attribute=current_temperature,badge_decimals=2,active_color,card_off_color=112233");
  button.entity = "number.kitchen_target";
  button.precision = "label";
  normalizeBadgeConfig(button);
  assert.equal(button.precision, "label");
  assert.equal(button.sensor, "");
  assert.equal(button.unit, "");
  assert.equal(button.entity, "number.kitchen_target");
  assert.equal(button.icon, "Home");
  assert.equal(button.options, "badge_action=slider,active_color,card_off_color=112233");
  assert.deepEqual(parseBadgeRow(serializeBadgeRow([button]))[0], button);
  button.precision = "value";
  normalizeBadgeConfig(button);
  assert.equal(parseBadgeRow(serializeBadgeRow([button]))[0].precision, "value");
});

test("badge rows round-trip Unicode and delimiters independently of card slots", () => {
  const source = badge("badge_action=subpage,badge_page=6,badge_attribute=current_temperature");
  source.label = "Kitchen | 21°C, 50% : now";
  source.unit = "°C";
  const rows = { 0: [source], 6: [badge("badge_action=toggle")] };
  const document = { deviceProfile: "panel", buttons: { 1: "light.kitchen;Kitchen" }, subpages: { 6: "B" }, settings: { button_order: "1,6", ...badgeRowSettings(rows) } };
  const decoded = decodePanelConfig(encodePanelConfig(document));
  assert.deepEqual(decoded.buttons, document.buttons);
  assert.equal(decoded.settings.button_order, "1,6");
  const restored = badgeRowsFromSettings(decoded.settings);
  assert.equal(restored[0][0].label, source.label);
  assert.equal(restored[0][0].sensor, source.sensor);
  assert.equal(restored[6][0].entity, source.entity);
  assert.equal(parseBadgeRow(serializeBadgeRow([])).length, 0);
});

test("badge row limits prevent oversized saved rows and reject non-badge entries", () => {
  assert.equal(parseBadgeRow("~|S,sensor.temperature,Temperature").length, 0);
  assert.throws(() => serializeBadgeRow(Array(9).fill(badge(""))), /eight badges/);
  const large = badge(""); large.label = "温".repeat(1000);
  assert.throws(() => serializeBadgeRow([large]), /too large/);
});

test("deleting or replacing a subpage clears its row in the same save while renaming preserves it", async () => {
  let document = { deviceProfile: "panel", buttons: { 1: ";Room;Home;Auto;;;subpage" }, subpages: {}, settings: { button_order: "1", badge_row_0: serializeBadgeRow([badge("")]), badge_row_1: serializeBadgeRow([badge("")]) } };
  const cleared = [];
  const controller = new NativePanelConfigController({
    fetch: async (path, request) => {
      if (request?.method === "PUT") document = decodePanelConfig(request.body);
      return { ok: true, status: 200, headers: { get: () => '"1"' },
        json: async () => ({ configuration: { read: true, write: true, document_versions: [1], badge_rows: true } }),
        arrayBuffer: async () => encodePanelConfig(document).buffer };
    },
    deviceProfile: () => "panel", slotCount: () => 2,
    entityName: key => key, entityNameForSlot: (key, slot) => key + "_" + slot,
    normalizeHexColor: value => value, showBanner() {}, delay: callback => callback(),
    onBadgeRowRemoved: slot => cleared.push(slot),
  });
  assert.equal(await controller.writeText("button_config_1", ";New name;Home;Auto;;;subpage"), "saved");
  assert(document.settings.badge_row_1);
  assert.equal(cleared.length, 0);
  assert.equal(await controller.writeButtonAndOrder(1, "light.kitchen;Kitchen", "1,2"), "saved");
  assert(!document.settings.badge_row_1);
  assert(document.settings.badge_row_0);
  assert.equal(document.settings.button_order, "1,2");
  assert.deepEqual(cleared, [1]);
});

test("card configurations survive badge conversion, including dormant Subpage data", () => {
  const { badgeCardType, badgeToCard, updateBadgeFromCard } = loadTypescriptTest("src/webserver/model/badge_card.ts");
  const { cardContractCardKeys, cardContractDefaultConfig } = loadTypescriptTest("src/webserver/generated/card_contract.ts");
  for (const type of cardContractCardKeys().filter(type => type !== "badge")) {
    const card = { ...cardContractDefaultConfig(type), entity: "light.kitchen", sensor: "sensor.kitchen",
      label: "Kitchen | 温度", icon: "Lightbulb Outline", icon_on: "Lightbulb", precision: "custom",
      options: "active_color,card_off_color=112233,native_payload=YWJjLGQ=" };
    const wrapped = badge("");
    updateBadgeFromCard(wrapped, card);
    normalizeBadgeConfig(wrapped);
    const restored = parseBadgeRow(serializeBadgeRow([wrapped]))[0];
    assert.equal(badgeCardType(restored), card.type);
    assert.deepEqual(badgeToCard(restored), card, type + " preserves its native fields and options");
  }
});

test("card badge mode changes preserve the new settings and remove stale navigation", () => {
  const { badgeCardType, badgeToCard, updateBadgeFromCard, cardBadgeUsesSlider } = loadTypescriptTest("src/webserver/model/badge_card.ts");
  const wrapped = badge("badge_card_type=subpage,badge_action=subpage,badge_page=12");
  const card = badgeToCard(wrapped);
  card.type = "light_temperature";
  card.options = "kelvin_min=2000,kelvin_max=6500,card_off_color=112233";
  updateBadgeFromCard(wrapped, card);
  normalizeBadgeConfig(wrapped);
  assert.equal(badgeCardType(wrapped), "light_temperature");
  assert.equal(configOptionValue(wrapped.options, "badge_page"), "");
  assert.deepEqual(badgeToCard(wrapped), card);
  assert(cardBadgeUsesSlider(card));
  assert(cardBadgeUsesSlider({ ...card, type: "media", sensor: "position" }));
  assert(!cardBadgeUsesSlider({ ...card, type: "media", sensor: "volume" }));
  assert.equal(badgeCardType(badge("badge_card_type=unknown")), null);
  for (const page of ["0", "33", "12oops", "12.1"]) {
    const invalid = badge("badge_card_type=subpage,badge_page=" + page);
    normalizeBadgeConfig(invalid);
    assert.equal(configOptionValue(invalid.options, "badge_page"), page);
  }
  const linked = badge("badge_card_type=subpage,badge_page=12");
  linked.label = "Renamed";
  normalizeBadgeConfig(linked);
  assert.equal(configOptionValue(parseBadgeRow(serializeBadgeRow([linked]))[0].options, "badge_page"), "12");
});

test("committed mirror failures still remove stale badge rows from the editor", async () => {
  let removed = 0;
  const controller = new NativePanelConfigController({
    fetch: null, deviceProfile: () => "panel", slotCount: () => 2,
    entityName: key => key, entityNameForSlot: (key, slot) => key + "_" + slot,
    normalizeHexColor: value => value, showBanner() {}, delay: callback => callback(),
    onBadgeRowRemoved: () => removed++,
  });
  controller.schedule = async () => "mirror-failed";
  assert.equal(await controller.writeText("button_config_1", "light.kitchen;Kitchen"), "mirror-failed");
  assert.equal(removed, 1);
  await controller.writeText("button_config_1", ";Room;Home;Auto;;;subpage");
  assert.equal(removed, 1, "a renamed subpage keeps its badge row");
});

test("portable backups retain rows independently of their device-specific native payload", () => {
  const { createBackupExportController } = loadTypescriptTest("src/webserver/features/backup_export_controller.ts");
  const { normalizeBackupEnvelope } = loadTypescriptTest("src/webserver/model/backup.ts");
  const rows = { badge_row_0: serializeBadgeRow([badge("")]), badge_row_4: serializeBadgeRow([badge("badge_action=toggle")]) };
  const exported = createBackupExportController({ serializeButtonConfig: () => "", serializeSubpageConfig: () => "" })
    .addNativeConfig({ buttons: [], subpages: {} }, { deviceProfile: "source-panel", buttons: [], subpages: {}, badgeRowSettings: rows });
  assert.deepEqual(exported.badge_rows, rows);
  delete exported.native_config;
  const imported = normalizeBackupEnvelope(exported, { buttons: [], subpages: {} });
  assert.deepEqual(imported.badge_rows, rows);
});
