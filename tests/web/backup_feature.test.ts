import type { CardConfig } from "../../src/webserver/contracts/types";
import { createBackupFeature, type FeatureSubpage } from "../../src/webserver/features/backup";
import {
  buildSubpageGrid,
  cloneCardConfig,
  decodePanelConfig,
  createPanelConfigBackupPayload,
  decodePanelConfigBackupPayload,
  encodePanelConfig,
  parseLegacySubpageConfig,
  serializeLegacySubpageConfig,
  subpageOrderForSerialize,
} from "../../src/webserver/model";

interface MigrationFixture {
  readonly scenarios: {
    readonly backup_restore: {
      readonly backup: Record<string, unknown>;
      readonly target: { readonly device: string; readonly slots: number };
      readonly expected: {
        readonly warning_count: number;
        readonly button_order: string;
        readonly button_on_color: string;
        readonly button_entities: readonly string[];
        readonly subpage_slots: readonly string[];
      };
    };
  };
}

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

export function runBackupFeatureTests(migrationFixture?: MigrationFixture): void {
  const backup = feature.createBackupConfig({
    device: "panel-a",
    slots: 2,
    exported_at: "2026-07-13T00:00:00.000Z",
    grid: [1, 2],
    sizes: { "2": 2 },
    buttons: [
      { entity: "light.kitchen", label: "Kitchen" },
      {
        entity: "media_player.living_room",
        label: "Speaker Group",
        icon: "Auto",
        sensor: "speaker_group",
        type: "media",
        options: "speaker_group_entity=media_player.compatible_speakers,volume_max=80",
      },
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

  const nativeDocument = encodePanelConfig({
    deviceProfile: "panel-a",
    buttons: { 1: "light.kitchen" },
    subpages: {},
    settings: { button_order: "1" },
  });
  const nativeBackup = feature.createBackupConfig({
    device: "panel-a",
    buttons: [],
    native_config: createPanelConfigBackupPayload(nativeDocument),
  });
  equal(nativeBackup.native_config?.device_profile, "panel-a", "native backup records its device profile");
  equal(nativeBackup.native_config?.document_version, 1, "native backup records its document version");

  const newerNativeDocument = feature.normalizeBackupConfig({
    version: 2,
    format: "espcontrol.backup",
    buttons: [],
    native_config: { document_version: 2, device_profile: "future-panel", payload: "future" },
  });
  equal(newerNativeDocument.native_config, undefined, "newer native payloads do not block readable backup import");

  const plan = feature.planBackupImport(backup, { device: "panel-b", slots: 3 });
  equal(plan.warnings.length, 2, "cross-device and slot-count warnings are retained");
  equal(plan.buttons.length, 3, "backup expands to the target slot count");
  equal(plan.buttons[1]?.sensor, "speaker_group", "backup preserves standalone speaker group mode");
  equal(
    plan.buttons[1]?.options,
    "speaker_group_entity=media_player.compatible_speakers,volume_max=80",
    "backup preserves speaker group helper and volume limit",
  );
  deepEqual(Object.keys(plan.subpages), ["1"], "subpages follow mapped home slots");
  equal(plan.subpages["1"]?.grid?.[0], 1, "imported subpage layout is rebuilt");

  let failure = "";
  try {
    feature.normalizeBackupConfig({ version: 3, format: "espcontrol.backup", buttons: [] });
  } catch (error) {
    failure = String((error as Error & { backupMessage?: string }).backupMessage || "");
  }
  equal(failure, "Backup was created by a newer version of EspControl", "future backup error remains exact");

  if (!migrationFixture) return;
  const scenario = migrationFixture.scenarios.backup_restore;
  const restoredBackup = feature.normalizeBackupConfig(scenario.backup);
  const restored = feature.planBackupImport(restoredBackup, scenario.target);
  const restoredNative = decodePanelConfig(decodePanelConfigBackupPayload(restoredBackup.native_config));
  equal(restored.warnings.length, scenario.expected.warning_count, "backup restore reports its target-size warning");
  equal(restoredNative.deviceProfile, scenario.target.device, "native backup matches the readable backup device");
  equal(restoredNative.settings.button_order, scenario.expected.button_order,
    "native backup matches the readable backup order");
  equal(restoredNative.settings.button_on_color, scenario.expected.button_on_color,
    "native backup matches the readable backup active colour");
  equal(restored.button_order, scenario.expected.button_order, "backup restore preserves button order");
  equal(restored.config.button_on_color, scenario.expected.button_on_color, "backup restore preserves active colour");
  deepEqual(restored.buttons.slice(0, scenario.expected.button_entities.length).map((button) => button.entity),
    scenario.expected.button_entities, "backup restore preserves button records");
  deepEqual(Object.keys(restored.subpages), scenario.expected.subpage_slots,
    "backup restore preserves structured subpages");
}
