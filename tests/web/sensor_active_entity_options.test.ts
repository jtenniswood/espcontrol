import { createConfigSensorOptionsFeature } from "../../src/webserver/application/config_sensor_options";
import type { CardConfig } from "../../src/webserver/contracts/types";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runSensorActiveEntityOptionsTests(): void {
  const feature = createConfigSensorOptionsFeature({ definitions: {} } as any);
  const card: CardConfig = {
    entity: "",
    label: "Dishwasher",
    icon: "Auto",
    icon_on: "Auto",
    sensor: "sensor.dishwasher_program",
    unit: "",
    type: "sensor",
    precision: "text",
    options: "active_color",
  };

  feature.setSensorActiveEntity(card, " binary_sensor.dishwasher_running ");
  equal(feature.sensorActiveEntity(card), "binary_sensor.dishwasher_running", "active entity is trimmed and read back");
  equal(card.options, "active_color,active_entity=binary_sensor.dishwasher_running", "active entity is stored with the sensor options");
  equal(feature.normalizeSensorOptions(card.options, "text"), card.options, "text sensor normalization preserves active entity");

  feature.setSensorActiveColorEnabled(card, false);
  equal(card.options, "active_entity=binary_sensor.dishwasher_running", "disabling highlight preserves the selected entity");
  equal(feature.normalizeSensorOptions(card.options, "time"), "", "time sensors drop unsupported active entity settings");
}
