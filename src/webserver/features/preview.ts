export interface Point {
  readonly x: number;
  readonly y: number;
}

export interface Rect {
  readonly left: number;
  readonly top: number;
  readonly right: number;
  readonly bottom: number;
}

export interface PositionedRect extends Rect {
  readonly pos: number;
}

export interface MenuPosition {
  readonly x: number;
  readonly y: number;
}

export interface CardTypeDefinition {
  readonly key?: string;
  readonly label?: string | (() => string);
  readonly pickerKey?: string | (() => string);
  readonly allowInSubpage?: boolean | (() => boolean);
  readonly isAvailable?: (context: { isSub: boolean }) => boolean;
}

export interface CardPickerOption {
  readonly key: string;
  readonly label: string;
  readonly icon: string;
  readonly description: string;
  readonly disabled: boolean;
}

interface PickerDetails {
  readonly icon: string;
  readonly description: string;
}

const INFO_ONLY_CARD_TYPES = new Set([
  "sensor",
  "calendar",
  "clock",
  "door_window",
  "image",
  "local_sensor",
  "presence",
  "timezone",
  "weather",
  "weather_forecast",
]);

const CARD_TYPE_PICKER_DETAILS: Readonly<Record<string, PickerDetails>> = {
  "": { icon: "toggle-switch", description: "Toggle lights, switches, helpers, or fans." },
  action: { icon: "flash", description: "Run a Home Assistant or local action." },
  alarm: { icon: "shield-home", description: "Control or trigger alarm panel actions." },
  calendar: { icon: "calendar-clock", description: "Show date, time, or world clock values." },
  climate: { icon: "thermostat", description: "Show climate status and temperature controls." },
  cover: { icon: "window-shutter", description: "Control blinds, curtains, or covers." },
  door_window: { icon: "door-open", description: "Show open or closed sensor state." },
  presence: { icon: "account", description: "Show person or presence status." },
  fan_speed: { icon: "fan", description: "Control fan speed, mode, or direction." },
  garage: { icon: "garage", description: "Show and control a garage door." },
  gate: { icon: "gate", description: "Show and control a gate." },
  image: { icon: "image", description: "Display an image card where supported." },
  internal: { icon: "power-plug", description: "Control built-in device relays." },
  light_brightness: { icon: "lightbulb", description: "Configure light switch, brightness, or temperature controls." },
  lawn_mower: { icon: "robot-mower", description: "Show or control a robotic lawn mower." },
  local_sensor: { icon: "gauge", description: "Show a sensor value from this device." },
  lock: { icon: "lock", description: "Show and control a lock." },
  media: { icon: "speaker", description: "Control media playback or volume." },
  media_control: { icon: "music", description: "Open all media controls and volume in a modal." },
  push: { icon: "gesture-tap-button", description: "Fire a momentary button event." },
  sensor: { icon: "gauge", description: "Display sensor values or states." },
  slider: { icon: "tune-vertical", description: "Adjust a numeric or brightness value." },
  subpage: { icon: "view-grid-plus", description: "Open a nested page of cards." },
  webhook: { icon: "webhook", description: "Send a direct HTTP request." },
  vacuum: { icon: "robot-vacuum", description: "Show or control a vacuum cleaner." },
  weather: { icon: "weather-partly-cloudy", description: "Show weather or forecast data." },
};

const CARD_TYPE_PICKER_DEFAULTS: Readonly<Record<string, string>> = {
  climate: "climate_control",
  light_brightness: "light_control",
  media_control: "media",
};

export function previewValue<T>(preview: Record<string, unknown> | null | undefined, key: string, fallback: T): T {
  return preview && Object.prototype.hasOwnProperty.call(preview, key) ? preview[key] as T : fallback;
}

export function registryValue<T>(definition: Record<string, unknown> | null | undefined, key: string, fallback: T): T {
  if (!definition || !Object.prototype.hasOwnProperty.call(definition, key)) return fallback;
  const candidate = definition[key];
  const value = typeof candidate === "function" ? (candidate as () => unknown)() : candidate;
  return value == null ? fallback : value as T;
}

export function infoOnlyCardVisible(key: string, infoOnly: boolean): boolean {
  return !infoOnly || INFO_ONLY_CARD_TYPES.has(key || "");
}

export function defaultCardTypeForPicker(key: string): string {
  return CARD_TYPE_PICKER_DEFAULTS[key] || key;
}

export function cardTypePickerDetails(key: string, label: string): PickerDetails {
  return CARD_TYPE_PICKER_DETAILS[key || ""] || {
    icon: "card-outline",
    description: `Configure a ${label || "card"} card.`,
  };
}

export function cardTypePickerOptions(
  definitions: Readonly<Record<string, CardTypeDefinition>>,
  disabledCardTypes: readonly string[],
  infoOnly: boolean,
  isSub: boolean,
  selectedTypeKey: string | null | undefined,
): CardPickerOption[] {
  const options: CardPickerOption[] = [];
  let selectedUnsupported: { key: string; label: string } | null = null;
  const hasSelectedType = selectedTypeKey !== null && selectedTypeKey !== undefined;
  for (const [typeKey, definition] of Object.entries(definitions)) {
    const rawDefinition = definition as Record<string, unknown>;
    const pickerKey = registryValue(rawDefinition, "pickerKey", "");
    const allowInSubpage = !!registryValue(rawDefinition, "allowInSubpage", false);
    const label = registryValue(rawDefinition, "label", definition.key || "Toggle");
    if (disabledCardTypes.includes(typeKey) || disabledCardTypes.includes(pickerKey)) continue;
    if (!infoOnlyCardVisible(typeKey, infoOnly) || (pickerKey && !infoOnlyCardVisible(pickerKey, infoOnly))) {
      if (hasSelectedType && (selectedTypeKey === typeKey || (pickerKey && selectedTypeKey === pickerKey))) {
        selectedUnsupported = { key: selectedTypeKey, label };
      }
      continue;
    }
    if (pickerKey && pickerKey !== typeKey) continue;
    if (isSub && !allowInSubpage) continue;
    if (definition.isAvailable && !definition.isAvailable({ isSub }) && selectedTypeKey !== typeKey) continue;
    options.push({ key: typeKey, label, disabled: false, ...cardTypePickerDetails(typeKey, label) });
  }
  if (selectedUnsupported) {
    const label = `${selectedUnsupported.label} (not available)`;
    options.push({
      key: selectedUnsupported.key,
      label,
      disabled: true,
      ...cardTypePickerDetails(selectedUnsupported.key, label),
    });
  }
  return options.sort((a, b) => a.label.localeCompare(b.label));
}

export function closestGridCell(point: Point, cells: readonly PositionedRect[]): number {
  let bestDistance = Number.POSITIVE_INFINITY;
  let bestPosition = -1;
  for (const cell of cells) {
    if (point.x >= cell.left && point.x <= cell.right && point.y >= cell.top && point.y <= cell.bottom) {
      return cell.pos;
    }
    const centerX = (cell.left + cell.right) / 2;
    const centerY = (cell.top + cell.bottom) / 2;
    const distance = (point.x - centerX) ** 2 + (point.y - centerY) ** 2;
    if (distance < bestDistance) {
      bestDistance = distance;
      bestPosition = cell.pos;
    }
  }
  return bestPosition;
}

export function swapGridCell(
  point: Point,
  container: Rect,
  gridCols: number,
  gridRows: number,
): number {
  const width = Math.max(1, container.right - container.left);
  const height = Math.max(1, container.bottom - container.top);
  const column = Math.max(0, Math.min(Math.floor((point.x - container.left) / (width / gridCols)), gridCols - 1));
  const row = Math.max(0, Math.min(Math.floor((point.y - container.top) / (height / gridRows)), gridRows - 1));
  return row * gridCols + column;
}

export function clampMenuPosition(
  point: Point,
  menuWidth: number,
  menuHeight: number,
  viewportWidth: number,
  viewportHeight: number,
  margin = 4,
): MenuPosition {
  return {
    x: Math.max(margin, Math.min(point.x, viewportWidth - menuWidth - margin)),
    y: Math.max(margin, Math.min(point.y, viewportHeight - menuHeight - margin)),
  };
}
