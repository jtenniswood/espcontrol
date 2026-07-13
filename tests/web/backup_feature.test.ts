import type { CardConfig } from "../../src/webserver/contracts/types";
import { createBackupFeature, type FeatureSubpage } from "../../src/webserver/features/backup";
import {
  buildSubpageGrid,
  cloneCardConfig,
  parseLegacySubpageConfig,
  serializeLegacySubpageConfig,
  subpageOrderForSerialize,
} from "../../src/webserver/model";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

function serializeSubpage(subpage: FeatureSubpage): string {
  const fields = subpage.buttons.map((button) => [
    button.entity,
    button.label,
    button.icon,
    button.icon_on,
    button.sensor,
    button.unit,
    button.type,
    button.precision,
    button.options,
  ]);
  return serializeLegacySubpageConfig(
    subpageOrderForSerialize(subpage.order, subpage.backLabel),
    fields,
  );
}

const feature = createBackupFeature({
  deviceId: "panel-a",
  gridCols: 3,
  numSlots: 6,
  normalizeButtonConfig: (button: CardConfig) => cloneCardConfig(button),
  parseSubpageConfig: parseLegacySubpageConfig,
  serializeSubpageConfig: serializeSubpage,
  buildSubpageGrid(subpage) {
    const result = buildSubpageGrid(subpage, 6, 3);
    subpage.sizes = result.sizes;
    return result.grid;
  },
});

export function runBackupFeatureTests(): void {
  const backup = feature.createBackupConfig({
    device: "panel-a",
    slots: 2,
    exported_at: "2026-07-13T00:00:00.000Z",
    grid: [1, 2],
    sizes: { "2": 2 },
    buttons: [
      { entity: "light.kitchen", label: "Kitchen" },
      { entity: "scene.movie", label: "Movie", type: "action" },
    ],
    subpages: {
      "1": {
        order: ["1", "B"],
        backLabel: "Return",
        buttons: [cloneCardConfig({ entity: "switch.fan", label: "Fan" })],
      },
    },
  });
  equal(backup.button_order, "1,2d", "backup preserves exact size tokens");
  equal(backup.subpage_objects["1"]?.back_label, "Return", "backup preserves subpage back labels");

  const plan = feature.planBackupImport(backup, { device: "panel-b", slots: 3 });
  equal(plan.warnings.length, 2, "cross-device and slot-count warnings are retained");
  equal(plan.buttons.length, 3, "backup expands to the target slot count");
  deepEqual(Object.keys(plan.subpages), ["1"], "subpages follow mapped home slots");
  equal(plan.subpages["1"]?.grid?.[0], 1, "imported subpage layout is rebuilt");

  let failure = "";
  try {
    feature.normalizeBackupConfig({ version: 3, format: "espcontrol.backup", buttons: [] });
  } catch (error) {
    failure = String((error as Error & { backupMessage?: string }).backupMessage || "");
  }
  equal(failure, "Backup was created by a newer version of EspControl", "future backup error remains exact");
}
