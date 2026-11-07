import type { PanelConfigDocument } from "./panel_config";
import { PanelConfigError } from "./panel_config";
import { encodePanelPageDocument } from "./panel_config_conversion";
import { childPageKey, pageDepth, pageOwnerSlot, parentPageKey } from "./page_keys";

/** Saved page values belonging to one folder, independent of its location. */
export interface PanelPageSubtree {
  readonly configuration: string | null;
  readonly children: Readonly<Record<number, string>>;
}

function folderKey(key: number): void {
  if (pageDepth(key) === 0) throw new PanelConfigError("Home is not a transferable folder");
}

export function capturePanelPageSubtree(document: PanelConfigDocument, key: number): PanelPageSubtree {
  folderKey(key);
  const children: Record<number, string> = Object.create(null) as Record<number, string>;
  for (const [savedKey, value] of Object.entries(document.subpages)) {
    const child = Number(savedKey);
    if (pageDepth(child) === 2 && parentPageKey(child) === key) children[pageOwnerSlot(child)] = value;
  }
  return {
    configuration: Object.hasOwn(document.subpages, key) ? document.subpages[key]! : null,
    children,
  };
}

/**
 * Plan one atomic replacement of saved page records. The caller stages owner
 * card/order changes in the same document before calling this function. No
 * input or live editor state is mutated, including when validation rejects it.
 */
export function replacePanelPageSubtree(
  document: PanelConfigDocument,
  target: number,
  subtree: PanelPageSubtree,
  removeSource?: number,
): PanelConfigDocument {
  folderKey(target);
  if (removeSource !== undefined) {
    folderKey(removeSource);
    if (removeSource !== target && parentPageKey(target) === removeSource)
      throw new PanelConfigError("A folder cannot be moved inside itself");
  }
  if (pageDepth(target) === 2 && Object.keys(subtree.children).length)
    throw new PanelConfigError("Subpages can be at most two levels deep");
  const subpages = { ...document.subpages };
  for (const savedKey of Object.keys(subpages)) {
    const key = Number(savedKey);
    if (key === target || parentPageKey(key) === target ||
        (removeSource !== undefined && (key === removeSource || parentPageKey(key) === removeSource)))
      delete subpages[key];
  }
  if (subtree.configuration !== null) subpages[target] = subtree.configuration;
  for (const [slot, value] of Object.entries(subtree.children)) {
    if (!/^[1-9][0-9]*$/.test(slot)) throw new PanelConfigError("Invalid descendant folder slot");
    subpages[childPageKey(target, Number(slot))] = value;
  }
  const candidate = { ...document, buttons: { ...document.buttons }, settings: { ...document.settings }, subpages };
  // Includes implicit empty folder targets, inactive data, per-value limits,
  // orphan checks and third-level folders in the pasted page's actual cards.
  encodePanelPageDocument(candidate);
  return candidate;
}
