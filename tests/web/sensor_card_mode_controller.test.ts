import type { CardConfig } from "../../src/webserver/contracts/types";
import { createSensorCardModeController } from "../../src/webserver/features/sensor_card_mode_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function card(overrides: Partial<CardConfig> = {}): CardConfig {
  return { entity: "sensor.room", label: "Room", icon: "mdi:home", icon_on: "mdi:home", sensor: "", unit: "°C", type: "sensor", precision: "2", options: "state_input=high", ...overrides };
}

export function runSensorCardModeControllerTests(): void {
  const controller = createSensorCardModeController({
    normalizeOptions: (options, precision) => `${precision}:${options}`,
  });

  const local = card();
  equal(controller.isLocal(local), false, "Home Assistant sensors are not local");
  equal(controller.selectSource(local, "local").join(","), "type,entity,label,sensor,unit,icon,icon_on,precision,options", "source changes persist every reset field");
  equal(local.sensor, "local", "local source uses the stable local marker");
  equal(local.entity, "", "source changes clear the previous entity");
  equal(controller.isLocal(local), true, "local source is detected after transition");

  const text = card();
  const textTransition = controller.selectDisplayMode(text, "text");
  equal(textTransition.mode, "text", "text display mode is selected");
  equal(text.precision, "text", "text mode stores its precision marker");
  equal(text.label, "", "text mode clears numeric labels");
  equal(text.unit, "", "text mode clears numeric units");
  equal(text.icon, "mdi:home", "text mode retains its display icon");
  equal(text.options, "text:state_input=high", "text mode normalizes state-label options");

  const time = card();
  controller.selectDisplayMode(time, "time");
  equal(time.precision, "time", "time mode stores its precision marker");
  equal(time.icon, "Auto", "time mode resets the icon");
  equal(time.icon_on, "Auto", "time mode resets the on icon");

  const numeric = card({ precision: "icon", options: "old" });
  const numericTransition = controller.selectDisplayMode(numeric, "unknown");
  equal(numericTransition.mode, "numeric", "unknown display modes use numeric mode");
  equal(numeric.precision, "", "numeric mode clears the mode marker");
  equal(numeric.icon, "Auto", "numeric mode resets the icon");
  equal(numeric.options, ":old", "numeric mode normalizes options with empty precision");
}
