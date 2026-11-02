/** Editor page paths are independent of native wire identities. */
export const PAGE_OWNER_SLOTS = 32;
export const MAX_PAGE_DEPTH = 2;
const MAX_PAGE_KEY = PAGE_OWNER_SLOTS + PAGE_OWNER_SLOTS * PAGE_OWNER_SLOTS;

export function pageDepth(key: number): number {
  if (!Number.isInteger(key) || key < 0 || key > MAX_PAGE_KEY) throw new Error("Invalid page path");
  return key === 0 ? 0 : key <= PAGE_OWNER_SLOTS ? 1 : 2;
}
export function childPageKey(parent: number, slot: number): number {
  if (!Number.isInteger(slot) || slot < 1 || slot > PAGE_OWNER_SLOTS) throw new Error("Invalid folder slot");
  const depth = pageDepth(parent);
  if (depth >= MAX_PAGE_DEPTH) throw new Error("Subpages can be at most two levels deep");
  return parent === 0 ? slot : PAGE_OWNER_SLOTS + (parent - 1) * PAGE_OWNER_SLOTS + slot;
}
export function parentPageKey(key: number): number {
  return pageDepth(key) < 2 ? 0 : Math.floor((key - PAGE_OWNER_SLOTS - 1) / PAGE_OWNER_SLOTS) + 1;
}
export function pageOwnerSlot(key: number): number {
  if (key === 0) throw new Error("Home has no owner card");
  return pageDepth(key) === 1 ? key : (key - PAGE_OWNER_SLOTS - 1) % PAGE_OWNER_SLOTS + 1;
}
