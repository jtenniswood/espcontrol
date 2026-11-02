/** Lossless bridge for negotiated editor saves and staged firmware compatibility. */
import { decodePanelConfig, encodePanelConfig, PanelConfigError, type PanelConfigDocument } from "./panel_config";
import { decodeNativeV2, encodeNativeV2, NativeV2Kind as K, type NativeV2Record, type NativeV2DecodedRecord } from "./panel_config_v2";
import { decodeConfigField, encodeConfigField } from "./config_primitives";
import { CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE, CARD_CONTRACT_SUBPAGE_TYPE_CODES } from "../generated/card_contract";

import { childPageKey, pageDepth } from "./page_keys";

const RAW = 1, ABSENT = 2, NO_ID = 65535;
function check(value: unknown, reason: string): asserts value {
  if (!value) throw new PanelConfigError(`Native configuration conversion: ${reason}`);
}
function mapped(table: Readonly<Record<string, string>>, key: string): string {
  return Object.hasOwn(table, key) ? table[key]! : key;
}
function fieldsFromRaw(raw: string, child: boolean, compact: boolean): string[] {
  const encoded = child ? compact : raw.startsWith("~");
  const parts = (child || !encoded ? raw : raw.slice(1)).split(encoded ? "," : child ? ":" : ";");
  const fields = Array<string>(9).fill("");
  if (child && encoded) {
    fields[6] = mapped(CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE, parts[0] ?? "");
    [0, 1, 2, 3, 4, 5, 7, 8].forEach((field, index) => { fields[field] = decodeConfigField(parts[index + 1]); });
  } else {
    for (let i = 0; i < 9; ++i) fields[i] = encoded ? decodeConfigField(parts[i]) : parts[i] ?? "";
  }
  // The existing subpage parser supplies these defaults before normalization.
  if (child) { fields[2] ||= "Auto"; fields[3] ||= "Auto"; }
  return fields;
}
function childDefaults(fields: readonly string[]): string[] {
  const copy = [...fields]; copy[2] ||= "Auto"; copy[3] ||= "Auto"; return copy;
}
function equalFields(a: readonly string[], b: readonly string[]): boolean {
  return a.length === 9 && b.length === 9 && a.every((value, i) => value === b[i]);
}
function trimmed(fields: readonly string[], separator: string): string {
  const result = [...fields];
  while (result.length > 1 && result.at(-1) === "") result.pop();
  return result.join(separator);
}
function fits(value: string): boolean { return new TextEncoder().encode(value).length <= 4095; }

/** Import exact v1 saved values into independent graph identities. */
export function convertNativeV1ToV2(input: Uint8Array): Uint8Array {
  return buildNativeGraph(decodePanelConfig(input), false);
}

/** Encode editable page paths directly; nested data never passes through v1. */
export function encodePanelPageDocument(source: PanelConfigDocument): Uint8Array {
  for (const key of Object.keys(source.subpages)) {
    check(/^[1-9][0-9]*$/.test(key) && pageDepth(Number(key)) > 0, "invalid page path");
  }
  // Retain deployed root/settings validation without applying v1's flat page
  // count or slot namespace to a nested graph.
  encodePanelConfig({ ...source, subpages: Object.fromEntries(Object.entries(source.subpages).filter(([key]) => Number(key) <= 32)) });
  return buildNativeGraph(source, true);
}

function buildNativeGraph(source: PanelConfigDocument, nested: boolean): Uint8Array {
  const records: NativeV2Record[] = [{ kind: K.Profile, text: source.deviceProfile }];
  const strings = new Map<string, number>(), entities = new Map<string, number>();
  function intern(text: string): number {
    if (!text) return 0;
    let id = strings.get(text);
    if (id === undefined) {
      id = strings.size + 1; check(id < NO_ID, "string dictionary is full"); strings.set(text, id);
      records.push({ kind: K.String, id, text });
    }
    return id;
  }
  function entity(text: string): number {
    if (!text) return 0;
    let id = entities.get(text);
    if (id === undefined) {
      id = entities.size + 1; check(id < NO_ID, "entity dictionary is full"); entities.set(text, id);
      records.push({ kind: K.Entity, id, first: intern(text) });
    }
    return id;
  }
  function card(id: number, page: number, slot: number, fields: string[], raw: string | undefined): void {
    const refs = fields.map((value, index) => index === 0 || index === 4 ? entity(value) : intern(value));
    const fieldMask = refs.reduce((mask, ref, index) => mask | (ref ? 1 << index : 0), 0);
    records.push({ kind: K.Card, id, first: page, second: slot, fields: refs, fieldMask,
      flags: raw === undefined ? ABSENT : RAW, rawSource: raw === undefined ? 0 : intern(raw) });
  }
  records.push({ kind: K.Page, id: 0, first: NO_ID, second: NO_ID,
    third: intern(Object.hasOwn(source.settings, "button_order") ? source.settings.button_order! : "") });
  const slots = [...new Set([...Object.keys(source.buttons), ...Object.keys(source.subpages).filter((key) => Number(key) <= 32)].map(Number))].sort((a, b) => a - b);
  let nextPage = 1, nextCard = 33;
  const visited = new Set<number>();
  function page(owner: number, parent: number, key: number, ownerFields: string[], depth: number): void {
    const hasPage = Object.hasOwn(source.subpages, key), folder = ownerFields[6] === "subpage";
    if (!hasPage && !folder) return;
    visited.add(key);
    const rawPage = hasPage ? source.subpages[key]! : "";
    check(typeof rawPage === "string" && fits(rawPage), "page exceeds the saved value byte limit");
    const compact = rawPage.startsWith("~"), parts = (compact ? rawPage.slice(1) : rawPage).split("|");
    const id = nextPage++;
    records.push({ kind: K.Page, id, first: parent, second: owner, third: intern(parts[0]!),
      flags: hasPage ? Number(compact) : depth === 1 ? ABSENT : 0 });
    if (folder) records.push({ kind: K.Navigation, id: owner, first: id });
    for (let i = 1; i < parts.length; ++i) {
      check(nextCard < NO_ID, "too many stored cards");
      const child = nextCard++, fields = fieldsFromRaw(parts[i]!, true, compact);
      card(child, id, i, fields, parts[i]!);
      if (!nested) continue;
      if (depth === 2) {
        check(fields[6] !== "subpage", "subpages can be at most two levels deep");
        continue;
      }
      if (fields[6] === "subpage" || (i <= 32 && Object.hasOwn(source.subpages, childPageKey(key, i))))
        page(child, id, childPageKey(key, i), fields, depth + 1);
    }
  }
  for (const slot of slots) {
    const present = Object.hasOwn(source.buttons, slot), raw = present ? source.buttons[slot]! : undefined;
    const fields = fieldsFromRaw(raw ?? "", false, false);
    card(slot, 0, slot, fields, raw);
    page(slot, 0, slot, fields, 1);
  }
  check(Object.keys(source.subpages).every((key) => visited.has(Number(key))), "orphaned page data");
  for (const key of Object.keys(source.settings).sort()) {
    records.push({ kind: K.Setting, id: intern(key), first: intern(source.settings[key]!) });
  }
  return encodeNativeV2(records);
}

/** Project only configurations that v1 can preserve; never drop unsupported data. */
export function convertNativeV2ToV1(input: Uint8Array): Uint8Array {
  return encodePanelConfig(projectNativeGraph(input, false));
}

/** Keep the familiar per-page editing values with explicit two-level paths. */
export function decodePanelPageDocument(input: Uint8Array): PanelConfigDocument {
  return projectNativeGraph(input, true);
}

function projectNativeGraph(input: Uint8Array, nested: boolean): PanelConfigDocument {
  const records = decodeNativeV2(input), byKind = new Map<number, Map<number, NativeV2DecodedRecord>>();
  for (const record of records) {
    if (!byKind.has(record.kind)) byKind.set(record.kind, new Map());
    byKind.get(record.kind)!.set(record.id, record);
  }
  const get = (kind: number, id: number) => byKind.get(kind)?.get(id);
  const text = (id: number) => id ? get(K.String, id)!.text : "";
  const fields = (r: NativeV2DecodedRecord) => r.fields.map((id, i) =>
    text((i === 0 || i === 4) && id ? get(K.Entity, id)!.first : id));
  const source: PanelConfigDocument = {
    deviceProfile: get(K.Profile, 0)!.text,
    buttons: Object.create(null) as Record<number, string>,
    subpages: Object.create(null) as Record<number, string>,
    settings: Object.create(null) as Record<string, string>,
  };
  const pageCards = new Map<number, NativeV2DecodedRecord[]>(), ownerPage = new Map<number, NativeV2DecodedRecord>();
  for (const r of records) {
    if (r.kind === K.Card) {
      if (!pageCards.has(r.first)) pageCards.set(r.first, []);
      pageCards.get(r.first)!.push(r);
    } else if (r.kind === K.Page && r.id) {
      check(nested || r.first === 0, "nested pages cannot be represented by v1"); ownerPage.set(r.second, r);
    } else if (r.kind === K.Setting) source.settings[text(r.id)] = text(r.first);
  }
  for (const list of pageCards.values()) list.sort((a, b) => a.second - b.second);
  const rootOrder = text(get(K.Page, 0)!.third);
  if (Object.hasOwn(source.settings, "button_order")) {
    check(source.settings.button_order === rootOrder, "root order disagrees with the button_order setting");
  } else if (rootOrder) source.settings.button_order = rootOrder;
  for (const card of pageCards.get(0) ?? []) {
    check(card.second <= 32, "root slot exceeds the v1 capacity");
    const saved = fields(card);
    check(saved[6] !== "subpage" || ownerPage.has(card.id), "folder is missing its explicit page target");
    if (card.flags === ABSENT) {
      check(card.fieldMask === 0 && ownerPage.has(card.id), "invalid absent owner-card provenance");
    } else if (card.flags === RAW) {
      const raw = text(card.rawSource);
      check(equalFields(fieldsFromRaw(raw, false, false), saved), "stale raw owner-card fields");
      source.buttons[card.second] = raw;
    } else {
      const compact = "~" + trimmed(saved.map(encodeConfigField), ",");
      const plain = trimmed(saved, ";");
      check(fits(compact) || (!plain.startsWith("~") && !saved.some((s) => s.includes(";")) && fits(plain)),
        "root card exceeds the v1 byte limit");
      source.buttons[card.second] = fits(compact) ? compact : plain;
    }
  }
  function key(page: NativeV2DecodedRecord): number {
    if (page.first === 0) return get(K.Card, page.second)!.second;
    const parent = get(K.Page, page.first)!;
    check(parent.first === 0, "subpages can be at most two levels deep");
    return childPageKey(key(parent), get(K.Card, page.second)!.second);
  }
  for (const page of ownerPage.values()) {
    const owner = get(K.Card, page.second)!, savedOwner = fields(owner), children = pageCards.get(page.id) ?? [];
    check(owner.second <= 32, "subpage owner slot exceeds the v1 capacity");
    const navigation = get(K.Navigation, owner.id);
    check((savedOwner[6] === "subpage") === !!navigation, "v1 would change folder navigation");
    if (page.flags === ABSENT) {
      check(page.third === 0 && children.length === 0 && savedOwner[6] === "subpage" && navigation,
        "invalid absent subpage provenance");
      continue;
    }
    const order = text(page.third), originalCompact = page.flags === 1;
    check(!order.includes("|"), "subpage order contains a legacy record separator");
    children.forEach((card, i) => {
      check(card.second === i + 1, "sparse child slots cannot be represented losslessly by v1");
      if (nested) check((fields(card)[6] === "subpage") === !!get(K.Navigation, card.id),
        "folder is missing its explicit page target");
      check(card.flags !== ABSENT, "absent-card provenance is only valid for root owners");
      if (card.flags === RAW) {
        const raw = text(card.rawSource);
        check(!raw.includes("|") && equalFields(fieldsFromRaw(raw, true, originalCompact), childDefaults(fields(card))),
          "stale raw child-card fields");
      }
    });
    function serialize(compact: boolean): string | undefined {
      if (!compact && order.startsWith("~")) return undefined;
      const tokens = [order];
      for (const card of children) {
        const saved = childDefaults(fields(card));
        if (card.flags === RAW) {
          if (compact !== originalCompact) return undefined;
          tokens.push(text(card.rawSource));
        } else if (compact) {
          const typeCode = mapped(CARD_CONTRACT_SUBPAGE_TYPE_CODES, saved[6]!);
          if (/[,|]/.test(typeCode) || mapped(CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE, typeCode) !== saved[6]) return undefined;
          saved[2] = saved[2] === "Auto" ? "" : saved[2]!;
          saved[3] = saved[3] === "Auto" ? "" : saved[3]!;
          tokens.push(trimmed([typeCode, ...[0, 1, 2, 3, 4, 5, 7, 8].map((i) => encodeConfigField(saved[i]))], ","));
        } else {
          if (saved.some((value) => /[:|]/.test(value))) return undefined;
          saved[2] = saved[2] === "Auto" ? "" : saved[2]!;
          saved[3] = saved[3] === "Auto" ? "" : saved[3]!;
          tokens.push(trimmed(saved, ":"));
        }
      }
      const value = (compact ? "~" : "") + tokens.join("|");
      return fits(value) ? value : undefined;
    }
    const raw = serialize(originalCompact) ?? serialize(!originalCompact);
    check(raw !== undefined, "page cannot preserve raw imports in a v1 spelling");
    source.subpages[key(page)] = raw;
  }
  // Reuse deployed v1 record, UTF-8 and byte limits before returning any result.
  encodePanelConfig({ ...source, subpages: Object.fromEntries(Object.entries(source.subpages).filter(([key]) => Number(key) <= 32)) });
  return source;
}
