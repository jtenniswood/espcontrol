import { CARD_SIZE_WIDE, CARD_SIZE_LARGE, CARD_SIZE_MAX_TALL, coveredCells, sizeColSpan, sizeRowSpan, sizeFitsAt, type SlotSizeMap } from "./grid";
import { parseRawButtonConfig } from "./card";
import { utf8ByteLength } from "./subpage";
import type { CardConfig } from "../contracts/types";
import { configOptionEnabled, configOptionValue, encodeConfigField } from "./config_primitives";

// Configuration only. Power calculation and live status remain firmware-owned.
export interface PowerCardConfigV1 {
  homeEntity: string;
  solarEntity: string;
  gridEntity: string;
  batteryEntity: string;
  batterySocEntity: string;
  invertGrid: boolean;
  invertBattery: boolean;
  installedComponents: number;
}

export function validPowerEntity(value: string): boolean {
  return /^sensor\.[a-z0-9_]+$/.test(value.trim());
}

export function decodePowerCardConfigV1(saved: CardConfig): PowerCardConfigV1 {
  return {
    homeEntity: saved.entity.trim(),
    solarEntity: configOptionValue(saved.options, "solar_entity").trim(),
    gridEntity: configOptionValue(saved.options, "grid_entity").trim(),
    batteryEntity: configOptionValue(saved.options, "battery_entity").trim(),
    batterySocEntity: configOptionValue(saved.options, "battery_soc_entity").trim(),
    invertGrid: configOptionEnabled(saved.options, "invert_grid"),
    invertBattery: configOptionEnabled(saved.options, "invert_battery"),
    installedComponents: ["solar", "home", "grid", "battery"].reduce((mask, name, i) =>
      name !== "home" && configOptionEnabled(saved.options, "exclude_" + name) ? mask & ~(1 << i) : mask, 15),
  };
}

export function normalizePowerOptions(options: string): string {
  const out: string[] = [];
  for (const name of ["solar_entity", "grid_entity", "battery_entity", "battery_soc_entity"]) {
    const value = configOptionValue(options, name).trim();
    if (value) out.push(name + "=" + encodeConfigField(value));
  }
  for (const name of ["invert_grid", "invert_battery", "exclude_solar", "exclude_grid", "exclude_battery"]) {
    if (configOptionEnabled(options, name)) out.push(name);
  }
  return out.join(",");
}

/** Restore preserves missing readings in old Power cards, but never invalid IDs. */
export function powerCardConfigError(card: CardConfig): string | null {
  if (card.type !== "power") return null;
  const config = decodePowerCardConfigV1(card);
  for (const [label, value, component] of [
    ["Home power", config.homeEntity, 2],
    ["Solar power", config.solarEntity, 1],
    ["Grid power", config.gridEntity, 4],
    ["Battery power", config.batteryEntity, 8],
    ["Battery SOC", config.batterySocEntity, 8],
  ] as const) {
    if ((config.installedComponents & component) && value && !validPowerEntity(value)) return "Choose a sensor entity (sensor.*) for " + label + ".";
  }
  if (String(card.options || "").split(",").some(option => /^(invert_(grid|battery)|exclude_(solar|grid|battery))=/.test(option)))
    return "Power polarity and component options must be flags, not values.";
  return null;
}

export function powerStoredConfigError(value: string, isSubpage = false, byteLimit = 255): string | null {
  const card = parseRawButtonConfig(value);
  if (card.type !== "power") return null;
  if (!isSubpage && utf8ByteLength(value) > byteLimit)
    return "Power settings exceed this panel's " + byteLimit + "-byte card limit. Shorten entity IDs or the label, omit optional SOC, or use a subpage.";
  return powerCardConfigError(card);
}

/** Power's numeric + two-label navigation layout needs two columns. */
export function powerMinimumSize(size: number): number {
  return sizeColSpan(size) >= 2 ? size : sizeRowSpan(size) >= 3
    ? CARD_SIZE_MAX_TALL : sizeRowSpan(size) === 2 ? CARD_SIZE_LARGE : CARD_SIZE_WIDE;
}
export function powerPlacementError(card: Pick<CardConfig, "type">, size: number,
  pos: number, grid: readonly number[], sizes: SlotSizeMap, slot: number,
  maxSlots: number, cols: number): string | null {
  if (card.type !== "power") return null;
  const help = "Power needs at least two columns. Choose Wide or larger in an empty area, or move neighboring cards yourself. No cards were moved or overwritten.";
  if (sizeColSpan(size) < 2 || !sizeFitsAt(pos, size, maxSlots, cols)) return help;
  const own = new Set(coveredCells(pos, sizes[String(slot)] || 1, maxSlots, cols, true));
  for (const cell of coveredCells(pos, size, maxSlots, cols, true)) {
    if (cell !== pos && !own.has(cell) && grid[cell] !== 0) return help;
  }
  return null;
}
export function powerGridLayoutError(buttons: readonly CardConfig[], grid: readonly number[],
  sizes: SlotSizeMap, maxSlots: number, cols: number): string | null {
  for (let pos = 0; pos < maxSlots; pos++) {
    const slot = grid[pos] || 0;
    if (slot <= 0 || !buttons[slot - 1]) continue;
    const error = powerPlacementError(buttons[slot - 1]!, sizes[String(slot)] || 1,
      pos, grid, sizes, slot, maxSlots, cols);
    if (error) return error;
  }
  return null;
}
