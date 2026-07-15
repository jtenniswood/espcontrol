import type { CardConfig } from "../contracts/types";
import { CARD_CONFIG_FIELDS, cloneCardConfig } from "./card";
import { CARD_SIZE_DEFINITIONS } from "./grid";
import {
  parseStructuredSubpageConfig,
  structuredSubpageFromParsed,
  type StructuredSubpageConfig,
} from "./subpage";

export const CARD_TRANSFER_VERSION = 1;
export const CARD_TRANSFER_FORMAT = "espcontrol.cards";
export const CARD_TRANSFER_MAX_BYTES = 64 * 1024;
export const CARD_TRANSFER_MAX_CARDS = 20;

export interface CardTransferSource {
  device: string;
  firmware: string;
}

export interface CardTransferEntry extends CardConfig {
  size: number;
  subpage?: StructuredSubpageConfig;
}

export interface CardTransferEnvelope {
  format: string;
  version: number;
  source: CardTransferSource;
  cards: CardTransferEntry[];
}

type CardTransferError = Error & { cardTransferMessage: string };

function transferError(message: string): CardTransferError {
  const err = new Error(message) as CardTransferError;
  err.cardTransferMessage = message;
  return err;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return !!value && typeof value === "object" && !Array.isArray(value);
}

function utf8ByteLength(value: string): number {
  let bytes = 0;
  for (let i = 0; i < value.length; i += 1) {
    const code = value.charCodeAt(i);
    if (code < 0x80) bytes += 1;
    else if (code >= 0xd800 && code <= 0xdbff) { bytes += 4; i += 1; }
    else if (code < 0x800) bytes += 2;
    else bytes += 3;
  }
  return bytes;
}

function requiredString(record: Record<string, unknown>, field: string, context: string): string {
  if (typeof record[field] !== "string") {
    throw transferError("Invalid card code - " + context + " has an invalid " + field + " field");
  }
  return record[field] as string;
}

function normalizeTransferCard(value: unknown, context: string): CardConfig {
  if (!isRecord(value)) throw transferError("Invalid card code - " + context + " must be an object");
  const fields: Partial<CardConfig> = {};
  for (const field of CARD_CONFIG_FIELDS) {
    fields[field] = requiredString(value, field, context);
  }
  return cloneCardConfig(fields);
}

function normalizeTransferSubpage(value: unknown, context: string): StructuredSubpageConfig {
  if (!isRecord(value)) throw transferError("Invalid card code - " + context + " must be an object");
  if (typeof value.back_label !== "string") {
    throw transferError("Invalid card code - " + context + " has an invalid back label");
  }
  if (!Array.isArray(value.buttons) || value.buttons.length > CARD_TRANSFER_MAX_CARDS) {
    throw transferError("Invalid card code - " + context + " has an invalid card list");
  }
  const buttons = value.buttons;
  if (!Array.isArray(value.order) || !value.order.every((item) => {
    if (typeof item !== "string") return false;
    if (!item) return true;
    if (/^B(?:d|w|b|t|x)?$/.test(item)) return true;
    const match = /^(\d+)(?:d|w|b|t|x|q)?$/.exec(item);
    return !!match && Number(match[1]) >= 1 && Number(match[1]) <= buttons.length;
  })) {
    throw transferError("Invalid card code - " + context + " has an invalid order");
  }
  buttons.forEach((button, index) => {
    normalizeTransferCard(button, context + " card " + (index + 1));
  });
  return structuredSubpageFromParsed(parseStructuredSubpageConfig(value));
}

function normalizeTransferEntry(value: unknown, index: number): CardTransferEntry {
  const context = "card " + (index + 1);
  if (!isRecord(value)) throw transferError("Invalid card code - " + context + " must be an object");
  const card = normalizeTransferCard(value, context);
  if (!Number.isInteger(value.size) ||
      !CARD_SIZE_DEFINITIONS.some((definition) => definition.size === value.size)) {
    throw transferError("Invalid card code - " + context + " has an invalid size");
  }
  const entry: CardTransferEntry = { ...card, size: value.size as number };
  if (Object.prototype.hasOwnProperty.call(value, "subpage")) {
    if (card.type !== "subpage") {
      throw transferError("Invalid card code - only a Subpage card can contain a subpage");
    }
    entry.subpage = normalizeTransferSubpage(value.subpage, context + " subpage");
  }
  return entry;
}

export function normalizeCardTransferEnvelope(value: unknown): CardTransferEnvelope {
  if (!isRecord(value)) throw transferError("Invalid card code - expected a JSON object");
  const version = Number(value.version);
  if (!Number.isInteger(version) || version < 1) {
    throw transferError("Invalid card code - missing a supported version");
  }
  if (version > CARD_TRANSFER_VERSION) {
    throw transferError("Card code was created by a newer version of EspControl");
  }
  if (value.format !== CARD_TRANSFER_FORMAT) {
    throw transferError("Invalid card code - unsupported format");
  }
  if (!isRecord(value.source) || typeof value.source.device !== "string" ||
      typeof value.source.firmware !== "string") {
    throw transferError("Invalid card code - missing source information");
  }
  if (!Array.isArray(value.cards) || value.cards.length < 1) {
    throw transferError("Invalid card code - no cards were found");
  }
  if (value.cards.length > CARD_TRANSFER_MAX_CARDS) {
    throw transferError("Card code contains too many cards");
  }
  return {
    format: CARD_TRANSFER_FORMAT,
    version: CARD_TRANSFER_VERSION,
    source: {
      device: value.source.device,
      firmware: value.source.firmware,
    },
    cards: value.cards.map(normalizeTransferEntry),
  };
}

export function createCardTransferCode(
  source: CardTransferSource,
  cards: readonly CardTransferEntry[],
): string {
  const envelope = normalizeCardTransferEnvelope({
    format: CARD_TRANSFER_FORMAT,
    version: CARD_TRANSFER_VERSION,
    source,
    cards,
  });
  const code = JSON.stringify(envelope);
  if (utf8ByteLength(code) > CARD_TRANSFER_MAX_BYTES) {
    throw transferError("Card code is too large to copy");
  }
  return code;
}

export function parseCardTransferCode(code: string): CardTransferEnvelope {
  const text = String(code || "").trim();
  if (!text) throw transferError("Paste a card code first");
  if (utf8ByteLength(text) > CARD_TRANSFER_MAX_BYTES) {
    throw transferError("Card code is too large to import");
  }
  let parsed: unknown;
  try {
    parsed = JSON.parse(text);
  } catch (_) {
    throw transferError("Invalid card code - could not read the JSON");
  }
  return normalizeCardTransferEnvelope(parsed);
}
