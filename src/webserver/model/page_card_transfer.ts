import type { CardConfig } from "../contracts/types";
import { CARD_CONFIG_FIELDS, cloneCardConfig, emptyCardConfig, parseRawButtonConfig } from "./card";
import { encodeConfigField } from "./config_primitives";
import { CARD_CONTRACT_SUBPAGE_TYPE_CODES, CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE } from "../generated/card_contract";
import { buildSubpageGrid, parseRawSubpageConfig, serializeSubpageGrid } from "./subpage";
import { coveredCells, parseGridOrder, serializeGridOrder, type SlotSizeMap } from "./grid";
import { findDuplicatePlacement, placeSlotAt } from "../features/preview_grid";
import { PanelConfigError, type PanelConfigDocument } from "./panel_config";
import { encodePanelPageDocument } from "./panel_config_conversion";
import { capturePanelPageSubtree, type PanelPageSubtree } from "./page_subtree";
import { childPageKey, pageDepth, pageOwnerSlot, parentPageKey } from "./page_keys";

export interface PageTransferLayout { readonly slots: number; readonly columns: number }
export interface PageTransferCard {
  readonly button: CardConfig;
  readonly size: number;
  /** Retain dormant page data even when its owner is no longer a folder. */
  readonly subtree: PanelPageSubtree | null;
}
export interface PageTransferOrigin {
  readonly page: number;
  readonly slots: readonly number[];
  readonly fingerprint: string;
}
export interface PageCardClipboard {
  readonly entries: readonly PageTransferCard[];
  readonly origin: PageTransferOrigin;
}
export class PageTransferConflict extends Error {}

const typeFromCode = (code: string): string => CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE[code] ?? code;
function check(value: unknown, message: string): asserts value { if (!value) throw new PanelConfigError(message); }
function cloneDocument(source: PanelConfigDocument): PanelConfigDocument {
  return { ...source, buttons: { ...source.buttons }, subpages: { ...source.subpages }, settings: { ...source.settings } };
}
function layoutCheck(layout: PageTransferLayout): void {
  check(Number.isInteger(layout.slots) && layout.slots > 1 && layout.slots <= 32 &&
    Number.isInteger(layout.columns) && layout.columns > 0 && layout.columns <= layout.slots, "Invalid transfer layout");
}
function cardSlot(slot: number, layout: PageTransferLayout): void {
  check(Number.isInteger(slot) && slot >= 1 && slot <= layout.slots, "Invalid card slot");
}
function pageExists(document: PanelConfigDocument, page: number): void {
  pageDepth(page);
  if (!page) return;
  const parent = parentPageKey(page);
  pageExists(document, parent);
  const slot = pageOwnerSlot(page);
  const owner = readPage(document, parent).buttons[slot - 1];
  check(owner?.type === "subpage", "The destination page is no longer a folder");
}
function readPage(document: PanelConfigDocument, page: number) {
  if (!page) return { buttons: Array.from({ length: 32 }, (_, i) => parseRawButtonConfig(document.buttons[i + 1])),
    order: document.settings.button_order ?? "", backLabel: "Back", backColor: "" };
  const raw = document.subpages[page] ?? "";
  const parsed = parseRawSubpageConfig(raw, typeFromCode);
  return { ...parsed, order: (raw.startsWith("~") ? raw.slice(1) : raw).split("|")[0] ?? "", backColor: parsed.backColor ?? "" };
}
function pageGrid(document: PanelConfigDocument, page: number, layout: PageTransferLayout) {
  const parsed = readPage(document, page);
  const placed = !page ? parseGridOrder(parsed.order, 32, layout.columns, {}, layout.slots)
    : buildSubpageGrid({ ...parsed, order: parseRawSubpageConfig(document.subpages[page] ?? "", typeFromCode).order }, layout.slots, layout.columns);
  return { ...parsed, grid: placed.grid, sizes: placed.sizes };
}
function subtreeAt(document: PanelConfigDocument, page: number, slot: number): PanelPageSubtree | null {
  if (pageDepth(page) === 2) return null;
  const tree = capturePanelPageSubtree(document, childPageKey(page, slot));
  return tree.configuration !== null || Object.keys(tree.children).length ? tree : null;
}
function fingerprint(document: PanelConfigDocument, page: number, slots: readonly number[]): string {
  const parent = !page ? { order: document.settings.button_order, cards: slots.map(slot => document.buttons[slot]) }
    : document.subpages[page];
  return JSON.stringify([document.deviceProfile, parent, slots.map(slot => subtreeAt(document, page, slot))]);
}

/** Capture the complete saved tree, including invisible cards and raw values. */
export function capturePageCards(document: PanelConfigDocument, page: number, slots: readonly number[], layout: PageTransferLayout): PageCardClipboard {
  layoutCheck(layout); pageExists(document, page);
  check(slots.length > 0 && new Set(slots).size === slots.length, "Choose distinct cards to copy");
  const source = pageGrid(document, page, layout);
  const entries = slots.map(slot => {
    cardSlot(slot, layout);
    check(source.grid.includes(slot), "A copied card is no longer on this page");
    return { button: cloneCardConfig(source.buttons[slot - 1]), size: source.sizes[String(slot)] ?? 1,
      subtree: subtreeAt(document, page, slot) };
  });
  return { entries, origin: { page, slots: [...slots], fingerprint: fingerprint(document, page, slots) } };
}
function verifyOrigin(document: PanelConfigDocument, origin: PageTransferOrigin): void {
  if (fingerprint(document, origin.page, origin.slots) !== origin.fingerprint)
    throw new PageTransferConflict("The copied cards changed. Copy them again before moving.");
}
function removeTree(document: PanelConfigDocument, key: number): void {
  for (const saved of Object.keys(document.subpages))
    if (Number(saved) === key || parentPageKey(Number(saved)) === key) delete document.subpages[Number(saved)];
}
function writeTree(document: PanelConfigDocument, page: number, slot: number, tree: PanelPageSubtree | null): void {
  if (pageDepth(page) === 2) {
    check(!tree || (tree.configuration === null && !Object.keys(tree.children).length), "Subpages can be at most two levels deep");
    return;
  }
  const key = childPageKey(page, slot);
  removeTree(document, key);
  if (!tree) return;
  check(pageDepth(key) < 2 || !Object.keys(tree.children).length, "Subpages can be at most two levels deep");
  if (tree.configuration !== null) document.subpages[key] = tree.configuration;
  for (const [child, raw] of Object.entries(tree.children)) document.subpages[childPageKey(key, Number(child))] = raw;
}
function rawFields(button: CardConfig): string[] { return CARD_CONFIG_FIELDS.map(field => button[field]); }
function compactChild(button: CardConfig): string {
  const fields = rawFields(button);
  const type = CARD_CONTRACT_SUBPAGE_TYPE_CODES[button.type] ?? button.type;
  return [type, ...[0, 1, 2, 3, 4, 5, 7, 8].map(i => encodeConfigField(fields[i]))].join(",");
}
/** Patch edited card tokens without rewriting sibling cards or trailing slots. */
function writeCards(document: PanelConfigDocument, page: number, edits: ReadonlyMap<number, CardConfig>, order: string): void {
  if (!page) {
    for (const [slot, button] of edits) {
      const fields = rawFields(button);
      document.buttons[slot] = fields.some(value => /[;\n\r]/.test(value)) || fields[0]?.startsWith("~")
        ? "~" + fields.map(encodeConfigField).join(",") : fields.join(";");
    }
    document.settings.button_order = order;
    return;
  }
  const raw = document.subpages[page] ?? "";
  let compact = raw.startsWith("~");
  const tokens = (compact ? raw.slice(1) : raw).split("|").slice(1);
  if (!compact && [...edits.values()].some(button => rawFields(button).some(field => /[:|]/.test(field)))) {
    compact = true;
    const old = parseRawSubpageConfig(raw, typeFromCode).buttons;
    for (let index = 0; index < tokens.length; ++index) tokens[index] = compactChild(old[index]!);
  }
  for (const [slot, button] of edits) {
    while (tokens.length < slot) tokens.push("");
    tokens[slot - 1] = compact ? compactChild(button) : rawFields(button).join(":");
  }
  document.subpages[page] = (compact ? "~" : "") + [order, ...tokens].join("|");
}
function updatedOrder(document: PanelConfigDocument, page: number, grid: readonly number[], sizes: SlotSizeMap, layout: PageTransferLayout): string {
  if (!page) return serializeGridOrder(grid, sizes);
  const previous = readPage(document, page);
  const next = serializeSubpageGrid(grid, sizes, previous.backLabel, previous.backColor);
  // Inactive records belong to the saved document, independently of this grid.
  const tail = previous.order.split(",").slice(layout.slots);
  if (tail.length) { while (next.length < layout.slots) next.push(""); next.push(...tail); }
  return next.join(",");
}
function clearGridSlots(grid: number[], sizes: SlotSizeMap, slots: readonly number[], layout: PageTransferLayout): void {
  for (const slot of slots) {
    const pos = grid.indexOf(slot);
    if (pos >= 0) for (const cell of coveredCells(pos, sizes[String(slot)] ?? 1, layout.slots, layout.columns, true)) grid[cell] = 0;
    delete sizes[String(slot)];
  }
}
function deleteCards(document: PanelConfigDocument, page: number, slots: readonly number[], layout: PageTransferLayout): void {
  const source = pageGrid(document, page, layout);
  for (const slot of slots) {
    cardSlot(slot, layout);
    check(source.grid.includes(slot), "A selected card is no longer on this page");
    if (pageDepth(page) < 2) removeTree(document, childPageKey(page, slot));
  }
  clearGridSlots(source.grid, source.sizes, slots, layout);
  writeCards(document, page, new Map(slots.map(slot => [slot, emptyCardConfig()])), updatedOrder(document, page, source.grid, source.sizes, layout));
}
export function planPageCardDelete(document: PanelConfigDocument, origin: PageTransferOrigin, layout: PageTransferLayout): PanelConfigDocument {
  layoutCheck(layout); pageExists(document, origin.page); verifyOrigin(document, origin);
  const candidate = cloneDocument(document);
  deleteCards(candidate, origin.page, origin.slots, layout);
  encodePanelPageDocument(candidate);
  return candidate;
}

/** Plan all owner, order, source and descendant changes before one native save. */
export function planPageCardPaste(document: PanelConfigDocument, target: number, start: number,
  entries: readonly PageTransferCard[], layout: PageTransferLayout, move?: PageTransferOrigin,
): { document: PanelConfigDocument; slots: number[]; resized: number } {
  layoutCheck(layout); pageExists(document, target);
  check(entries.length > 0 && Number.isInteger(start) && start >= 0 && start < layout.slots, "Invalid paste position");
  const candidate = cloneDocument(document);
  if (move) {
    verifyOrigin(document, move);
    check(entries.length === move.slots.length, "The move has an invalid card list");
    for (const slot of move.slots) if (pageDepth(move.page) < 2) {
      const source = childPageKey(move.page, slot);
      check(target !== source && parentPageKey(target) !== source, "A folder cannot be moved inside itself");
    }
    deleteCards(candidate, move.page, move.slots, layout);
  }
  const destination = pageGrid(candidate, target, layout), edits = new Map<number, CardConfig>();
  const slots: number[] = []; let resized = 0;
  for (const entry of entries) {
    check(pageDepth(target) < 2 || entry.button.type !== "subpage", "Subpages can be at most two levels deep");
    let slot = 0;
    for (let possible = 1; possible <= layout.slots; ++possible) {
      const button = destination.buttons[possible - 1];
      const empty = !button || CARD_CONFIG_FIELDS.every(field => !button[field] ||
        ((field === "icon" || field === "icon_on") && button[field] === "Auto"));
      if (!destination.grid.includes(possible) && !edits.has(possible) && empty && !subtreeAt(candidate, target, possible)) { slot = possible; break; }
    }
    check(slot, "There is not enough room to paste every card without replacing saved cards");
    const placement = findDuplicatePlacement(destination.grid.slice(0, layout.slots), start, entry.size, layout.slots, layout.columns);
    check(placement.pos >= 0, "There is not enough room to paste every card");
    if (placement.size !== entry.size) ++resized;
    if (placement.size === 1) delete destination.sizes[String(slot)]; else destination.sizes[String(slot)] = placement.size;
    placeSlotAt(destination.grid, slot, placement.pos, placement.size, layout.columns);
    edits.set(slot, cloneCardConfig(entry.button)); slots.push(slot);
    writeTree(candidate, target, slot, entry.subtree);
  }
  writeCards(candidate, target, edits, updatedOrder(candidate, target, destination.grid, destination.sizes, layout));
  // Validate implicit empty targets and every inactive page too, after the full batch.
  encodePanelPageDocument(candidate);
  return { document: candidate, slots, resized };
}
