import {
  parseSensorColourRules,
  sensorColourForState,
  serializeSensorColourRules,
  validateSensorColourRules,
  type SensorColourRules,
} from "../../src/webserver/features/sensor_colour_rules";
import { configOptionValue, setConfigOptionValue } from "../../src/webserver/model/config_primitives";
import { sensorColourCasefold } from "../../src/webserver/generated/sensor_colour_casefold";

export function runSensorColourRulesTests(unicodeMappings: [string, string][] = [], defaultFixtures: { payload: string; valid: boolean; defaultColour?: string }[] = [], whitespaceFixtures: { trimCodepoints: number[]; retainedCodepoints: number[] } = { trimCodepoints: [], retainedCodepoints: [] }): void {
  function equal(actual: unknown, expected: unknown): void {
    if (JSON.stringify(actual) !== JSON.stringify(expected)) throw new Error(`Expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
  }
  const rules: SensorColourRules = {
    source: "sensor.battery_power",
    conditions: [
      { kind: "number", lower: "", lowerInclusive: true, upper: "-1000", upperInclusive: false, colour: "FF0000" },
      { kind: "number", lower: "-1000", lowerInclusive: true, upper: "0", upperInclusive: false, colour: "FFFF00" },
      { kind: "number", lower: "0", lowerInclusive: true, upper: "0", upperInclusive: true, colour: "0000FF" },
      { kind: "text", state: "Løbning, running|again", colour: "00FF00" },
    ],
  };
  for (const codepoint of whitespaceFixtures.trimCodepoints) {
    const space = String.fromCodePoint(codepoint);
    const text: SensorColourRules = { source: "", conditions: [{ kind: "text", state: space + "ready" + space, colour: "00FF00" }] };
    equal(sensorColourForState(text, space + "READY" + space), "00FF00");
    equal(sensorColourForState(text, "ready"), "00FF00");
    equal(sensorColourForState(text, space), null);
    equal(sensorColourForState(rules, space + "-1000" + space), "FFFF00");
    for (const state of ["unknown", "unavailable"]) {
      equal(sensorColourForState({ source: "", conditions: [{ kind: "text", state, colour: "00FF00" }] }, space + state + space), null);
    }
  }
  for (const codepoint of whitespaceFixtures.retainedCodepoints) {
    const space = String.fromCodePoint(codepoint);
    equal((space + "ready" + space).trim(), space + "ready" + space);
    equal(sensorColourForState({ source: "", conditions: [{ kind: "text", state: "ready", colour: "00FF00" }] }, space + "ready" + space), null);
  }
  for (const fixture of defaultFixtures) {
    const parsed = parseSensorColourRules(fixture.payload);
    equal(parsed !== null, fixture.valid);
    if (parsed) equal(parsed.defaultColour, fixture.defaultColour);
  }
  const encoded = serializeSensorColourRules(rules);
  equal(parseSensorColourRules(encoded), rules);
  const withDefault = { ...rules, defaultColour: "FFFFFF" };
  equal(parseSensorColourRules(serializeSensorColourRules(withDefault)), withDefault);
  equal(parseSensorColourRules("v2||t,running,00FF00|000000")?.defaultColour, "000000");
  equal(parseSensorColourRules("v1||t,running,00FF00")?.defaultColour, undefined);
  equal(parseSensorColourRules("v2||t,running,00FF00|invalid"), null);
  equal(validateSensorColourRules({ ...rules, defaultColour: "notacolour" }), "Choose a valid default colour.");
  const option = setConfigOptionValue("", "sensor_colours", encoded);
  equal(parseSensorColourRules(configOptionValue(option, "sensor_colours")), rules);
  equal(sensorColourForState(rules, "-1000.1"), "FF0000");
  equal(sensorColourForState(rules, "-1000"), "FFFF00");
  equal(sensorColourForState(rules, "-0.04"), "FFFF00");
  equal(sensorColourForState(rules, "0"), "0000FF");
  equal(sensorColourForState(rules, "0.001"), null);
  equal(sensorColourForState(rules, "  LØBNING, RUNNING|AGAIN  "), "00FF00");
  for (const [state, incoming] of [["привет", "ПРИВЕТ"], ["κόσμος", "ΚΌΣΜΟΣ"], ["straße", "STRASSE"], ["ﬃ", "FFI"], ["հայերեն", "ՀԱՅԵՐԵՆ"], ["𐐨", "𐐀"]]) {
    const textRules: SensorColourRules = { source: "", conditions: [{ kind: "text", state: state!, colour: "00FF00" }] };
    equal(sensorColourForState(textRules, incoming!), "00FF00");
  }
  for (const [text, folded] of unicodeMappings) equal(sensorColourCasefold(text), folded);
  equal(sensorColourCasefold("测试🙂"), "测试🙂");
  equal(validateSensorColourRules({ ...rules, source: "Sensor.Battery_Power" }), "Choose a valid Home Assistant sensor entity for the colour source.");
  equal(parseSensorColourRules("v1|Sensor.Battery_Power|t,running,00FF00"), null);
  equal(sensorColourForState(rules, "unavailable"), null);
  equal(sensorColourForState(rules, "unknown"), null);
  equal(sensorColourForState(rules, "NaN"), null);
  equal(sensorColourForState(rules, "Infinity"), null);
  equal(sensorColourForState(rules, "0b10"), null);
  equal(sensorColourForState({ source: "", conditions: [
    { kind: "number", lower: "0", lowerInclusive: true, upper: "10", upperInclusive: true, colour: "FF0000" },
    { kind: "number", lower: "5", lowerInclusive: true, upper: "15", upperInclusive: true, colour: "00FF00" },
  ] }, "7"), "FF0000");
  equal(validateSensorColourRules({ source: "", conditions: Array.from({ length: 7 }, () => ({ kind: "text" as const, state: "ready", colour: "FFFFFF" })) }), "Add between 1 and 6 colour conditions.");
  equal(validateSensorColourRules({ source: "", conditions: [{ kind: "number", lower: "0", lowerInclusive: false, upper: "0", upperInclusive: true, colour: "FFFFFF" }] }), "Condition 1 has an empty or reversed range.");
  equal(parseSensorColourRules("v2||t,running,00FF00"), null);
}
