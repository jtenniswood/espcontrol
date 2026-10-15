import type { CardConfig } from "../contracts/types";
import { cloneCardConfig } from "./card";
import { decodeConfigField, encodeConfigField } from "./config_primitives";
import { applySpans, sizeFromToken, sizeToken, type SlotSizeMap } from "./grid";

export interface BackOrderToken {
  token: string;
  label: string;
  color?: string;
}

export interface ParsedSubpageOrder {
  order: string[];
  backLabel: string;
  backColor?: string;
}

export interface ParsedSubpageConfig {
  order: string[];
  buttons: CardConfig[];
  backLabel: string;
  backColor?: string;
}

export interface StructuredSubpageConfig {
  order: string[];
  back_label: string;
  back_color?: string;
  buttons: CardConfig[];
}

export interface SubpageGridSource {
  order?: readonly string[];
  grid?: readonly number[];
  sizes?: SlotSizeMap;
  buttons?: readonly unknown[];
  backLabel?: string;
  backColor?: string;
}

const BACK_TOKENS = new Set(["B", "Bd", "Bw", "Bb", "Bt", "Bx"]);

function isRecord(value: unknown): value is Record<string, unknown> {
  return !!value && typeof value === "object" && !Array.isArray(value);
}

function stringField(record: Record<string, unknown>, key: string, fallback = ""): string {
  const value = record[key];
  return value == null ? fallback : String(value || fallback);
}

export function isBackOrderToken(token: string | null | undefined): boolean {
  return BACK_TOKENS.has(String(token || ""));
}

function normalizeBackColor(value: unknown): string {
  const hex = String(value || "").trim().replace(/^#/, "").toUpperCase();
  return /^[0-9A-F]{6}$/.test(hex) ? hex : "";
}

export function parseBackOrderToken(value: string | null | undefined): BackOrderToken {
  const raw = String(value || "").trim();
  const eq = raw.indexOf("=");
  const token = eq >= 0 ? raw.substring(0, eq) : raw;
  const separator = eq >= 0 ? raw.indexOf(":", eq + 1) : -1;
  const label = eq >= 0 ? decodeConfigField(raw.substring(eq + 1, separator < 0 ? undefined : separator)) : "Back";
  const color = separator >= 0 ? normalizeBackColor(raw.substring(separator + 1)) : "";
  if (!BACK_TOKENS.has(token)) {
    return { token: raw, label: "Back" };
  }
  return { token, label: label || "Back", ...(color ? { color } : {}) };
}

export function backOrderToken(baseToken: string, label: string | null | undefined, color?: string | null): string {
  const token = parseBackOrderToken(baseToken).token;
  const text = label || "Back";
  const hex = normalizeBackColor(color);
  // An empty label keeps the firmware's translated Back text when a colour is set.
  if (text === "Back") return hex ? token + "=:" + hex : token;
  return token + "=" + encodeConfigField(text) + (hex ? ":" + hex : "");
}

export function backLabelFromOrder(order: readonly string[] | null | undefined): string {
  for (const item of order || []) {
    const parsed = parseBackOrderToken(item);
    if (BACK_TOKENS.has(parsed.token)) {
      return parsed.label || "Back";
    }
  }
  return "Back";
}

export function parseSubpageOrder(orderStr: string | null | undefined): ParsedSubpageOrder {
  const order: string[] = [];
  let backLabel = "Back";
  let backColor = "";
  if (orderStr) {
    const parts = orderStr.split(",");
    for (const part of parts) {
      const parsed = parseBackOrderToken(part);
      order.push(parsed.token);
      if (BACK_TOKENS.has(parsed.token)) {
        backLabel = parsed.label || "Back";
        backColor = parsed.color || "";
      }
    }
  }
  return { order, backLabel, ...(backColor ? { backColor } : {}) };
}

export function subpageOrderForSerialize(
  order: readonly string[] | null | undefined,
  backLabel?: string | null,
  backColor?: string | null,
): string[] {
  const out: string[] = [];
  for (const item of order || []) {
    const parsed = parseBackOrderToken(item);
    if (BACK_TOKENS.has(parsed.token)) {
      out.push(backOrderToken(parsed.token, backLabel || parsed.label || "Back", backColor ?? parsed.color));
    } else {
      out.push(parsed.token);
    }
  }
  return out;
}

export function parseLegacySubpageConfig(value: string | null | undefined): ParsedSubpageConfig {
  if (!value || !value.trim()) return { order: [], buttons: [], backLabel: "Back" };
  const parts = value.split("|");
  const parsedOrder = parseSubpageOrder(parts[0] || "");
  const buttons: CardConfig[] = [];
  for (let i = 1; i < parts.length; i += 1) {
    const fields = (parts[i] || "").split(":");
    buttons.push({
      entity: fields[0] || "",
      label: fields[1] || "",
      icon: fields[2] || "Auto",
      icon_on: fields[3] || "Auto",
      sensor: fields[4] || "",
      unit: fields[5] || "",
      type: fields[6] || "",
      precision: fields[7] || "",
      options: fields[8] || "",
    });
  }
  return {
    order: parsedOrder.order,
    buttons,
    backLabel: parsedOrder.backLabel,
    ...(parsedOrder.backColor ? { backColor: parsedOrder.backColor } : {}),
  };
}

export function parseCompactSubpageConfig(
  value: string | null | undefined,
  typeFromCode: (code: string) => string,
): ParsedSubpageConfig {
  if (!value || value.length < 2) return { order: [], buttons: [], backLabel: "Back" };
  const parts = value.substring(1).split("|");
  const parsedOrder = parseSubpageOrder(parts[0] || "");
  const buttons: CardConfig[] = [];
  for (let i = 1; i < parts.length; i += 1) {
    const fields = (parts[i] || "").split(",");
    buttons.push({
      type: typeFromCode(fields[0] || ""),
      entity: decodeConfigField(fields[1]),
      label: decodeConfigField(fields[2]),
      icon: decodeConfigField(fields[3]) || "Auto",
      icon_on: decodeConfigField(fields[4]) || "Auto",
      sensor: decodeConfigField(fields[5]),
      unit: decodeConfigField(fields[6]),
      precision: decodeConfigField(fields[7]),
      options: decodeConfigField(fields[8]),
    });
  }
  return {
    order: parsedOrder.order,
    buttons,
    backLabel: parsedOrder.backLabel,
    ...(parsedOrder.backColor ? { backColor: parsedOrder.backColor } : {}),
  };
}

export function parseRawSubpageConfig(
  value: string | null | undefined,
  typeFromCode: (code: string) => string,
): ParsedSubpageConfig {
  if (value && value.charAt(0) === "~") return parseCompactSubpageConfig(value, typeFromCode);
  return parseLegacySubpageConfig(value);
}

export function structuredSubpageFromParsed(
  subpage: ParsedSubpageConfig | null | undefined,
): StructuredSubpageConfig {
  const backColor = normalizeBackColor(subpage?.backColor ?? parseSubpageOrder((subpage?.order || []).join(",")).backColor);
  return {
    ...(backColor ? { back_color: backColor } : {}),
    order: (subpage?.order || []).map((item) => parseBackOrderToken(item).token),
    back_label: subpage?.backLabel || backLabelFromOrder(subpage?.order) || "Back",
    buttons: (subpage?.buttons || []).map((button) => cloneCardConfig(button)),
  };
}

export function parseStructuredSubpageConfig(value: unknown): ParsedSubpageConfig {
  if (!isRecord(value)) return { order: [], buttons: [], backLabel: "Back" };

  const orderValues = Array.isArray(value.order)
    ? value.order.map((item) => String(item || ""))
    : [];
  const parsedOrder = parseSubpageOrder(orderValues.join(","));
  const backLabel = stringField(value, "back_label", stringField(value, "backLabel", parsedOrder.backLabel));
  const backColor = normalizeBackColor(value.back_color ?? value.backColor ?? parsedOrder.backColor);
  const rawButtons = Array.isArray(value.buttons) ? value.buttons : [];
  const buttons = rawButtons.map((button) => {
    const record = isRecord(button) ? button : {};
    return cloneCardConfig({
      entity: stringField(record, "entity"),
      label: stringField(record, "label"),
      icon: stringField(record, "icon", "Auto"),
      icon_on: stringField(record, "icon_on", "Auto"),
      sensor: stringField(record, "sensor"),
      unit: stringField(record, "unit"),
      type: stringField(record, "type"),
      precision: stringField(record, "precision"),
      options: stringField(record, "options"),
    });
  });

  return {
    order: parsedOrder.order,
    buttons,
    backLabel: backLabel || "Back",
    ...(backColor ? { backColor } : {}),
  };
}

export function legacySubpageFieldsSafe(buttonFields: readonly (readonly string[])[]): boolean {
  for (const fields of buttonFields) {
    for (const field of fields) {
      const value = String(field || "");
      if (value.indexOf("|") >= 0 || value.indexOf(":") >= 0) return false;
    }
  }
  return true;
}

export function serializeLegacySubpageConfig(
  order: readonly string[],
  buttonFields: readonly (readonly string[])[],
): string {
  if (!buttonFields.length) return order.join(",");
  let out = order.join(",");
  for (const fields of buttonFields) {
    out += "|" + fields.join(":");
  }
  return out;
}

export function serializeCompactSubpageConfig(
  order: readonly string[],
  buttonFields: readonly (readonly string[])[],
): string {
  if (!buttonFields.length) return "";
  let out = "~" + order.join(",");
  for (const fields of buttonFields) {
    out += "|" + fields.join(",");
  }
  return out;
}

export function chooseSerializedSubpageConfig(
  order: readonly string[],
  buttonCount: number,
  legacy: string,
  compact: string,
): string {
  if (!buttonCount && order.length > 0) return order.join(",");
  if (!compact) return legacy;
  if (!legacy) return compact;
  return compact.length < legacy.length ? compact : legacy;
}

function utf8ByteLength(str: string): number {
  let bytes = 0;
  for (let i = 0; i < str.length; i++) {
    const code = str.charCodeAt(i);
    if (code < 0x80) bytes += 1;
    else if (code >= 0xd800 && code <= 0xdbff) { bytes += 4; i += 1; } // surrogate pair → U+10000+
    else if (code < 0x800) bytes += 2;
    else bytes += 3;
  }
  return bytes;
}

export function splitSubpageConfigChunks(
  value: string | null | undefined,
  chunkCount: number,
  chunkSize = 255,
): string[] | null {
  const full = String(value || "");
  // Device text entities enforce max_length in BYTES, not characters.
  // Split on character boundaries so each chunk's UTF-8 byte count ≤ chunkSize.
  if (chunkCount < 1 || chunkSize < 1 || utf8ByteLength(full) > chunkCount * chunkSize) return null;
  const chunks: string[] = [];
  let charPos = 0;
  for (let i = 0; i < chunkCount; i += 1) {
    let bytes = 0;
    let end = charPos;
    while (end < full.length) {
      const code = full.charCodeAt(end);
      const charBytes =
        code < 0x80 ? 1 :
        (code >= 0xd800 && code <= 0xdbff) ? 4 :
        code < 0x800 ? 2 : 3;
      if (bytes + charBytes > chunkSize) break;
      bytes += charBytes;
      end += code >= 0xd800 && code <= 0xdbff ? 2 : 1; // surrogate pairs use 2 JS chars
    }
    chunks.push(full.substring(charPos, end));
    charPos = end;
  }
  // UTF-8 boundaries can leave unused bytes in each chunk. A total byte
  // count alone does not guarantee that the complete value fitted.
  return charPos === full.length ? chunks : null;
}

export function buildSubpageGrid(
  subpage: SubpageGridSource,
  maxSlots: number,
  gridCols: number,
): { grid: number[]; sizes: SlotSizeMap } {
  const grid = Array<number>(maxSlots).fill(0);
  const sizes: SlotSizeMap = { ...(subpage.sizes || {}) };
  const order = subpage.order || [];
  const buttonCount = (subpage.buttons || []).length;
  if (order.length > 0) {
    const hasBack = order.some((item) => isBackOrderToken(parseBackOrderToken(item).token));
    if (hasBack) {
      for (let i = 0; i < order.length && i < maxSlots; i += 1) {
        const token = parseBackOrderToken(order[i]).token;
        if (!token) continue;
        if (isBackOrderToken(token)) {
          grid[i] = -2;
          const backSize = sizeFromToken(token.charAt(1));
          if (backSize > 1) sizes[String(-2)] = backSize;
          else delete sizes[String(-2)];
          continue;
        }
        const last = token.charAt(token.length - 1);
        const parsedSize = sizeFromToken(last);
        const slot = parseInt(token, 10);
        if (slot >= 1 && slot <= buttonCount && !Number.isNaN(slot)) {
          grid[i] = slot;
          if (parsedSize > 1) sizes[String(slot)] = parsedSize;
        }
      }
    } else {
      grid[0] = -2;
      delete sizes[String(-2)];
      for (let i = 0; i < order.length && i + 1 < maxSlots; i += 1) {
        const token = parseBackOrderToken(order[i]).token;
        if (!token) continue;
        const last = token.charAt(token.length - 1);
        const parsedSize = sizeFromToken(last);
        const slot = parseInt(token, 10);
        if (slot >= 1 && slot <= buttonCount && !Number.isNaN(slot)) {
          grid[i + 1] = slot;
          if (parsedSize > 1) sizes[String(slot)] = parsedSize;
        }
      }
    }
  } else {
    grid[0] = -2;
    delete sizes[String(-2)];
  }
  applySpans(grid, sizes, maxSlots, gridCols);
  return { grid, sizes };
}

export function serializeSubpageGrid(
  grid: readonly number[],
  sizes: SlotSizeMap,
  backLabel?: string | null,
  backColor?: string | null,
): string[] {
  let last = -1;
  for (let i = grid.length - 1; i >= 0; i -= 1) {
    const slot = grid[i] ?? 0;
    if (slot > 0 || slot === -2) {
      last = i;
      break;
    }
  }
  if (last < 0) return [];
  const order: string[] = [];
  for (let i = 0; i <= last; i += 1) {
    const slot = grid[i] ?? 0;
    if (slot === -2) {
      order.push(backOrderToken("B" + sizeToken(sizes[String(-2)]), backLabel || "Back", backColor));
    } else if (slot <= 0) {
      order.push("");
    } else {
      order.push(String(slot) + sizeToken(sizes[String(slot)]));
    }
  }
  return order;
}
