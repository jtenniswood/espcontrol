/** Versioned native document for on-device panel configuration. */
export const PANEL_CONFIG_DOCUMENT_VERSION = 1;
export const PANEL_CONFIG_HEADER_SIZE = 16;
export const PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES = 64;
export const PANEL_CONFIG_MAX_RECORD_BODY_BYTES = 2048;
export const PANEL_CONFIG_MAX_RECORD_COUNT = 128;
export const PANEL_CONFIG_MAX_SLOT_COUNT = 32;
export const PANEL_CONFIG_MAX_SETTING_KEY_BYTES = 63;

const MAGIC = [0x45, 0x50, 0x43, 0x46]; // EPCF

enum RecordType { DeviceProfile = 1, Button = 2, Subpage = 3, Setting = 4 }

export interface PanelConfigDocument {
  deviceProfile: string;
  buttons: Record<number, string>;
  subpages: Record<number, string>;
  settings: Record<string, string>;
}

/** Readable JSON representation of a native PanelConfig document in a backup. */
export interface PanelConfigBackupPayload {
  document_version: number;
  device_profile: string;
  payload: string;
}

export class PanelConfigError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "PanelConfigError";
  }
}

function fail(message: string): never { throw new PanelConfigError(message); }

const BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

function encodeBase64(input: Uint8Array): string {
  let output = "";
  for (let offset = 0; offset < input.length; offset += 3) {
    const first = input[offset]!;
    const second = input[offset + 1];
    const third = input[offset + 2];
    output += BASE64_ALPHABET[first >>> 2]!;
    output += BASE64_ALPHABET[((first & 0x03) << 4) | ((second || 0) >>> 4)]!;
    output += second === undefined ? "=" : BASE64_ALPHABET[((second & 0x0f) << 2) | ((third || 0) >>> 6)]!;
    output += third === undefined ? "=" : BASE64_ALPHABET[third & 0x3f]!;
  }
  return output;
}

function decodeBase64(value: unknown): Uint8Array {
  if (typeof value !== "string" || value.length === 0 || value.length % 4 !== 0 ||
      !/^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$/.test(value)) {
    fail("PanelConfig backup payload is not valid base64");
  }
  const padding = value.endsWith("==") ? 2 : value.endsWith("=") ? 1 : 0;
  const output = new Uint8Array((value.length / 4) * 3 - padding);
  let writeOffset = 0;
  for (let offset = 0; offset < value.length; offset += 4) {
    const first = BASE64_ALPHABET.indexOf(value[offset]!);
    const second = BASE64_ALPHABET.indexOf(value[offset + 1]!);
    const third = value[offset + 2] === "=" ? 0 : BASE64_ALPHABET.indexOf(value[offset + 2]!);
    const fourth = value[offset + 3] === "=" ? 0 : BASE64_ALPHABET.indexOf(value[offset + 3]!);
    output[writeOffset++] = (first << 2) | (second >>> 4);
    if (value[offset + 2] !== "=") output[writeOffset++] = ((second & 0x0f) << 4) | (third >>> 2);
    if (value[offset + 3] !== "=") output[writeOffset++] = ((third & 0x03) << 6) | fourth;
  }
  return output;
}

function encodeString(value: string, label: string, maxLength: number, allowEmpty = true): Uint8Array {
  if (typeof value !== "string" || (!allowEmpty && value.length === 0)) {
    fail(`${label} must be ${allowEmpty ? "a string" : "a non-empty string"}`);
  }
  const encoded = new TextEncoder().encode(value);
  if ((!allowEmpty && encoded.length === 0) || encoded.length > maxLength) {
    fail(`${label} exceeds its supported byte length`);
  }
  return encoded;
}

function writeU16(output: Uint8Array, offset: number, value: number): void {
  output[offset] = value & 0xff;
  output[offset + 1] = (value >>> 8) & 0xff;
}

function writeU32(output: Uint8Array, offset: number, value: number): void {
  output[offset] = value & 0xff;
  output[offset + 1] = (value >>> 8) & 0xff;
  output[offset + 2] = (value >>> 16) & 0xff;
  output[offset + 3] = (value >>> 24) & 0xff;
}

function readU16(input: Uint8Array, offset: number): number {
  return input[offset]! | (input[offset + 1]! << 8);
}

function readU32(input: Uint8Array, offset: number): number {
  return (input[offset]! | (input[offset + 1]! << 8) | (input[offset + 2]! << 16) |
    (input[offset + 3]! << 24)) >>> 0;
}

function sortedSlotEntries(values: Record<number, string>, label: string): Array<[number, string]> {
  if (!values || typeof values !== "object" || Array.isArray(values)) fail(`${label} must be an object`);
  const slots = new Set<number>();
  const entries = Object.entries(values).map(([rawSlot, value]) => {
    const slot = Number(rawSlot);
    if (!Number.isInteger(slot) || slot < 1 || slot > PANEL_CONFIG_MAX_SLOT_COUNT) fail(`${label} has an invalid slot`);
    if (typeof value !== "string") fail(`${label} values must be strings`);
    if (slots.has(slot)) fail(`${label} has duplicate slots`);
    slots.add(slot);
    return [slot, value] as [number, string];
  });
  entries.sort(([left], [right]) => left - right);
  return entries;
}

function sortedSettingEntries(values: Record<string, string>): Array<[string, string]> {
  if (!values || typeof values !== "object" || Array.isArray(values)) fail("settings must be an object");
  const entries = Object.entries(values);
  for (const [key, value] of entries) {
    encodeString(key, "setting key", PANEL_CONFIG_MAX_SETTING_KEY_BYTES, false);
    if (typeof value !== "string") fail("setting values must be strings");
  }
  entries.sort(([left], [right]) => left.localeCompare(right));
  return entries;
}

function appendRecord(records: Uint8Array[], type: RecordType, body: Uint8Array): void {
  if (body.length > PANEL_CONFIG_MAX_RECORD_BODY_BYTES) fail("configuration record exceeds its supported byte length");
  const record = new Uint8Array(3 + body.length);
  record.set(Uint8Array.of(type, body.length & 0xff, body.length >>> 8));
  record.set(body, 3);
  records.push(record);
}

export function encodePanelConfig(document: PanelConfigDocument): Uint8Array {
  const deviceProfile = encodeString(document?.deviceProfile, "device profile", PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES, false);
  const records: Uint8Array[] = [];
  appendRecord(records, RecordType.DeviceProfile, deviceProfile);
  for (const [slot, value] of sortedSlotEntries(document.buttons, "buttons")) {
    const encoded = encodeString(value, "button configuration", PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1);
    appendRecord(records, RecordType.Button, Uint8Array.of(slot, ...encoded));
  }
  for (const [slot, value] of sortedSlotEntries(document.subpages, "subpages")) {
    const encoded = encodeString(value, "subpage configuration", PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1);
    appendRecord(records, RecordType.Subpage, Uint8Array.of(slot, ...encoded));
  }
  for (const [key, value] of sortedSettingEntries(document.settings)) {
    const encodedKey = encodeString(key, "setting key", PANEL_CONFIG_MAX_SETTING_KEY_BYTES, false);
    const encodedValue = encodeString(value, "setting value", PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1 - encodedKey.length);
    appendRecord(records, RecordType.Setting, Uint8Array.of(encodedKey.length, ...encodedKey, ...encodedValue));
  }
  if (records.length > PANEL_CONFIG_MAX_RECORD_COUNT) fail("configuration contains too many records");
  const payloadLength = records.reduce((total, record) => total + record.length, 0);
  const output = new Uint8Array(PANEL_CONFIG_HEADER_SIZE + payloadLength);
  output.set(MAGIC, 0);
  writeU16(output, 4, PANEL_CONFIG_DOCUMENT_VERSION);
  writeU16(output, 6, PANEL_CONFIG_HEADER_SIZE);
  writeU32(output, 8, payloadLength);
  writeU16(output, 12, records.length);
  let offset = PANEL_CONFIG_HEADER_SIZE;
  for (const record of records) { output.set(record, offset); offset += record.length; }
  return output;
}

function decodeString(bytes: Uint8Array, label: string, allowEmpty = true): string {
  if (!allowEmpty && bytes.length === 0) fail(`${label} must not be empty`);
  try { return new TextDecoder("utf-8", { fatal: true }).decode(bytes); } catch { return fail(`${label} is not valid UTF-8`); }
}

export function decodePanelConfig(input: Uint8Array): PanelConfigDocument {
  if (!(input instanceof Uint8Array) || input.length < PANEL_CONFIG_HEADER_SIZE ||
    MAGIC.some((value, index) => input[index] !== value) || readU16(input, 4) !== PANEL_CONFIG_DOCUMENT_VERSION ||
    readU16(input, 6) !== PANEL_CONFIG_HEADER_SIZE || readU16(input, 14) !== 0 ||
    readU32(input, 8) !== input.length - PANEL_CONFIG_HEADER_SIZE) fail("invalid PanelConfig document header");
  const recordCount = readU16(input, 12);
  if (recordCount > PANEL_CONFIG_MAX_RECORD_COUNT) fail("PanelConfig contains too many records");
  const result: PanelConfigDocument = {
    deviceProfile: "",
    buttons: {},
    subpages: {},
    settings: Object.create(null) as Record<string, string>,
  };
  let offset = PANEL_CONFIG_HEADER_SIZE;
  for (let recordIndex = 0; recordIndex < recordCount; recordIndex += 1) {
    if (offset + 3 > input.length) fail("truncated PanelConfig record");
    const type = input[offset]!;
    const bodyLength = readU16(input, offset + 1);
    offset += 3;
    if (bodyLength > PANEL_CONFIG_MAX_RECORD_BODY_BYTES || offset + bodyLength > input.length) fail("invalid PanelConfig record length");
    const body = input.slice(offset, offset + bodyLength);
    offset += bodyLength;
    if (type === RecordType.DeviceProfile) {
      if (result.deviceProfile || body.length === 0 || body.length > PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES) fail("invalid device profile record");
      result.deviceProfile = decodeString(body, "device profile", false);
    } else if (type === RecordType.Button || type === RecordType.Subpage) {
      const slot = body[0] ?? 0;
      if (body.length < 1 || slot < 1 || slot > PANEL_CONFIG_MAX_SLOT_COUNT) fail("invalid slot record");
      const target = type === RecordType.Button ? result.buttons : result.subpages;
      if (Object.prototype.hasOwnProperty.call(target, slot)) fail("duplicate slot record");
      target[slot] = decodeString(body.slice(1), "slot configuration");
    } else if (type === RecordType.Setting) {
      const keyLength = body[0] ?? 0;
      if (body.length < 2 || keyLength === 0 || keyLength > PANEL_CONFIG_MAX_SETTING_KEY_BYTES || keyLength >= body.length) fail("invalid setting record");
      const key = decodeString(body.slice(1, 1 + keyLength), "setting key", false);
      if (Object.prototype.hasOwnProperty.call(result.settings, key)) fail("duplicate setting record");
      result.settings[key] = decodeString(body.slice(1 + keyLength), "setting value");
    } else {
      fail("unsupported PanelConfig record type");
    }
  }
  if (offset !== input.length || !result.deviceProfile) fail("incomplete PanelConfig document");
  return result;
}

export function createPanelConfigBackupPayload(document: Uint8Array): PanelConfigBackupPayload {
  const decoded = decodePanelConfig(document);
  return {
    document_version: PANEL_CONFIG_DOCUMENT_VERSION,
    device_profile: decoded.deviceProfile,
    payload: encodeBase64(document),
  };
}

export function decodePanelConfigBackupPayload(value: unknown): Uint8Array {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    fail("PanelConfig backup section is invalid");
  }
  const payload = value as Partial<PanelConfigBackupPayload>;
  if (payload.document_version !== PANEL_CONFIG_DOCUMENT_VERSION ||
      typeof payload.device_profile !== "string" || payload.device_profile.length === 0) {
    fail("PanelConfig backup version or device profile is invalid");
  }
  const document = decodeBase64(payload.payload);
  const decoded = decodePanelConfig(document);
  if (decoded.deviceProfile !== payload.device_profile) {
    fail("PanelConfig backup device profile does not match its document");
  }
  return document;
}
