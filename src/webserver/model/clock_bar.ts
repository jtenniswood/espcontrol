/** Compact clock bar elements. Catalogue entries can be added without changing the editor. */
export const CLOCK_BAR_CATALOG = {
  temperature: { label: "Temperature", unit: "°", sample: "21.5", precision: 1 },
  humidity: { label: "Humidity", unit: "%", sample: "48", precision: 0 },
  battery: { label: "Battery level", unit: "%", sample: "85", precision: 0 },
  power: { label: "Power", unit: "W", sample: "320", precision: 0 },
  energy: { label: "Energy", unit: "kWh", sample: "4.2", precision: 1 },
  illuminance: { label: "Illuminance", unit: "lx", sample: "250", precision: 0 },
  pressure: { label: "Pressure", unit: "hPa", sample: "1013", precision: 0 },
  co2: { label: "CO₂", unit: "ppm", sample: "650", precision: 0 },
  time: { label: "Clock", unit: "", sample: "12:34", precision: 0 },
  network: { label: "Connectivity / settings", unit: "", sample: "", precision: 0 },
  night: { label: "Night mode", unit: "", sample: "", precision: 0 },
  device_battery: { label: "Device battery status", unit: "", sample: "", precision: 0 },
} as const;
export type ClockBarType = keyof typeof CLOCK_BAR_CATALOG;
export type ClockBarGroup = "left" | "middle" | "right";
export interface ClockBarElement {
  id: string;
  type: ClockBarType;
  group: ClockBarGroup;
  source: "ha" | "local";
  entity: string;
  label: string;
  unit: string;
  precision: number;
}
export interface ClockBarLayout { version: 1; items: ClockBarElement[] }
export const CLOCK_BAR_MAX_ITEMS = 12;
export const CLOCK_BAR_MAX_BYTES = 4096;
export function isClockBarSensor(type: ClockBarType): boolean {
  return !["time", "network", "night", "device_battery"].includes(type);
}
export function createClockBarElement(type: ClockBarType, group: ClockBarGroup, existing: readonly ClockBarElement[]): ClockBarElement {
  let index = 1;
  while (existing.some(item => item.id === "cb" + index)) index++;
  return { id: "cb" + index, type, group, source: "ha", entity: "", label: "",
    unit: CLOCK_BAR_CATALOG[type].unit, precision: CLOCK_BAR_CATALOG[type].precision };
}
/** Reject malformed documents rather than silently losing configured elements. Empty means legacy layout. */
export function parseClockBarLayout(value: unknown): ClockBarLayout | null {
  if (value === "" || value == null) return null;
  const raw = typeof value === "string" ? JSON.parse(value) : value;
  if (!raw || raw.version !== 1 || !Array.isArray(raw.items) || raw.items.length > CLOCK_BAR_MAX_ITEMS)
    throw new Error("Unsupported clock bar layout");
  const ids = new Set<string>();
  const items: ClockBarElement[] = raw.items.map((item: any) => {
    if (!item || typeof item.id !== "string" || !/^[a-zA-Z0-9_-]{1,24}$/.test(item.id) || ids.has(item.id) ||
        !Object.hasOwn(CLOCK_BAR_CATALOG, item.type) || !["left", "middle", "right"].includes(item.group) ||
        !["ha", "local"].includes(item.source) || typeof item.entity !== "string" || item.entity.length > 120 ||
        typeof item.label !== "string" || item.label.length > 32 || typeof item.unit !== "string" || item.unit.length > 12 ||
        !Number.isInteger(item.precision) || item.precision < 0 || item.precision > 3)
      throw new Error("Invalid clock bar element");
    ids.add(item.id);
    return { id: item.id, type: item.type, group: item.group, source: item.source, entity: item.entity,
      label: item.label, unit: item.unit, precision: item.precision };
  });
  const result: ClockBarLayout = { version: 1, items };
  if (new TextEncoder().encode(JSON.stringify(result)).length > CLOCK_BAR_MAX_BYTES)
    throw new Error("Clock bar configuration is too large");
  return result;
}
export function moveClockBarElement(layout: ClockBarLayout, id: string, group: ClockBarGroup, beforeId?: string): ClockBarLayout {
  const moving = layout.items.find(item => item.id === id);
  if (!moving || beforeId === id) return layout;
  const items = layout.items.filter(item => item.id !== id);
  const before = beforeId ? items.findIndex(item => item.id === beforeId && item.group === group) : -1;
  items.splice(before < 0 ? items.length : before, 0, { ...moving, group });
  return { version: 1, items };
}
