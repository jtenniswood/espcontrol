import type { CardConfig, SavedConfigField } from "../contracts/types";

export type DraftCardConfig = CardConfig & {
  _whenOnActive?: unknown;
  _whenOnMode?: unknown;
};

export const CARD_CONFIG_FIELDS: readonly SavedConfigField[] = [
  "entity",
  "label",
  "icon",
  "icon_on",
  "sensor",
  "unit",
  "type",
  "precision",
  "options",
];

export function emptyCardConfig(type?: string): CardConfig {
  return {
    entity: "",
    label: "",
    icon: "Auto",
    icon_on: "Auto",
    sensor: "",
    unit: "",
    type: type || "",
    precision: "",
    options: "",
  };
}

export function cloneCardConfig(src?: Partial<CardConfig> & Partial<DraftCardConfig>): DraftCardConfig {
  const button: DraftCardConfig = {
    entity: src?.entity || "",
    label: src?.label || "",
    icon: src?.icon || "Auto",
    icon_on: src?.icon_on || "Auto",
    sensor: src?.sensor || "",
    unit: src?.unit || "",
    type: src?.type || "",
    precision: src?.precision || "",
    options: src?.options || "",
  };
  if (src && Object.prototype.hasOwnProperty.call(src, "_whenOnActive")) {
    button._whenOnActive = src._whenOnActive;
  }
  if (src && Object.prototype.hasOwnProperty.call(src, "_whenOnMode")) {
    button._whenOnMode = src._whenOnMode;
  }
  return button;
}

export function copyCardConfig(
  target: Partial<CardConfig> & Partial<DraftCardConfig>,
  src?: Partial<CardConfig> & Partial<DraftCardConfig>,
): DraftCardConfig {
  const button = cloneCardConfig(src);
  for (const field of CARD_CONFIG_FIELDS) {
    target[field] = button[field];
  }
  target._whenOnActive = button._whenOnActive;
  target._whenOnMode = button._whenOnMode;
  return target as DraftCardConfig;
}

export function cardConfigChanged(before: Partial<CardConfig>, after: Partial<CardConfig>): boolean {
  for (const field of CARD_CONFIG_FIELDS) {
    if ((before[field] || "") !== (after[field] || "")) return true;
  }
  return false;
}

export function trimConfigFields(fields: string[]): string[] {
  while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
  return fields;
}

export function encodeConfigField(value: string | number | boolean | null | undefined): string {
  return String(value || "").replace(/[%,;|:]/g, (ch) => {
    const hex = ch.charCodeAt(0).toString(16).toUpperCase();
    return "%" + (hex.length < 2 ? "0" : "") + hex;
  });
}

export function decodeConfigField(value: string | number | boolean | null | undefined): string {
  const str = String(value || "");
  // Decode %XX sequences as UTF-8 using decodeURIComponent, which correctly
  // handles multi-byte sequences like %C2%B0 → ° (instead of byte-by-byte
  // String.fromCharCode which produces Mojibake: Â°).
  return str.replace(/(%[0-9a-fA-F]{2})+/g, (run) => {
    try {
      return decodeURIComponent(run);
    } catch {
      return run;
    }
  });
}

export function legacyButtonConfigSafe(fields: readonly string[]): boolean {
  return fields.join(";").charAt(0) !== "~" && fields.every((field) => {
    return String(field || "").indexOf(";") < 0;
  });
}

export function parseRawButtonConfig(value: string | null | undefined): CardConfig {
  const compact = !!(value && value.charAt(0) === "~");
  const parts = compact ? value.substring(1).split(",") : (value || "").split(";");
  const decoded = compact ? parts.map(decodeConfigField) : parts;
  return {
    entity: decoded[0] || "",
    label: decoded[1] || "",
    icon: decoded[2] || "Auto",
    icon_on: decoded[3] || "Auto",
    sensor: decoded[4] || "",
    unit: decoded[5] || "",
    type: decoded[6] || "",
    precision: decoded[7] || "",
    options: decoded[8] || "",
  };
}
