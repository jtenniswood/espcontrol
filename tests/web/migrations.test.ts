import { cloneCardConfig, parseRawButtonConfig } from "../../src/webserver/model";
import { migrateSavedConfigActionLegacy } from "../../src/webserver/generated/saved_config_action";
import { migrateSavedConfigSensorLegacy } from "../../src/webserver/generated/saved_config_sensor";
import { migrateSavedConfigVacuumLegacy } from "../../src/webserver/generated/saved_config_vacuum";
import { migrateSavedConfigWeatherLegacy } from "../../src/webserver/generated/saved_config_weather";

interface LegacyFixture {
  readonly input: string;
  readonly expected: Record<string, string>;
}

type LegacyFixtures = Readonly<Record<string, LegacyFixture>>;

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runMigrationTests(fixtures: LegacyFixtures): void {
  const generatedCases = [
    {
      name: "weather forecast",
      fixture: fixtures.weather_forecast_alias,
      migrate: migrateSavedConfigWeatherLegacy,
      fields: ["type", "precision"] as const,
    },
    {
      name: "text sensor",
      fixture: fixtures.text_sensor_alias,
      migrate: migrateSavedConfigSensorLegacy,
      fields: ["type", "entity", "label", "unit", "precision", "icon_on"] as const,
    },
  ];
  for (const migration of generatedCases) {
    if (!migration.fixture) throw new Error(`${migration.name}: shared compatibility fixture is missing`);
    const config = parseRawButtonConfig(migration.fixture.input);
    equal(migration.migrate(config), true, `${migration.name} alias is detected`);
    for (const field of migration.fields) {
      equal(config[field], migration.fixture.expected[field], `${migration.name} ${field}`);
    }
  }

  const actionCases = [
    { type: "local", expectedType: "action", expectedSensor: "local" },
    { type: "option_select", expectedType: "action", expectedSensor: "input_select.select_option" },
  ];
  for (const migration of actionCases) {
    const config = cloneCardConfig({ type: migration.type, sensor: "legacy", options: "stale" });
    equal(migrateSavedConfigActionLegacy(config), true, `${migration.type} alias is detected`);
    equal(config.type, migration.expectedType, `${migration.type} canonical type`);
    equal(config.sensor, migration.expectedSensor, `${migration.type} canonical action`);
    equal(config.options, "", `${migration.type} stale options are retired`);
  }

  const vacuumCases = [
    { action: "vacuum.start", mode: "start_stop" },
    { action: "vacuum.return_to_base", mode: "dock" },
  ];
  for (const migration of vacuumCases) {
    const config = cloneCardConfig({ type: "action", sensor: migration.action, options: "stale" });
    equal(migrateSavedConfigVacuumLegacy(config), true, `${migration.action} alias is detected`);
    equal(config.type, "vacuum", `${migration.action} canonical type`);
    equal(config.sensor, migration.mode, `${migration.action} canonical mode`);
    equal(config.options, "", `${migration.action} stale options are retired`);
  }

  const canonical = cloneCardConfig({ type: "sensor", sensor: "sensor.temperature" });
  equal(migrateSavedConfigSensorLegacy(canonical), false, "canonical cards are not reported as migrations");
}
