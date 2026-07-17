import {
  normalizeBackupEnvelope,
  planBackupButtonLayout,
  validateBackupEnvelope,
} from "../../src/webserver/model";

interface LegacyBackupFixture {
  readonly version: number;
  readonly device: string;
  readonly button_order: string;
  readonly buttons: readonly Record<string, string>[];
  readonly subpages: Readonly<Record<string, string>>;
}

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runBackupCompatibilityTests(fixture: LegacyBackupFixture): void {
  const validated = validateBackupEnvelope(fixture);
  equal(validated.version, 1, "legacy version remains accepted");

  const plan = planBackupButtonLayout(fixture.buttons, fixture.button_order, 6, 3);
  equal(plan.buttons.length, 6, "legacy backup expands to the target device slot count");
  equal(plan.importedCount, fixture.buttons.length, "legacy backup retains its source button count");
  equal(plan.buttons[0]?.entity, "weather.home", "legacy wide-card order maps the expected first card");

  const normalized = normalizeBackupEnvelope(validated, {
    buttons: plan.buttons,
    subpages: { ...fixture.subpages },
    button_order: plan.button_order,
  });
  equal(normalized.version, 2, "legacy backup is normalized to the current envelope version");
  equal(normalized.source.device, fixture.device, "legacy device identity is preserved");
  equal(normalized.subpages["2"], fixture.subpages["2"], "legacy subpage payload is preserved for parsing");
}
