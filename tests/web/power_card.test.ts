import { cardContractDefaultConfig, cardContractHidden, cardContractSubpageTypeCode, cardContractSubpageTypeFromCode, cardRuntimeSpec } from "../../src/webserver/generated/card_contract";
import { normalizeSavedConfigPower } from "../../src/webserver/generated/saved_config_power";
import { decodePowerCardConfigV1, normalizePowerOptions, validPowerEntity, powerCardConfigError, powerStoredConfigError, powerMinimumSize, powerPlacementError, powerGridLayoutError } from "../../src/webserver/model/power_card";
import { parseRawButtonConfig, CARD_CONFIG_FIELDS } from "../../src/webserver/model/card";
import { createPanelConfigBackupPayload, decodePanelConfigBackupPayload, encodePanelConfig, decodePanelConfig } from "../../src/webserver/model/panel_config";
import { createCardTransferCode, parseCardTransferCode } from "../../src/webserver/model/card_transfer";
import { CARD_SIZE_SINGLE } from "../../src/webserver/model/grid";
import { parseCompactSubpageConfig, serializeCompactSubpageConfig, splitSubpageConfigChunks, utf8ByteLength } from "../../src/webserver/model/subpage";
import { encodeConfigField } from "../../src/webserver/model/config_primitives";
import { createCardEditorValidationController } from "../../src/webserver/features/card_editor_validation_controller";
import { createCardRegistry } from "../../src/webserver/application/card_registry";
import { cardTypePickerOptions } from "../../src/webserver/features/preview";
import { previewEffectiveTheme, PREVIEW_THEME_COLORS } from "../../src/webserver/state/preview_theme";
import { registerPowerCardTypes } from "../../src/webserver/cards/power";

function equal(actual: unknown, expected: unknown): void {
  if (JSON.stringify(actual) !== JSON.stringify(expected))
    throw new Error(`expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
}

export function runPowerCardTests(): void {
  equal(CARD_CONFIG_FIELDS.length, 9);
  const defaults = cardContractDefaultConfig("power");
  equal(defaults.label, "Power Dashboard");
  equal(decodePowerCardConfigV1(defaults), {
    homeEntity: "", solarEntity: "", gridEntity: "", batteryEntity: "", batterySocEntity: "",
    invertGrid: false, invertBattery: false, installedComponents: 15,
  });
  equal(cardRuntimeSpec("power")?.driver, "power");
  equal(cardRuntimeSpec("power")?.capabilities.actions, true); // native modal navigation only
  equal(cardRuntimeSpec("power")?.capabilities.subscriptions, true);
  equal(cardRuntimeSpec("power")?.capabilities.modal, true);
  equal(cardRuntimeSpec("power")?.capabilities.runtimeAllocation, true);
  equal(cardContractHidden("power"), false);
  const registry = createCardRegistry();
  registerPowerCardTypes(registry, { cardSensorPreviewHtml: () => "sensor-preview" });
  equal(registry.definitions.power?.isAvailable, null);
  equal(typeof registry.definitions.power?.renderSettings, "function");
  equal(typeof registry.definitions.power?.renderPreview, "function");
  for (const sub of [false, true]) {
    equal(cardTypePickerOptions(registry.definitions, [], false, sub, null).some(option => option.key === "power"), true);
    equal(cardTypePickerOptions(registry.definitions, ["power"], false, sub, null).some(option => option.key === "power"), false);
  }
  equal(validPowerEntity("sensor.home"), true);
  for (const id of ["switch.home", "sensor.", "sensor.bad;injection", "sensor.home,grid", "sensor.UPPER", ""]) equal(validPowerEntity(id), false);
  equal(powerCardConfigError(defaults), null); // unmeasured is explicit readings-only, never zero
  // Old saved partial configurations retain unavailable readings on restore.
  equal(powerCardConfigError(defaults), null);
  const preview = registry.definitions.power!.renderPreview!(defaults, { escHtml: (s: string) => s.replace(/</g, "&lt;"), cardSize: 3 }) as { iconHtml: string; buttonClass: string; labelHtml: string };
  equal(preview.iconHtml, "sensor-preview");
  equal(preview.buttonClass, "sp-power-card");
  equal(preview.labelHtml.includes("Power Dashboard"), true);
  equal(preview.labelHtml.includes("Unavailable"), true); // never fake Idle
  equal(preview.labelHtml.includes("mdi-chevron-right"), true);
  const escaped = registry.definitions.power!.renderPreview!({ ...defaults, label: "<script>" }, { escHtml: (s: string) => s.replace(/</g, "&lt;"), cardSize: 3 }) as { iconHtml: string; buttonClass: string; labelHtml: string };
  equal(escaped.labelHtml.includes("<script>"), false);
  for (const mode of ["Dark", "Light", "Auto"] as const) {
    equal(previewEffectiveTheme({ themeMode: mode, themeActive: "Light" }), mode === "Dark" ? "Dark" : "Light");
  }
  equal(PREVIEW_THEME_COLORS.Light.surfaceSensor, "F5F5F5");
  equal(PREVIEW_THEME_COLORS.Dark.surfaceSensor, "212121");
  for (const size of [1, 2, 5]) equal(powerMinimumSize(size) > 1, true);
  const grid = [1, 0, 2, 0, 0, 0, 0, 0, 0];
  const snapshot = JSON.stringify(grid);
  equal(powerPlacementError(defaults, 1, 0, grid, {}, 1, 9, 3) !== null, true);
  equal(powerPlacementError(defaults, 3, 0, grid, {}, 1, 9, 3), null);
  equal(powerPlacementError(defaults, 6, 0, grid, {}, 1, 9, 3) !== null, true); // neighbor protected
  equal(powerPlacementError(defaults, 3, 2, grid, {}, 2, 9, 3) !== null, true); // row edge
  equal(powerPlacementError(defaults, 3, 0, grid, {}, 1, 9, 1) !== null, true); // actual subpage cols
  equal(powerPlacementError({ type: "sensor" }, 1, 0, grid, {}, 1, 9, 3), null);
  equal(powerGridLayoutError([defaults], grid, { 1: 3 }, 9, 3), null);
  equal(powerGridLayoutError([defaults], grid, {}, 9, 3) !== null, true);
  equal(JSON.stringify(grid), snapshot); // no allocation/movement as a side effect
  const narrow = registry.definitions.power!.renderPreview!(defaults, { escHtml: (s: string) => s, cardSize: 1 }) as { iconHtml: string; labelHtml: string };
  equal(narrow.iconHtml, ""); equal(narrow.labelHtml.includes("Unavailable"), false);
  equal(cardContractSubpageTypeCode("power"), "PW");
  equal(cardContractSubpageTypeFromCode("PW"), "power");
  // Existing compact codes must not move when a new card is registered.
  equal(cardContractSubpageTypeFromCode("H"), "climate");

  const encoded = "sensor.home;My Power;Auto;Auto;;;power;1;solar_entity=sensor.solar,grid_entity=sensor.grid,battery_entity=sensor.battery,battery_soc_entity=sensor.soc,invert_grid,invert_battery";
  const card = parseRawButtonConfig(encoded);
  equal(normalizeSavedConfigPower(card, normalizePowerOptions), true);
  equal(decodePowerCardConfigV1(card), {
    homeEntity: "sensor.home", solarEntity: "sensor.solar", gridEntity: "sensor.grid",
    batteryEntity: "sensor.battery", batterySocEntity: "sensor.soc", invertGrid: true, invertBattery: true, installedComponents: 15,
  });
  equal(powerCardConfigError(card), null);
  for (const field of ["solar_entity", "grid_entity", "battery_entity"]) {
    equal(powerCardConfigError({ ...card, options: card.options.split(",").filter(value => !value.startsWith(field + "=")).join(",") }), null);
  }
  equal(powerCardConfigError({ ...card, options: normalizePowerOptions(card.options.replace("battery_soc_entity=sensor.soc,", "")) }), null);
  equal(powerCardConfigError({ ...card, options: card.options + ",invert_grid=true" }) !== null, true);
  equal(powerCardConfigError({ ...card, options: card.options.replace("sensor.soc", "number.soc") }) !== null, true);
  equal(powerStoredConfigError(encoded), null);
  for (let mask=0;mask<16;mask++) {
    const names=["solar","home","grid","battery"];
    const excluded=names.filter((_,i)=>!(mask&(1<<i))).map(name=>"exclude_"+name);
    const partial={...card,precision:"2",options:normalizePowerOptions(card.options+","+excluded.join(","))};
    equal(decodePowerCardConfigV1(partial).installedComponents,mask|2);
    equal(powerCardConfigError(partial),null);
    const before={...partial};normalizeSavedConfigPower(partial,normalizePowerOptions);equal(partial,before);
    equal(partial.precision,"2"); // obsolete precision is accepted and round-trips
    const raw=[partial.entity,partial.label,partial.icon,partial.icon_on,partial.sensor,partial.unit,partial.type,partial.precision,partial.options].join(";");
    equal(decodePowerCardConfigV1(parseRawButtonConfig(raw)).installedComponents,mask|2);
    equal(powerStoredConfigError(raw,false,4096),null);
    const native=encodePanelConfig({deviceProfile:"esp32-p4-86",buttons:{1:raw},subpages:{},settings:{button_order:"1"}});
    const restored=decodePanelConfigBackupPayload(createPanelConfigBackupPayload(native));
    equal(decodePowerCardConfigV1(parseRawButtonConfig(decodePanelConfig(restored).buttons[1]!)).installedComponents,mask|2);
  }
  const legacyExcludedHome={...card,options:card.options+",exclude_solar,exclude_home,exclude_grid,exclude_battery"};
  equal(decodePowerCardConfigV1(legacyExcludedHome).installedComponents,2);
  equal(powerCardConfigError(legacyExcludedHome),null);
  normalizeSavedConfigPower(legacyExcludedHome,normalizePowerOptions);
  equal(legacyExcludedHome.entity,card.entity);
  equal(legacyExcludedHome.options.includes("exclude_home"),false);
  equal(decodePowerCardConfigV1(legacyExcludedHome).installedComponents,2);
  equal(powerCardConfigError({...card,options:card.options+",exclude_solar=true"})!==null,true);
  equal(powerCardConfigError({...card,entity:""}),null);
  const canonical = { ...card };
  normalizeSavedConfigPower(card, normalizePowerOptions);
  equal(card, canonical); // normalization is idempotent
  const legacyCard = parseRawButtonConfig("light.kitchen;Kitchen;Auto;Auto;;;;;");
  const unchanged = { ...legacyCard };
  equal(normalizeSavedConfigPower(legacyCard, normalizePowerOptions), false);
  equal(legacyCard, unchanged);

  const subpage = serializeCompactSubpageConfig(["1", "B"], [["PW",
    ...[card.entity, card.label, "", "", card.sensor, card.unit, card.precision, card.options].map(encodeConfigField),
  ]]);
  equal(parseCompactSubpageConfig(subpage, cardContractSubpageTypeFromCode).buttons[0],
    { type: card.type, entity: card.entity, label: card.label, icon: card.icon, icon_on: card.icon_on,
      sensor: card.sensor, unit: card.unit, precision: card.precision, options: card.options });
  const document = {
    deviceProfile: "esp32-p4-86", buttons: { 1: encoded, 2: "light.kitchen;Kitchen;Auto;Auto;;;;;" },
    subpages: { 3: subpage },
    settings: { button_order: "1,2,3" },
  };
  equal(decodePanelConfig(encodePanelConfig(document)), document);
  equal(decodePanelConfig(decodePanelConfigBackupPayload(createPanelConfigBackupPayload(encodePanelConfig(document)))), document);
  const transfer = parseCardTransferCode(createCardTransferCode(
    { device: "esp32-p4-86", firmware: "phase-1" }, [{ ...card, size: CARD_SIZE_SINGLE }],
  ));
  equal(transfer.cards[0]?.options, card.options);
  equal(transfer.cards[0]?.type, "power");
  // Native backup codec capacity is not the live main-grid text entity limit.
  const longId = "sensor." + "power_".repeat(30);
  const longCard = { ...defaults, entity: longId, options:
    ["solar_entity", "grid_entity", "battery_entity", "battery_soc_entity"].map(name => name + "=" + longId).join(",") };
  const longEncoded = CARD_CONFIG_FIELDS.map(field => longCard[field]).join(";");
  const longDocument = { ...document, buttons: { 1: longEncoded } };
  equal(decodePanelConfig(encodePanelConfig(longDocument)), longDocument);
  const validator = createCardEditorValidationController();
  const save = (length: number) => validator.validateSave({ fields: [], isSubpage: false,
    serializedConfigLength: length, imageCardCount: 0, imageCardCapacity: 6 });
  equal(save(encoded.length).valid, true);
  equal(save(longEncoded.length).reason, "config-size"); // retain 255-byte protection
  const realistic = "sensor.house_total_consumption_power;Power Dashboard;Auto;Auto;;;power;;" +
    "solar_entity=sensor.inverter_solar_output_power,grid_entity=sensor.inverter_grid_exchange_power," +
    "battery_entity=sensor.inverter_battery_charge_discharge_power,battery_soc_entity=sensor.inverter_battery_state_of_charge";
  equal(realistic.length, 288);
  equal(utf8ByteLength("\u00e9".repeat(128)), 256);
  equal(save(255).valid, true);
  equal(save(256).reason, "config-size");
  equal(powerStoredConfigError(realistic) !== null, true);
  equal(powerStoredConfigError(realistic, true), null);
  // Upstream #2237 advertises 4 KiB; legacy firmware retains its 255-byte limit.
  equal(powerStoredConfigError(realistic, false, 4096), null);
  equal(validator.validateSave({ fields: [], isSubpage: false,
    serializedConfigLength: realistic.length, configByteLimit: 4096,
    imageCardCount: 0, imageCardCapacity: 6 }).valid, true);
  equal(powerStoredConfigError(realistic + "x".repeat(4096), false, 4096) !== null, true);
  equal(splitSubpageConfigChunks(subpage, 4) !== null, true);
  const longSubpage = serializeCompactSubpageConfig(["1", "B"], [["PW", ...[longCard.entity, longCard.label, "", "", "", "", "", longCard.options].map(encodeConfigField)]]);
  equal(splitSubpageConfigChunks(longSubpage, 4), null); // aggregate capacity enforced
  equal(splitSubpageConfigChunks(longSubpage, 8) !== null, true);
  equal(save(realistic.length).reason, "config-size"); // no truncation or storage expansion
}
