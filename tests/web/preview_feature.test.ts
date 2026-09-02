import {
  cardTypePickerOptions,
  cardTypeVisibleForConnector,
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
  equal(cardTypeVisibleForConnector("slider", "home_assistant"), true, "Home Assistant-only cards remain in the Home Assistant picker");
  equal(cardTypeVisibleForConnector("slider", "mac_companion"), false, "Home Assistant-only cards are hidden from Companion");
  equal(cardTypeVisibleForConnector("action", "mac_companion"), true, "mixed action cards appear for Companion");
  equal(cardTypeVisibleForConnector("webhook", "home_assistant"), true, "shared webhook cards appear for Home Assistant");
  equal(cardTypeVisibleForConnector("webhook", "mac_companion"), true, "shared webhook cards appear for Companion");

  const definitions = {
    action: { label: "Action", allowInSubpage: true },
    climate: { label: "Climate", allowInSubpage: false },
    climate_control: { label: "Climate controls", pickerKey: "climate", allowInSubpage: false },
    sensor: { label: "Sensor", allowInSubpage: true },
    wifi_qr: { label: "Wifi Sharing", allowInSubpage: true },
    wifi_qr_card: { label: "QR Card", pickerKey: "wifi_qr", allowInSubpage: true },
  };
  deepEqual(
    cardTypePickerOptions(definitions, [], false, true, null).map((option) => option.key),
    ["action", "sensor", "wifi_qr"],
    "subpage picker filters unsupported and aliased entries",
  );
  deepEqual(
    cardTypePickerOptions({ ...definitions, slider: { label: "Slider", allowInSubpage: true } }, [], false, false, null, "mac_companion").map((option) => option.key),
    ["action", "sensor", "wifi_qr"],
    "Companion picker keeps shared cards and excludes HA-only cards",
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
