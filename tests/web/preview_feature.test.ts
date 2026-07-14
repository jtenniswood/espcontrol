import {
  cardTypePickerOptions,
  clampMenuPosition,
  closestGridCell,
  defaultCardTypeForPicker,
  infoOnlyCardVisible,
  previewValue,
  swapGridCell,
} from "../../src/webserver/features/preview";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

export function runPreviewFeatureTests(): void {
  equal(previewValue({ iconHtml: "custom" }, "iconHtml", "fallback"), "custom", "custom preview values win");
  equal(previewValue(null, "iconHtml", "fallback"), "fallback", "missing preview values use fallback");
  equal(infoOnlyCardVisible("sensor", true), true, "sensors remain visible in info-only mode");
  equal(infoOnlyCardVisible("action", true), false, "actions are hidden in info-only mode");
  equal(defaultCardTypeForPicker("climate"), "climate_control", "picker aliases retain their defaults");

  const definitions = {
    action: { label: "Action", allowInSubpage: true },
    climate: { label: "Climate", allowInSubpage: false },
    climate_control: { label: "Climate controls", pickerKey: "climate", allowInSubpage: false },
    sensor: { label: "Sensor", allowInSubpage: true },
  };
  deepEqual(
    cardTypePickerOptions(definitions, [], false, true, null).map((option) => option.key),
    ["action", "sensor"],
    "subpage picker filters unsupported and aliased entries",
  );
  const infoOnlyOptions = cardTypePickerOptions(definitions, [], true, false, "action");
  equal(infoOnlyOptions[0]?.key, "action", "selected hidden type remains visible for editing");
  equal(infoOnlyOptions[0]?.disabled, true, "selected hidden type is labelled unavailable");
  equal(infoOnlyOptions[1]?.key, "sensor", "supported info-only card remains selectable");

  equal(
    swapGridCell({ x: 99, y: 75 }, { left: 0, top: 0, right: 100, bottom: 100 }, 2, 2),
    3,
    "swap targeting resolves the containing grid cell",
  );
  equal(
    closestGridCell({ x: 24, y: 10 }, [
      { pos: 0, left: 0, top: 0, right: 10, bottom: 20 },
      { pos: 1, left: 20, top: 0, right: 30, bottom: 20 },
    ]),
    1,
    "drag targeting chooses the closest rendered cell",
  );
  deepEqual(
    clampMenuPosition({ x: 198, y: 99 }, 40, 30, 200, 100),
    { x: 156, y: 66 },
    "context menus stay inside the viewport",
  );
}
