import {
  legacyRestoreFailureMessage,
  restoreLegacyLayoutDocument,
} from "../../src/webserver/features/legacy_layout_restore";
import type { PanelConfigDocument } from "../../src/webserver/model";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function document(): PanelConfigDocument {
  const buttons: Record<number, string> = {};
  for (let slot = 1; slot <= 20; slot += 1) buttons[slot] = `button-${slot}`;
  return {
    deviceProfile: "panel-a",
    buttons,
    subpages: { 11: "abcdefgh", 12: "ijkl", 13: "mnopqr" },
    settings: { button_on_color: "0073FF", button_order: "1,2,3,16,17,18,19,20" },
  };
}

function dependencies(states: Record<string, string>, posts: string[]) {
  return {
    slotCount: 20,
    subpageEntityKeys: ["subpage", "subpage_ext"],
    entityName: (key: string) => key,
    entityNameForSlot: (key: string, slot: number) => `${key}_${slot}`,
    splitSubpageConfigChunks: (value: string, count: number, chunkSize: number) => {
      const chunks: string[] = [];
      for (let offset = 0; offset < value.length; offset += chunkSize) chunks.push(value.slice(offset, offset + chunkSize));
      return chunks.length <= count ? chunks : null;
    },
    postText: async (name: string, value: string) => { posts.push(`${name}=${value}`); states[name] = value; },
    readText: async (name: string) => Object.prototype.hasOwnProperty.call(states, name) ? { value: states[name] } : null,
  };
}

export async function runLegacyLayoutRestoreTests(): Promise<void> {
  const states: Record<string, string> = { subpage_ext_11: "obsolete", subpage_5: "old" };
  const posts: string[] = [];
  const result = await restoreLegacyLayoutDocument(document(), dependencies(states, posts));
  equal(result.ok, true, "a complete legacy mirror verifies");
  equal(states.button_config_20, "button-20", "slots after fifteen are restored");
  equal(states.subpage_11, "abcdefgh", "subpage content is restored");
  equal(states.subpage_ext_11, "", "obsolete trailing chunks are cleared");
  equal(states.subpage_5, "", "subpages omitted from the document are cleared");
  equal(posts.at(-1), "button_order=1,2,3,16,17,18,19,20", "button order is written last");

  const truncatedStates: Record<string, string> = {};
  const truncatedDependencies = {
    ...dependencies(truncatedStates, []),
    postText: async (name: string, value: string) => {
      truncatedStates[name] = name === "button_config_16" ? "" : value;
    },
  };
  const truncated = await restoreLegacyLayoutDocument(document(), truncatedDependencies);
  equal(truncated.ok, false, "truncated legacy state fails verification");
  equal(truncated.mismatches[0]?.name, "button_config_16", "the truncated slot is identified");
  equal(legacyRestoreFailureMessage(truncated).includes("button_config_16"), true,
    "the failure message identifies affected state");

  const missingStates: Record<string, string> = {};
  const missingDependencies = {
    ...dependencies(missingStates, []),
    readText: async (name: string): Promise<unknown | null> => name === "subpage_13"
      ? null
      : { state: missingStates[name] },
  };
  const missing = await restoreLegacyLayoutDocument(document(), missingDependencies);
  equal(missing.ok, false, "a missing text endpoint fails verification");
  equal(missing.mismatches.some((entry) => entry.name === "subpage_13"), true,
    "the missing subpage endpoint is identified");
}
