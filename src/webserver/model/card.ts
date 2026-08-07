import type {
  CardAppearance,
  CardConfig,
  NormalizedCardConfig,
  SavedConfigField,
} from "../contracts/types";
import {
  configOptionValue,
  decodeConfigField,
  encodeConfigField,
  legacyButtonConfigSafe,
  setConfigOptionValue,
} from "./config_primitives";

export type DraftCardConfig = NormalizedCardConfig & {
  _whenOnActive?: unknown;
  _whenOnMode?: unknown;
};

export const CARD_BACKGROUND_IMAGE_OPTION = "bg_image";

export function normalizeCardBackgroundAssetId(value: unknown): string {
  const id = String(value || "").trim().toLowerCase();
  return /^[a-z0-9-]{1,40}$/.test(id) ? id : "";
}

function optionBackgroundAssetId(options: unknown): string {
  return normalizeCardBackgroundAssetId(configOptionValue(options, CARD_BACKGROUND_IMAGE_OPTION));
}

function attachAppearance<T extends CardConfig>(card: T, appearance: CardAppearance): T & NormalizedCardConfig {
  Object.defineProperty(card, "appearance", {
    configurable: true,
    enumerable: false,
    writable: true,
    value: appearance,
  });
  return card as T & NormalizedCardConfig;
}

export function cardBackgroundAssetId(card?: Partial<CardConfig> | null): string {
  if (!card) return "";
  if (card.appearance) return normalizeCardBackgroundAssetId(card.appearance.backgroundAssetId);
  return optionBackgroundAssetId(card.options);
}

export function cardOptionsWithAppearance(options: unknown, card?: Partial<CardConfig> | null): string {
  return setConfigOptionValue(options, CARD_BACKGROUND_IMAGE_OPTION, cardBackgroundAssetId(card));
}

export function setCardBackgroundAssetId<T extends Partial<CardConfig>>(
  card: T,
  value: unknown,
): string {
  const id = normalizeCardBackgroundAssetId(value);
  card.options = setConfigOptionValue(card.options, CARD_BACKGROUND_IMAGE_OPTION, id);
  if (card.appearance) {
    card.appearance.backgroundAssetId = id;
  } else {
    attachAppearance(card as T & CardConfig, { backgroundAssetId: id });
  }
  return card.options;
}

function normalizedAppearance(src?: Partial<CardConfig>): CardAppearance {
  const id = src?.appearance
    ? normalizeCardBackgroundAssetId(src.appearance.backgroundAssetId)
    : optionBackgroundAssetId(src?.options);
  return { backgroundAssetId: id };
}

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

export function emptyCardConfig(type?: string): NormalizedCardConfig {
  return attachAppearance({
    entity: "",
    label: "",
    icon: "Auto",
    icon_on: "Auto",
    sensor: "",
    unit: "",
    type: type || "",
    precision: "",
    options: "",
  }, { backgroundAssetId: "" });
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
    appearance: { backgroundAssetId: "" },
  };
  const appearance = normalizedAppearance(src);
  button.options = setConfigOptionValue(button.options, CARD_BACKGROUND_IMAGE_OPTION, appearance.backgroundAssetId);
  attachAppearance(button, appearance);
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
  attachAppearance(target as CardConfig, { backgroundAssetId: button.appearance.backgroundAssetId });
  target._whenOnActive = button._whenOnActive;
  target._whenOnMode = button._whenOnMode;
  return target as DraftCardConfig;
}

export function cardConfigChanged(before: Partial<CardConfig>, after: Partial<CardConfig>): boolean {
  for (const field of CARD_CONFIG_FIELDS) {
    if ((before[field] || "") !== (after[field] || "")) return true;
  }
  return cardBackgroundAssetId(before) !== cardBackgroundAssetId(after);
}

export function parseRawButtonConfig(value: string | null | undefined): NormalizedCardConfig {
  const compact = !!(value && value.charAt(0) === "~");
  const parts = compact ? value.substring(1).split(",") : (value || "").split(";");
  const decoded = compact ? parts.map(decodeConfigField) : parts;
  const card: CardConfig = {
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
  return attachAppearance(card, normalizedAppearance(card));
}
