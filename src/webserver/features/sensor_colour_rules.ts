import { decodeConfigField, encodeConfigField } from "../model/config_primitives";
import { sensorColourCasefold } from "../generated/sensor_colour_casefold";

export const SENSOR_COLOUR_RULES_OPTION = "sensor_colours";
export const SENSOR_COLOUR_RULES_MAX = 6;

export type SensorColourCondition =
  | { kind: "number"; lower: string; lowerInclusive: boolean; upper: string; upperInclusive: boolean; colour: string }
  | { kind: "text"; state: string; colour: string };

export interface SensorColourRules {
  source: string;
  conditions: SensorColourCondition[];
  defaultColour?: string;
}

function validColour(value: unknown): string {
  const text = String(value || "").trim().replace(/^#/, "").toUpperCase();
  return /^[0-9A-F]{6}$/.test(text) ? text : "";
}

function finiteBound(value: string): boolean {
  return parseNumericValue(value) !== null;
}

function parseNumericValue(value: string): number | null {
  const text = value.trim();
  if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(text)) return null;
  const number = Number(text);
  return Number.isFinite(number) ? number : null;
}

export function validateSensorColourRules(rules: SensorColourRules): string | null {
  if (rules.defaultColour !== undefined && !validColour(rules.defaultColour)) return "Choose a valid default colour.";
  if (rules.source.trim() && !/^(sensor|binary_sensor|text_sensor)\.[a-z0-9_]+$/.test(rules.source.trim())) {
    return "Choose a valid Home Assistant sensor entity for the colour source.";
  }
  if (!Array.isArray(rules.conditions) || rules.conditions.length < 1 || rules.conditions.length > SENSOR_COLOUR_RULES_MAX) {
    return `Add between 1 and ${SENSOR_COLOUR_RULES_MAX} colour conditions.`;
  }
  for (const [index, condition] of rules.conditions.entries()) {
    if (!validColour(condition.colour)) return `Condition ${index + 1} needs a valid six-digit colour.`;
    if (condition.kind === "text") {
      if (!condition.state.trim()) return `Condition ${index + 1} needs a text state.`;
      continue;
    }
    const hasLower = condition.lower.trim() !== "";
    const hasUpper = condition.upper.trim() !== "";
    if ((!hasLower && !hasUpper) ||
        (hasLower && !finiteBound(condition.lower)) || (hasUpper && !finiteBound(condition.upper))) {
      return `Condition ${index + 1} needs a finite numeric bound.`;
    }
    if (hasLower && hasUpper) {
      const lower = parseNumericValue(condition.lower)!;
      const upper = parseNumericValue(condition.upper)!;
      if (lower > upper || (lower === upper && (!condition.lowerInclusive || !condition.upperInclusive))) {
        return `Condition ${index + 1} has an empty or reversed range.`;
      }
    }
  }
  return null;
}

export function serializeSensorColourRules(rules: SensorColourRules): string {
  const error = validateSensorColourRules(rules);
  if (error) throw new Error(error);
  const fields = ["v2", rules.source.trim()];
  const conditions = rules.conditions.map(condition => condition.kind === "text"
    ? ["t", condition.state.trim(), validColour(condition.colour)].map(encodeConfigField).join(",")
    : ["n", condition.lower.trim(), condition.lowerInclusive ? "1" : "0", condition.upper.trim(), condition.upperInclusive ? "1" : "0", validColour(condition.colour)].map(encodeConfigField).join(","));
  return `${fields.map(encodeConfigField).join("|")}|${conditions.join(";")}|${rules.defaultColour ? validColour(rules.defaultColour) : ""}`;
}

export function parseSensorColourRules(value: string): SensorColourRules | null {
  const fields = String(value || "").split("|");
  const version = decodeConfigField(fields[0] || "");
  if (!((version === "v1" && fields.length === 3) || (version === "v2" && fields.length === 4))) return null;
  const source = decodeConfigField(fields[1] || "");
  const conditions: SensorColourCondition[] = [];
  for (const raw of (fields[2] || "").split(";")) {
    if (!raw) continue;
    const values = raw.split(",").map(decodeConfigField);
    if (values[0] === "t" && values.length === 3) {
      conditions.push({ kind: "text", state: values[1] || "", colour: values[2] || "" });
    } else if (values[0] === "n" && values.length === 6) {
      conditions.push({ kind: "number", lower: values[1] || "", lowerInclusive: values[2] === "1", upper: values[3] || "", upperInclusive: values[4] === "1", colour: values[5] || "" });
    } else return null;
  }
  const rules: SensorColourRules = { source, conditions };
  if (version === "v2" && fields[3]) {
    const colour = decodeConfigField(fields[3]);
    if (!/^[0-9a-fA-F]{6}$/.test(colour)) return null;
    rules.defaultColour = validColour(colour);
  }
  return validateSensorColourRules(rules) ? null : rules;
}

export function sensorColourForState(rules: SensorColourRules, rawState: string): string | null {
  if (rawState.trim() === "" || /^(unknown|unavailable)$/i.test(rawState.trim())) return null;
  for (const condition of rules.conditions) {
    if (condition.kind === "text") {
      if (sensorColourCasefold(rawState.trim()) === sensorColourCasefold(condition.state.trim())) return validColour(condition.colour);
      continue;
    }
    const value = parseNumericValue(rawState);
    if (value === null) continue;
    if (condition.lower.trim() !== "") {
      const lower = parseNumericValue(condition.lower)!;
      if (condition.lowerInclusive ? value < lower : value <= lower) continue;
    }
    if (condition.upper.trim() !== "") {
      const upper = parseNumericValue(condition.upper)!;
      if (condition.upperInclusive ? value > upper : value >= upper) continue;
    }
    return validColour(condition.colour);
  }
  return null;
}
