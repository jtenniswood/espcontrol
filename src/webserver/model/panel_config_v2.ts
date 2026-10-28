/** Structural native-v2 codec. Not yet advertised or used for saves/backups. */
import { PanelConfigError } from "./panel_config";

export enum NativeV2Kind {
  Profile = 1, String, Entity, Page, Card, Navigation, Setting,
}

/** IDs are scoped by kind; first/second/third follow compatibility/native-document-v2.md. */
export interface NativeV2Record {
  kind: NativeV2Kind;
  id?: number;
  first?: number;
  second?: number;
  third?: number;
  fieldMask?: number;
  flags?: number;
  fields?: readonly number[];
  rawSource?: number;
  text?: string;
}

export interface NativeV2DecodedRecord extends NativeV2Record {
  id: number;
  first: number;
  second: number;
  third: number;
  fieldMask: number;
  flags: number;
  fields: readonly number[];
  rawSource: number;
  text: string;
}

const NO_ID = 65535;
const HEADER = 16;
const MAX_BODY = 4097;
const encoder = new TextEncoder();
// UTF-8 BOM is configuration data, not a transport prefix.
const decoder = new TextDecoder("utf-8", { fatal: true, ignoreBOM: true });
const K = NativeV2Kind;
function fail(): never { throw new PanelConfigError("Invalid native-v2 document"); }
function requireValid(value: unknown): asserts value { if (!value) fail(); }
function identity(value: number): boolean { return value > 0 && value < NO_ID; }
function u16(view: DataView, offset: number): number { return view.getUint16(offset, true); }
function utf8(bytes: Uint8Array): string {
  try { return decoder.decode(bytes); } catch { return fail(); }
}
function popcount(value: number): number {
  let count = 0;
  for (; value; value &= value - 1) ++count;
  return count;
}

function decodeRecord(kind: number, body: Uint8Array): NativeV2DecodedRecord {
  const n = body.byteLength;
  requireValid(n <= MAX_BODY);
  const r: NativeV2DecodedRecord = {
    kind, id: 0, first: 0, second: 0, third: 0, fieldMask: 0,
    flags: 0, fields: Array<number>(9).fill(0), rawSource: 0, text: "",
  };
  if (kind === K.Profile) {
    requireValid(n > 0 && n <= 64);
    r.text = utf8(body);
    return r;
  }
  requireValid(n >= 2);
  const view = new DataView(body.buffer, body.byteOffset, n);
  r.id = u16(view, 0);
  if (kind === K.String) {
    requireValid(identity(r.id) && n > 2);
    r.text = utf8(body.subarray(2));
  } else if (kind === K.Entity || kind === K.Navigation || kind === K.Setting) {
    requireValid(n === 4 && identity(r.id));
    r.first = u16(view, 2);
    requireValid(r.first !== NO_ID);
  } else if (kind === K.Page) {
    requireValid(n === 9 && r.id !== NO_ID);
    r.first = u16(view, 2); r.second = u16(view, 4); r.third = u16(view, 6); r.flags = body[8]!;
    requireValid((r.flags & ~1) === 0 && r.third !== NO_ID);
  } else {
    requireValid(kind === K.Card && n >= 9 && identity(r.id));
    r.first = u16(view, 2); r.second = u16(view, 4); r.fieldMask = u16(view, 6); r.flags = body[8]!;
    requireValid(r.first !== NO_ID && r.second > 0 && (r.fieldMask & ~511) === 0 && (r.flags & ~1) === 0);
    requireValid(n === 9 + 2 * popcount(r.fieldMask) + (r.flags ? 2 : 0));
    let offset = 9;
    const fields = [...r.fields];
    for (let i = 0; i < 9; ++i) {
      if (!(r.fieldMask & (1 << i))) continue;
      fields[i] = u16(view, offset); offset += 2;
      requireValid(identity(fields[i]!));
    }
    r.fields = fields;
    if (r.flags) { r.rawSource = u16(view, offset); requireValid(r.rawSource !== NO_ID); }
  }
  return r;
}

function validateGraph(records: readonly NativeV2DecodedRecord[]): void {
  const index = new Map<string, NativeV2DecodedRecord>();
  const key = (kind: number, id: number) => `${kind}:${id}`;
  for (const r of records) {
    const k = key(r.kind, r.id);
    requireValid(!index.has(k)); index.set(k, r);
  }
  const get = (kind: number, id: number) => index.get(key(kind, id));
  const text = (id: number) => get(K.String, id)?.text ?? "";
  const optionalString = (id: number) => id === 0 || !!get(K.String, id);
  const root = get(K.Page, 0);
  requireValid(get(K.Profile, 0) && root && root.first === NO_ID && root.second === NO_ID && root.flags === 0);
  const owners = new Set<number>();
  const positions = new Set<string>();
  const settings = new Set<string>();
  for (const r of records) {
    switch (r.kind) {
      case K.Entity:
        requireValid(r.first && get(K.String, r.first));
        break;
      case K.Page: {
        requireValid(optionalString(r.third));
        if (r.id) {
          const owner = get(K.Card, r.second);
          requireValid(r.first !== NO_ID && r.first !== r.id && get(K.Page, r.first) && owner?.first === r.first);
        }
        requireValid(!owners.has(r.second)); owners.add(r.second);
        let id = r.id;
        for (let depth = 0; id; ++depth) {
          const page = get(K.Page, id);
          requireValid(depth < 2 && page); id = page.first;
        }
        break;
      }
      case K.Card: {
        requireValid(get(K.Page, r.first) && optionalString(r.rawSource));
        const position = `${r.first}:${r.second}`;
        requireValid(!positions.has(position)); positions.add(position);
        r.fields.forEach((id, field) => {
          requireValid(!id || get(field === 0 || field === 4 ? K.Entity : K.String, id));
        });
        break;
      }
      case K.Navigation: {
        const owner = get(K.Card, r.id), page = get(K.Page, r.first);
        requireValid(r.first && owner && page && page.second === owner.id &&
          page.first === owner.first && text(owner.fields[6]!) === "subpage");
        break;
      }
      case K.Setting: {
        const name = get(K.String, r.id);
        requireValid(name && encoder.encode(name.text).length <= 63 && optionalString(r.first));
        requireValid(!settings.has(name.text)); settings.add(name.text);
        break;
      }
    }
  }
}

/** Decode into owned values so later mutation of the input cannot change results. */
export function decodeNativeV2(input: Uint8Array): NativeV2DecodedRecord[] {
  requireValid(input instanceof Uint8Array && input.length >= HEADER);
  const view = new DataView(input.buffer, input.byteOffset, input.byteLength);
  requireValid(input[0] === 69 && input[1] === 80 && input[2] === 67 && input[3] === 70 &&
    u16(view, 4) === 2 && u16(view, 6) === HEADER && u16(view, 14) === 0 &&
    view.getUint32(8, true) === input.length - HEADER);
  const count = u16(view, 12), payloadSize = input.length - HEADER;
  requireValid(count >= 2 && payloadSize >= 16 && count <= (payloadSize - 4) / 6);
  const records: NativeV2DecodedRecord[] = [];
  let offset = HEADER;
  for (let i = 0; i < count; ++i) {
    requireValid(input.length - offset >= 3);
    const kind = input[offset]!, size = u16(view, offset + 1); offset += 3;
    requireValid(size <= input.length - offset);
    records.push(decodeRecord(kind, input.subarray(offset, offset + size))); offset += size;
  }
  requireValid(offset === input.length);
  validateGraph(records);
  return records.sort((a, b) => a.kind - b.kind || a.id - b.id);
}

const RECORD_KEYS = new Set(["kind", "id", "first", "second", "third", "fieldMask", "flags", "fields", "rawSource", "text"]);
function encodeRecord(input: NativeV2Record): Uint8Array {
  requireValid(input && typeof input === "object" && !Array.isArray(input) &&
    Object.keys(input).every((key) => RECORD_KEYS.has(key)));
  const numbers = [input.kind, input.id ?? 0, input.first ?? 0, input.second ?? 0,
    input.third ?? 0, input.fieldMask ?? 0, input.flags ?? 0, input.rawSource ?? 0];
  requireValid(numbers.every((v) => Number.isInteger(v) && v >= 0 && v <= NO_ID));
  const [kind, id, first, second, third, mask, flags, raw] = numbers as [number, number, number, number, number, number, number, number];
  const fields = input.fields === undefined ? Array<number>(9).fill(0) : input.fields;
  requireValid(Array.isArray(fields) && fields.length === 9 && fields.every((v) => Number.isInteger(v) && v >= 0 && v <= NO_ID));
  const text = input.text ?? "";
  requireValid(typeof text === "string");
  const bytes = encoder.encode(text);
  // TextEncoder otherwise replaces lone surrogates and silently loses input.
  requireValid(utf8(bytes) === text);
  let size = 0;
  if (kind === K.Profile) size = bytes.length;
  else if (kind === K.String) size = 2 + bytes.length;
  else if (kind === K.Entity || kind === K.Navigation || kind === K.Setting) size = 4;
  else if (kind === K.Page) size = 9;
  else if (kind === K.Card) size = 9 + 2 * popcount(mask) + (flags ? 2 : 0);
  else fail();
  requireValid(size <= MAX_BODY);
  const body = new Uint8Array(size), view = new DataView(body.buffer);
  const put = (offset: number, value: number) => view.setUint16(offset, value, true);
  if (kind === K.Profile) body.set(bytes);
  else {
    requireValid(size >= 2); put(0, id);
    if (kind === K.String) body.set(bytes, 2);
    else {
      put(2, first);
      if (kind === K.Page || kind === K.Card) {
        put(4, second); put(6, kind === K.Page ? third : mask); body[8] = flags;
        if (kind === K.Card) {
          let offset = 9;
          for (let i = 0; i < 9; ++i) {
            if (!(mask & (1 << i))) continue;
            put(offset, fields[i]!); offset += 2;
          }
          if (flags) put(offset, raw);
        }
      }
    }
  }
  const decoded = decodeRecord(kind, body);
  // Reject values that do not belong to this record shape, rather than drop them.
  requireValid(decoded.id === id && decoded.first === first && decoded.second === second &&
    decoded.third === third && decoded.fieldMask === mask && decoded.flags === flags &&
    decoded.rawSource === raw && decoded.text === text && decoded.fields.every((v, i) => v === fields[i]));
  return body;
}

/** Validate before returning a complete document; v1 remains the live default. */
export function encodeNativeV2(records: readonly NativeV2Record[]): Uint8Array {
  requireValid(Array.isArray(records) && records.length >= 2 && records.length <= NO_ID);
  const entries = Array.from(records, (record) => ({ record, body: encodeRecord(record) }))
    .sort((a, b) => a.record.kind - b.record.kind || (a.record.id ?? 0) - (b.record.id ?? 0));
  const bodies = entries.map((entry) => entry.body);
  const size = bodies.reduce((sum, b) => sum + 3 + b.length, HEADER);
  requireValid(Number.isSafeInteger(size) && size - HEADER <= 0xffffffff);
  const output = new Uint8Array(size), view = new DataView(output.buffer);
  output.set([69, 80, 67, 70]); view.setUint16(4, 2, true); view.setUint16(6, HEADER, true);
  view.setUint32(8, size - HEADER, true); view.setUint16(12, records.length, true);
  let offset = HEADER;
  bodies.forEach((body, i) => {
    output[offset] = entries[i]!.record.kind; view.setUint16(offset + 1, body.length, true);
    output.set(body, offset + 3); offset += 3 + body.length;
  });
  decodeNativeV2(output);
  return output;
}
