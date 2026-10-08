import {
  chooseSerializedSubpageConfig,
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
  setCardColor,
  lighterCardColor,
  setConfigOptionValue,
  normalizedCardColorOptions,
  withoutCardColorOptions,
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

  options = setConfigOptionValue(options, "large_numbers", "off");
  options = setConfigOptionValue(options, "card_on_color", "E91E63");
  options = setCardColor(options, "#3f51b5");
  equal(normalizedCardColorOptions(options), "card_off_color=3F51B5", "only the selected card colour is saved");
  equal(configOptionValue(options, "card_on_color"), "", "choosing a card colour removes any old active override");
  equal(withoutCardColorOptions(options), "active_color,confirm_message=Run%2C now?,large_numbers=off", "card colours can be separated before type-specific normalization");
  equal(normalizedCardColorOptions("card_on_color=E91E63,card_off_color=3F51B5"), "card_off_color=3F51B5", "existing card colour takes priority over a separate active override");
  equal(normalizedCardColorOptions("card_on_color=#e91e63"), "card_off_color=E91E63", "old active-only colour migrates to the single card colour");
  equal(normalizedCardColorOptions("card_off_color=invalid,card_on_color=00BCD4"), "card_off_color=00BCD4", "a valid legacy colour survives an invalid card colour");
  equal(lighterCardColor("3F51B5"), "7985CB", "active colour is derived from the selected card colour");
  equal(lighterCardColor("000000"), "4D4D4D", "active colour channel rounding matches firmware");
  equal(lighterCardColor("FFFFFF"), "FFFFFF", "white remains white in the active state");
  equal(lighterCardColor("invalid"), "", "invalid colours do not create an active override");
  options = setCardColor(options, "invalid");
  equal(normalizedCardColorOptions(options), "", "invalid card colours are dropped safely");
  equal(setCardColor("confirm_on,card_on_color=FFFFFF,card_off_color=3F51B5", ""), "confirm_on", "reset clears old and new colour options while retaining other settings");

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
