import type { CardConfig } from "../contracts/types";
import { cloneCardConfig } from "./card";
import { normalizeBadgeConfig } from "./badge";
import { encodeConfigField } from "./config_primitives";
import { parseCompactSubpageConfig, serializeCompactSubpageConfig } from "./subpage";

export const MAX_ROW_BADGES = 8;
export const BADGE_ROW_PREFIX = "badge_row_";
export function badgeRowKey(page: number): string { return BADGE_ROW_PREFIX + page; }

export function parseBadgeRow(value: string | undefined): CardConfig[] {
  return parseCompactSubpageConfig(value, code => code === "BD" ? "badge" : "")
    .buttons.filter(button => button.type === "badge").slice(0, MAX_ROW_BADGES)
    .map(button => { normalizeBadgeConfig(button); return button; });
}

export function serializeBadgeRow(badges: readonly CardConfig[]): string {
  if (badges.length > MAX_ROW_BADGES) throw new Error("A row supports up to eight badges.");
  const fields = badges.map(source => {
    const b = cloneCardConfig(source);
    normalizeBadgeConfig(b);
    return ["BD", b.entity, b.label, b.icon, b.icon_on, b.sensor, b.unit, b.precision, b.options]
      .map((field, index) => index === 0 ? field : encodeConfigField(field));
  });
  const value = serializeCompactSubpageConfig([], fields);
  // A native setting has a key-length byte, the key, and its value in 2048 bytes.
  if (new TextEncoder().encode(value).length > 2034) throw new Error("Badge settings are too large. Shorten labels or entity names.");
  return value;
}

export function badgeRowsFromSettings(settings: Record<string, string>): Record<string, CardConfig[]> {
  const rows: Record<string, CardConfig[]> = {};
  for (let page = 0; page <= 32; page++) {
    const badges = parseBadgeRow(settings[badgeRowKey(page)]);
    if (badges.length) rows[String(page)] = badges;
  }
  return rows;
}

export function badgeRowSettings(rows: Record<string, readonly CardConfig[]>): Record<string, string> {
  const settings: Record<string, string> = {};
  for (const [page, badges] of Object.entries(rows)) {
    const value = serializeBadgeRow(badges);
    if (value) settings[badgeRowKey(Number(page))] = value;
  }
  return settings;
}
