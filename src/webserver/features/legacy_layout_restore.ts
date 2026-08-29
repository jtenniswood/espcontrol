import type { PanelConfigDocument } from "../model";

export interface LegacyLayoutMismatch {
  readonly name: string;
  readonly expected: string;
  readonly actual: string | null;
}

export interface LegacyLayoutRestoreResult {
  readonly ok: boolean;
  readonly mismatches: readonly LegacyLayoutMismatch[];
}

export interface LegacyLayoutRestoreDependencies {
  readonly slotCount: number;
  readonly subpageEntityKeys: readonly string[];
  readonly entityName: (key: string) => string;
  readonly entityNameForSlot: (key: string, slot: number) => string;
  readonly splitSubpageConfigChunks: (value: string, count: number, chunkSize: number) => string[] | null;
  readonly postText: (name: string, value: string) => Promise<unknown>;
  readonly readText: (name: string) => Promise<unknown | null>;
}

interface ExpectedTextValue {
  readonly name: string;
  readonly value: string;
}

function textStateValue(data: unknown): string | null {
  if (!data || typeof data !== "object") return null;
  const record = data as Record<string, unknown>;
  const value = record.value ?? record.state;
  return value == null ? null : String(value);
}

function expectedLegacyValues(
  document: PanelConfigDocument,
  dependencies: LegacyLayoutRestoreDependencies,
): ExpectedTextValue[] {
  const values: ExpectedTextValue[] = [{
    name: dependencies.entityName("button_on_color"),
    value: String(document.settings.button_on_color || ""),
  }];

  for (let slot = 1; slot <= dependencies.slotCount; slot += 1) {
    values.push({
      name: dependencies.entityNameForSlot("button_config", slot),
      value: String(document.buttons[slot] || ""),
    });
  }

  for (let slot = 1; slot <= dependencies.slotCount; slot += 1) {
    const full = String(document.subpages[slot] || "");
    const chunks = dependencies.splitSubpageConfigChunks(
      full,
      dependencies.subpageEntityKeys.length,
      255,
    );
    if (!chunks) throw new Error(`Subpage ${slot} is too large to restore.`);
    for (let index = 0; index < dependencies.subpageEntityKeys.length; index += 1) {
      values.push({
        name: dependencies.entityNameForSlot(dependencies.subpageEntityKeys[index]!, slot),
        value: chunks[index] || "",
      });
    }
  }

  values.push({
    name: dependencies.entityName("button_order"),
    value: String(document.settings.button_order || ""),
  });
  return values;
}

/** Restores and verifies the complete legacy text-entity mirror of a native document. */
export async function restoreLegacyLayoutDocument(
  document: PanelConfigDocument,
  dependencies: LegacyLayoutRestoreDependencies,
): Promise<LegacyLayoutRestoreResult> {
  const expected = expectedLegacyValues(document, dependencies);
  for (const entry of expected) await dependencies.postText(entry.name, entry.value);

  const mismatches: LegacyLayoutMismatch[] = [];
  for (const entry of expected) {
    const actual = textStateValue(await dependencies.readText(entry.name));
    if (actual !== entry.value) mismatches.push({ name: entry.name, expected: entry.value, actual });
  }
  return { ok: mismatches.length === 0, mismatches };
}

export function legacyRestoreFailureMessage(result: LegacyLayoutRestoreResult): string {
  const names = result.mismatches.slice(0, 3).map((entry) => entry.name);
  const remainder = result.mismatches.length - names.length;
  const affected = names.join(", ") + (remainder > 0 ? ` and ${remainder} more` : "");
  return "The layout could not be restored completely because native configuration is unavailable. " +
    `The device did not retain: ${affected}. Some settings may have changed; restore again after native ` +
    "configuration is available.";
}
