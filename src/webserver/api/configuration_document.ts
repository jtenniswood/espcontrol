export type ConfigurationDomain = "text" | "select" | "number" | "switch";

export interface ConfigurationRecord {
  domain: ConfigurationDomain;
  objectId: string;
  value: string;
}

export interface ConfigurationDocument {
  records: ConfigurationRecord[];
}

const DOCUMENT_MAGIC = 0x31464345;
const DOCUMENT_FORMAT = 1;
const HEADER_SIZE = 8;
const RECORD_HEADER_SIZE = 4;

const DOMAIN_CODES: Readonly<Record<ConfigurationDomain, number>> = {
  text: 1,
  select: 2,
  number: 3,
  switch: 4,
};

export function configurationDomainCode(domain: ConfigurationDomain): number {
  return DOMAIN_CODES[domain];
}

function domainFromCode(code: number): ConfigurationDomain {
  if (code === 1) return "text";
  if (code === 2) return "select";
  if (code === 3) return "number";
  if (code === 4) return "switch";
  throw new Error("Unsupported configuration entity domain");
}

export function configurationRecordKey(domain: ConfigurationDomain, objectId: string): string {
  return `${domain}:${objectId}`;
}

export function decodeConfigurationDocument(input: ArrayBuffer | Uint8Array): ConfigurationDocument {
  const bytes = input instanceof Uint8Array ? input : new Uint8Array(input);
  if (bytes.byteLength < HEADER_SIZE) throw new Error("Configuration document is truncated");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (view.getUint32(0, true) !== DOCUMENT_MAGIC) throw new Error("Configuration document has an invalid marker");
  if (view.getUint16(4, true) !== DOCUMENT_FORMAT) throw new Error("Configuration document format is unsupported");
  const recordCount = view.getUint16(6, true);
  const decoder = new TextDecoder("utf-8", { fatal: true });
  const records: ConfigurationRecord[] = [];
  const identities = new Set<string>();
  let offset = HEADER_SIZE;
  for (let index = 0; index < recordCount; index += 1) {
    if (offset + RECORD_HEADER_SIZE > bytes.byteLength) throw new Error("Configuration record is truncated");
    const domain = domainFromCode(view.getUint8(offset));
    const objectIdSize = view.getUint8(offset + 1);
    const valueSize = view.getUint16(offset + 2, true);
    offset += RECORD_HEADER_SIZE;
    if (objectIdSize === 0 || offset + objectIdSize + valueSize > bytes.byteLength) {
      throw new Error("Configuration record has invalid lengths");
    }
    const objectId = decoder.decode(bytes.subarray(offset, offset + objectIdSize));
    offset += objectIdSize;
    const value = decoder.decode(bytes.subarray(offset, offset + valueSize));
    offset += valueSize;
    const key = configurationRecordKey(domain, objectId);
    if (identities.has(key)) throw new Error("Configuration document contains duplicate entities");
    identities.add(key);
    records.push({ domain, objectId, value });
  }
  if (offset !== bytes.byteLength) throw new Error("Configuration document contains trailing bytes");
  return { records };
}

export function encodeConfigurationDocument(document: ConfigurationDocument): Uint8Array {
  const encoder = new TextEncoder();
  if (document.records.length > 0xffff) throw new Error("Configuration document has too many records");
  const encoded = document.records.map((record) => {
    const objectId = encoder.encode(record.objectId);
    const value = encoder.encode(record.value);
    if (objectId.byteLength === 0 || objectId.byteLength > 0xff || value.byteLength > 0xffff) {
      throw new Error("Configuration record is too large");
    }
    return { record, objectId, value };
  });
  const size = encoded.reduce(
    (total, item) => total + RECORD_HEADER_SIZE + item.objectId.byteLength + item.value.byteLength,
    HEADER_SIZE,
  );
  const bytes = new Uint8Array(size);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, DOCUMENT_MAGIC, true);
  view.setUint16(4, DOCUMENT_FORMAT, true);
  view.setUint16(6, encoded.length, true);
  let offset = HEADER_SIZE;
  for (const item of encoded) {
    view.setUint8(offset, configurationDomainCode(item.record.domain));
    view.setUint8(offset + 1, item.objectId.byteLength);
    view.setUint16(offset + 2, item.value.byteLength, true);
    offset += RECORD_HEADER_SIZE;
    bytes.set(item.objectId, offset);
    offset += item.objectId.byteLength;
    bytes.set(item.value, offset);
    offset += item.value.byteLength;
  }
  return bytes;
}
