import {
  cardBackgroundAssetId,
  cardOptionsWithAppearance,
  chooseSerializedSubpageConfig,
  cloneCardConfig,
  configOptionEnabled,
  configOptionValue,
  decodeConfigField,
  encodeConfigField,
  legacyButtonConfigSafe,
  parseRawButtonConfig,
  parseRawSubpageConfig,
  serializeCompactSubpageConfig,
  serializeLegacySubpageConfig,
  setConfigOption,
  setConfigOptionValue,
  setCardBackgroundAssetId,
  splitSubpageConfigChunks,
  trimConfigFields,
} from "../../src/webserver/model";
import {
  cardContractSubpageTypeCode,
  cardContractSubpageTypeFromCode,
} from "../../src/webserver/generated/card_contract";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

export function runEncodingTests(): void {
  const fieldCases = [
    "plain text",
    "50%, warm;cool|night:mode",
    "23°C",
    "emoji 🌤️",
  ];
  for (const value of fieldCases) {
    equal(decodeConfigField(encodeConfigField(value)), value, `field round-trip for ${value}`);
  }
  equal(encodeConfigField("first,second"), "first%2Csecond", "compact field commas are escaped");
  equal(decodeConfigField("broken%ZZvalue"), "broken%ZZvalue", "invalid percent runs are preserved");
  deepEqual(trimConfigFields(["one", "", ""]), ["one"], "trailing empty fields are removed");
  equal(legacyButtonConfigSafe(["light.kitchen", "Kitchen"]), true, "plain cards use legacy encoding");
  equal(legacyButtonConfigSafe(["light.kitchen", "Kitchen;Main"]), false, "delimiter-bearing cards use compact encoding");

  let options = setConfigOption("active_color", "confirm_on", true);
  equal(configOptionEnabled(options, "confirm_on"), true, "flag options can be enabled");
  options = setConfigOptionValue(options, "confirm_message", "Run, now?");
  equal(configOptionValue(options, "confirm_message"), "Run, now?", "valued options round-trip reserved characters");
  options = setConfigOption(options, "confirm_on", false);
  equal(configOptionEnabled(options, "confirm_on"), false, "flag options can be disabled");

  const appearanceCard = cloneCardConfig({ options: "active_color,bg_image=Kitchen-Scene" });
  equal(cardBackgroundAssetId(appearanceCard), "kitchen-scene", "appearance is normalized from legacy options");
  equal(Object.keys(appearanceCard).includes("appearance"), false,
    "typed appearance does not change saved or backup JSON");
  appearanceCard.options = "confirm_on";
  equal(cardOptionsWithAppearance(appearanceCard.options, appearanceCard),
    "confirm_on,bg_image=kitchen-scene", "typed appearance survives unrelated option normalization");
  setCardBackgroundAssetId(appearanceCard, "new-scene");
  equal(appearanceCard.appearance.backgroundAssetId, "new-scene", "typed appearance updates with the card");
  equal(configOptionValue(appearanceCard.options, "bg_image"), "new-scene",
    "legacy options stay synchronized at the compatibility boundary");

  deepEqual(parseRawButtonConfig("light.kitchen;Kitchen;Lightbulb;Auto;;;;;active_color"), {
    entity: "light.kitchen",
    label: "Kitchen",
    icon: "Lightbulb",
    icon_on: "Auto",
    sensor: "",
    unit: "",
    type: "",
    precision: "",
    options: "active_color",
  }, "legacy card parse");
  deepEqual(parseRawButtonConfig("~light.kitchen,Kitchen%2C%20Main,Lightbulb,Auto,,,,,active_color"), {
    entity: "light.kitchen",
    label: "Kitchen, Main",
    icon: "Lightbulb",
    icon_on: "Auto",
    sensor: "",
    unit: "",
    type: "",
    precision: "",
    options: "active_color",
  }, "compact card parse");

  const legacy = serializeLegacySubpageConfig(["1", "B"], [[
    "light.kitchen", "Kitchen", "Lightbulb", "Auto", "", "", "", "", "active_color",
  ]]);
  const compact = serializeCompactSubpageConfig(["1", "B"], [[
    cardContractSubpageTypeCode(""), "light.kitchen", "Kitchen", "Lightbulb", "Auto", "", "", "", "active_color",
  ]]);
  equal(chooseSerializedSubpageConfig(["1", "B"], 1, legacy, compact), compact.length < legacy.length ? compact : legacy,
    "subpage serializer chooses the shortest compatible representation");
  const parsed = parseRawSubpageConfig(compact, cardContractSubpageTypeFromCode);
  equal(parsed.buttons[0]?.entity, "light.kitchen", "compact subpage entity round-trip");
  equal(parsed.buttons[0]?.options, "active_color", "compact subpage option round-trip");

  const utf8Subpage = "Door 🌤️|".repeat(16);
  const chunks = splitSubpageConfigChunks(utf8Subpage, 4, 64);
  if (!chunks) throw new Error("UTF-8 subpage data should fit the requested chunks");
  equal(chunks.join(""), utf8Subpage, "UTF-8 chunks reassemble exactly");
  if (chunks.some((chunk) => new TextEncoder().encode(chunk).length > 64)) {
    throw new Error("UTF-8 chunks must respect the device byte limit");
  }
}
